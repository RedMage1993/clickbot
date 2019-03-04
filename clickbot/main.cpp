#pragma comment(lib, "Winmm")

#include <Windows.h>
#include <WinBase.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

bool improveSleepAcc(bool activate = true);
void closeThread(HANDLE& hThread);

DWORD WINAPI recordProc(LPVOID lpParameter);
DWORD WINAPI playbackProc(LPVOID lpParameter);

DWORD WINAPI playbackCoreProc(LPVOID lpParameter);

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

HANDLE hRecordProc = 0;
HANDLE hPlaybackProc = 0;

HANDLE hPlaybackCoreProc = 0;

enum State {
	idling, recording, playing
};

State state = idling;

void installHook();
void uninstallHook();

HHOOK hLLMouseHook = 0;

struct LeftClick {
	POINT pt;
	DWORD wait;
};

vector<LeftClick> recorded;
DWORD lastLeftClickTime = 0;

int main()
{
	char enter;

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

	return 0;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT *msllHookStruct;

	if (nCode >= 0 && wParam == WM_LBUTTONDOWN && state == recording)
	{
		msllHookStruct = reinterpret_cast<MSLLHOOKSTRUCT*> (lParam);

		// TODO: - when storing the time, you want it to be interpreted as "time to wait until next click"
		
	}

	return CallNextHookEx(hLLMouseHook, nCode, wParam, lParam);
}

void installHook()
{
	if (hLLMouseHook != 0) { return; }

	recorded.clear();

	hLLMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, 0, 0);
}

void uninstallHook()
{
	if (hLLMouseHook == 0) { return; }

	UnhookWindowsHookEx(hLLMouseHook);
	hLLMouseHook = 0;
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
	if (hThread == 0) { return; }

	DWORD dwExitCode;

	GetExitCodeThread(hThread, &dwExitCode);
	TerminateThread(hThread, dwExitCode);
	CloseHandle(hThread);
	hThread = 0;
}

DWORD WINAPI recordProc(LPVOID lpParameter)
{
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

		switch (state) {
		case idling:
			installHook();
			state = recording;
			break;
		case recording:
			uninstallHook();
			state = idling;
			break;
		case playing:
			closeThread(hPlaybackCoreProc);
			installHook();
			state = recording;
			break;
		}
	}

	return 0;
}

DWORD WINAPI playbackProc(LPVOID lpParameter)
{
	bool active = true;

	UNREFERENCED_PARAMETER(lpParameter);

	while (active)
	{
		while (!(GetAsyncKeyState(VK_F4) & 0x8000))
		{
			improveSleepAcc(true);
			Sleep(40);
			improveSleepAcc(false);
		}

		switch (state) {
		case idling:
			if (recorded.size() == 0) { break; }
			hPlaybackCoreProc = CreateThread(0, 0, playbackCoreProc, 0, 0, 0);
			state = playing;
			break;
		case recording:
			uninstallHook();
			hPlaybackCoreProc = CreateThread(0, 0, playbackCoreProc, 0, 0, 0);
			state = playing;
			break;
		case playing:
			closeThread(hPlaybackCoreProc);
			break;
		}
	}

	return 0;
}

DWORD WINAPI playbackCoreProc(LPVOID lpParameter)
{
	UNREFERENCED_PARAMETER(lpParameter);

	if (recorded.size() == 0) {
		state = idling;
		return;
	}

	// playback recorded clicks

	state = idling;

	return 0;
}