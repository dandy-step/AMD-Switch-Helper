#pragma once
#include <stdio.h>
#include <io.h>
#include <wchar.h>
#include <stdlib.h>
#include <Windows.h>
#include "AMDSwitchHelper.h"

typedef struct {
	wchar_t* filePath;
	wchar_t* exeName;
	PowerMode mode;
} XMLAppEntry;

XMLAppEntry* CheckXMLEntryRepeat(XMLAppEntry** entries, int entryCount, wchar_t* filePath, wchar_t* exeName);

XMLAppEntry** ParseXMLEntries(FILE* file, int* parsedEntryCount, wchar_t* userFilePath, PowerMode userMode);

void* GenerateXMLAppEntries(HWND hWnd, XMLAppEntry** entries, int count);

bool TroubleUnicodeCheck(HWND windowHandle, XMLAppEntry* entry);

