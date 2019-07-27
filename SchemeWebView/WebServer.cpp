// A web view; is more useful by far; when combined with a web server.
// web server from: https://github.com/yhirose/cpp-httplib
// see license.html

#include <httplib.h>
#include <fmt/format.h>
#include <scheme/scheme.h>
#include "commonview.h"
#include <atlenc.h>

// web server
// one web server runs at a time.
bool server_logging = false;
int server_port = 8087;
std::string server_base_dir = "./docs";
std::wstring navigate_first = L"http://localhost:8087";
HANDLE server_thread = nullptr;
HANDLE g_web_server;
std::string get_exe_folder();
void web_view_exec(const std::wstring& script);
void  wait(const long ms);



// wait for scheme engine.
// this queues for engine access on a single thread
bool spin(const int turns)
{
	auto dw_wait_result = WaitForSingleObject(g_script_mutex, 15);
	auto time_out = turns;
	while (--time_out > 0 && (dw_wait_result == WAIT_TIMEOUT))
	{
		Sleep(25);
		dw_wait_result = WaitForSingleObject(g_script_mutex, 15);
	}
	//
	if (time_out == 0)
	{
		// we are out of turns. 
		return true;
	}
	return false;
}

// only sensible while still in UI thread.
// this attempts to keep processing events while waiting,
bool spin_wait(const int turns)
{
	auto dw_wait_result = WaitForSingleObject(g_script_mutex, 5);
	auto time_out = turns;
	while (--time_out > 0 && (dw_wait_result == WAIT_TIMEOUT))
	{
		wait(50);
		dw_wait_result = WaitForSingleObject(g_script_mutex, 5);
	}
	//
	if (time_out == 0)
	{
		// we are out of turns. 
		return true;
	}
	return false;
}


// https://github.com/yhirose/cpp-httplib
httplib::Server svr;


std::string dump_headers(const httplib::Headers& headers) {

	std::string s;
	s += "Server Root:" + server_base_dir + "\n";
	for (const auto& x : headers)
	{
		s += fmt::format("<p>{0}: {1}</p>\n", x.first, x.second);
	}
	return s;
}

std::string server_log(const httplib::Request& req, const httplib::Response& res) {
	std::string s;
	char buf[BUFSIZ];
	s += "================================\n";

	s += "Root:" + server_base_dir + "\n";
	s += "================================\n";

	snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
		req.version.c_str(), req.path.c_str());
	s += buf;

	std::string query;
	for (auto it = req.params.begin(); it != req.params.end(); ++it) {
		const auto& x = *it;
		snprintf(buf, sizeof(buf), "%c%s=%s",
			(it == req.params.begin()) ? '?' : '&', x.first.c_str(),
			x.second.c_str());
		query += buf;
	}
	snprintf(buf, sizeof(buf), "%s\n", query.c_str());
	s += buf;

	s += dump_headers(req.headers);

	s += "--------------------------------\n";

	snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
	s += buf;
	s += dump_headers(res.headers);
	s += "\n";

	if (!res.body.empty()) { s += res.body; }

	s += "\n";

	return s;
}



std::string do_scheme_eval(const char* text)
{
	// std::string result;
	// const auto scheme_string = CALL1("eval->string", Sstring(text));
	// if (scheme_string != Snil && Sstringp(scheme_string))
	// {
	// 	result = Assoc::Sstring_to_charptr(scheme_string);
	// }
	// Sleep(0);
	// return result;;
}

// api call  - deprecated due to direct comms channel
std::string do_scheme_api_call(const int n, std::string v1)
{
	std::string result;

	const auto scheme_string = CALL2("api-call", Sfixnum(n), Sstring(v1.c_str()));
	if (scheme_string != Snil && Sstringp(scheme_string))
	{
		result = Assoc::Sstring_to_charptr(scheme_string);
	}
	return result;
}

// https://stackoverflow.com/a/34571089/5155484
typedef unsigned char uchar;
static const std::string b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static std::string base64_encode(const std::string& in) {
	std::string out;

	int val = 0, valb = -6;
	for (uchar c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(b[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back(b[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

void cancel_messages()
{
	WaitForSingleObject(g_messages_mutex, INFINITE);
	while (!messages.empty())
	{
		messages.pop_front();
	}
	messages.shrink_to_fit();
	ReleaseMutex(g_messages_mutex);
}



uint64_t event_id;
std::string create_event(uint64_t offset) {

	std::string eventdata;
	if (event_id == 0)
		eventdata = "retry: 15000\n\n";
	else
	{
		eventdata += "id: ";
		eventdata += std::to_string(event_id);

		while (messages.empty())
		{
			Sleep(10);
		};
		WaitForSingleObject(g_messages_mutex, INFINITE);
		if (!messages.empty())
		{
			eventdata += "\ndata:" + base64_encode(messages.front()) + "\n\n";
			messages.pop_front();
		}
		 
		ReleaseMutex(g_messages_mutex);
	}

	event_id++;
	Sleep(0);
	return eventdata.c_str();
}


DWORD WINAPI start_server(LPVOID p)
{
	using namespace httplib;

	while (true) {

		if (svr.is_running())
		{
			svr.stop();
		}
		if (WaitForSingleObject(g_web_server, 1000) == WAIT_TIMEOUT)
		{
			if (WaitForSingleObject(g_web_server, 1000) == WAIT_TIMEOUT)
			{
				return 0;
			}
		}

		// re-start web server	

		try
		{

			svr.set_error_handler([](const auto& req, auto& res) {
				auto fmt = "<p>Error Status: <span style='color:red;'>{0}</span></p>";
				auto s = fmt::format(fmt, res.status);
				res.set_content(s, "text/html");
			});

			svr.Get("/stop",
				[&](const Request& /*req*/, Response& /*res*/) { svr.stop(); });


			svr.Post("/evaluate",
				[&](const Request& req, Response& res)
			{
				const auto response = req.body;
				if (req.body.c_str() != nullptr) {
					eval_text(_strdup(response.c_str()));
				}
				res.set_content("::eval_pending:", "text/plain");
			});

			svr.Get("/logsON",
				[&](const Request& /*req*/, Response& /*res*/) { server_logging = true; });

			svr.Get("/logsOFF",
				[&](const Request& /*req*/, Response& /*res*/) { server_logging = false; });

			svr.Get("/", [=](const Request& /*req*/, Response& res) {
				res.set_redirect("/README.html");
			});

			svr.Get("/dump", [](const Request& req, Response& res) {
				res.set_content(dump_headers(req.headers), "text/plain");
			});

			// api call n (0..63), v1.(.v4)
			// generic API call handler with call number and params.
			svr.Get(R"(/api/(\d+))", [](const Request& req, Response& res) {
				const auto numbers = req.matches[1];
				std::string query;

				// acquire script lock access or fail..
				if (spin(10))
				{
					res.set_content("timed_out:busy", "text/plain");
					return;
				}
				try
				{
					const auto n = std::stoi(numbers);
					if (n > 63 || n < 0)
					{
						res.set_content("exceed api call slots n must be from 0-63", "text/plain");
						return;
					}

					std::string v1;

					for (const auto& param : req.params)
					{
						if (param.first == "v1")
							v1 = param.second;

					}
					const auto result = do_scheme_api_call(n, v1);
					if (result.empty())
					{
						res.set_content(";; error response", "text/plain");
						return;
					}

					res.set_content(result, "text/plain");
					return;
				}
				catch (...)
				{
					ReleaseMutex(g_script_mutex);
				}

				res.status = 500;
				res.set_content("Error", "text/plain");
				return;
			});

			// generic API call handler with call number and params.
			svr.Post(R"(/api/(\d+))", [](const Request& req, Response& res) {
				const auto numbers = req.matches[1];
				std::string query;

				// acquire script lock access or fail..
				if (spin(10))
				{
					res.set_content("timed_out", "text/plain");
				}
				try
				{
					const auto n = std::stoi(numbers);
					if (n > 63 || n < 0)
					{
						res.status = 500;
						res.set_content("exceed api call slots n must be from 0-63", "text/plain");
						return;
					}
					const auto v1 = req.body;

					const auto result = do_scheme_api_call(n, v1);
					if (result.empty())
					{
						res.set_content(";; error response", "text/plain");
						return;
					}
					res.set_content(result, "text/plain");
					return;

				}
				catch (...)
				{

					ReleaseMutex(g_script_mutex);
				}

				res.status = 500;
				res.set_content("Error", "text/plain");
				return;

			});

			// even source feeds regular events to browser from server.
			svr.Get("/eventsrc", [](const Request& req, Response& res) {
				res.content_producer = create_event;
				res.set_header("Content-Type", "text/event-stream");
			});

			svr.set_logger([](const Request& req, const Response& res) {
				if (server_logging) {
					const auto s = fmt::format("Web Server:{0}\r\n", server_log(req, res));
					// where to send logs..
				}
			});

			svr.set_base_dir(server_base_dir.c_str());
			svr.listen("localhost", server_port);

		}
		catch (...) {

			ReleaseMutex(g_web_server);
			return 0;
		}

		ReleaseMutex(g_web_server);


	}
	return 0;
}

extern "C" __declspec(dllexport) ptr stop_web_server(int port, char* base)
{
	if (svr.is_running())
	{
		svr.stop();
	}
	return Strue;
}

// called indirectly from base.ss
int start_web_server(int port, const std::string& base)
{
	server_port = port;
	server_base_dir = base;
	server_thread = CreateThread(
		nullptr,
		0,
		start_server,
		nullptr,
		0,
		nullptr);
	return 1;
}

int init_web_server()
{
	g_web_server = CreateMutex(nullptr, FALSE, nullptr);
	g_messages_mutex = CreateMutex(nullptr, FALSE, nullptr);
	g_commands_mutex = CreateMutex(nullptr, FALSE, nullptr);
	return 0;
}
