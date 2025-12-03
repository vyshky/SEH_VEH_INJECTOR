#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

typedef struct _CPU_REGS {
	UINT64 RAX, RBX, RCX, RDX, RSI, RDI, RSP, RBP, RIP;
} CPU_REGS;

extern void callProcessFunctionEx(LPVOID ADDRESS, CPU_REGS* CPU_REGS);



// ѕростой лог в MessageBox (дл€ демонстрации)
void Log(const char* s) {
	MessageBoxA(NULL, s, "ExceptionMSB", MB_OK | MB_TOPMOST);
}

LONG CALLBACK MyVehHandler(PEXCEPTION_POINTERS ep)
{
	char buf[512];

	snprintf(buf, sizeof(buf),
		"VEH caught exception!\n"
		"Code: 0x%08X\n"
		"Address: %p\n"
		"EIP/RIP: %p\n"
		"Returning CONTINUE_SEARCH",
		ep->ExceptionRecord->ExceptionCode,
		ep->ExceptionRecord->ExceptionAddress,
#ifdef _WIN64
		(void*)ep->ContextRecord->Rip
#else
		(void*)ep->ContextRecord->Eip
#endif
	);

	Log(buf);

	return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI callProcessFunction(LPVOID lpParam)
{
	__try {
		Log("callProcessFunctionEx()");
		CPU_REGS cpu_regs = { 0 };
		callProcessFunctionEx(0x0, &cpu_regs);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		Log("SEH handler: exception caught while calling function");
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	static PVOID veh_cookie = NULL;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// «арегистрируем VEH с приоритетом "перед всеми" (first=True)
		veh_cookie = AddVectoredExceptionHandler(1, MyVehHandler);
		if (!veh_cookie) {
			Log("AddVectoredExceptionHandler failed");
		}
		else {
			Log("VEH registered (returns CONTINUE_SEARCH)");
		}

		// —оздадим поток, внутри которого мы контролируем SEH
		{
			HANDLE hThread = CreateThread(NULL, 0, callProcessFunction, NULL, 0, NULL);
			if (hThread) {
				CloseHandle(hThread);
			}
			else {
				Log("CreateThread failed");
			}
		}
		break;

	case DLL_PROCESS_DETACH:
		if (veh_cookie) {
			RemoveVectoredExceptionHandler(veh_cookie);
			veh_cookie = NULL;
		}
		break;
	}
	return TRUE;
}
