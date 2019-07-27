// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

// Server only - supports  browser - no view control

#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/result.h>
#include <scheme/scheme.h>
#include <fmt/format.h>
#include <Shlwapi.h>

#pragma comment(lib, "csv952.lib")

#include "WebView2.h"
#include <locale>
#include <codecvt>
#include "resource.h"
#include "commonview.h"
#include <vector>
#include <deque>


using namespace Microsoft::WRL;


HWND main_window;

// The main window class name.
static TCHAR szWindowClass[] = _T("SchemeServer");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Server");

HINSTANCE hInst;
HANDLE g_messages_mutex;
HANDLE g_commands_mutex;
// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
 

std::wstring s2_ws(const std::string& str)
{
	using convert_type_x = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type_x, wchar_t> converter_x;
	return converter_x.from_bytes(str);
}

std::string ws_2s(const std::wstring& wstr)
{
	using convert_type_x = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type_x, wchar_t> converter_x;
	return converter_x.to_bytes(wstr);
}

// wait at least ms while also feeding the windows event loop.
void wait(const long ms)
{
	const auto end = clock() + ms;
	do
	{
		MSG msg;
		// main window
		if (::PeekMessage(&msg, main_window, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		// any window in thread
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		::Sleep(0);
	} while (clock() < end);
}


void do_events(int turns)
{
	for (int i = 0; i <turns; i++) {
		::Sleep(0);
		MSG msg;
		// main window
		if (::PeekMessage(&msg, main_window, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		// any window in thread
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}


ptr scheme_wait(int ms)
{
	wait(ms);
	return Strue;
}


ptr scheme_yield(int ms)
{
	// yield and wait
	ReleaseMutex(g_script_mutex);
	wait(ms);
	// now we try to get back to life
	auto dw_wait_result = WaitForSingleObject(g_script_mutex, 5);
	while (dw_wait_result == WAIT_TIMEOUT)
	{
		wait(10);
		dw_wait_result = WaitForSingleObject(g_script_mutex, 5);
	}
	// finally we can run again.
	return Strue;
}



HRESULT web_view_navigate(const std::string& url) {
	return -1;
}

void web_view_exec(const std::wstring& script) {
 
}

 
// post back in; from any thread.
std::deque<std::string> messages;

ptr scheme_post_message(const char* msg) {
	WaitForSingleObject(g_messages_mutex, INFINITE);
	messages.emplace_back(msg);
	ReleaseMutex(g_messages_mutex);
	return Strue;
}

// scheme call into web view.
ptr scheme_web_view_exec(const char* cmd, char* cbname)
{
	// not callback capable yet
	WaitForSingleObject(g_messages_mutex, INFINITE);
	messages.emplace_back(cmd);
	ReleaseMutex(g_messages_mutex);
	return Strue;
}



std::wstring wide_get_exe_folder()
{
	TCHAR buffer[MAX_PATH];
	GetModuleFileName(nullptr, buffer, MAX_PATH);
	const auto pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

size_t get_size_of_file(const std::wstring& path)
{
	struct _stat fileinfo {};
	_wstat(path.c_str(), &fileinfo);
	return fileinfo.st_size;
}

std::wstring load_utf8_file_to_string(const std::wstring& filename)
{
	std::wstring buffer;  // stores file contents
	FILE* f;
	const auto err = _wfopen_s(&f, filename.c_str(), L"rtS, ccs=UTF-8");
	// Failed to open file
	if (f == nullptr || err != 0)
	{
		return buffer;
	}
	const auto file_size = get_size_of_file(filename);
	// Read entire file contents in to memory
	if (file_size > 0)
	{
		buffer.resize(file_size);
		const auto chars_read = fread(&(buffer.front()), sizeof(wchar_t), file_size, f);
		buffer.resize(chars_read);
		buffer.shrink_to_fit();
	}
	fclose(f);
	return buffer;
}



ptr scheme_load_document_from_file(const char* relative_file_name)
{
	return Strue;
}

// capture screen to file.
ptr scheme_capture_screen(const char* relative_file_name)
{
	return Snil;
}


ptr scheme_get_source()
{
	return Snil;
}


// we control the horizontal and the vertical..
void GetDesktopResolution(int& horizontal, int& vertical)
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

std::string text_message;

// displays on the server panel; if we are busy or not
DWORD WINAPI  update_status(LPVOID x)
{
	RECT rect;
	int last_pending_commands = -1;
	int last_pending_messages = -1;
	while (true) {
		if (main_window != nullptr) {
			int pending_commands;
			int pending_messages;
			WaitForSingleObject(g_commands_mutex, INFINITE);
			pending_commands = commands.size();
			ReleaseMutex(g_commands_mutex);
			WaitForSingleObject(g_messages_mutex, INFINITE);
			pending_messages = messages.size();
			text_message = fmt::format("Queue: {0}:{1}",
				pending_commands, pending_messages);
			ReleaseMutex(g_messages_mutex);

			if (pending_commands != last_pending_commands
				|| pending_messages != last_pending_messages) {

				GetClientRect(main_window, &rect);
				rect.left = 300;
				rect.top = 20;
				InvalidateRect(main_window, &rect, TRUE);
				last_pending_commands = pending_commands;
				last_pending_messages = pending_messages;
			}
			 
		}
		Sleep(250);
	}
}


int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{

	WNDCLASSEX window_class;
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = WndProc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = hInstance;
	window_class.hIcon = LoadIcon(hInstance, IDI_ASTERISK);
	window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
	window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = szWindowClass;
	window_class.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	if (!RegisterClassEx(&window_class))
	{
		MessageBox(nullptr,
			_T("Call to RegisterClassEx failed!"),
			_T("Windows Scheme WebView2 Shell"),
			NULL);

		return 1;
	}


	int h = 0;
	int v = 0;
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	GetDesktopResolution(h, v);
	hInst = hInstance;

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		300, 200,
		600,150,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		MessageBox(nullptr,
			_T("Call to CreateWindow failed!"),
			_T("SchemeWebView"),
			NULL);

		return 1;
	}
	main_window = hWnd;

	auto s = init_web_server();

	UpdateWindow(hWnd);
	ShowWindow(hWnd, nCmdShow);
	auto engine_started = start_scheme_engine();
 
	CreateThread(
		nullptr,
		0,
		update_status,
		nullptr,
		0,
		nullptr);


	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return static_cast<int>(msg.wParam);
}

// cancel pending messages and commands.
void cancel_pressed()
{
	cancel_commands();
	cancel_messages();
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR greeting[] = _T("WebView2andScheme");
	HDC         hdc;
	PAINTSTRUCT ps;
	RECT        rect;

	switch (message)
	{

	case WM_CREATE:

		CreateWindowEx(0, L"button", L"Quit",
			WS_CHILD | WS_VISIBLE, 45, 10, 100, 40,
			hWnd, reinterpret_cast<HMENU>(9002), 
			reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance , nullptr);

		CreateWindowEx(0, L"button", L"Cancel",
			WS_CHILD | WS_VISIBLE, 165, 10, 100, 40,
			hWnd, reinterpret_cast<HMENU>(9003),
			reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);
		return 0;

	case WM_COMMAND:
		if (LOWORD(wParam) == 9002) {
			PostQuitMessage(0);
		}
		if (LOWORD(wParam) == 9003) {
			cancel_pressed();
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(main_window, &ps);
		GetClientRect(main_window, &rect);
		SetTextColor(hdc, 0x00000000);
		SetBkMode(hdc, TRANSPARENT);
		rect.left = 300;
		rect.top = 20;
		DrawTextA(hdc, text_message.c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
		EndPaint(main_window, &ps);
		break;

	case WM_SIZE:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		// post from another thread	
	case WM_USER + 501:
	{
		 
		return 0;
	}


	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}
