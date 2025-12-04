#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

typedef struct _CPU_REGS {
    UINT64 RAX, RBX, RCX, RDX, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15, RSP;
} CPU_REGS;

extern void callProcessFunctionEx(LPVOID address, CPU_REGS* regs);

void Log(const char* s) {
    MessageBoxA(NULL, s, "ExceptionMSB", MB_OK | MB_TOPMOST);
}

void LogInt(const char* prefix, UINT64 value) {
    char buf[512];
    sprintf_s(buf, sizeof(buf), "%s: %llu", prefix, value);
    Log(buf);
}

LONG CALLBACK MyVehHandler(PEXCEPTION_POINTERS ep) {
    char buf[512];
    sprintf_s(buf, sizeof(buf),
        "VEH caught exception!\n"
        "Code: 0x%08X\n"
        "Address: %p\n"
        "EIP/RIP: %p\n"
        "Returning CONTINUE_SEARCH",
        ep->ExceptionRecord->ExceptionCode,
        ep->ExceptionRecord->ExceptionAddress,
        (void*)ep->ContextRecord->Rip
    );
    Log(buf);
    return EXCEPTION_CONTINUE_SEARCH;
}

// Тестовая функция с правильным соглашением о вызовах
int __stdcall TestFunction(int a, int b, int c, int d) {
    return a + b + c + d;
}

DWORD WINAPI TestThread(LPVOID lpParam) {
    __try {
        Log("Starting test...");

        CPU_REGS regs = { 0 };

        // Устанавливаем параметры
        regs.RCX = 10;  // первый параметр
        regs.RDX = 20;  // второй параметр
        regs.R8 = 30;   // третий параметр
        regs.R9 = 40;   // четвертый параметр

        Log("Calling TestFunction...");

        // Вызываем функцию
        callProcessFunctionEx(TestFunction, &regs);

        // Проверяем результат
        char result[100];
        sprintf_s(result, sizeof(result), "Result: %llu (expected: 100)", regs.RAX);
        Log(result);

        // Тест с падающей функцией
        Log("Testing NULL pointer call...");
        CPU_REGS regs2 = { 0 };
        regs2.RCX = 0;
        regs2.RDX = 0;
        callProcessFunctionEx(NULL, &regs2);  // Упадет

        Log("Should not reach here!");
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        char buf[100];
        sprintf_s(buf, sizeof(buf), "SEH caught exception: 0x%08X", GetExceptionCode());
        Log(buf);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Отключаем вызовы DllMain для потоков
        DisableThreadLibraryCalls(hModule);

        // Регистрируем VEH
        PVOID veh = AddVectoredExceptionHandler(1, MyVehHandler);

        if (!veh) {
            Log("Failed to register VEH");
        }

        // Запускаем тест в отдельном потоке
        HANDLE hThread = CreateThread(NULL, 0, TestThread, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
        else {
            Log("Failed to create thread");
        }
    }

    return TRUE;
}