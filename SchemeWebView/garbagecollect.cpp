#include <scheme/scheme.h>
#include <windows.h>

#include "commonview.h"

// spare a second to take out the garbage.
DWORD WINAPI  garbage_collect(LPVOID cmd)
{
	while (true) {
		// every couple of seconds; wait briefly; if engine not busy; do gc; and shrink deque.
		const auto dw_wait_result = WaitForSingleObject(g_script_mutex, 25);
		if (dw_wait_result != WAIT_TIMEOUT) {
			
			CALL0("gc"); 
			ReleaseMutex(g_script_mutex);
			WaitForSingleObject(g_messages_mutex, INFINITE);
			messages.shrink_to_fit();
			ReleaseMutex(g_messages_mutex);
			WaitForSingleObject(g_commands_mutex, INFINITE);
			commands.shrink_to_fit();
			ReleaseMutex(g_commands_mutex);
		}
		Sleep(2000);
	}
 
}