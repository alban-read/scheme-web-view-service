#include <scheme/scheme.h>
#include <windows.h>

#include "commonview.h"

// spare a second to take out the garbage.
DWORD WINAPI  garbage_collect(LPVOID cmd)
{
	while (true) {
 
			WaitForSingleObject(g_script_mutex, INFINITE);
			CALL0("gc"); 
			ReleaseMutex(g_script_mutex);

			WaitForSingleObject(g_messages_mutex, INFINITE);
			messages.shrink_to_fit();
			ReleaseMutex(g_messages_mutex);

			WaitForSingleObject(g_commands_mutex, INFINITE);
			commands.shrink_to_fit();
			ReleaseMutex(g_commands_mutex);
	 
			Sleep(10000);
	}
 
}