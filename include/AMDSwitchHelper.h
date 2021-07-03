#pragma once
#include <Windows.h>
#include <string>

#define GPU_PERFORMANCE_VALUE L"HighPerfGPUAffinity"
#define GPU_POWERSAVE_VALUE L"PowerSavGPUAffinity"
#define GPU_BASED_ON_POWER_SOURCE_VALUE L"GlobalGPUAffinity"
#define MAX_BACKUP_COUNT 5

typedef enum class PowerMode {
	GPU_PERFORMANCE,
	GPU_POWERSAVE,
	GPU_BASED_ON_POWER_SOURCE
};

struct BackupSearchResult {
	FILETIME creationTime = { };
	std::wstring fileName;
};