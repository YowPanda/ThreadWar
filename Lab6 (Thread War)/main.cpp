// main.cpp

/*
	������������ ���� Thread War
	��� ���� ����������� ������� "�����" � "������", ����� ���������� �����
	��� �������� ����������� ������� "������"

	���� ���������, ���� 30 ������ ����� � ������ ���������������
	�� ������� ������� ���������� ������ �� 1 ����
*/


#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>


// ������� �������������
// ���������� ������ ���������� ������ ���� �����
HANDLE screenLock;
// M���� ���������� ������ ��� ���� ������
HANDLE bulletSem;
// ���� ���������� � �������� ������� "�����" ��� "������"
HANDLE startEvent;
// ����������� �������
HANDLE conIn, conOut;
// �������� �����
HANDLE mainThread;
// ����������� ������ ����� ����
CRITICAL_SECTION gameOver;
// ���������� � �������
CONSOLE_SCREEN_BUFFER_INFO conInfo; 
// ���������� ��������� � ��������
long hits = 0;					
long misses = 0;
// ����������� ��������
long delayFactor = 5; 


// �������� ���������� ����� � ���������� �� a �� b
int RandomValueBetween(int a, int b)
{
	if (a == 0 && b == 1) return rand() % 2;
	return rand() % (a - b) + a;
}


// ������� ������ �������
void ConsoleCleaning()
{
	// ��������� ��������� (������, �������)
	COORD coord = {0, 0};
	// ����� ����� ��� ������
	DWORD numberOfChar;
	// ���������� ����� ������� �� �������, ������� � ��������� (0, 0)
	FillConsoleOutputCharacter(conOut, ' ', conInfo.dwSize.X * conInfo.dwSize.Y, coord, &numberOfChar);
}


// ������� �� ������� (�, �) �������� ������
void WriteOnPosition(short x, short y, char ch)
{
	// ����������� ����� �� ����� ��� ������ ��������
	WaitForSingleObject(screenLock, INFINITE);
	// ���������� ���� ����� 
	COORD coord = {x,y};
	// ����� ����� ��� ������
	DWORD numberOfChar;
	// ������� ��� ��������, ������������ � �������� ����� (�, �)
	WriteConsoleOutputCharacter(conOut, &ch, 1, coord, &numberOfChar);
	// ������������ �������
	ReleaseMutex(screenLock);
}


// �������� ������� �� ������� (������� ����������� � repeatCount)
int Keystroke(int &repeatCount)
{
	// ���������� ��� ������ ������� ����� ������
	INPUT_RECORD inputBuffer;
	// ����� ����� ��� ������
	DWORD numberOfChar;

	// ����������� ����
	while (true)
	{
		// ������ ������ �� ������ ������� 
		ReadConsoleInput(conIn, &inputBuffer, 1, &numberOfChar);
		// ���������� �������, �� ���������� �������� ������
		if (inputBuffer.EventType != KEY_EVENT) continue;
		// ���������� ��, ��� ������� ��������� (��� ���������� ������ �������)
		if (!inputBuffer.Event.KeyEvent.bKeyDown) continue;
		// ������� ����������
		repeatCount = inputBuffer.Event.KeyEvent.wRepeatCount;

		// ���������� ����������� ��� �������
		return inputBuffer.Event.KeyEvent.wVirtualKeyCode;
	}
}


// ���������� ������ � �������� ������� ������
int GetOnPosition(short x, short y)
{
	// ������ � ������������ (�, �)
	char ch;
	DWORD numberOfChar;
	COORD coord = {x, y};
	// ��������� ������ � ������� �� ��� ���, ���� ��������� �� ����� ���������
	WaitForSingleObject(screenLock, INFINITE);
	// ������ ������ �� ������ �������
	ReadConsoleOutputCharacter(conOut, &ch, 1, coord, &numberOfChar);
	// ������������ �������
	ReleaseMutex(screenLock);

	// ���������� ������ � ������������ (�, �)
	return ch;
}


// ���������� ���� � ��������� ����, ��������� ������� ���������� ����
void Score()
{
	// ������ ��������� ����
	char str[128];
	// ���������� � ��� ���������� �������� � ���������
	sprintf_s(str, "Thread War! Hits:%d  Misses:%d", hits, misses);
	SetConsoleTitle(str);

	// ���� �������� 30 ��� ����� ������
	if (misses >= 30)
	{
		// ��������� ������ ����������� ������
		EnterCriticalSection(&gameOver);
		// ���������������� ������ �������� ������
		SuspendThread(mainThread);
		// ������� ���� � ����������� � ���������
		MessageBox(NULL, "Game Over!", "Thread War", MB_OK | MB_SETFOREGROUND);
		// �������� ���������� 
		exit(0);
	}

	// ���� ����� ��������� � �������� ������� �� 20 ��� �������
	if ((hits + misses) % 20 == 0)
		// ��������� ����������� ��������
		InterlockedDecrement(&delayFactor);
}

// �������� ������������� ����� 
char enemyTransformation[] = "-\\|/";

// ����� �����
void enemyThread(void* _y)
{
	// ��������� ���������� �� y
	int y = (int)_y;
	// ����� ��������� ����������� �����
	int direction;
	// ���������� ���������� �� �
	int x;

	// ������ ���, ����� ����� � ��������� ������������ ���������� �����, � � ������� - ������
	x = y % 2 ? 0 : conInfo.dwSize.X;
	// ������������� ����������� ����� � ����������� �� ��������� �������
	direction = x ? -1 : 1;

	// ���� ��������� ��������� � �������� ������
	while ((direction == 1 && x != conInfo.dwSize.X) || (direction == -1 && x != 0))
	{
		// ����������� ��������
		int delayFact;
		// ������ �� �� �����
		BOOL hitEnemy = FALSE;
		// �������� ��������� ���� �� �����
		if (GetOnPosition(x, y) == '*')
			hitEnemy = TRUE;
		// "������������" �����
		WriteOnPosition(x, y, enemyTransformation[x % 4]);
		// ��� ���� �������� �� ���������
		if (GetOnPosition(x, y) == '*')
			hitEnemy = TRUE;
		// �������� �� ��������� (����� �������� � ������ ���������)
		if (delayFactor < 3)
			delayFact = 3;
		else delayFact = delayFactor + 3;

		for (int i = 0; i < delayFact; i++)
		{
			Sleep(40);
			// ���� �� ������� ����� ����
			if (GetOnPosition(x, y) == '*')
			{
				hitEnemy = TRUE;
				break;
			}
		}
		// "�������" ��� � ����
		WriteOnPosition(x, y, ' ');

		// ��� ���� �������� �� ���������
		if (GetOnPosition(x, y) == '*')
			hitEnemy = TRUE;
		// ���� ������ �� �����
		if (hitEnemy)
		{
			// ��� ��������� � ���������� ������ ����
			MessageBeep(-1);
			// ����������� ���������� ���������
			InterlockedIncrement(&hits);
			// ������ ��������� ����
			Score();
			// ����������� ����� � ����������� �������
			_endthread();
		}
		// ���������� �����
		x += direction;
	}
	// ����������� ���������� ��������� ������
	InterlockedIncrement(&misses);
	// ������ ��������� ����
	Score();
}


// ������� ��������� ������
void createEnemyThreads(void*)
{
	// ���� ������ �� ����������, ���� 15 ������ �� ������ ����
	WaitForSingleObject(startEvent, 15000);
	// ��������� ������� ������� �����
	// ������ 5 ������ ���������� ���� �������
	// ���������� � ������������ �� 1 �� 10
	while (true)
	{
		if (RandomValueBetween(0, 100) < (hits + misses) / 25 + 20)
			// C� �������� ����������� �������������
			_beginthread(enemyThread, 0, (void *)(RandomValueBetween(1, 10)));
		// ������������������ �� �������
		Sleep(1000);
	}
}


// ����� ���� (������ ���� - ��� ��������� �����)
void bulletThread(void* _xy)
{
	// ���������� ����
	COORD xy = *(COORD*)_xy;
	// ���� �� ��������������� ������� ��� ����� ����
	if (GetOnPosition(xy.X, xy.Y) == '*')
		return;

	// ��������� ������� � �� ������ �������, ���� ��� 3 ����
	if (WaitForSingleObject(bulletSem, 0) == WAIT_TIMEOUT) return;

	while (--xy.Y)
	{
		// ������ ���� �� �������� �����������
		WriteOnPosition(xy.X, xy.Y, '*');
		Sleep(80);
		// ������� ����
		WriteOnPosition(xy.X, xy.Y, ' ');
	}
	// ��� ������ �������� �������� �������
	ReleaseSemaphore(bulletSem, 1, NULL);
}


// �������� ���������
void main()
{
	HANDLE gun;
	// ��������� ���������� ����������
	conIn = GetStdHandle(STD_INPUT_HANDLE);
	conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleMode(conIn, ENABLE_WINDOW_INPUT);

	// ��������� ���������������� �������� ������
	gun = GetCurrentThread();
	// ������ ����� �����������
	DuplicateHandle(GetCurrentProcess(), gun, GetCurrentProcess(), &mainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	// ������� ������ �������
	startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// ������� �������
	screenLock = CreateMutex(NULL, FALSE, NULL);
	InitializeCriticalSection(&gameOver);
	// ������� ������� � ������������ ����������� ���������, ������ ����
	bulletSem = CreateSemaphore(NULL, 3, 3, NULL);
	GetConsoleScreenBufferInfo(conOut, &conInfo);

	// �������������� ����������� ���������� �� �����
	Score();

	// ����������� ��������� ��������� �����
	srand((unsigned)time(NULL));

	ConsoleCleaning(); // �� ����� ���� �� �����

	// ������������� ��������� ������������
	int y = conInfo.dwSize.Y - 1;
	int x = conInfo.dwSize.X / 2;

	// ��������� ����� � ��������� ������, ���� �������� ��� ������� 15 ������
	_beginthread(createEnemyThreads, 0, NULL);

	// �������� ���� ����
	while (true)
	{
		int symbol, repetNumber;
		// ������ �����
		WriteOnPosition(x, y, '|');
		symbol = Keystroke(repetNumber);   // �������� ������
		switch (symbol)
		{
			// ��������� ������� �� "������"
			case VK_SPACE:
			{
				static COORD xy;
				xy.X = x;
				xy.Y = y;
				_beginthread(bulletThread, 0, (void*)&xy);
				// ���� ����� ���� �������
				Sleep(100);
				break;
			}

			// ��������� ������� �� ������� "�����"
			case VK_LEFT:
			{
				// ����� � ������� ��������
				SetEvent(startEvent);
				// ������� �����
				WriteOnPosition(x, y, ' ');
				// ������������
				while (repetNumber--)
					if (x)
						x--;
				break;
			}

			// ��������� ������� �� ������� "�����"
			case VK_RIGHT:
			{
				// ����� � ������� ��������
				SetEvent(startEvent);
				// ������� �����
				WriteOnPosition(x, y, ' ');
				// ������������
				while (repetNumber--)
					if (x != conInfo.dwSize.X - 1)
						x++;
				break;
			}
		}
	}
}