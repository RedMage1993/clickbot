#pragma comment(lib, "Winmm")

#include <Windows.h>
#include <WinBase.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>

using namespace std;

bool improveSleepAcc(bool activate = true);
void closeThread(HANDLE& hThread);

DWORD WINAPI recordProc(LPVOID lpParameter);
DWORD WINAPI playbackProc(LPVOID lpParameter);

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

HANDLE hRecordProc = 0;
HANDLE hPlaybackProc = 0;

enum State {
	idling, recording, playing
};

State state = idling;

void installHook();
void uninstallHook();

HHOOK hLLMouseHook = 0;

int main()
{
	char enter;
	HANDLE hKillProc;

	srand(static_cast<unsigned int> (time(reinterpret_cast<time_t*> (NULL))));

	cout << "Clickbot by Fritz Ammon.\n\n";
	cout << std::boolalpha;

	cout << "  F2 to start recording clicks, F4 to playback, F6 to interrupt playback\n";
	cout << "  Press Enter to shut down\n";

	hRecordProc = CreateThread(0, 0, recordProc, 0, 0, 0);
	hPlaybackProc = CreateThread(0, 0, playbackProc, 0, 0, 0);

	cin >> enter;
	cout << endl;

	cin.clear();
	cin.ignore(200, '\n');

	closeThread(hRecordProc);
	closeThread(hPlaybackProc);
	closeThread(hKillProc);

	return 0;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0 && wParam == WM_LBUTTONDOWN)
	{

	}

	return CallNextHookEx(hLLMouseHook, nCode, wParam, lParam);
}

void installHook()
{
	hLLMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, 0, 0);
}

void uninstallHook()
{
	UnhookWindowsHookEx(hLLMouseHook);
}

bool improveSleepAcc(bool activate)
{
	TIMECAPS tc;
	MMRESULT mmr;

	// Fill the TIMECAPS structure.
	if (timeGetDevCaps(&tc, sizeof(tc)) != MMSYSERR_NOERROR)
		return false;

	if (activate)
		mmr = timeBeginPeriod(tc.wPeriodMin);
	else
		mmr = timeEndPeriod(tc.wPeriodMin);

	if (mmr != TIMERR_NOERROR)
		return false;

	return true;
}

void closeThread(HANDLE& hThread)
{
	DWORD dwExitCode;

	GetExitCodeThread(hThread, &dwExitCode);
	TerminateThread(hThread, dwExitCode);
	CloseHandle(hThread);
	hThread = 0;
}

DWORD WINAPI recordProc(LPVOID lpParameter)
{
	DWORD dwExtraInfo = GetMessageExtraInfo();
	bool active = true;

	UNREFERENCED_PARAMETER(lpParameter);

	while (active)
	{
		while (!(GetAsyncKeyState(VK_F2) & 0x8000))
		{
			improveSleepAcc(true);
			Sleep(40);
			improveSleepAcc(false);
		}

		// TODO: - check state for appropriate action (if idling, then record. if recording, then stop recording. if playing, then stop playing and start recording.)

	}

	return 0;
}

DWORD WINAPI playbackProc(LPVOID lpParameter)
{
	bool active = true;

	UNREFERENCED_PARAMETER(lpParameter);

	while (active)
	{
		while (!(GetAsyncKeyState(VK_F4) & 0x8000)) // || clicks.length == 0
		{
			improveSleepAcc(true);
			Sleep(40);
			improveSleepAcc(false);
		}

		// TODO: - playback. if idling, check if clicks.length == 0, if not then play. if recording, then stop recording and start playing. if playing, then stop playing.
	}

	return 0;
}