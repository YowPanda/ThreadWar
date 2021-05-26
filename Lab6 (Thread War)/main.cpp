// main.cpp

/*
	Комрьютерная игра Thread War
	Для игры используйте клавиши "влево" и "вправо", чтобы перемешать пушку
	Для выстрела используйте клавишу "пробел"

	Игра проиграна, если 30 врагов уйдут с экрана неуничтоженными
	За каждого убитого противника дается по 1 очку
*/


#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>


// Объекты синхронизации
// Изменением экрана занимается только один поток
HANDLE screenLock;
// Mожно выстрелить только три раза подряд
HANDLE bulletSem;
// Игра начинается с нажатием клавиши "влево" или "вправо"
HANDLE startEvent;
// Дескрипторы консоли
HANDLE conIn, conOut;
// Основной поток
HANDLE mainThread;
// Критическая секция конца игры
CRITICAL_SECTION gameOver;
// Информация о консоли
CONSOLE_SCREEN_BUFFER_INFO conInfo; 
// Количество попаданий и промахов
long hits = 0;					
long misses = 0;
// Коэффициент задержки
long delayFactor = 5; 


// Создание случайного числа в промежутке от a до b
int RandomValueBetween(int a, int b)
{
	if (a == 0 && b == 1) return rand() % 2;
	return rand() % (a - b) + a;
}


// Очистка экрана консоли
void ConsoleCleaning()
{
	// Структура координат (ширина, долгота)
	COORD coord = {0, 0};
	// Число ячеек для записи
	DWORD numberOfChar;
	// Записываем знаки пробела на консоль, начиная с координат (0, 0)
	FillConsoleOutputCharacter(conOut, ' ', conInfo.dwSize.X * conInfo.dwSize.Y, coord, &numberOfChar);
}


// Вывести на позицию (х, у) заданный символ
void WriteOnPosition(short x, short y, char ch)
{
	// Блокировать вывод на экран при помощи мьютекса
	WaitForSingleObject(screenLock, INFINITE);
	// Координаты куда пишем 
	COORD coord = {x,y};
	// Число ячеек для записи
	DWORD numberOfChar;
	// Выводит ряд символов, начинающихся в заданном месте (х, у)
	WriteConsoleOutputCharacter(conOut, &ch, 1, coord, &numberOfChar);
	// Освобождение объекта
	ReleaseMutex(screenLock);
}


// Получить нажатие на клавишу (счетчик повторейний в repeatCount)
int Keystroke(int &repeatCount)
{
	// Используем для записи событий ввода данных
	INPUT_RECORD inputBuffer;
	// Число ячеек для записи
	DWORD numberOfChar;

	// Бесконечный цикл
	while (true)
	{
		// Читает данные из буфера консоли 
		ReadConsoleInput(conIn, &inputBuffer, 1, &numberOfChar);
		// Игнорируем события, не являющиеся нажатием кнопки
		if (inputBuffer.EventType != KEY_EVENT) continue;
		// Игнорируем то, что клавишу отпустили (нас интересуют только нажатия)
		if (!inputBuffer.Event.KeyEvent.bKeyDown) continue;
		// Счетчик повторений
		repeatCount = inputBuffer.Event.KeyEvent.wRepeatCount;

		// Возвращаем виртуальный код клавиши
		return inputBuffer.Event.KeyEvent.wVirtualKeyCode;
	}
}


// Определить символ в заданной позиции экрана
int GetOnPosition(short x, short y)
{
	// Символ с координатами (х, у)
	char ch;
	DWORD numberOfChar;
	COORD coord = {x, y};
	// Блокируем доступ к консоли до тех пор, пока процедура не будет выполнена
	WaitForSingleObject(screenLock, INFINITE);
	// Читает данные из буфера консоли
	ReadConsoleOutputCharacter(conOut, &ch, 1, coord, &numberOfChar);
	// Освобождение объекта
	ReleaseMutex(screenLock);

	// Возвращаем символ с координатами (х, у)
	return ch;
}


// Отображаем очки в заголовке окна, проверяем условие завершения игры
void Score()
{
	// Строка заголовка окна
	char str[128];
	// Записываем в нее количество промахов и попаданий
	sprintf_s(str, "Thread War! Hits:%d  Misses:%d", hits, misses);
	SetConsoleTitle(str);

	// Если упустили 30 или более врагов
	if (misses >= 30)
	{
		// Объявляем начало критической секции
		EnterCriticalSection(&gameOver);
		// Приостанавливаем работу главного потока
		SuspendThread(mainThread);
		// Выводим окно с информацией о проигрыше
		MessageBox(NULL, "Game Over!", "Thread War", MB_OK | MB_SETFOREGROUND);
		// Успешное завершение 
		exit(0);
	}

	// Если сумма попаданий и промахов делится на 20 без остатка
	if ((hits + misses) % 20 == 0)
		// Уменьшаем коэффициент задержки
		InterlockedDecrement(&delayFactor);
}

// Вариации трансформации врага 
char enemyTransformation[] = "-\\|/";

// Поток врага
void enemyThread(void* _y)
{
	// Случайная координата по y
	int y = (int)_y;
	// Здесь сохраняем направление врага
	int direction;
	// Координата противника по х
	int x;

	// Делаем так, чтобы враги с нечетными координатами появлялись слева, а с четными - справа
	x = y % 2 ? 0 : conInfo.dwSize.X;
	// Устанавливаем направление врага в зависимости от начальной позиции
	direction = x ? -1 : 1;

	// Пока противник находится в пределах экрана
	while ((direction == 1 && x != conInfo.dwSize.X) || (direction == -1 && x != 0))
	{
		// Коэффициент задержки
		int delayFact;
		// Попали ли во врага
		BOOL hitEnemy = FALSE;
		// Проверка попадания пули во врага
		if (GetOnPosition(x, y) == '*')
			hitEnemy = TRUE;
		// "Поворачиваем" врага
		WriteOnPosition(x, y, enemyTransformation[x % 4]);
		// Еще одна проверка на попадание
		if (GetOnPosition(x, y) == '*')
			hitEnemy = TRUE;
		// Проверка на попадание (когда задержка у врагов небольшая)
		if (delayFactor < 3)
			delayFact = 3;
		else delayFact = delayFactor + 3;

		for (int i = 0; i < delayFact; i++)
		{
			Sleep(40);
			// Если на позиции врага пуля
			if (GetOnPosition(x, y) == '*')
			{
				hitEnemy = TRUE;
				break;
			}
		}
		// "Стираем" его с поля
		WriteOnPosition(x, y, ' ');

		// Еще одна проверка на попадание
		if (GetOnPosition(x, y) == '*')
			hitEnemy = TRUE;
		// Если попали во врага
		if (hitEnemy)
		{
			// При попадании в противника издаем звук
			MessageBeep(-1);
			// Увеличиваем количество попаданий
			InterlockedIncrement(&hits);
			// Меняем заголовок окна
			Score();
			// Заканчиваем поток и освобождаем ресурсы
			_endthread();
		}
		// Перемещаем врага
		x += direction;
	}
	// Увеличиваем количество убежавших врагов
	InterlockedIncrement(&misses);
	// Меняем заголовок окна
	Score();
}


// Создаем вражеские потоки
void createEnemyThreads(void*)
{
	// Если ничего не происходит, ждем 15 секунд до начала игры
	WaitForSingleObject(startEvent, 15000);
	// Случайным образом создаем врага
	// Каждые 5 секунд появляется шанс создать
	// Противника с координатами от 1 до 10
	while (true)
	{
		if (RandomValueBetween(0, 100) < (hits + misses) / 25 + 20)
			// Cо временем вероятность увеличивается
			_beginthread(enemyThread, 0, (void *)(RandomValueBetween(1, 10)));
		// Приостанавливаемся на секунду
		Sleep(1000);
	}
}


// Поток пули (каждая пуля - это отдельный поток)
void bulletThread(void* _xy)
{
	// Координаты пули
	COORD xy = *(COORD*)_xy;
	// Если на просматриваемой позиции уже стоит пуля
	if (GetOnPosition(xy.X, xy.Y) == '*')
		return;

	// Проверяем семафор и не делаем выстрел, если уже 3 пули
	if (WaitForSingleObject(bulletSem, 0) == WAIT_TIMEOUT) return;

	while (--xy.Y)
	{
		// Рисуем пулю по заданным координатам
		WriteOnPosition(xy.X, xy.Y, '*');
		Sleep(80);
		// Стираем пулю
		WriteOnPosition(xy.X, xy.Y, ' ');
	}
	// При каждом выстреле изменяем семафор
	ReleaseSemaphore(bulletSem, 1, NULL);
}


// Основная программа
void main()
{
	HANDLE gun;
	// Настройка глобальных переменных
	conIn = GetStdHandle(STD_INPUT_HANDLE);
	conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleMode(conIn, ENABLE_WINDOW_INPUT);

	// Извлекаем псевдодескриптор текущего потока
	gun = GetCurrentThread();
	// Делаем копию дескриптора
	DuplicateHandle(GetCurrentProcess(), gun, GetCurrentProcess(), &mainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	// Создаем объект событие
	startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// Создаем мьютекс
	screenLock = CreateMutex(NULL, FALSE, NULL);
	InitializeCriticalSection(&gameOver);
	// Создаем семафор с максимальным количеством обращений, равным трем
	bulletSem = CreateSemaphore(NULL, 3, 3, NULL);
	GetConsoleScreenBufferInfo(conOut, &conInfo);

	// Инициализируем отображение информации об очках
	Score();

	// Настраиваем генератор случайных чисел
	srand((unsigned)time(NULL));

	ConsoleCleaning(); // на самом деле не нужно

	// Устанавливаем начальную позициюпушки
	int y = conInfo.dwSize.Y - 1;
	int x = conInfo.dwSize.X / 2;

	// Запускаем поток с созданием врагов, ждем движений или ожидаем 15 секунд
	_beginthread(createEnemyThreads, 0, NULL);

	// Основной цикл игры
	while (true)
	{
		int symbol, repetNumber;
		// Рисуем пушку
		WriteOnPosition(x, y, '|');
		symbol = Keystroke(repetNumber);   // получить символ
		switch (symbol)
		{
			// Обработка нажатия на "пробел"
			case VK_SPACE:
			{
				static COORD xy;
				xy.X = x;
				xy.Y = y;
				_beginthread(bulletThread, 0, (void*)&xy);
				// Даем время пуле улететь
				Sleep(100);
				break;
			}

			// Обработка нажатия на клавишу "влево"
			case VK_LEFT:
			{
				// Поток с врагами работает
				SetEvent(startEvent);
				// Стираем пушку
				WriteOnPosition(x, y, ' ');
				// Перемещаемся
				while (repetNumber--)
					if (x)
						x--;
				break;
			}

			// Обработка нажатия на клавишу "влево"
			case VK_RIGHT:
			{
				// Поток с врагами работает
				SetEvent(startEvent);
				// Стираем пушку
				WriteOnPosition(x, y, ' ');
				// Перемещаемся
				while (repetNumber--)
					if (x != conInfo.dwSize.X - 1)
						x++;
				break;
			}
		}
	}
}