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

struct MouseClick {
	POINT pt;
	DWORD wait;
	WPARAM type;
};

vector<MouseClick> recorded;
DWORD lastMouseClickTime = 0;

void inputFromMouseClick(const MouseClick& mouseClick, INPUT& input);
void sendInputFromMouseClicks();

int main()
{
	MSG msg;

	srand(static_cast<unsigned int> (time(reinterpret_cast<time_t*> (NULL))));

	cout << "Clickbot by Fritz Ammon.\n\n";
	cout << std::boolalpha;

	cout << "  F2 to start recording clicks, F4 to playback\n";

	hRecordProc = CreateThread(0, 0, recordProc, 0, 0, 0);
	hPlaybackProc = CreateThread(0, 0, playbackProc, 0, 0, 0);

	installHook();

	while (GetMessage(&msg, 0, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//uninstallHook();
	//closeThread(hRecordProc);
	//closeThread(hPlaybackProc);

	return 0;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT *msllHookStruct;

	if (nCode >= 0 && (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) && state == recording)
	{
		msllHookStruct = reinterpret_cast<MSLLHOOKSTRUCT*> (lParam);

		MouseClick mouseClick;

		mouseClick.pt = msllHookStruct->pt;
		mouseClick.type = wParam;

		if (lastMouseClickTime == 0) {
			mouseClick.wait = 0;
		} else {
			mouseClick.wait = msllHookStruct->time - lastMouseClickTime;
		}

		lastMouseClickTime = msllHookStruct->time;

		recorded.push_back(mouseClick);

		cout << "\nRecorded " << ((mouseClick.type == WM_LBUTTONDOWN) ? "left " : "right ") << "click (" << mouseClick.pt.x << ", " << mouseClick.pt.y << ") at " << lastMouseClickTime;
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

void clearRecordedMouseClicks()
{
	recorded.clear();
	lastMouseClickTime = 0;
	cout << "\nCleared recorded mouse clicks";
}

void installHook()
{
	if (hLLMouseHook != 0) { return; }

	hLLMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(0), 0);
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
			/*installHook();*/
			clearRecordedMouseClicks();
			state = recording;
			cout << "\nRecording...";
			break;
		case recording:
			/*uninstallHook();*/
			state = idling;
			cout << "\nIdling...";
			break;
		case playing:
			closeThread(hPlaybackCoreProc);
			clearRecordedMouseClicks();
			/*installHook();*/
			state = recording;
			cout << "\nRecording...";
			break;
		}

		improveSleepAcc(true);
		Sleep(1000);
		improveSleepAcc(false);
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
			cout << "\nPlaying...";
			break;
		case recording:
			/*uninstallHook();*/
			hPlaybackCoreProc = CreateThread(0, 0, playbackCoreProc, 0, 0, 0);
			state = playing;
			cout << "\nPlaying...";
			break;
		case playing:
			closeThread(hPlaybackCoreProc);
			state = idling;
			cout << "\nIdling...";
			break;
		}

		improveSleepAcc(true);
		Sleep(1000);
		improveSleepAcc(false);
	}

	return 0;
}

void inputFromMouseClick(const MouseClick& mouseClick, INPUT& input)
{
	DWORD extraInfo = GetMessageExtraInfo();
	LONG screenWidth = GetSystemMetrics(0);
	LONG screenHeight = GetSystemMetrics(1);
	const LONG ABSOLUTE_MAX_COORDINATE = 65535;

	SecureZeroMemory(&input, sizeof(INPUT));

	input.type = INPUT_MOUSE;

	input.mi.dwExtraInfo = extraInfo;

	// Normalization
	input.mi.dx = static_cast<double> (mouseClick.pt.x) / screenWidth * ABSOLUTE_MAX_COORDINATE;
	input.mi.dy = static_cast<double> (mouseClick.pt.y) / screenHeight * ABSOLUTE_MAX_COORDINATE;

	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
}

void sendInputFromMouseClicks()
{
	INPUT inputs[3];

	for (int i = 0; i < recorded.size(); i++) {
		improveSleepAcc(true);
		Sleep(recorded[i].wait);
		improveSleepAcc(false);

		inputFromMouseClick(recorded[i], inputs[0]);
		inputs[0].mi.dwFlags |= MOUSEEVENTF_MOVE;

		inputFromMouseClick(recorded[i], inputs[1]);
		inputs[1].mi.dwFlags |= (recorded[i].type == WM_LBUTTONDOWN) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;

		inputFromMouseClick(recorded[i], inputs[2]);
		inputs[2].mi.dwFlags |= (recorded[i].type == WM_LBUTTONDOWN) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;

		SendInput(3, inputs, sizeof(INPUT));

		cout << "\nClicking at (" << recorded[i].pt.x << ", " << recorded[i].pt.y << ")";
	}
}

DWORD WINAPI playbackCoreProc(LPVOID lpParameter)
{
	LPINPUT inputs = 0;
	UNREFERENCED_PARAMETER(lpParameter);

	if (recorded.size() == 0) {
		state = idling;
		cout << "\nIdling...";
		return 0;
	}

	cout << "\nPlaying back " << recorded.size() << " mouse events";

	sendInputFromMouseClicks();

	state = idling;
	cout << "\nIdling...";
	return 0;
}