#include "XMLParsing.h"

XMLAppEntry* CheckXMLEntryRepeat(XMLAppEntry** entries, int entryCount, wchar_t* filePath, wchar_t* exeName) {
	for (int i = 0; i < entryCount; i++) {
		if ((!lstrcmpW(entries[i]->filePath, filePath)) && (!lstrcmpW(entries[i]->exeName, exeName))) {
			return entries[i];
		}
	}

	return ((XMLAppEntry*)NULL);
}

XMLAppEntry** ParseXMLEntries(FILE* file, int* parsedEntryCount, wchar_t* userFilePath, PowerMode userMode) {
	XMLAppEntry** appEntries = (XMLAppEntry**)calloc(1024, sizeof(XMLAppEntry*));
	if (file) {
		fpos_t fileSize = 0;
		fseek(file, 0, SEEK_END);
		fgetpos(file, &fileSize);

		if (fileSize > 0) {
			bool finished = false;
			int currEntry = 0;
			rewind(file);

			wchar_t* entry = (wchar_t*)calloc(1, fileSize * 2);
			while (!finished) {
				fgetws(entry, fileSize * 2, file);
				wchar_t* entryStart = wcsstr(entry, L"<application Title=\"+{path=");
				if (entryStart != NULL) {
					wchar_t pathTag[] = L"+{path=";
					wchar_t exeNameTag[] = L"File=\"";
					wchar_t* pathStart = (wchar_t*)(wcsstr(entry, pathTag) + lstrlenW(pathTag));
					wchar_t* exeNameStart = (wchar_t*)(wcsstr(entry, exeNameTag) + lstrlenW(exeNameTag));

					if (pathStart && exeNameStart) {
						int pathDataSize = (wcsstr(entry, L"}\"") - pathStart);
						int exeDataSize = (wcsstr(entry, L"\">") - exeNameStart);

						wchar_t* pathData = (wchar_t*)calloc(1, (pathDataSize + 1) * sizeof(wchar_t));
						wchar_t* exeData = (wchar_t*)calloc(1, (exeDataSize + 1) * sizeof(wchar_t));
						PowerMode mode = PowerMode::GPU_PERFORMANCE;

						//get the <use>part for the GPU profile

						if (pathData && exeData) {
							wcsncpy_s(pathData, pathDataSize + 1, pathStart, pathDataSize);
							wcsncpy_s(exeData, exeDataSize + 1, exeNameStart, exeDataSize);

							//get next line, current GPU setting
							fgetws(entry, fileSize * 2, file);
							wchar_t* GPUSettingStart = wcsstr(entry, L"\"PXDynamic\">");
							if (GPUSettingStart) {
								GPUSettingStart = wcsstr(GPUSettingStart, L">");
								GPUSettingStart++;	//advance 1
								wchar_t* GPUSettingEnd = wcsstr(GPUSettingStart, L"</use>");
								if (GPUSettingEnd) {
									wchar_t GPUSettingData[2048] = L"";
									wcsncpy_s(GPUSettingData, GPUSettingStart, (GPUSettingEnd - GPUSettingStart));

									if (!lstrcmpW(GPU_PERFORMANCE_VALUE, GPUSettingData)) {
										mode = PowerMode::GPU_PERFORMANCE;
									}
									else if (!lstrcmpW(GPU_POWERSAVE_VALUE, GPUSettingData)) {
										mode = PowerMode::GPU_POWERSAVE;
									}
									else if (!lstrcmpW(GPU_BASED_ON_POWER_SOURCE_VALUE, GPUSettingData)) {
										mode = PowerMode::GPU_BASED_ON_POWER_SOURCE;
									}
									else {
										//uh oh, unrecognized value
									}
								}
								else {
									//ditto
								}
							}
							else {
								//failed to find GPUSetting! This should never happen!
							}

							if (!CheckXMLEntryRepeat(appEntries, currEntry, pathData, exeData)) {
								appEntries[currEntry] = (XMLAppEntry*)calloc(1, sizeof(XMLAppEntry));
								appEntries[currEntry]->filePath = pathData;
								appEntries[currEntry]->exeName = exeData;
								appEntries[currEntry++]->mode = mode;
							}
						}
					}
				}

				if (feof(file)) {
					finished = true;

					//check if executable path that user sent us is already an entry
					wchar_t* exeName = (wchar_t*)calloc(1, sizeof(wchar_t) * lstrlenW(userFilePath));
					wchar_t* trimmedPath = (wchar_t*)calloc(1, sizeof(wchar_t) * lstrlenW(userFilePath));
					wcsncpy_s(exeName, lstrlenW(userFilePath), (wchar_t*)(wcsrchr(userFilePath, L'\\') + 1), (wcsrchr(userFilePath, L'\\') + 1) - userFilePath);
					wcsncpy_s(trimmedPath, lstrlenW(userFilePath), userFilePath, wcsrchr(userFilePath, L'\\') - userFilePath);

					XMLAppEntry* existingValue = CheckXMLEntryRepeat(appEntries, currEntry, trimmedPath, exeName);

					//if it is, update GPU mode to what user passed; if not, add new entry
					if (existingValue) {
						existingValue->mode = userMode;
					}
					else {
						appEntries[currEntry] = (XMLAppEntry*)calloc(1, sizeof(XMLAppEntry));
						if (appEntries[currEntry]) {
							appEntries[currEntry]->filePath = trimmedPath;
							appEntries[currEntry]->exeName = exeName;
							appEntries[currEntry++]->mode = userMode;
						}
						else {
							//HANDLE: Failed to allocate entry
						}
					}

					*parsedEntryCount = currEntry;
				}
			}

			free(entry);
		}
	}

	return appEntries;
}

void* GenerateXMLAppEntries(HWND hWnd, XMLAppEntry** entries, int count) {
	void* data = calloc(count, sizeof(wchar_t) * 2048);;

	if (data) {
		for (int i = 0; i < count; i++) {
			wchar_t powerModeValue[2048] = L"";
			if (entries[i]->mode == PowerMode::GPU_PERFORMANCE) {
				wcscat_s(powerModeValue, GPU_PERFORMANCE_VALUE);
			}
			else if (entries[i]->mode == PowerMode::GPU_POWERSAVE) {
				wcscat_s(powerModeValue, GPU_POWERSAVE_VALUE);
			}
			else {
				wcscat_s(powerModeValue, GPU_BASED_ON_POWER_SOURCE_VALUE);
			}

			int test = lstrlenW(powerModeValue);

			TroubleUnicodeCheck(hWnd, (XMLAppEntry*)entries[i]);
			wchar_t appEntryFormattedBuffer[2048] = L"";
			swprintf_s(appEntryFormattedBuffer, L"\t<application Title=\"+{path=%s}\" File=\"%s\">\n\t\t\t<use Area=\"PXDynamic\">%s</use>\n\t\t</application>\n\t\0", entries[i]->filePath, entries[i]->exeName, powerModeValue);
			wcscat_s((wchar_t*)data, (2048 * count) - lstrlenW((wchar_t*)data), appEntryFormattedBuffer);
		}
	}

	return data;
}

//AMD's XML parser doesn't seem to handle certain characters correctly, for example, certain double-width unicode characters, and XML escape characters. This function handles the fixup by replacing the trouble characters or expanding the escape characters
bool TroubleUnicodeCheck(HWND windowHandle, XMLAppEntry* entry) {
	bool needToFixPath = false;
	wchar_t troubleCharacters[] = { 0x001A, 0x00FF, 0x001B };
	wchar_t escapeCharacters[] = { '&' };
	wstring fullPath = entry->filePath;
	wstring fixedPath;
	fullPath += L'\\';
	fullPath += entry->exeName;

	//check for troublesome unicode character cases
	for (int c = 0; c < fullPath.length(); c++) {
		fixedPath += fullPath[c];
		for (int i = 0; i < sizeof(troubleCharacters); i++) {
			if (wchar_t(fullPath[c] & 0x00FF) == (wchar_t)troubleCharacters[i]) {
				needToFixPath = true;
				fixedPath.back() = L'_';
			}
		}
	}

	//check for escape characters
	for (int i = 0; i < sizeof(escapeCharacters); i++) {
		size_t findPos = fixedPath.find(escapeCharacters[i]);
		while (findPos != std::wstring::npos) {
			wchar_t ampersandReplacement[] = L"&amp;";
			fixedPath.replace(findPos, 1, ampersandReplacement);
			findPos = fixedPath.substr(findPos + 1, fixedPath.length()).find('&');	//advance one
			needToFixPath = true;
		}
	}


	//if (needToFixPath) {
	//	//if path exists, ask user for permission to replace, else just replace
	//	bool fixPath = false;
	//	wstring infoMessage = L"Found entry with troublesome path \"";
	//	infoMessage += fullPath.c_str();

	//	WIN32_FIND_DATA findData = {};
	//	HANDLE fileHandle = FindFirstFile(fullPath.c_str(), &findData);
	//	if (fileHandle != INVALID_HANDLE_VALUE) {
	//		//ask for rename file permission
	//		infoMessage += L"\"\nThis path exists, but in order to process it, we need to rename the path\n(new path: ";
	//		infoMessage += fixedPath;
	//		infoMessage += L"\")";
	//		infoMessage += L"\nIs this acceptable? Alternatively, you could manually rename it outside of this application.";
	//		int promptValue = MessageBox(windowHandle, infoMessage.c_str(), L"", MB_ICONQUESTION | MB_YESNOCANCEL);
	//		if (promptValue == IDYES) {
	//			fixPath = true;
	//		}
	//	} else {
	//		infoMessage += L"\, which needs to be renamed in order to proceed.\nThis path doesn't currently exist, so it's likely an old application you have deleted or moved in the meantime. Renamed it to ";
	//		infoMessage += fixedPath;
	//		MessageBox(windowHandle, infoMessage.c_str(), L"", MB_ICONQUESTION);
	//		fixPath = true;
	//	}

	//	if (fixPath) {
	//		wstring newPath = fixedPath.substr(0, fixedPath.find_last_of(L'\\'));
	//		wstring newExeName = fixedPath.substr(fixedPath.find_last_of(L'\\') + 1);
	//		if (entry->filePath) {
	//			free(entry->filePath);
	//		}

	//		if (entry->exeName) {
	//			free(entry->exeName);
	//		}

	//		entry->filePath = (wchar_t*)calloc(newPath.length(), sizeof(wchar_t));
	//		entry->exeName = (wchar_t*)calloc(newExeName.length(), sizeof(wchar_t));
	//		lstrcatW(entry->filePath, newPath.c_str());
	//		lstrcatW(entry->exeName, newExeName.c_str());
	//	}
	//}

	return true;
}