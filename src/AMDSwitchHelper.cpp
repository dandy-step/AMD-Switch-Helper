#include <windows.h>
#include <ShObjIdl_core.h>
#include <ShlObj.h>
#include <cstring>
#include <string>
#include <WinUser.h>
#include <wchar.h>
#include <processthreadsapi.h>
#include <strsafe.h>
#include <io.h>
#include <ctime>
#include <string>
#include <winreg.h>
#include <vector>
#include <algorithm>
#include <winsvc.h>
#include <winuser.h>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define UNICODE 1
#define GPU_PERFORMANCE_VALUE L"HighPerfGPUAffinity"
#define GPU_POWERSAVE_VALUE L"PowerSavGPUAffinity"
#define GPU_BASED_ON_POWER_SOURCE_VALUE L"GlobalGPUAffinity"
#define MAX_BACKUP_COUNT 5

using namespace std;

wchar_t workingDir[2048] = L"";
bool failed = false;
bool trimBackups = false;

struct BackupSearchResult {
	FILETIME creationTime = { };
	std::wstring fileName;
};

enum class PowerMode {
	GPU_PERFORMANCE,
	GPU_POWERSAVE,
	GPU_BASED_ON_POWER_SOURCE
};

typedef struct XMLAppEntry {
	wchar_t* filePath;
	wchar_t* exeName;
	PowerMode mode;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void AddApplicationToXMLFile(HWND, wchar_t*, PowerMode);

void CheckRegistryKeys() {
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

bool TimeSortFunction(BackupSearchResult in, BackupSearchResult _in) {
	return (in.creationTime.dwHighDateTime < _in.creationTime.dwHighDateTime);
}

bool TroublePathCheck() {
	//correct for &amp and other control characters
	//check for 0xFF1A, warn user
}

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nShowCmd) {
	WNDCLASSW windowClass = {};
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"AMDSwitchHelper";

	RegisterClass(&windowClass);
	HWND windowHandle = CreateWindow(windowClass.lpszClassName, windowClass.lpszClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if (windowHandle != 0) {
		ShowWindow(windowHandle, NULL);

		//query for necessary programs and paths
		//check that atiapfxx exists
		//check that there's a User.blb already

		HRESULT res;
		wchar_t* sys32Path = NULL;
		const wchar_t* fileName = L"\\atiapfxx.exe";
		const wchar_t* blobPath = L"\\ATI\\ACE\\APL\\User.blb";

		//get path for executable
		wstring test;
		GetModuleFileName(NULL, workingDir, test.max_size());

		//work on registry here, since we're stripping the exe name from the working path going forward
		CheckRegistryKeys();

		wcsncpy_s(workingDir, workingDir, (wcsrchr(workingDir, L'\\') + 1) - workingDir);

		res = SHGetKnownFolderPath(FOLDERID_System, 0, 0, (PWSTR*)&sys32Path);
		if (res == S_OK) {
			void* oldRedirectInfo = NULL;
			wchar_t filePath[1024] = L"";
			wcscat_s(filePath, sys32Path);
			wcscat_s(filePath, fileName);
			
			WIN32_FIND_DATA findData = {};
			Wow64DisableWow64FsRedirection(&oldRedirectInfo);

			HANDLE searchHandle = FindFirstFileW(filePath, &findData);
			while (searchHandle != INVALID_HANDLE_VALUE) {
				if (lstrcmpW(findData.cFileName, L"atiapfxx.exe") == 0) {

					res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
					if ((res != S_OK) && (res != S_FALSE) && (res != RPC_E_CHANGED_MODE)) {
						MessageBox(windowHandle, L"Failed to initialize COM, application will not run.", L"Error", MB_ICONERROR);
						return 0;
					}

					//kill CCC.exe, as we might get a race condition if it's running and sets its own profile for the app
					ShellExecute(windowHandle, NULL, L"taskkill", L"/F /T /IM CCC.exe", NULL, SW_HIDE);

					SHELLEXECUTEINFO shellExecInfo = {};
					shellExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
					shellExecInfo.hwnd = windowHandle;
					shellExecInfo.lpVerb = L"open";
					shellExecInfo.lpFile = filePath;
					shellExecInfo.lpParameters = L"-r -user -s User.xml";
					shellExecInfo.lpDirectory = workingDir;
					shellExecInfo.nShow = SW_HIDE;
					shellExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
					if (ShellExecuteEx(&shellExecInfo)) {
						if (shellExecInfo.hProcess) {
							WaitForSingleObject(shellExecInfo.hProcess, 30000);	//wait, we need the file to proceed
							CloseHandle(shellExecInfo.hProcess);
						} else {
							//this might happen
						}
					}
					else {
						//FAIL: process didn't launch
					}

					//check for file creation?

					//create folder for date and backup
					wchar_t backupDirName[1024] = L"";
					wchar_t timeString[512] = L"";
					__time64_t currTime;
					tm theTime;
					_time64(&currTime);
					_localtime64_s(&theTime, &currTime);
					_wasctime_s(timeString, &theTime);

					//strip colons from time string
					wchar_t* badCharPtr = wcschr(timeString, L':');
					while (badCharPtr != NULL) {
						*badCharPtr = L'_';
						badCharPtr = wcschr(timeString, L':');
					}

					//strip paragraph from time string
					badCharPtr = wcschr(timeString, L'\n');
					while (badCharPtr != NULL) {
						*badCharPtr = L'\0';
						badCharPtr = wcschr(timeString, L'\n');
					}

					lstrcatW(backupDirName, workingDir);
					lstrcatW(backupDirName, L"Backups\\");

					wchar_t backupDirFileSearchPattern[2048] = L"";		//for backup file cleanup later

					std::wstring backupFileSearchPattern;
					backupFileSearchPattern += backupDirName;

					lstrcatW(backupDirName, L"_");
					lstrcatW(backupDirName, timeString);
					lstrcatW(backupDirName, L"\\");

					if (!CreateDirectoryW(backupDirName, NULL)) {
						DWORD err = GetLastError();
						err = err;
					}

					//generate current User.xml at backup folder
					shellExecInfo.lpParameters = L"-r -user -s User.xml";
					shellExecInfo.lpDirectory = backupDirName;
					if (ShellExecuteEx(&shellExecInfo)) {
						if (shellExecInfo.hProcess) {
							WaitForSingleObject(shellExecInfo.hProcess, 30000);	//wait for command to conclude, since we need the file to proceed;
							CloseHandle(shellExecInfo.hProcess);
							//ShellExecute(windowHandle, L"open", filePath, L"-r -user -s User.xml", backupDirName, SW_HIDE);
						}
					}

					//check for backup XML creation

					wchar_t fullBlobPath[2048];
					GetEnvironmentVariableW(L"LocalAppData", fullBlobPath, sizeof(fullBlobPath));
					lstrcatW(fullBlobPath, blobPath);
					FILE* blobFile;
					_wfopen_s(&blobFile, fullBlobPath, L"rb");

					//copy existing .blb to backup folder
					if (blobFile) {
						fpos_t fileSize = NULL;
						fseek(blobFile, 0, SEEK_END);
						fgetpos(blobFile, &fileSize);
						rewind(blobFile);
						void* blobFileData = calloc(1, fileSize + 1);
						fread_s(blobFileData, fileSize + 1, fileSize, 1, blobFile);
						fclose(blobFile);

						FILE* blobFileBackup = NULL;
						lstrcatW(backupDirName, L"User.blb");
						_wfopen_s(&blobFileBackup, backupDirName, L"wb+");

						if (blobFileBackup) {
							fwrite(blobFileData, fileSize, 1, blobFileBackup);
							fclose(blobFileBackup);
						}

						free(blobFileData);
					}

					//check for backup blb creation


					//analyze backup folder, delete any entry older than the most recent 50 items
					//get number of backups

					std::wstring backupFolderPath = backupFileSearchPattern.c_str();
					backupFileSearchPattern += L"_*";
					WIN32_FIND_DATA backupFindData = {};
					HANDLE backupFileSearchHandle = FindFirstFile(backupFileSearchPattern.c_str(), &backupFindData);
					int fileCount = 0;
					while (backupFileSearchHandle != INVALID_HANDLE_VALUE) {
						fileCount++;
						if (!FindNextFileW(backupFileSearchHandle, &backupFindData)) {
							if (GetLastError() == ERROR_NO_MORE_FILES) {
								backupFileSearchHandle = INVALID_HANDLE_VALUE;
							} else {
								//failed, but not because it couldn't find more files
							}
						}
					}

					//if number of backups exceeds our max, grab all file creation timestamps
					if (trimBackups) {
						if (fileCount > MAX_BACKUP_COUNT) {
							backupFileSearchHandle = FindFirstFile(backupFileSearchPattern.c_str(), &backupFindData);
							vector<BackupSearchResult> backupTimestamps(fileCount);
							int fileIterator = 0;

							while (backupFileSearchHandle != INVALID_HANDLE_VALUE) {
								backupTimestamps[fileIterator].creationTime = backupFindData.ftCreationTime;
								backupTimestamps[fileIterator++].fileName += backupFindData.cFileName;
								if (!FindNextFile(backupFileSearchHandle, &backupFindData)) {
									if (GetLastError() == ERROR_NO_MORE_FILES) {
										backupFileSearchHandle = INVALID_HANDLE_VALUE;
									}
									else {}
								}
							}

							//sort timestamps
							sort(backupTimestamps.begin(), backupTimestamps.end(), TimeSortFunction);

							for (int i = 0; i < MAX_BACKUP_COUNT; i++) {
								wstring filePath = backupFolderPath.c_str();
								filePath += backupTimestamps[i].fileName;
								//filePath += L"\\";
								filePath += L'\0';
								SHFILEOPSTRUCT delFileStruct = {};
								delFileStruct.wFunc = FO_DELETE;
								delFileStruct.pFrom = filePath.c_str();
								delFileStruct.fFlags = FOF_NO_UI | FOF_ALLOWUNDO;
								//SHFileOperation(&delFileStruct);
								/*if (!RemoveDirectory(filePath.c_str())) {
									DWORD err = GetLastError();
									err = err;
								}*/
							}
							backupTimestamps = backupTimestamps;
						}
					}

					//tokenize args
					if (lstrlenW(pCmdLine) > 0) {
						wchar_t* exeCheck = wcsstr(pCmdLine, L".exe");
						if (exeCheck != NULL) {
							//we were given a path to an exe

							exeCheck = wcsstr(exeCheck, L" ");
							int pathSize = 0;
							if (exeCheck) {
								//found empty space after the path, use pointer to calculate path size and set pointer to start of args after path
								pathSize = (exeCheck - pCmdLine);
							} else {
								//we only have the path as argument, no power mode specificed (should we even support this?)
								pathSize = lstrlenW(pCmdLine);
							}

							wchar_t* filePath = (wchar_t*)calloc(1, sizeof(wchar_t) * pathSize + 1);
							(filePath, pCmdLine, pathSize);
							wcsncat_s(filePath, pathSize + 1, pCmdLine, pathSize);

							PowerMode mode = PowerMode::GPU_PERFORMANCE;	//high performance mode by default
							if (exeCheck) {
								//check for power mode and further args
								wchar_t* context = NULL;
								wchar_t* token = wcstok_s(exeCheck, L" ", &context);
								while (token != NULL) {
									if (!lstrcmpW(token, L"GPU_PERFORMANCE")) {
										mode = PowerMode::GPU_PERFORMANCE;
									} else if (!lstrcmpW(token, L"GPU_POWERSAVE")) {
										mode = PowerMode::GPU_POWERSAVE;
									} else if (!lstrcmpW(token, L"GPU_BASED_ON_POWER_SOURCE")) {
										mode = PowerMode::GPU_BASED_ON_POWER_SOURCE;
									}

									token = wcstok_s(NULL, L" ", &context);
								}
							}

							AddApplicationToXMLFile(windowHandle, filePath, mode);
						} else {
							//no path given, just return?
							return 0;
						}
					} else {
						//wchar_t filePath[] = L"C:\\Games\\Doraemon Story of Seasons\\DORaEMON STORY OF SEASONS.exe";
						wchar_t filePath[] = L"C:\\testêHÇ◊Çƒ\\nufile.exe";
						AddApplicationToXMLFile(windowHandle, filePath, PowerMode::GPU_POWERSAVE);
						pCmdLine = filePath;
					}

					wstring generateBlobCommand = L"";
					generateBlobCommand += L"-u -s NewUser.xml -b \"";
					generateBlobCommand += fullBlobPath;
					generateBlobCommand += L"\" -l blobgen.log";
					ShellExecute(windowHandle, L"open", filePath, generateBlobCommand.c_str(), workingDir, SW_SHOWMAXIMIZED);
					searchHandle = INVALID_HANDLE_VALUE;

					//must kill AMD External Events Utility service
					SC_HANDLE serviceManager = OpenSCManager(L"", SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
					if (serviceManager) {
						SC_HANDLE eventUtilityService = OpenService(serviceManager, L"AMD External Events Utility", SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_ENUMERATE_DEPENDENTS);
						if (eventUtilityService) {
							SERVICE_STATUS servStatus = {};
							if (QueryServiceStatus(eventUtilityService, &servStatus)) {
								bool serviceStopped = false;

								ControlService(eventUtilityService, SERVICE_CONTROL_STOP, &servStatus);
								while (!serviceStopped) {
									if (QueryServiceStatus(eventUtilityService, &servStatus)) {
										serviceStopped = (servStatus.dwCurrentState == SERVICE_STOPPED);
									} else {
										//somehow failed to query serv status!
										break;
									}

									Sleep(100);
								}

								if (serviceStopped) {
									if (!StartService(eventUtilityService, NULL, NULL)) {
										DWORD err = GetLastError();
										err = err;
									}
									//ControlService(eventUtilityService, SERVICE_CONTROL_ENABLE)
								} else {
									failed = true;
								}
							}
						}
					}

					if (!failed) {
						MessageBeep(MB_OK);
					} else {
						MessageBeep(MB_ICONHAND);
					}

				}
				else {
					if (!FindNextFileW(searchHandle, &findData)) {
						DWORD ERR = GetLastError();
						searchHandle = INVALID_HANDLE_VALUE;
					}
				}
			}
		}

		CoTaskMemFree(sys32Path);
	}
}

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
					} else {
						appEntries[currEntry] = (XMLAppEntry*)calloc(1, sizeof(XMLAppEntry));
						if (appEntries[currEntry]) {
							appEntries[currEntry]->filePath = trimmedPath;
							appEntries[currEntry]->exeName = exeName;
							appEntries[currEntry++]->mode = userMode;
						} else {
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

void* GenerateXMLAppEntries(XMLAppEntry** entries, int count) {
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

			wchar_t appEntryFormattedBuffer[2048] = L"";
			swprintf_s(appEntryFormattedBuffer, L"\t<application Title=\"+{path=%s}\" File=\"%s\">\n\t\t\t<use Area=\"PXDynamic\">%s</use>\n\t\t</application>\n\t\0", entries[i]->filePath, entries[i]->exeName, powerModeValue);
			wcscat_s((wchar_t*)data, (2048 * count) - lstrlenW((wchar_t*)data), appEntryFormattedBuffer);
		}
	}

	return data;
}

void AddApplicationToXMLFile(HWND wHandle, wchar_t* filePath, PowerMode mode) {
	wstring findPath = L"";
	findPath += workingDir;
	findPath += L"User.xml";
	WIN32_FIND_DATA findData = {};
	HANDLE searchResult = FindFirstFile(findPath.c_str(), &findData);
	if (searchResult != INVALID_HANDLE_VALUE) {
		if (lstrcmpW(findData.cFileName, L"User.xml") == 0) {
			//MessageBox(wHandle, L"Found User.xml", L"", NULL);
			FILE* xmlFile = NULL;
			_wfopen_s(&xmlFile, findPath.c_str(), L"rb");

			if (xmlFile != NULL) {
				fpos_t fileSize = NULL;
				fseek(xmlFile, 0, SEEK_END);
				fgetpos(xmlFile, &fileSize);
				rewind(xmlFile);

				if (fileSize > 0) {
					void* originalData = (wchar_t*)calloc(fileSize + 1, sizeof(wchar_t));
					if (originalData) {
						size_t elemsRead = fread(originalData, sizeof(wchar_t), fileSize, xmlFile);
						if (elemsRead == (fileSize / sizeof(wchar_t))) {
							void* fileStartData = NULL;
							void* fileEndData = NULL;

							void* appEntryStart = (void*)wcsstr((wchar_t*)originalData, L"<applications>");
							if (appEntryStart) {
								appEntryStart = (void*)(wcsstr((wchar_t*)appEntryStart, L">") + 1);
							} else {
								//couldn't find <applications> tag!
							}

							void* appEntryEnd = (void*)wcsstr((wchar_t*)originalData, L"</applications>");
							if (appEntryEnd) {
								fileStartData = calloc(1, ((char*)appEntryStart - (char*)originalData) + 2);
								fileEndData = calloc(1, (((char*)originalData + fileSize) - (char*)appEntryEnd) + 2);
							} else {
								//couldn't find </applications> tag!
							}

							if (fileStartData && fileEndData) {
								wcsncpy_s((wchar_t*)fileStartData, ((wchar_t*)appEntryStart - (wchar_t*)originalData) + 1, (wchar_t*)originalData, (wchar_t*)appEntryStart - (wchar_t*)originalData);
								wcsncpy_s((wchar_t*)fileEndData, ((((wchar_t*)originalData + (fileSize / sizeof(wchar_t)) - (wchar_t*)appEntryEnd)) + 1), (wchar_t*)appEntryEnd, (((wchar_t*)originalData + (fileSize / sizeof(wchar_t)) - (wchar_t*)appEntryEnd)));

								rewind(xmlFile);
								int entryCount = 0;
								XMLAppEntry** entries = ParseXMLEntries(xmlFile, &entryCount, filePath, mode);
								void* data = GenerateXMLAppEntries(entries, entryCount);

								FILE* newFile = NULL;
								wchar_t newFilePath[2048] = L"";
								lstrcatW(newFilePath, workingDir);
								lstrcatW(newFilePath, L"NewUser.xml");
								_wfopen_s(&newFile, newFilePath, L"wb+");
								if (newFile != NULL) {
									size_t startSize = lstrlenW((wchar_t*)fileStartData);
									size_t appDataSize = lstrlenW((wchar_t*)data);
									size_t endSize = lstrlenW((wchar_t*)fileEndData);

									wchar_t* finalFileData = (wchar_t*)calloc(startSize + appDataSize + endSize + 1, sizeof(wchar_t));
									if (finalFileData) {
										wcsncat_s(finalFileData, (startSize + appDataSize + endSize) + 1, (wchar_t*)fileStartData, startSize);
										wcsncat_s(finalFileData, (startSize + appDataSize + endSize) + 1, (wchar_t*)data, appDataSize);
										wcsncat_s(finalFileData, (startSize + appDataSize + endSize) + 1, (wchar_t*)fileEndData, endSize);
									} else {
										//couldn't allocate memory for final file!
									}

									fwrite(fileStartData, lstrlenW((wchar_t*)fileStartData) * sizeof(wchar_t), 1, newFile);
									fwrite(data, lstrlenW((wchar_t*)data) * sizeof(wchar_t), 1, newFile);
									fwrite(fileEndData, lstrlenW((wchar_t*)fileEndData) * sizeof(wchar_t), 1, newFile);
									fflush(newFile);
									fclose(newFile);
								}

								free(fileStartData);
								free(fileEndData);
								free(data);
							}
						}
					}

					free(originalData);
				}

				fclose(xmlFile);
			}
		}
	}
	return;
}