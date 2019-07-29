#pragma once
 
#include <scheme/scheme.h>
#include <string>
#include <deque>
#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)
#ifndef ABNORMAL_EXIT
#define ABNORMAL_EXIT ((void (*)(void))0)
#endif /* ABNORMAL_EXIT */
extern HANDLE g_messages_mutex;
extern HANDLE g_commands_mutex;
extern HANDLE g_script_mutex;
extern HANDLE g_web_server;
extern HANDLE server_thread;
extern HWND main_window;

int start_web_server(int port, const std::string& base);
DWORD WINAPI  garbage_collect(LPVOID cmd);
std::wstring s2_ws(const std::string& str);
extern std::wstring navigate_first;
ptr scheme_web_view_exec(const char* cmd, char* cbname);
ptr scheme_web_view_exec_threaded(const char* cmd, char* cbname);
ptr scheme_load_document_from_file(const char* relative_file_name);
ptr scheme_wait(int ms);
ptr scheme_yield(int ms);
ptr scheme_post_message(const char* msg);
bool spin(const int turns);
bool spin_wait(const int turns);
HRESULT web_view_navigate(const std::string& url);
int start_scheme_engine();
int init_web_server();
void eval_text(const char* cmd);
ptr scheme_capture_screen(const char* relative_file_name);
ptr scheme_get_source();
void do_events(int turns);
extern std::deque<std::string> messages;
extern std::deque<std::string> commands;
void cancel_commands();
void cancel_messages();
extern bool cancelling;
void cancel_pressed();

namespace Assoc {
	ptr cons_sfixnum(const char* symbol, const int value, ptr l);
	ptr constUTF8toSstring(std::string s);
	ptr constUTF8toSstring(const char* s);
	ptr cons_sstring(const char* symbol, const char* value, ptr l);
	const char* Sstring_to_charptr(ptr sparam);
}