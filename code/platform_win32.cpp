
#include "base.h"
#include "platform_win32.h"
#include "platform_main.h"

#include <intrin.h>
//#define WIN32_LEAN_AND_MEAN //TODO: (Ahmayk) do this
#include "Windows.h"
#include <minwinbase.h>
#include <minwindef.h>
#include <shobjidl.h>
#include "minidumpapiset.h"
#include "winhttp.h"
#include "sysinfoapi.h"

#include "nonRepo.h"

#if DEBUG_MODE
#include  "stdio.h"
#endif

struct WorkQueueEntry
{
	void *data;
	PlatformWorkQueueCallback *callback;
};

struct PlatformWorkQueue
{
	unsigned int volatile completionGoalIndex;
	unsigned int volatile nextInQueueIndex;
	HANDLE semaphoreHandle;
	WorkQueueEntry entries[256];
};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	for (;;)
	{
		PlatformWorkQueue *queue = (PlatformWorkQueue *)lpParameter;

		unsigned int nextInQueueIndex = queue->nextInQueueIndex;
		if (queue->completionGoalIndex != nextInQueueIndex)
		{
			ASSERT(queue->nextInQueueIndex < ARRAY_COUNT(queue->entries));
			unsigned int newNextEntryToRead = (queue->nextInQueueIndex + 1) % ARRAY_COUNT(queue->entries);
			unsigned int index = InterlockedCompareExchange((LONG volatile *)&queue->nextInQueueIndex, newNextEntryToRead, nextInQueueIndex);
			if (index == nextInQueueIndex)
			{
				WorkQueueEntry entry = queue->entries[index];
				ASSERT(entry.callback);
				entry.callback(queue, entry.data);
				// InterlockedIncrement((LONG volatile *)&queue->entryCompletionCount);
			}
		}
		else
		{
			WaitForSingleObjectEx(queue->semaphoreHandle, INFINITE, FALSE);
		}
	}
}

void PlatformAddThreadWorkEntry(PlatformWorkQueue *queue, PlatformWorkQueueCallback *callback, void *data)
{
	unsigned int countFoo = ARRAY_COUNT(queue->entries);

	ASSERT(queue->nextInQueueIndex < ARRAY_COUNT(queue->entries));

	WorkQueueEntry *entry = &queue->entries[queue->nextInQueueIndex];
	entry->data = data;
	entry->callback = callback;
	ASSERT(callback);
	ASSERT(data);

	queue->completionGoalIndex = ModNextU32(queue->completionGoalIndex, ARRAY_COUNT(queue->entries));

	_WriteBarrier();

	ReleaseSemaphore(queue->semaphoreHandle, 1, 0);
}

PlatformWorkQueue *SetupThreads(unsigned int threadCount, GameMemory *gameMemory)
{
	ASSERT(threadCount > 0);

	PlatformWorkQueue *result = ARENA_PUSH_STRUCT(&gameMemory->permanentArena, PlatformWorkQueue);

	unsigned int initialThreadCount = 0;
	result->semaphoreHandle = CreateSemaphoreExA(0, initialThreadCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

	for (u32 i = 0; i < threadCount; i++)
	{
		DWORD threadID;
		HANDLE threadHandle = CreateThread(0, 0, ThreadProc, result, 0, &threadID);
		CloseHandle(threadHandle);
	}

	return result;
}

unsigned int GetWCharLength(const wchar_t *chars)
{
	unsigned int result = {};
	while (chars[result])
	{ // could use wcslen() here, but Clang ASan's wcslen() can be bugged sometimes
		result++;
	}

	return result;
}

bool GetPngImageFilePathFromUser(char *buffer, unsigned int bufferSize, unsigned int *filePathLength)
{
	bool result = false;

	if (buffer && bufferSize > 0)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		IFileSaveDialog *pFileSaveDialog = NULL;

		if (SUCCEEDED(hr))
		{
			hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (void **)&pFileSaveDialog);
			if (SUCCEEDED(hr))
			{
				COMDLG_FILTERSPEC imageFileTypes[] = {
					{L"PNG", L"*.png"},
				};
				UINT fileTypeCount = sizeof(imageFileTypes) / sizeof(imageFileTypes[0]);

				hr = pFileSaveDialog->SetFileTypes(fileTypeCount, imageFileTypes);
				if (SUCCEEDED(hr))
				{
					hr = pFileSaveDialog->SetFileTypeIndex(0);
					if (SUCCEEDED(hr))
					{
						hr = pFileSaveDialog->Show(NULL);
						if (SUCCEEDED(hr))
						{
							IShellItem *pItem;
							hr = pFileSaveDialog->GetResult(&pItem);
							if (SUCCEEDED(hr))
							{
								PWSTR filePathW;
								hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePathW);
								pItem->Release();

								if (SUCCEEDED(hr))
								{
									u32 pathLen = GetWCharLength(filePathW);
									if (pathLen <= (bufferSize - 1))
									{
										WideCharToMultiByte(CP_UTF8, 0, filePathW, -1, buffer, bufferSize - 1, NULL, NULL);
										result = true;
										//TODO: append filetype if not there
										//TODO: (Marc) don't use strlen lol
										*filePathLength = (u32) strlen(buffer);
									}
									else
									{
										hr = E_NOT_SUFFICIENT_BUFFER;
									}
									CoTaskMemFree(filePathW);
								}
							}
						}
					}
				}
				pFileSaveDialog->Release();
			}
		}
	}

	return result;
}

void ExitChildProcess(PROCESS_INFORMATION pi)
{
	TerminateProcess(pi.hProcess, 1);                  // Terminate before detaching, so you don't see Windows Error Reporting.
	DebugActiveProcessStop(GetProcessId(pi.hProcess)); // Detach
	WaitForSingleObject(pi.hProcess, 2000);            // Wait for child to die, but not forever.
}

bool CreateDirectoryIfNotExists(const char *path)
{
	bool result = false;

	DWORD attributes = GetFileAttributesA(path);

	if ((attributes == INVALID_FILE_ATTRIBUTES && CreateDirectoryA(path, NULL)) ||
			(attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
	{
		result = true;
	}

	return result;
}

inline wchar_t *ConvertToWideString(String s)
{
	wchar_t *result = ARENA_PUSH_ARRAY(StringArena(), s.length, wchar_t);
	MultiByteToWideChar(CP_UTF8, 0, s.chars, -1, result, s.length);

	return result;
}

bool SendGodDammitErrorWindow(HWND hWnd, String error)
{
	String string = STRING("...oops, that didn't work. ") + error + " Try again?";
	wchar_t *stringW = ConvertToWideString(string);

	int windowsResult = MessageBoxW(hWnd, stringW, L"OH NO THAT DIDN'T WORK AAAAAAAAAAAAAAAAAAAAAAAAAAAA", MB_RETRYCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL);
	bool result = windowsResult == IDRETRY;

	return result;
}

#define SendGodDammitErrorWindowInLoop(hWnd, error) \
{                                               \
	if (SendGodDammitErrorWindow(hWnd, error))  \
	{                                           \
		continue;                               \
	}                                           \
	else                                        \
	{                                           \
		break;                                  \
	}                                           \
}

static String filenameString = {};

#define CRASH_ERROR_TITLE "OH FISHSTICKS WE JUST CRASHED"
#define CRASH_ERROR_TITLE_WIDE L"OH FISHSTICKS WE JUST CRASHED"

#define CRASH_ERROR_MESSAGE "CRAP! The game just crashed! That wasn't supposed to happen!!!"                     \
	"\n\nThis is a problem that needs to be fixed. "                                     \
	"Would you like to send an anonymous crash report to Ahmaykmewsik? "                 \
	"I can send it to them via Discord so they can fix whatever the hell just happened." \
	"\n\nIf you have a moment, please explain briefly what you were doing right before I crashed."

#define ErrBox(msg, flags) MessageBoxW(nullptr, L"" msg, CRASH_ERROR_TITLE_WIDE, flags | MB_SYSTEMMODAL | MB_SETFOREGROUND)
#define fatal_init_error(s) ErrBox("SORRY I FUCKED UP BAD. \n s \nPlease report this to Ahmaykmewsik.", MB_OK | MB_ICONERROR)

#define crash_report_failure(str)                                                                                \
	ErrBox(                                                                                                      \
			"OH CRAP. We done messed up BAD. "                                                                       \
			"We messed up so bad that I couldn't even make a crash report! "                                         \
			"Don't even ask, man, that really freaked me out.\n\n" str " \n\n"                                       \
			"Please report this to notfungamesdeveloper@gmail.com or to Ahmaykmewsik on Discord, if you know them.", \
			MB_OK | MB_ICONERROR);

const char *crashDirectory = "./crashdumps/";

static u32 CRASH_REPORT_WINDOW_MARGIN = 10;

static u32 CRASH_REPORT_DESC_HEIGHT = 175;
static u32 CRASH_REPORT_INPUT_HEIGHT = 100;
static u32 CRASH_REPORT_BUTTON_HEIGHT = 50;

static u32 CRASH_REPORT_DESC_YPOS = CRASH_REPORT_WINDOW_MARGIN;
static u32 CRASH_REPORT_INPUT_YPOS = CRASH_REPORT_DESC_YPOS + CRASH_REPORT_DESC_HEIGHT + CRASH_REPORT_WINDOW_MARGIN;
static u32 CRASH_REPORT_BUTTON_YPOS = CRASH_REPORT_INPUT_YPOS + CRASH_REPORT_INPUT_HEIGHT + CRASH_REPORT_WINDOW_MARGIN;

static iv2 CRASH_REPORT_WINDOW_DIM = iv2{600, 400};

static char CRASH_USER_DETAILS_CHARS[1500] = {};

static bool G_CRASH_REPORT_SEND_CRASH_DUMP_PLEASE = false;
static bool G_CRASH_REPORT_FUCKED_BY_USER = false;

static HWND G_HEDIT = {};

LRESULT CALLBACK EditProcCallack(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int MAX_TEXT_LENGTH = 1500;

	switch (uMsg)
	{
		case WM_CHAR:
			if (GetWindowTextLength(hwnd) >= MAX_TEXT_LENGTH && wParam != VK_BACK)
				return 0; // Ignore the character input when the maximum limit is reached
			break;

		case WM_PASTE:
			{
				unsigned int windowTextLength = GetWindowTextLength(hwnd);
				bool blockPaste = false;

				if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(hwnd))
				{
					HANDLE hClipboardData = GetClipboardData(CF_TEXT);
					if (hClipboardData != NULL)
					{
						char *pszClipboardText = static_cast<char *>(GlobalLock(hClipboardData));
						if (pszClipboardText != nullptr)
						{
							unsigned int clipboardLength = NullTerminatedCharLength(pszClipboardText);
							if (clipboardLength + windowTextLength > MAX_TEXT_LENGTH)
							{
								blockPaste = true;
							}

							// Release the global lock and close the clipboard
							GlobalUnlock(hClipboardData);
						}
					}

					CloseClipboard();
				}

				if (blockPaste)
					return 0;
			}
	}

	return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CrashReportWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			{
				// Create the font
				HFONT hFont = CreateFont(
						/* Specify the font attributes */
						-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						"Arial" /* Specify the font name */
						);

				HWND hStatic = CreateWindowEx(
						0, "STATIC", CRASH_ERROR_MESSAGE,
						WS_CHILD | WS_VISIBLE | SS_LEFT,
						CRASH_REPORT_WINDOW_MARGIN,
						CRASH_REPORT_DESC_YPOS,
						CRASH_REPORT_WINDOW_DIM.x - (CRASH_REPORT_WINDOW_MARGIN * 4),
						CRASH_REPORT_DESC_HEIGHT,
						hwnd, NULL, NULL, NULL);

				SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

				HFONT hFontSmaller = CreateFont(
						/* Specify the font attributes */
						-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						"Ariel" /* Specify the font name */
						);

				G_HEDIT = {};

				// Create an edit control
				G_HEDIT = CreateWindowEx(
						0, "EDIT", NULL,
						WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN,
						CRASH_REPORT_WINDOW_MARGIN,
						CRASH_REPORT_INPUT_YPOS,
						CRASH_REPORT_WINDOW_DIM.x - (CRASH_REPORT_WINDOW_MARGIN * 4),
						CRASH_REPORT_INPUT_HEIGHT,
						hwnd, NULL, NULL, NULL);

				SendMessage(G_HEDIT, WM_SETFONT, (WPARAM)hFontSmaller, MAKELPARAM(TRUE, 0));
				SetWindowLongPtr(G_HEDIT, GWLP_USERDATA, (LONG_PTR)GetWindowLongPtr(G_HEDIT, GWLP_WNDPROC));
				SetWindowLongPtr(G_HEDIT, GWLP_WNDPROC, (LONG_PTR)EditProcCallack);

				SetFocus(hwnd);

				break;
			}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					{
						if (G_HEDIT)
						{
							GetWindowTextA(G_HEDIT, CRASH_USER_DETAILS_CHARS, sizeof(CRASH_USER_DETAILS_CHARS));
							G_CRASH_REPORT_SEND_CRASH_DUMP_PLEASE = true;
						}
						DestroyWindow(hwnd);

						break;
					}
				case IDNO:
					{
						G_CRASH_REPORT_FUCKED_BY_USER = true;
						DestroyWindow(hwnd);
						break;
					}
				default:
					{
						//nuthin
						break;
					}
			}

			break;
		case WM_CHAR:
			{
				unsigned int len = GetWindowTextLength(G_HEDIT);
				if (G_HEDIT && len >= 1500)
					return 0;
				break;
			}
		case WM_CLOSE:
			// Close the window
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			// Quit the application
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//NOTE: crash report code modified from Phillip Trudeau-Tavara (thanks!)
//https://lance.handmade.network/blog/p/8491-automated_crash_reporting_in_basically_one_400-line_function

void CrashHandler(HINSTANCE instance, GameMemory *gameMemory)
{
	// Get the command line
	int argc = 0;
	wchar_t *commandLine = GetCommandLineW();
	if (!commandLine || !commandLine[0])
	{
		return; // Error: just run the app without a crash handler.
	}

	wchar_t **wargv = CommandLineToArgvW(commandLine, &argc); // Passing nullptr here crashes!
	if (wargv && wargv[0])
	{
		// Parse the command line for -no-crash-handler
		bool crashHandler = true;
		for (int i = 0; i < argc; ++i)
		{
			if (!wcscmp(wargv[i], L"-no-crash-handler"))
			{
				crashHandler = false;
				// We already *are* the subprocess - continue with the main program!
				return;
			}
		}
	}

	// Concatenate -no-crash-handler onto the command line for the subprocess

	int commandLineLength = GetWCharLength(commandLine);

	const wchar_t *append = L" -no-crash-handler";
	int appendLen = GetWCharLength(append);

	wchar_t *cmdNew = ARENA_PUSH_ARRAY(&gameMemory->temporaryArena, commandLineLength + appendLen + 1, wchar_t);
	if (!cmdNew)
	{
		return; // Error: just run the app without a crash handler.
	}

	memcpy(cmdNew, commandLine, commandLineLength * sizeof(wchar_t));
	memcpy(cmdNew + commandLineLength, append, appendLen * sizeof(wchar_t));

	for (;;)
	{
		// Parameters for starting the subprocess
		STARTUPINFOW siw = {};
		siw.cb = sizeof(siw);
		siw.dwFlags = STARTF_USESTDHANDLES;
		siw.hStdInput = GetStdHandle(STD_INPUT_HANDLE); // @Leak: CloseHandle()
		siw.hStdOutput = GetStdHandle(STD_ERROR_HANDLE);
		siw.hStdError = GetStdHandle(STD_OUTPUT_HANDLE);
		PROCESS_INFORMATION processInformation = {}; // @Leak: CloseHandle()

		SetEnvironmentVariableA("_NO_DEBUG_HEAP", "1");

		// Launch suspended, then read-modify-write the PEB (see below), then resume -p 2022-03-04
		if (!CreateProcessW(nullptr, cmdNew, nullptr, nullptr, true,
					CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS, nullptr, nullptr, &siw, &processInformation))
		{
			// If we couldn't create a subprocess, then just run the program without a crash handler.
			// That's not great, but it's presumably better than stopping the user from running at all!
			return;
		}

		// NOTE: SteamAPI_Init() takes WAY longer On My Machine(tm) when a debugger is present.
		//       (The DLL file steam_api64.dll does indeed call IsDebuggerPresent() sometimes.)
		//       It's clear that Steam does extra niceness for us when debugging, but we DO NOT
		//       want this to destroy our load times; I measure 3.5x slowdown (0.6s -> 2.1s).
		//       The only way I know to trick the child process into thinking it is free of a
		//       debugger is to clear the BeingDebugged byte in the Process Environment Block.
		//       If we are unable to perform this advanced maneuver, we will gracefully step back
		//       and allow Steam to ruin our loading times. -p 2022-03-04
#if 0
		auto persuade_process_no_debugger_is_present = [](HANDLE hProcess)
		{
			// Load NTDLL
			HMODULE ntdll = LoadLibraryA("ntdll.dll");
			if (!ntdll)
				return;

			// Get NtQueryInformationProcess function
			auto NtQueryInformationProcess = (/*__kernel_entry*/ NTSTATUS(*)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG))GetProcAddress(ntdll, "NtQueryInformationProcess");
			if (!NtQueryInformationProcess)
				return;

			// Query process information to find the PEB address
			PROCESS_BASIC_INFORMATION pbi = {};
			DWORD queryBytesRead = 0;
			if (NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &queryBytesRead) != 0 || queryBytesRead != sizeof(pbi))
				return;

			// Read the PEB of the child process
			PEB peb = {};
			SIZE_T processBytesRead = NULL;
			if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &processBytesRead) || processBytesRead != sizeof(peb))
				return;
			print_log("Child process's peb.BeingDebugged is %d, setting to 0...\n", peb.BeingDebugged);

			// Gaslight the child into believing we are not watching
			peb.BeingDebugged = 0;

			// Write back the modified PEB
			SIZE_T processBytesWritten = NULL;
			if (!WriteProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &processBytesWritten) || processBytesWritten != sizeof(peb))
				return;
		};
#endif
		// persuade_process_no_debugger_is_present(processInformation.hProcess);

		// Kick off the subprocess
		if (ResumeThread(processInformation.hThread) != 1)
		{
			ExitChildProcess(processInformation);
			fatal_init_error("Could not start main game thread");
			ExitProcess(1); // @Note: could potentially "return;" here instead if you wanted.
		}

		// Debugger loop: catch (and ignore) all debug events until the program exits or hits a last-chance exception
		HANDLE file = nullptr;
		for (;;)
		{
			// Print("---------Debugger strutting it's stuff right now you know it--------");

			// Get debug event
			DEBUG_EVENT debugEvent = {};
			if (!WaitForDebugEvent(&debugEvent, INFINITE))
			{
				ExitChildProcess(processInformation);
				fatal_init_error("Waiting for debug event failed");
				ExitProcess(1);
			}

			// If the process exited, nag about failure, or silently exit on success
			if (debugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT && debugEvent.dwProcessId == processInformation.dwProcessId)
			{
				// If the process exited unsuccessfully, prompt to restart it
				// @Todo: in these cases, no dump can be made, so upload just the stdout log and profiling trace
				if (debugEvent.u.ExitThread.dwExitCode != 0)
				{
					// Terminate & detach just to be safe
					ExitChildProcess(processInformation);

					// Prompt to restart
					MessageBeep(MB_ICONINFORMATION); // MB_ICONQUESTION makes no sound
					if (MessageBoxW(nullptr,
								L"The game had a fatal error and must close.\n"
								"Unfortunately, a crash report could not be generated. Sorry!\n"
								"Please report this to notfungamesdeveloper@gmail.com or to Ahmaykmewsik on Discord, if you know them.\n"
								"Restart the game?\n",
								CRASH_ERROR_TITLE_WIDE,
								MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL | MB_SETFOREGROUND) == IDYES)
					{
						return CrashHandler(instance, gameMemory);
					}
				}

				// Bubble up the failure code - this is where successful program runs will end up!
				ExitProcess(debugEvent.u.ExitThread.dwExitCode);
			}

			// If the process had some other debug stuff, we don't care.
			if (debugEvent.dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
			{
				ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
				continue;
			}

			// Skip first-chance exceptions or exceptions for processes we don't care about (shouldn't ever happen).
			if (debugEvent.u.Exception.dwFirstChance || debugEvent.dwProcessId != GetProcessId(processInformation.hProcess))
			{
				ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
				continue;
			}

			// By here, we have hit a real, last-chance exception. This is a crash we should generate a dump for.

			// Create crash dump directory
			if (!CreateDirectoryIfNotExists(crashDirectory))
			{
				ExitChildProcess(processInformation);
				crash_report_failure("I couldn't write the crash dump. Sorry! For the record, it's window's fault. Windows failed to...create the crash directory? "
						"Did you open this inside of a zip file? You can't do that! Unzip me first!");
				ExitProcess(1);
			}

			// Create crash dump filename
			FILETIME fileTime;
			GetSystemTimeAsFileTime(&fileTime);

			// Convert FILETIME to a 64-bit integer representing 100-nanosecond intervals since Jan 1, 1601
			ULARGE_INTEGER uli;
			uli.LowPart = fileTime.dwLowDateTime;
			uli.HighPart = fileTime.dwHighDateTime;
			unsigned long long unixTime = WINDOWS_FILETIME_TO_UNIXTIME(uli.QuadPart);
			filenameString = STRING("BadpaintCrashDump_") + U64ToString(unixTime, StringArena()) + ".dmp";

			// Create crash dump file
			String directoryToSaveFile = STRING(crashDirectory) + "/" + filenameString;
			wchar_t *directoryToSaveFileW = ConvertToWideString(directoryToSaveFile);
			file = CreateFileW(directoryToSaveFileW, GENERIC_WRITE | GENERIC_READ, 0, nullptr,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE)
			{
				ExitChildProcess(processInformation);
				crash_report_failure("I couldn't write the crash dump. Sorry! For the record, it's window's fault. Windows failed to create a file? "
						"Did you open this inside of a zip file? You can't do that! Unzip me first!");
				ExitProcess(1);
			}

			// Generate exception pointers out of excepting thread context
			CONTEXT c = {};
			if (HANDLE thread = OpenThread(THREAD_ALL_ACCESS, true, debugEvent.dwThreadId))
			{
				c.ContextFlags = CONTEXT_ALL;
				GetThreadContext(thread, &c);
				CloseHandle(thread);
			}
			EXCEPTION_POINTERS ep = {};
			ep.ExceptionRecord = &debugEvent.u.Exception.ExceptionRecord;
			ep.ContextRecord = &c;
			MINIDUMP_EXCEPTION_INFORMATION mei = {};
			mei.ThreadId = debugEvent.dwThreadId;
			mei.ExceptionPointers = &ep;
			mei.ClientPointers = false;

			// You could add some others here, but these should be good.
			int flags = MiniDumpNormal | MiniDumpWithHandleData | MiniDumpScanMemory | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData | MiniDumpWithThreadInfo | MiniDumpIgnoreInaccessibleMemory;

			// Write minidump
			if (!MiniDumpWriteDump(processInformation.hProcess, GetProcessId(processInformation.hProcess), file,
						(MINIDUMP_TYPE)flags, &mei, nullptr, nullptr))
			{
				ExitChildProcess(processInformation);
				crash_report_failure("I couldn't write the crash dump. Sorry! For the record, it's not my fault, it's Window's. Their MiniDumpWriteDump() function just failed for some reason!");
				ExitProcess(1);
			}

			// @Todo: ZIP compress the crash dump files here, with graceful fallback to uncompressed dumps.

			// Cleanup: Destroy subprocess now that we have a dump.
			// Note that we want to do this before doing any blocking interface dialogs,
			// because otherwise you would leave an arbitrarily broken program lying around
			// longer than you need to.
			ExitChildProcess(processInformation);
			break;
		}

		// Register the window class
		const char CLASS_NAME[] = "BadPaintCrashReport";

		WNDCLASS wc = {0};
		wc.lpfnWndProc = CrashReportWindowProc;
		wc.hInstance = instance;
		wc.lpszClassName = CLASS_NAME;

		RegisterClass(&wc);

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		iv2 windowPosition = {};
		windowPosition.x = RoundI32((screenWidth * 0.5f) - (CRASH_REPORT_WINDOW_DIM.x * 0.5f));
		windowPosition.y = RoundI32((screenHeight * 0.5f) - (CRASH_REPORT_WINDOW_DIM.y * 0.5f));

		// Create the window
		HWND hwnd = CreateWindowEx(
				WS_EX_TOPMOST,
				CLASS_NAME,
				CRASH_ERROR_TITLE,
				WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
				windowPosition.x,
				windowPosition.y,
				CRASH_REPORT_WINDOW_DIM.x,
				CRASH_REPORT_WINDOW_DIM.y,
				NULL,
				NULL,
				instance,
				NULL);

		if (hwnd == NULL)
		{
			ExitChildProcess(processInformation);
			crash_report_failure("I couldn't write the crash dump. Sorry! For the record it's not my fault, it's Window's. "
					"Windows failed to create the crash report window! (but can still show this dialogue box?!?)");
			ExitProcess(1);
		}

		G_CRASH_REPORT_FUCKED_BY_USER = false;
		G_CRASH_REPORT_SEND_CRASH_DUMP_PLEASE = false;

		if (hwnd)
		{
			// Create the font
			HFONT hFont = CreateFont(
					/* Specify the font attributes */
					-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
					ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					"Ariel" /* Specify the font name */
					);

			HWND hButtonSend = CreateWindowEx(0,
					"BUTTON",
					"Send Crash Report",
					WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					CRASH_REPORT_WINDOW_MARGIN,
					CRASH_REPORT_BUTTON_YPOS,
					200,
					CRASH_REPORT_BUTTON_HEIGHT,
					hwnd,
					(HMENU)IDOK,
					instance,
					NULL);

			SendMessage(hButtonSend, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

			HWND hButtonNo = CreateWindowEx(0,
					"BUTTON",
					"No!",
					WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					370,
					CRASH_REPORT_BUTTON_YPOS,
					200,
					CRASH_REPORT_BUTTON_HEIGHT,
					hwnd,
					(HMENU)IDNO,
					instance,
					NULL);

			SendMessage(hButtonNo, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

			ShowWindow(hwnd, SW_SHOW);

			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0) > 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		bool uploadedCrashDump = false;

		if (G_CRASH_REPORT_SEND_CRASH_DUMP_PLEASE)
		{
			// Setup window class for progress window
			WNDCLASSEXW wcex = {sizeof(wcex)};
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpszClassName = L"bar";
			wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
			wcex.hCursor = LoadCursor(GetModuleHandleA(nullptr), IDC_ARROW);
			wcex.lpfnWndProc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT
			{
				return m == WM_QUIT || m == WM_CLOSE || m == WM_DESTROY ? 0 : DefWindowProcW(h, m, w, l);
			};
			wcex.hInstance = GetModuleHandleA(nullptr);
			if (!RegisterClassExW(&wcex))
			{
				ExitProcess(1);
			}
			HWND hWnd = nullptr;
			HWND ctrl = nullptr;

			// Initialize common controls for progress bar
			INITCOMMONCONTROLSEX iccex = {sizeof(iccex)};
			iccex.dwICC = ICC_PROGRESS_CLASS;
			if (InitCommonControlsEx(&iccex))
			{
				// Create progress window and progress bar child-window
				hWnd = CreateWindowExW(0, wcex.lpszClassName, L"Uploading...",
						WS_SYSMENU | WS_CAPTION | WS_VISIBLE, CW_USEDEFAULT, SW_SHOW,
						320, 80, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);

				ctrl = CreateWindowExW(0, PROGRESS_CLASSW, L"",
						WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 10, 10,
						280, 20, hWnd, (HMENU)12345, GetModuleHandleA(nullptr), nullptr);
			}
			else
			{
				ExitProcess(1);
			}

			// Infinite loop: Attempt to upload the crash dump until the user cancels or it succeeds
			for (;;)
			{
				// Position the progress window to the centre of the screen
				RECT r;
				GetWindowRect(hWnd, &r);
				int ww = r.right - r.left, wh = r.bottom - r.top;
				int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
				SetWindowPos(hWnd, HWND_TOP, (sw - ww) / 2, (sh - wh) / 2, 0, 0, SWP_NOSIZE);

				// Helper function to set the loading bar to a certain position.
				auto update_loading_bar = [&](float amt)
				{
					if (hWnd && ctrl)
					{
						SendMessageW(ctrl, PBM_SETPOS, (WPARAM)(amt * 100), 0);
						ShowWindow(hWnd, SW_SHOW);
						UpdateWindow(hWnd);
						MSG msg = {};
						while (PeekMessageW(&msg, nullptr, 0, 0, 1) > 0)
						{
							TranslateMessage(&msg);
							DispatchMessageW(&msg);
						}
					}
				};

				float x = 0;
				update_loading_bar(x);

				// Build MIME multipart-form payload
				unsigned int discordMaxLength = 8000000;

				char *body = ARENA_PUSH_ARRAY(&gameMemory->temporaryArena, discordMaxLength, char);

				String discordMessageContent = STRING(":art: WE HAVE A CRASH REPORT from BADPAINT!! :art:") + "\\n `VERSION: " VERSION_NUMBER + "\\nBuild: " + buildDate + " " + buildTime + "` ";
				String userMessage = STRING(CRASH_USER_DETAILS_CHARS);
				if (userMessage.length)
					discordMessageContent += "```" + CleanStringForDiscord(userMessage, StringArena()) + "```";

				String header = STRING("--19024605111143684786787635207\r\n"
						"Content-Disposition: form-data; name=\"payload_json\"\r\n\r\n{\"content\":\"") +
					discordMessageContent +
					"\"}\r\n--19024605111143684786787635207\r\n"
					"Content-Disposition: form-data; name=\"files[0]\"; filename=\"" +
					filenameString +
					"\"\r\n"
					"Content-Type: application/octet-stream\r\n"
					"\r\n";

				unsigned int headerLength = header.length;

				String bodyPostfix = STRING("\r\n--19024605111143684786787635207--\r\n");

				memcpy(body, header.chars, header.length);

				update_loading_bar(x += 0.1f);

				// Get crash dump file size
				LARGE_INTEGER fileSizeInt = {};
				GetFileSizeEx(file, &fileSizeInt);
				u32 fileSize = (u32) fileSizeInt.QuadPart;
				if (fileSize >= discordMaxLength)
					SendGodDammitErrorWindowInLoop(hWnd, STRING("The crash dump file is too big for discord!"));

				update_loading_bar(x += 0.1f);

				// Seek file to start
				if (SetFilePointer(file, 0, nullptr, FILE_BEGIN) != 0)
					SendGodDammitErrorWindowInLoop(hWnd, STRING("Windows doesn't know how to handle files right now!"));

				// Copy entire file into the space after the body infix
				DWORD bytesRead = 0;
				if (!ReadFile(file, body + headerLength, fileSize, &bytesRead, nullptr))
					SendGodDammitErrorWindowInLoop(hWnd, STRING("Windows doens't know how to read files right now!"));

				if (bytesRead != fileSize)
					SendGodDammitErrorWindowInLoop(hWnd, STRING("Windows apparently thinks the crash dump is empty when it isn't!"));

				update_loading_bar(x += 0.1f);

				// Print the body postfix after the data file (overflow already checked)
				u32 bodyTotalSize = headerLength + fileSize + bodyPostfix.length;
				memcpy(body + headerLength + fileSize, bodyPostfix.chars, bodyPostfix.length);
				update_loading_bar(x += 0.1f);

				// Windows HTTPS stuff from here on out...
				HINTERNET hSession = WinHttpOpen(L"Discord Crashdump Webhook",
						WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
						WINHTTP_NO_PROXY_BYPASS, 0);
				if (!hSession)
				{
					WinHttpCloseHandle(hSession);
					SendGodDammitErrorWindowInLoop(hWnd, STRING("I couldn't connect to the discord webhook!"));
				};
				update_loading_bar(x += 0.1f);

				// Connect to domain
				HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
				if (!hConnect)
				{
					WinHttpCloseHandle(hConnect);
					SendGodDammitErrorWindowInLoop(hWnd, STRING("I couldn't connect to discord!"));
				};
				update_loading_bar(x += 0.1f);

				// Begin POST request to the discord webhook endpoint
				// NOTE: Discord webhook endpoint defined in noRepo.h and not in public repo so spammers won't see it!
				HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
						L"" DISCORD_WEBHOOK_ENDPOINT,
						nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
				if (!hRequest)
				{
					WinHttpCloseHandle(hRequest);
					SendGodDammitErrorWindowInLoop(hWnd, STRING("I couldn't open the HTTP request!"));
				};
				update_loading_bar(x += 0.1f);

				// Send request once - don't handle auth challenge, credentials, reauth, redirects
				const wchar_t ContentType[] = L"Content-Type: multipart/form-data; boundary=19024605111143684786787635207";
				if (!WinHttpSendRequest(hRequest, ContentType, ARRAY_COUNT(ContentType), body, bodyTotalSize, bodyTotalSize, 0))
				{
					SendGodDammitErrorWindowInLoop(hWnd, STRING("I couldn't send the HTTP request!"));
				}
				update_loading_bar(x += 0.1f);

				// Wait for response
				if (!WinHttpReceiveResponse(hRequest, nullptr))
					SendGodDammitErrorWindowInLoop(hWnd, STRING("I sent the crash report to discord, but didn't get a response back!"));

				update_loading_bar(x += 0.1f);

				DWORD dwStatusCode, dwSize = sizeof(dwStatusCode);
				// Pull headers from response
				if (!WinHttpQueryHeaders(hRequest,
							WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
							nullptr, &dwStatusCode, &dwSize, nullptr))
					SendGodDammitErrorWindowInLoop(hWnd, STRING("Windows couldn't make any sense of what discord sent back to me for some reason!"));

				if (dwStatusCode != 200)
				{
					String errorMsg = STRING("Discord said it didn't like what we sent it! It sent this status code: ") + U32ToString(dwStatusCode, StringArena());
					SendGodDammitErrorWindowInLoop(hWnd, errorMsg);
				}

				update_loading_bar(x += 0.1f);

				uploadedCrashDump = true;
				break;
			}

			// Cleanup
			if (hWnd)
			{
				DestroyWindow(hWnd);
			}
			UnregisterClassW(wcex.lpszClassName, GetModuleHandleA(nullptr));
		}

		// Cleanup
		CloseHandle(file);

		const wchar_t *uploadedString = (uploadedCrashDump)
			? L"Thank you! Restart?"
			: (G_CRASH_REPORT_FUCKED_BY_USER)
			? L"Alright. Shall I restart?"
			: L"Restart?";

		if (MessageBoxW(nullptr, uploadedString, L"Happy crash window",
					MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL | MB_SETFOREGROUND) != IDYES)
		{
			break;
		}
	}

	// Return 1 because the game crashed, not because the crash report failed
	ExitProcess(1);
}

int CALLBACK WinMain(HINSTANCE instance,
		HINSTANCE prevInstance,
		LPSTR commandLine,
		int showCode)
{

#if DEBUG_MODE
	AllocConsole();
	HWND consoleWindow = GetConsoleWindow();
	SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	GameMemory gameMemory = {};
	gameMemory.permanentArena = ArenaInit(MegaByte * 15);
	gameMemory.circularNotificationBuffer = ArenaInitFromArena(&gameMemory.permanentArena, MegaByte * 1);
	gameMemory.circularNotificationBuffer.flags |= ARENA_FLAG_CIRCULAR;

	gameMemory.temporaryArena = ArenaInit(MegaByte * 500);

	b32 memoryAllocSuccessful = gameMemory.permanentArena.memory && gameMemory.temporaryArena.memory;
	if (memoryAllocSuccessful)
	{
		//NOTE: (Ahmayk) memory is allocated and freed elsewhere when image is loaded
		//Memory allocated size is chosen based on size of image imported so that our memory
		//usage scales with the size of the image we're editing (which could be very large)

		CrashHandler(instance, &gameMemory);

		//NOTE: Thanks phillip and martins :D
		char *buffer = NULL;
		size_t len = 0;
		errno_t err = _dupenv_s(&buffer, &len, "_NO_DEBUG_HEAP");
		ASSERT(err == 0 && buffer != NULL); // Ensure no error and variable exists

		unsigned int threadCount = 8;

		SYSTEM_INFO systemInfo;
		GetNativeSystemInfo(&systemInfo);
		if (systemInfo.dwNumberOfProcessors)
			threadCount = systemInfo.dwNumberOfProcessors;

		PlatformWorkQueue *threadWorkQueue = SetupThreads(threadCount, &gameMemory);

		RunApp(threadWorkQueue, &gameMemory, threadCount);
	}
	else
	{
		MessageBoxW(nullptr, L"Failed to start badpaint. Where's your computer's memory, dude? I couldn't allocate what I needed to start the program.", L"you okay? get a drink of water", MB_OK | MB_ICONERROR| MB_SYSTEMMODAL | MB_SETFOREGROUND);
	}

	return 0;
}
