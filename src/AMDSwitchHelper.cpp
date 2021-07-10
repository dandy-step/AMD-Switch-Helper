//global includes
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
#include <vector>
#include <algorithm>
#include <winsvc.h>
#include <winuser.h>
#include <windows.h>
using namespace std;

#define WIN32_LEAN_AND_MEAN
#define UNICODE 1

//global vars
const wchar_t* profilePathFileName = L"\\atiapfxx.exe";
const wchar_t* blobPath = L"\\ATI\\ACE\\APL\\User.blb";
wchar_t workingDir[2048] = L"";
bool failed = false;
bool trimBackups = false;

//personal project includes
#include "AMDSwitchHelper.h"
#include "RegistryMenu.h"
#include "RegistryMenu.cpp"
#include "XMLParsing.h"
#include "XMLParsing.cpp"

//forward declarations
void AddApplicationToXMLFile(HWND, wchar_t*, PowerMode);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool TimeSortFunction(BackupSearchResult in, BackupSearchResult _in) {
	return (in.creationTime.dwHighDateTime < _in.creationTime.dwHighDateTime);
}

bool CheckApplicationRequirements(HWND windowHandle) {
	//admin permission check
	HANDLE procTokenHandle = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &procTokenHandle)) {
		TOKEN_ELEVATION elevated;
		DWORD dataSize = NULL;
		if (GetTokenInformation(procTokenHandle, TokenElevation, &elevated, sizeof(TOKEN_ELEVATION), &dataSize)) {
			if (!elevated.TokenIsElevated) {
				MessageBoxA(windowHandle, "This application needs to run as administrator!", "Error", MB_ICONERROR);
				return false;
			}
		}
	}

	//AMD generator search
	wchar_t* sys32Path;
	HRESULT res = SHGetKnownFolderPath(FOLDERID_System, 0, 0, (PWSTR*)&sys32Path);
	if (res == S_OK) {
		void* oldRedirectInfo = NULL;
		wchar_t filePath[1024] = L"";
		wcscat_s(filePath, sys32Path);
		wcscat_s(filePath, profilePathFileName);

		WIN32_FIND_DATA findData = {};
		Wow64DisableWow64FsRedirection(&oldRedirectInfo);

		HANDLE genSearchHandle = FindFirstFile(filePath, &findData);
		if (genSearchHandle == INVALID_HANDLE_VALUE) {
			FindClose(genSearchHandle);
			MessageBox(windowHandle, L"Couldn't find required driver files. Your system is not compatible, or has an unsupported driver version.", L"Error", MB_ICONERROR);
			return false;
		}
	}
	 
	//User.blb exists check
	WIN32_FIND_DATA findData = {};
	wchar_t fullBlobPath[2048];
	GetEnvironmentVariableW(L"LocalAppData", fullBlobPath, sizeof(fullBlobPath) / sizeof(wchar_t));
	lstrcatW(fullBlobPath, blobPath);
	HANDLE blobFileHandle = FindFirstFile(fullBlobPath, &findData);
	if (blobFileHandle == INVALID_HANDLE_VALUE) {
		FindClose(blobFileHandle);
		MessageBox(windowHandle, L"Couldn't find existing profile blob - this is to be expected if you have just installed new drivers. Add at least one application manually through Catalyst Control Center to use this application. If you already have and are seeing this, it's likely that your driver version or hardware is unsupported.", L"Error", MB_ICONERROR);
		return false;
	}

	//AMD Event service exists check
	wchar_t servicePath[2048] = L"";
	//GetEnvironmentVariableW(L"LocalAppData", servicePath, sizeof(servicePath) / sizeof(wchar_t));
	wcscat_s(servicePath, sys32Path);
	lstrcatW(servicePath, L"\\atiesrxx.exe");
	HANDLE servFileHandle = FindFirstFile(sys32Path, &findData);
	if (servFileHandle == INVALID_HANDLE_VALUE) {
		FindClose(servFileHandle);
		MessageBox(windowHandle, L"Couldn't find AMD External Events service. It's likely that your driver version or hardware is unsupported.", L"Error", MB_ICONERROR);
		return false;
	}
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

		//get working path for executable
		GetModuleFileName(NULL, workingDir, sizeof(workingDir) / sizeof(wchar_t));

		if (!CheckApplicationRequirements(windowHandle)) {
			return 0;
		}

		//handle automatic install/uninstall if we run the application without args
		if (lstrlenW(pCmdLine) == 0) {
			if (CheckRegistryKeyInstall()) {
				if (!IsRegistryInstallCurrentPath(workingDir)) {
					InstallRegistryKeys();
					wstring msg = L"Updated application install path to ";
					msg += workingDir;
					MessageBox(windowHandle, msg.c_str(), L"Info", MB_ICONINFORMATION);
					return 0;
				}
				else {
					if (UninstallRegistryKeys()) {
						MessageBox(windowHandle, L"Unregistered application from right-click menu. Run this application again to reinstall it, or delete it to complete uninstallation.", L"Info", MB_ICONINFORMATION);
					} else {
						MessageBox(windowHandle, L"Failed to unregister application from right-click menu!", L"Error", MB_ICONERROR);
					}

					return 0;
				}
			}
			else {
				InstallRegistryKeys();
				MessageBox(windowHandle, L"Installed application!\nRight-click any executable or shortcut and use the AMDSwitchHelper menu to associate that application with a GPU.", L"Success!", MB_ICONINFORMATION);
				return 0;
			}
		}

		wcsncpy_s(workingDir, workingDir, (wcsrchr(workingDir, L'\\') + 1) - workingDir);

		HRESULT res;
		wchar_t* sys32Path = NULL;

		res = SHGetKnownFolderPath(FOLDERID_System, 0, 0, (PWSTR*)&sys32Path);
		if (res == S_OK) {
			void* oldRedirectInfo = NULL;
			wchar_t filePath[1024] = L"";
			wcscat_s(filePath, sys32Path);
			wcscat_s(filePath, profilePathFileName);
			
			WIN32_FIND_DATA findData = {};
			Wow64DisableWow64FsRedirection(&oldRedirectInfo);

			HANDLE searchHandle = FindFirstFile(filePath, &findData);
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
					GetEnvironmentVariableW(L"LocalAppData", fullBlobPath, sizeof(fullBlobPath) / sizeof(wchar_t));
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
								FindClose(backupFileSearchHandle);
							} else {
								//failed, but not because it couldn't find more files
							}
						}
					}

					//tokenize args
					if (lstrlenW(pCmdLine) > 0) {
						wchar_t* exeCheck = wcsstr(pCmdLine, L".exe");
						if (exeCheck != NULL) {
							//we were given a path to an exe

							exeCheck = wcsstr(exeCheck, L" ");
							rsize_t pathSize = 0;
							if (exeCheck) {
								//found empty space after the path, use pointer to calculate path size and set pointer to start of args after path
								pathSize = (exeCheck - pCmdLine);
							} else {
								//we only have the path as argument, no power mode specificed (should we even support this?)
								pathSize = lstrlenW(pCmdLine);
							}

							wchar_t* filePath = (wchar_t*)calloc(1, sizeof(wchar_t) * pathSize + 1);
							(filePath, pCmdLine, pathSize);
							if (filePath) {
								wcsncat_s(filePath, pathSize + 1, pCmdLine, pathSize);

								PowerMode mode = PowerMode::GPU_PERFORMANCE;	//high performance mode by default
								if (exeCheck) {
									//check for power mode and further args
									wchar_t* context = NULL;
									wchar_t* token = wcstok_s(exeCheck, L" ", &context);
									while (token != NULL) {
										if (!lstrcmpW(token, L"GPU_PERFORMANCE")) {
											mode = PowerMode::GPU_PERFORMANCE;
										}
										else if (!lstrcmpW(token, L"GPU_POWERSAVE")) {
											mode = PowerMode::GPU_POWERSAVE;
										}
										else if (!lstrcmpW(token, L"GPU_BASED_ON_POWER_SOURCE")) {
											mode = PowerMode::GPU_BASED_ON_POWER_SOURCE;
										}

										token = wcstok_s(NULL, L" ", &context);
									}
								}

								AddApplicationToXMLFile(windowHandle, filePath, mode);
							}
						} else {
							//no path given, just return?
							return 0;
						}
					} else {
						//wchar_t filePath[] = L"C:\\Games\\Doraemon Story of Seasons\\DORaEMON STORY OF SEASONS.exe";
						wchar_t filePath[] = L"C:\\testH‚×‚Ä\\nufile.exe";
						AddApplicationToXMLFile(windowHandle, filePath, PowerMode::GPU_POWERSAVE);
						pCmdLine = filePath;
					}

					wstring generateBlobCommand = L"";
					generateBlobCommand += L"-u -s NewUser.xml -b \"";
					generateBlobCommand += fullBlobPath;
					generateBlobCommand += L"\" -l blobgen.log";
					ShellExecute(windowHandle, L"open", filePath, generateBlobCommand.c_str(), workingDir, SW_SHOWMAXIMIZED);
					FindClose(searchHandle);
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
						FindClose(searchHandle);
						searchHandle = INVALID_HANDLE_VALUE;
					}
				}
			}
		}

		CoTaskMemFree(sys32Path);
	}
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
								void* data = GenerateXMLAppEntries(wHandle, entries, entryCount);

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
				FindClose(searchResult);
			}
		}
	}
	return;
}