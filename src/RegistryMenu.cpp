#include "RegistryMenu.h"

void CheckAndUpdateRegistryKeys() {
	HKEY exefileKey = NULL;
	HKEY performanceKey = NULL;
	HKEY powersaveKey = NULL;
	HKEY basedOnPowerKey = NULL;
	DWORD keyDisposition = NULL;

	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, L"exefile\\shell\\AMDSwitchHelper", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &exefileKey, &keyDisposition) == ERROR_SUCCESS) {
		LSTATUS errCode = NULL;
		wchar_t appMUIVerb[] = L"AMDSwitchHelper";
		wchar_t appSubCommands[] = L"AMDSwitchHelper.GPUPerformance;AMDSwitchHelper.GPUPowersave;AMDSwitchHelper.GPUBasedOnPowerSource";
		errCode = RegSetValueEx(exefileKey, L"Icon", NULL, REG_SZ, (BYTE*)workingDir, sizeof(workingDir));
		errCode = RegSetValueEx(exefileKey, L"MUIVerb", NULL, REG_SZ, (BYTE*)appMUIVerb, sizeof(appMUIVerb));
		errCode = RegSetValueEx(exefileKey, L"SubCommands", NULL, REG_SZ, (BYTE*)appSubCommands, sizeof(appSubCommands));

		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPerformance", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &performanceKey, &keyDisposition) == ERROR_SUCCESS) {
			HKEY performanceCommand = NULL;

			wchar_t performanceOptionText[] = L"With Dedicated GPU";
			wchar_t* performanceIconPath = (wchar_t*)calloc(sizeof(workingDir), sizeof(wchar_t));
			lstrcatW(performanceIconPath, workingDir);
			lstrcatW(performanceIconPath, L",-1");

			RegSetValueEx(performanceKey, L"", NULL, REG_SZ, (BYTE*)performanceOptionText, sizeof(performanceOptionText));
			RegSetValueEx(performanceKey, L"Icon", NULL, REG_SZ, (BYTE*)performanceIconPath, lstrlenW(performanceIconPath) * sizeof(wchar_t));
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPerformance\\Command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &performanceCommand, &keyDisposition) == ERROR_SUCCESS) {
				wchar_t commandString[2048] = L"";
				lstrcatW(commandString, workingDir);
				lstrcatW(commandString, L" %L GPU_PERFORMANCE");

				RegSetValueEx(performanceCommand, L"", NULL, REG_SZ, (BYTE*)commandString, sizeof(commandString));
			}

			free(performanceIconPath);
		}

		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPowersave", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &powersaveKey, &keyDisposition) == ERROR_SUCCESS) {
			HKEY powersaveCommand = NULL;
			wchar_t powersaveOptionText[] = L"With Integrated GPU";
			wchar_t* powersaveIconPath = (wchar_t*)calloc(sizeof(workingDir), sizeof(wchar_t));
			lstrcatW(powersaveIconPath, workingDir);
			lstrcatW(powersaveIconPath, L",-2");

			RegSetValueEx(powersaveKey, L"", NULL, REG_SZ, (BYTE*)powersaveOptionText, sizeof(powersaveOptionText));
			RegSetValueEx(powersaveKey, L"Icon", NULL, REG_SZ, (BYTE*)powersaveIconPath, lstrlenW(powersaveIconPath) * sizeof(wchar_t));
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPowersave\\Command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &powersaveCommand, &keyDisposition) == ERROR_SUCCESS) {
				wchar_t commandString[2048] = L"";
				lstrcatW(commandString, workingDir);
				lstrcatW(commandString, L" %L GPU_POWERSAVE");

				RegSetValueEx(powersaveCommand, L"", NULL, REG_SZ, (BYTE*)commandString, sizeof(commandString));
			}

			free(powersaveIconPath);
		}

		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUBasedOnPowerSource", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &basedOnPowerKey, &keyDisposition) == ERROR_SUCCESS) {
			HKEY basedOnPowerCommand = NULL;
			wchar_t basedOnPowerOptionText[] = L"Based On Power Source";
			wchar_t* basedOnPowerIconPath = (wchar_t*)calloc(sizeof(workingDir), sizeof(wchar_t));
			lstrcatW(basedOnPowerIconPath, workingDir);
			lstrcatW(basedOnPowerIconPath, L",-3");

			RegSetValueEx(basedOnPowerKey, L"", NULL, REG_SZ, (BYTE*)basedOnPowerOptionText, sizeof(basedOnPowerOptionText));
			RegSetValueEx(basedOnPowerKey, L"Icon", NULL, REG_SZ, (BYTE*)basedOnPowerIconPath, lstrlenW(basedOnPowerIconPath) * sizeof(wchar_t));
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUBasedOnPowerSource\\Command", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &basedOnPowerCommand, &keyDisposition) == ERROR_SUCCESS) {
				wchar_t commandString[2048] = L"";
				lstrcatW(commandString, workingDir);
				lstrcatW(commandString, L" %L GPU_BASED_ON_POWER_SOURCE");

				RegSetValueEx(basedOnPowerCommand, L"", NULL, REG_SZ, (BYTE*)commandString, sizeof(commandString));
			}

			free(basedOnPowerIconPath);
		}
	}
}