#pragma once

#include<windows.h>
#include <stdio.h>
#include <winbase.h>
#include <Commdlg.h>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>
#include "PrintfColoring.h"

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



void start_app() {

}