#include <windows.h>

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved)  // reserved
{
	// Perform actions based on the reason for calling.
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		MessageBoxA(NULL, "VEH DLL Injected Successfully!", "VEH DLL", MB_OK | MB_ICONINFORMATION);
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		break;

	case DLL_THREAD_ATTACH:
		MessageBoxA(NULL, "VEH DLL Thread Attached!", "VEH DLL", MB_OK | MB_ICONINFORMATION);
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		MessageBoxA(NULL, "VEH DLL Thread Detached!", "VEH DLL", MB_OK | MB_ICONINFORMATION);
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		MessageBoxA(NULL, "VEH DLL Process Detached!", "VEH DLL", MB_OK | MB_ICONINFORMATION);
		if (lpvReserved != NULL)
		{
			break; // do not do cleanup if process termination scenario
		}

		// Perform any necessary cleanup.
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}