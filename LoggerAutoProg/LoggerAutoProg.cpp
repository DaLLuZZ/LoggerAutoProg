// LoggerAutoProg.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "LoggerAutoProg.h"
#include <thread>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#define MAX_LOADSTRING 100

// Глобальные переменные:
HWND hWnd;
HINSTANCE hInst;                                // текущий экземпляр
HWND hCheckbox;                                 // checkbox
HWND hButtonConnect;                            // кнопка "подключиться"
HWND hButtonLoad;                               // кнопка "прошить"
HWND hButtonAccept;                             // кнопка "подтвердить"
HWND hWndEdit;                                  // Окно для ввода текста
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
WCHAR szText[MAX_LOADSTRING];                   // буффер для отображаемого текста
WCHAR szSelectedFile[260];                      // имя выбранного файла
UINT32 logger_id = 0xFFFFFFFF;                  // logger_id, находящийся в файле
WCHAR szBuf[1024];                              // небольшой вспомогательный буффер
WCHAR szConnectionStatus[128];                  // статус подключения

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HANDLE              CheckFileValid(LPOPENFILENAMEW ofn);
bool                CheckTempFileValid(bool bFinalCheck);
void                LaunchAndControlConsoleApp(bool load);
bool                SetNewIdInSelectedBinFile(UINT32 log_id_new);

//setup converter
using convert_type = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_type, wchar_t> converter;

bool fileExists(const WCHAR* filePath) {
    DWORD fileAttr = GetFileAttributes(filePath);
    return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

bool onButtonAcceptClick()
{
    WCHAR buffer[32];
    GetWindowText(hWndEdit, buffer, sizeof(buffer) / 2);
    UINT32 i;
    swscanf_s(buffer, L"%i", &i);
    if (SetNewIdInSelectedBinFile(i))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return true;
    }

    return false;
}

bool onButtonConnectClick()
{
    LaunchAndControlConsoleApp(false);
    if (!CheckTempFileValid(false))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return false;
    }

    // Здесь мы уверены, что логгер подключен нормально и готов прошиваться
    // Можно активировать кнопку прошивки

    EnableWindow(hButtonLoad, true);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    return true;
}

bool onButtonLoadClick()
{
    EnableWindow(hButtonLoad, false);
    LaunchAndControlConsoleApp(true);

    if (!CheckTempFileValid(true))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return false;
    }

    // Здесь мы уверены, что логгер прошился нормально, проверены Logger Id и MAGIC
    // Можно отключать логгер и увеличивать айдишку в файле (+1)
    logger_id++;
    if (SetNewIdInSelectedBinFile(logger_id))
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return true;
    }

    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    return false;
}

void LaunchAndControlConsoleApp(bool load) {
    // Создание пайпов для перенаправления ввода и вывода
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Пайп для стандартного ввода
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

    // Пайп для стандартного вывода
    HANDLE hReadOutput, hWriteOutput;
    CreatePipe(&hReadOutput, &hWriteOutput, &sa, 0);

    // Настройка структуры для создания процесса
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES; // Использовать стандартные дескрипторы
    si.hStdInput = hReadPipe;           // Перенаправляем стандартный ввод
    //si.hStdOutput = hWriteOutput;       // Перенаправляем стандартный вывод
    //si.hStdError = hWriteOutput;        // Перенаправляем стандартный вывод ошибок

    ZeroMemory(&pi, sizeof(pi));

    // Удаляем файл temp.bin
    if (fileExists(L"temp.bin")) {
        if (!DeleteFile(L"temp.bin"))
        {
            DWORD error = GetLastError();
            wsprintf(szBuf, L"Не удалось удалить temp.bin! Ошибка: 0x%X", error);
            MessageBox(NULL, szBuf, L"Ошибка!", MB_OK | MB_ICONERROR);
            return;
        }
    }    

    // Запуск консольного приложения
    if (!CreateProcess(L"GD_Link_CLI.exe", NULL, NULL, NULL, TRUE,
        0, NULL, NULL, &si, &pi)) {
        MessageBox(NULL, L"Не удалось создать процесс (GD_Link_CLI.exe)!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return;
    }

    CloseHandle(hReadPipe);   // Закрываем дескриптор записи в пайп
    CloseHandle(hWriteOutput); // Закрываем дескриптор чтения из пайпа

    // Отправка команд в консольное приложение
    DWORD bytesWritten;
    DWORD bytesRead;
    char buffer[128];

    //WriteFile(hWritePipe, ("r\n"), 2, &bytesWritten, NULL);
    WriteFile(hWritePipe, ("sleep 50\n"), 9, &bytesWritten, NULL);
    if (load)
    {
        char szcharBuff[512];
        sprintf_s(szcharBuff, "load %s 0x08000000\n", (converter.to_bytes(std::wstring(szSelectedFile))).c_str());
        WriteFile(hWritePipe, ("setopt opt.bin\n"), 15, &bytesWritten, NULL);
        WriteFile(hWritePipe, ("sleep 50\n"), 9, &bytesWritten, NULL);
        WriteFile(hWritePipe, ("erase\n"), 6, &bytesWritten, NULL);
        WriteFile(hWritePipe, ("sleep 50\n"), 9, &bytesWritten, NULL);
        WriteFile(hWritePipe, szcharBuff, strlen(szcharBuff), &bytesWritten, NULL);
        WriteFile(hWritePipe, ("sleep 50\n"), 9, &bytesWritten, NULL);
    }

    WriteFile(hWritePipe, ("mem32 0x0800FFF8 2\n"), 19, &bytesWritten, NULL);
    WriteFile(hWritePipe, ("sleep 50\n"), 9, &bytesWritten, NULL);
    WriteFile(hWritePipe, ("savebin temp.bin 0x0800FFF8 8\n"), 30, &bytesWritten, NULL);
    WriteFile(hWritePipe, ("sleep 50\n"), 9, &bytesWritten, NULL);
    WriteFile(hWritePipe, ("q\n"), 2, &bytesWritten, NULL);

    // simulate infinity loop
    ReadFile(hReadOutput, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    buffer[bytesRead] = '\0'; // Завершаем строку

    // Завершение процесса и очистка ресурсов
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadOutput); // Закрываем дескриптор чтения из пайпа
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

    szSelectedFile[0] = L'/0';

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LOGGERAUTOPROG, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LOGGERAUTOPROG));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGGERAUTOPROG));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LOGGERAUTOPROG);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 600, 480, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            hWndEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
                120, 100, 100, 30,
                hWnd,
                nullptr,
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                nullptr
            );
            hCheckbox = CreateWindowEx(
                0,
                L"BUTTON",  // Класс кнопки
                L"Safety", // Текст на кнопке
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX, // Стиль флажка
                10, 10, 100, 30, // Позиция и размер
                hWnd,
                NULL,
                hInst,
                NULL
            );
            SendMessage(hCheckbox, BM_SETCHECK, BST_CHECKED, 0);
            hButtonConnect = CreateWindow(
                L"BUTTON",  // Предопределенный класс кнопки
                L"Подключить", // Текст на кнопке
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // Стиль кнопки
                120,         // Положение по X
                10,         // Положение по Y
                100,        // Ширина
                30,         // Высота
                hWnd,       // Родительское окно
                (HMENU)IDM_BUTTON_CONNECT,   // ID кнопки
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), // Дескриптор приложения
                NULL);      // Дополнительные параметры
            EnableWindow(hButtonConnect, false);
            hButtonLoad = CreateWindow(
                L"BUTTON",  // Предопределенный класс кнопки
                L"Прошить", // Текст на кнопке
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // Стиль кнопки
                220,         // Положение по X
                10,         // Положение по Y
                100,        // Ширина
                30,         // Высота
                hWnd,       // Родительское окно
                (HMENU)IDM_BUTTON_LOAD,   // ID кнопки
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), // Дескриптор приложения
                NULL);      // Дополнительные параметры
            EnableWindow(hButtonLoad, false);
            hButtonAccept = CreateWindow(
                L"BUTTON",  // Предопределенный класс кнопки
                L"Ввести id", // Текст на кнопке
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // Стиль кнопки
                220,         // Положение по X
                100,         // Положение по Y
                100,        // Ширина
                30,         // Высота
                hWnd,       // Родительское окно
                (HMENU)IDM_BUTTON_ACCEPT,   // ID кнопки
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), // Дескриптор приложения
                NULL);      // Дополнительные параметры
            EnableWindow(hButtonAccept, false);
            
        }
        break;
    case WM_COMMAND:
        {
            if ((HWND)lParam == hCheckbox)
            {
                if (LOWORD(wParam) == BN_CLICKED) {
                    // Обработка нажатия на флажок
                    if (SendMessage(hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        SendMessage(hCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
                        EnableWindow(hButtonConnect, false);
                        EnableWindow(hButtonLoad, false);
                        if (szSelectedFile[0] != L'/0')
                            EnableWindow(hButtonAccept, true);
                        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                    }
                    else {
                        SendMessage(hCheckbox, BM_SETCHECK, BST_CHECKED, 0);
                        if (logger_id != 0xFFFFFFFF)
                        {
                            EnableWindow(hButtonConnect, true);
                        }
                        EnableWindow(hButtonAccept, false);
                        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                    }
                }
            }
            
                int wmId = LOWORD(wParam);
                // Разобрать выбор в меню:
                switch (wmId)
                {
                case IDM_OPENFILE:
                    {
                        OPENFILENAME ofn;       // Структура для диалога открытия файла
                        wchar_t szFile[260];   // Буфер для имени файла

                        // Обнуление структуры
                        ZeroMemory(&ofn, sizeof(ofn));
                        szFile[0] = L'\0';
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hWnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = L"Binary Firmware File (.bin)\0*.bin\0\0";
                        ofn.nFilterIndex = 1;
                        ofn.lpstrFileTitle = NULL;
                        ofn.nMaxFileTitle = 0;
                        ofn.lpstrInitialDir = NULL;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                        // Показать диалог открытия файла
                        if (GetOpenFileName(&ofn)) {
                            CheckFileValid(&ofn);
                            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                            if (logger_id != 0xFFFFFFFF && SendMessage(hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED)
                                EnableWindow(hButtonConnect, true);
                        }
                        else
                        {
                                DWORD dwError = CommDlgExtendedError();
                                if (dwError != 0) {
                                    // Выводим код ошибки
                                    wchar_t errorMessage[256];
                                    swprintf(errorMessage, 256, L"Ошибка: 0x%X", dwError);
                                    MessageBox(hWnd, errorMessage, L"Ошибка", MB_OK | MB_ICONERROR);
                                }
                        }
                    }
                    break;
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                case IDM_BUTTON_CONNECT:
                {
                    std::thread t1(onButtonConnectClick);
                    t1.detach();
                }
                break;
                case IDM_BUTTON_LOAD:
                {
                    std::thread t2(onButtonLoadClick);
                    t2.detach();
                }
                break;
                case IDM_BUTTON_ACCEPT:
                {
                    std::thread t3(onButtonAcceptClick);
                    t3.detach();
                }
                break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

            if (logger_id == 0xFFFFFFFF)
            {
                wsprintf(szBuf, L"не задан");
            }
            else
            {
                wsprintf(szBuf, L"%u", logger_id);
            }
            wsprintf(szText, L"Открыт файл: %s", szSelectedFile);
            TextOut(hdc, 10, 40, szText, wcslen(szText));
            wsprintf(szText, L"Текущий logger_id : %s", szBuf);
            TextOut(hdc, 10, 60, szText, wcslen(szText));
            wsprintf(szText, L"Статус: %s", szConnectionStatus);
            TextOut(hdc, 10, 80, szText, wcslen(szText));
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CHAR:
    {
        // Получаем символ, который был введен
        char ch = (char)wParam;

        // Проверяем, является ли символ цифрой или управляющим символом (например, Backspace)
        if (!isdigit(ch) && ch != VK_BACK) {
            return 0; // Игнорируем ввод, если это не цифра или Backspace
        }
        return DefWindowProc(hWnd, message, wParam, lParam); // Обрабатываем остальные сообщения
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

HANDLE CheckFileValid(LPOPENFILENAMEW ofn)
{
    logger_id = 0xFFFFFFFF;

    HANDLE hFile = CreateFile(
        ofn->lpstrFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    DWORD fileSize = GetFileSize(hFile, NULL);

    if (fileSize != 65536)
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Ошибка при открытии файла: неправильный размер!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    // Проверка сигнатуры
    // Указываем адрес, с которого хотим начать чтение
    DWORD offset = 0xFFFC;

    // Перемещаем указатель файла
    if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Не удалось установить указатель внутри файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    // Читаем байт из файла
    BYTE byte[4];
    DWORD bytesRead;

    if (!ReadFile(hFile, &byte, sizeof(byte), &bytesRead, NULL))
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Не удалось прочитать файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    if (bytesRead == 0)
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Не удалось прочитать байт!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    if (*(unsigned int*)byte != 0xDEADBEEF)
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Файл поврежден или несовместим с текущей версией программы!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    // Указываем адрес, с которого хотим начать чтение
    offset = 0xFFF8;

    // Перемещаем указатель файла
    if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Не удалось установить указатель внутри файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    // Читаем байт из файла

    if (!ReadFile(hFile, &byte, sizeof(byte), &bytesRead, NULL))
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Не удалось прочитать файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }

    if (bytesRead == 0)
    {
        CloseHandle(hFile);
        MessageBox(hWnd, L"Не удалось прочитать байт!", L"Ошибка!", MB_OK | MB_ICONERROR);
        wsprintf(szSelectedFile, L"\0");
        return NULL;
    }
    
    logger_id = *(unsigned int*)byte;

    wcscpy_s(szSelectedFile, ofn->lpstrFile);

    CloseHandle(hFile);

    if (SendMessage(hCheckbox, BM_GETCHECK, 0, 0) != BST_CHECKED)
        EnableWindow(hButtonAccept, true);

    return hFile;
}

bool CheckTempFileValid(bool bFinalCheck)
{
    HANDLE hFile = CreateFile(
        L"temp.bin",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    DWORD fileSize = GetFileSize(hFile, NULL);

    if (fileSize != 8)
    {
        CloseHandle(hFile);
        wsprintf(szConnectionStatus, L"Не удалось подключиться к логгеру (код ошибки %i)", 1);
        MessageBox(NULL, L"Ошибка при подключении к логгеру!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Проверка сигнатуру
    // Указываем адрес, с которого хотим начать чтение
    DWORD offset = 0x4;

    // Перемещаем указатель файла
    if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        wsprintf(szConnectionStatus, L"Не удалось подключиться к логгеру (код ошибки %i)", 2);
        MessageBox(NULL, L"Не удалось установить указатель внутри temp-файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Читаем байт из файла
    BYTE byte[4];
    DWORD bytesRead;

    if (!ReadFile(hFile, &byte, sizeof(byte), &bytesRead, NULL))
    {
        CloseHandle(hFile);
        wsprintf(szConnectionStatus, L"Не удалось подключиться к логгеру (код ошибки %i)", 3);
        MessageBox(NULL, L"Не удалось прочитать temp-файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    if (bytesRead == 0)
    {
        CloseHandle(hFile);
        wsprintf(szConnectionStatus, L"Не удалось подключиться к логгеру (код ошибки %i)", 4);
        MessageBox(NULL, L"Не удалось прочитать байт из temp-файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    if (*(unsigned int*)byte == 0x00000000)
    {
        CloseHandle(hFile);
        wsprintf(szConnectionStatus, L"Не удалось подключиться к логгеру (код ошибки %i)", 5);
        MessageBox(NULL, L"Не удалось подключиться к логгеру (write-protected)!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }
    else if (*(unsigned int*)byte == 0xFFFFFFFF)
    {
        wsprintf(szConnectionStatus, L"Подключенный логгер не прошит");
    }
    else if (*(unsigned int*)byte == 0xDEADBEEF)
    {
        // Перемещаем указатель файла
        if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            CloseHandle(hFile);
            MessageBox(NULL, L"Не удалось установить указатель внутри temp-файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
            return false;
        }
        // Читаем его id-шник
        if (!ReadFile(hFile, &byte, sizeof(byte), &bytesRead, NULL))
        {
            CloseHandle(hFile);
            MessageBox(NULL, L"Не удалось прочитать temp-файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
            return false;
        }
        wsprintf(szConnectionStatus, L"Подключенный логгер прошит, id: %i", *(unsigned int*)byte);
    }
    else
    {
        wsprintf(szConnectionStatus, L"Микроконтроллер имеет какую-то левую прошивку, сигнатура не совпадает");
        if (bFinalCheck)
        {
            CloseHandle(hFile);
            return false;
        }
    }

    if (bFinalCheck)
    {
        if (*(unsigned int*)byte != logger_id)
        {
            wsprintf(szConnectionStatus, L"Проблема с прошивкой: id не совпал после проверки: bin %i, device %i", logger_id, *(unsigned int*)byte);
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);

    return true;
}

bool SetNewIdInSelectedBinFile(UINT32 log_id_new)
{
    HANDLE hFile = CreateFile(
        szSelectedFile,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    DWORD fileSize = GetFileSize(hFile, NULL);

    if (fileSize != 65536)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Ошибка при открытии файла: неправильный размер!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Проверка сигнатуры
    // Указываем адрес, с которого хотим начать чтение
    DWORD offset = 0xFFFC;

    // Перемещаем указатель файла
    if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось установить указатель внутри файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Читаем байт из файла
    BYTE byte[4];
    DWORD bytesRead;

    if (!ReadFile(hFile, &byte, sizeof(byte), &bytesRead, NULL))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось прочитать файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    if (bytesRead == 0)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось прочитать байт!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    if (*(unsigned int*)byte != 0xDEADBEEF)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Файл поврежден или несовместим с текущей версией программы!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Указываем адрес, с которого хотим начать чтение
    offset = 0xFFF8;

    // Перемещаем указатель файла
    if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось установить указатель внутри файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Пишем байт в файл

    if (!WriteFile(hFile, (BYTE*)&log_id_new, 4, &bytesRead, NULL))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось записать в bin-файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    if (bytesRead == 0)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось записать новый id в bin-файл!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    // Указываем адрес, с которого хотим начать чтение
    offset = 0xFFF8;

    // Перемещаем указатель файла
    if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось установить указатель внутри файла!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!ReadFile(hFile, &byte, sizeof(byte), &bytesRead, NULL))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Не удалось прочитать записанный id из bin-файла для контрольной проверки!", L"Ошибка!", MB_OK | MB_ICONERROR);
        return false;
    }

    logger_id = *(unsigned int*)byte;

    CloseHandle(hFile);

    return true;
}