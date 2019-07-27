

#include <scheme/scheme.h>
#include <winerror.h>
#include <wil/result.h>
#include <string>
#include <WebView2.h>
#include "commonview.h"

#ifndef ABNORMAL_EXIT
#define ABNORMAL_EXIT ((void (*)(void))0)
#endif /* ABNORMAL_EXIT */


HANDLE g_script_mutex;
 
 
std::string get_exe_folder()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	const auto pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}


void load_script_if_exists(const std::string& script_name)
{
	const auto locate_script = get_exe_folder() + script_name;
	if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(locate_script.c_str()) &&
		GetLastError() == ERROR_FILE_NOT_FOUND))
	{
	 
		CALL1("load", Sstring(locate_script.c_str()));
 
	}
}

bool register_boot_file(const std::string& boot_file)
{
	const auto locate_boot = get_exe_folder() + boot_file;
	if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(locate_boot.c_str()) &&
		GetLastError() == ERROR_FILE_NOT_FOUND))
	{
		Sregister_boot_file(locate_boot.c_str());
		return true;
	}
	return false;
}

static void custom_init()
{
}

ptr scheme_navigate(const char* u)
{
	const std::string url = u;
	web_view_navigate(url);
	return Strue;
}

ptr scheme_start_web_server(int port, const char* base)
{
	const auto base_dir = get_exe_folder() + "//" + base;
	start_web_server(port, base_dir);
	return Strue;
}

ptr scheme_home_page(const char* first)
{
	navigate_first = s2_ws(first);
	return Strue;
}


extern "C" __declspec(dllexport) ptr EscapeKeyPressed()
{
	if (GetAsyncKeyState(VK_ESCAPE) != 0)
	{
		return Strue;
	}
	return Sfalse;
}


std::deque<std::string> commands;

// cancel pending commands.
void cancel_commands()
{
	WaitForSingleObject(g_commands_mutex, INFINITE);
	while (!commands.empty())
	{
		commands.pop_front();
	}
	commands.shrink_to_fit();
	ReleaseMutex(g_commands_mutex);
}


DWORD WINAPI  process_commands(LPVOID x)
{
	while (true) {
		while (commands.empty())
		{
			Sleep(10);
		}
	
		std::string eval;
		WaitForSingleObject(g_commands_mutex, INFINITE);
	 
		// otherwise process them
		if (!commands.empty()) {
			
			eval = commands.front();
			commands.pop_front();
		}
		ReleaseMutex(g_commands_mutex);

		WaitForSingleObject(g_script_mutex, INFINITE);
		try {
			CALL1("eval->string-post-back", Sstring(eval.c_str()));
		}
		catch (...) {

		}
		ReleaseMutex(g_script_mutex);
		::Sleep(20);
	}
}

void eval_text(const char* cmd)
{
	WaitForSingleObject(g_commands_mutex, INFINITE);
	commands.emplace_back(cmd);
	ReleaseMutex(g_commands_mutex);
}


int start_scheme_engine() {

	try
	{
		g_script_mutex = CreateMutex(nullptr, FALSE, nullptr);
 
		Sscheme_init(ABNORMAL_EXIT);

		if (!register_boot_file("\\boot\\petite.boot") ||
			!register_boot_file("\\boot\\scheme.boot"))
		{
			exit(01);
		}

		Sbuild_heap("SchemeWebServer.exe", custom_init);

		// define some functions that can be called from scheme code
		Sforeign_symbol("scheme_home_page", static_cast<ptr>(scheme_home_page));
		Sforeign_symbol("start_web_server", static_cast<ptr>(scheme_start_web_server));
		Sforeign_symbol("EscapeKeyPressed", static_cast<ptr>(EscapeKeyPressed));
		Sforeign_symbol("web_exec", static_cast<ptr>(scheme_web_view_exec));
		Sforeign_symbol("web_load_document", static_cast<ptr>(scheme_load_document_from_file));
		Sforeign_symbol("scheme_wait", static_cast<ptr>(scheme_wait));
		Sforeign_symbol("scheme_yield", static_cast<ptr>(scheme_yield));
		Sforeign_symbol("scheme_post_message", static_cast<ptr>(scheme_post_message));
		Sforeign_symbol("scheme_capture_screen", static_cast<ptr>(scheme_capture_screen));
		Sforeign_symbol("scheme_get_source", static_cast<ptr>(scheme_get_source));
		
 
		// base prepare, env - configure, init - start
		load_script_if_exists("\\scripts\\base.ss");
		load_script_if_exists("\\scripts\\env.ss");
		load_script_if_exists("\\scripts\\init.ss");

		CALL1("suppress-greeting", Strue);
		CALL1("waiter-prompt-string", Sstring(""));

		// spin of some periodic gc.
		static auto gc_thread = CreateThread(
			nullptr,
			0,
			garbage_collect,
			nullptr,
			0,
			nullptr);

		// script exec background thread
		static auto cmd_thread = CreateThread(
			nullptr,
			0,
			process_commands,
			nullptr,
			0,
			nullptr);

	}
	catch (const std::exception& e)
	{
		const auto what = s2_ws(e.what());
		MessageBox(nullptr, what.c_str(), what.c_str(), MB_ICONERROR);
		return -1;
	}

	return 0;
}


