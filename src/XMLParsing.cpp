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

			TroubleUnicodeCheck(hWnd, entries[i]);
			wchar_t appEntryFormattedBuffer[2048] = L"";
			swprintf_s(appEntryFormattedBuffer, L"\t<application Title=\"+{path=%s}\" File=\"%s\">\n\t\t\t<use Area=\"PXDynamic\">%s</use>\n\t\t</application>\n\t\0", entries[i]->filePath, entries[i]->exeName, powerModeValue);
			wcscat_s((wchar_t*)data, (2048 * count) - lstrlenW((wchar_t*)data), appEntryFormattedBuffer);
		}
	}

	return data;
}

bool TroubleUnicodeCheck(HWND windowHandle, XMLAppEntry* entry) {
	wstring str = wstring(entry->filePath);
	size_t findPos = str.find((wchar_t)0xFF1A);

	if (findPos != wstring::npos) {
		wstring warnMessage = L"Tried to process name or path ";
		warnMessage.append(entry->filePath);
		warnMessage.append(L" with known troublesome characters. Please change the path or rename the executable.");

		MessageBox(windowHandle, warnMessage.c_str(), L"Error", MB_ICONERROR);
	}

	findPos = str.find(L'&');
	while (findPos != wstring::npos) {
		wchar_t ampersandReplacement[] = L"&amp";
		str.replace(findPos, 1, ampersandReplacement);
		findPos = str.find('&');
	}

	//correct for &amp and other control characters
	//check for 0xFF1A, warn user
}