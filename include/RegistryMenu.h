#pragma once
#include <windows.h>
#include <winreg.h>

//warning: these macros include the KEY + SUBKEY value (comma separated), so they expand to TWO parameters for the registry functions
#define APP_MAIN_REG_KEY HKEY_CLASSES_ROOT, L"exefile\\shell\\AMDSwitchHelper"
#define APP_PERFORMANCE_REG_KEY HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPerformance"
#define APP_PERFORMANCE_REG_KEY_COMMAND HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPerformance\\Command"
#define APP_POWERSAVE_REG_KEY HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPowersave"
#define APP_POWERSAVE_REG_KEY_COMMAND HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUPowersave\\Command"
#define APP_POWERSOURCE_REG_KEY HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUBasedOnPowerSource"
#define APP_POWERSOURCE_REG_KEY_COMMAND HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\AMDSwitchHelper.GPUBasedOnPowerSource\\Command"


bool CheckRegistryKeyInstall();

bool IsRegistryInstallCurrentPath(wchar_t* currWorkingDir);

void UninstallRegistryKeys();

void InstallRegistryKeys();