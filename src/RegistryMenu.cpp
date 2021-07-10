#include "RegistryMenu.h"

bool CheckRegistryKeyInstall() {
	//check if our keys exist
	HKEY exeFileKey = NULL;
	if (RegOpenKeyEx(APP_MAIN_REG_KEY, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &exeFileKey) == ERROR_SUCCESS) {
		RegCloseKey(exeFileKey);
		return true;
	} else {
		return false;
	}
}

bool IsRegistryInstallCurrentPath(wchar_t* currWorkingDir) {
	bool res = false;
	HKEY performanceCmdKey = NULL;
	void* buff = NULL;

	if (RegOpenKeyEx(APP_PERFORMANCE_REG_KEY_COMMAND, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &performanceCmdKey) == ERROR_SUCCESS) {
		DWORD dataSize = 4096;
		buff = calloc(1, dataSize);
		if (buff) {
			HKEY perfCommandKey = NULL;
			if (RegGetValue(performanceCmdKey, NULL, NULL, RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, NULL, buff, &dataSize) == ERROR_SUCCESS) {
				wstring commandString = (wchar_t*)buff;
				if (commandString.rfind(currWorkingDir, 0) != wstring::npos) {
					res = true;
				}
			}
		}
	}

	if (buff) {
		free(buff);
	}

	return res;
}

void UninstallRegistryKeys() {
	//
}

void InstallRegistryKeys() {
	HKEY exefileKey = NULL;
	HKEY performanceKey = NULL;
	HKEY powersaveKey = NULL;
	HKEY basedOnPowerKey = NULL;
	DWORD keyDisposition = NULL;

	if (RegCreateKeyEx(APP_MAIN_REG_KEY, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &exefileKey, &keyDisposition) == ERROR_SUCCESS) {
		LSTATUS errCode = NULL;
		wchar_t appMUIVerb[] = L"AMDSwitchHelper";
		wchar_t appSubCommands[] = L"AMDSwitchHelper.GPUPerformance;AMDSwitchHelper.GPUPowersave;AMDSwitchHelper.GPUBasedOnPowerSource";
		errCode = RegSetValueEx(exefileKey, L"Icon", NULL, REG_SZ, (BYTE*)workingDir, sizeof(workingDir));
		errCode = RegSetValueEx(exefileKey, L"MUIVerb", NULL, REG_SZ, (BYTE*)appMUIVerb, sizeof(appMUIVerb));
		errCode = RegSetValueEx(exefileKey, L"SubCommands", NULL, REG_SZ, (BYTE*)appSubCommands, sizeof(appSubCommands));

		if (RegCreateKeyEx(APP_PERFORMANCE_REG_KEY, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &performanceKey, &keyDisposition) == ERROR_SUCCESS) {
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

		if (RegCreateKeyEx(APP_POWERSAVE_REG_KEY, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &powersaveKey, &keyDisposition) == ERROR_SUCCESS) {
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

		if (RegCreateKeyEx(APP_POWERSOURCE_REG_KEY, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &basedOnPowerKey, &keyDisposition) == ERROR_SUCCESS) {
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