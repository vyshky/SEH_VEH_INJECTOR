#pragma once
#include "pch.h"

char path_to_dll[256] = { 0 };
char process_name[256] = { 0 };

void select_dll_file()
{
	char ini_path[MAX_PATH];
	GetModuleFileNameA(NULL, ini_path, MAX_PATH);

	char* slash = strrchr(ini_path, '\\');
	if (slash) *(slash + 1) = 0; // оставить только каталог exe

	strcat_s(ini_path, MAX_PATH, "settings.ini"); // concatenate ini file name

	GetPrivateProfileStringA(
		"INJECTOR",        // секция
		"PATH_TO_DLL",     // ключ
		"",     // значение по умолчанию
		path_to_dll,       // буфер
		sizeof(path_to_dll),
		ini_path       // путь к ini
	);

	GetPrivateProfileStringA(
		"INJECTOR",        // секция
		"PROCESS_NAME",     // ключ
		"",     // значение по умолчанию
		process_name,       // буфер
		sizeof(process_name),
		ini_path       // путь к ini
	);

	printf("DLL Path: %s\n", path_to_dll);
	printf("Process Name: %s\n", process_name);
}

DWORD select_pid(char process_name[]) {
	DWORD processes[3072], needed;
	DWORD pid;
	char target[256] = { 0 };
	strcpy_s(target, sizeof(target), process_name);

	if (!EnumProcesses(processes, sizeof(processes), &needed))
	{
		printf_red("Error: EnumProcesses.\n");
		return 1;
	}

	DWORD count = needed / sizeof(DWORD);

	for (DWORD i = 0; i < count; i++)
	{
		pid = processes[i];
		if (pid == 0)
			continue;

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (hProcess)
		{
			char exeName[MAX_PATH] = { 0 };

			if (GetProcessImageFileNameA(hProcess, exeName, MAX_PATH))
			{
				const char* base = strrchr(exeName, '\\'); // ищет последнее вхождение '\'
				base = base ? base + 1 : exeName; // сместить указатель на следующий символ после '\', или использовать весь путь, если '\' не найден
				if (_stricmp(base, target) == 0)
				{
					CloseHandle(hProcess);
					return pid;
				}
			}
			CloseHandle(hProcess);
		}
	}
	return 0;
}



int start_app()
{
	printf_green("1. Load settings.ini file...\n");
	select_dll_file();
	if (process_name[0] == '\0')
	{
		printf_red("Process name is empty. Please check settings.ini file.\n");
		return 1;
	}
	if (path_to_dll[0] == '\0')
	{
		printf_red("DLL path is empty. Please check settings.ini file.\n");
		return 1;
	}
	printf_green("2. Find process id\n");
	DWORD pid = select_pid(process_name);
	if (pid == 0)
	{
		printf_red("Process '%s' not found. Please make sure the process is running.\n", process_name);
		return 1;
	}
	printf("Process ID: %d\n", pid);
	printf_green("3. Inject DLL into process...\n");
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
	{
		printf("Failed to open process. Error: % d\n", GetLastError());
		return 1;
	}
	LPVOID allocated_mem = VirtualAllocEx(hProcess, NULL, strlen(path_to_dll) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (allocated_mem == NULL)
	{
		printf("Failed to allocate memory in target process. Error: % d\n", GetLastError());
		CloseHandle(hProcess);
		return 1;
	}
	WriteProcessMemory(hProcess, allocated_mem, path_to_dll, strlen(path_to_dll) + 1, NULL);
	HMODULE kernel32Base = GetModuleHandleA("kernel32.dll");
	FARPROC load_library_address = GetProcAddress(kernel32Base, "LoadLibraryA");
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)load_library_address, allocated_mem, 0, NULL);
	if (hThread == NULL)
	{
		printf("Failed to create remote thread. Error: % d\n", GetLastError());
		VirtualFreeEx(hProcess, allocated_mem, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return 1;
	}
	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(hProcess, allocated_mem, 0, MEM_RELEASE);
	CloseHandle(hThread);
	return 0;
}