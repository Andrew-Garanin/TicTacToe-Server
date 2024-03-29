﻿
// TicTacToe(Server)Dlg.cpp: файл реализации
//

#include "pch.h"
#include "framework.h"
#include "TicTacToe(Server).h"
#include "TicTacToe(Server)Dlg.h"
#include "afxdialogex.h"
#include <winsock2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_SOCKET 11
#define PORT 5150			// Порт по умолчанию
#define DATA_BUFSIZE 8192 	// Размер буфера по умолчанию

int  iPort = PORT; 	 // Порт для прослушивания подключений
 
typedef struct _SOCKET_INFORMATION {
	CHAR Buffer[DATA_BUFSIZE];
	WSABUF DataBuf;
	SOCKET Socket;
	DWORD BytesSEND;
	DWORD BytesRECV;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

typedef struct Pair {
	LPSOCKET_INFORMATION firstPerson;
	LPSOCKET_INFORMATION secondPerson;
} PAIR_INFORMATION, * LPPAIR_INFORMATION;

BOOL CreateSocketInformation(SOCKET s, char* Str, CListBox* pLB);

void ScroolToLastItem(HWND hWnd_LB);

void FreeSocketInformation(DWORD Event, char* Str, CListBox* pLB);

void FreePairInformation(DWORD Event, char* Str, CListBox* pLB);

UINT ListenThread(PVOID lpParam);

DWORD EventTotal = 0;
DWORD PairTotal = 0;
HWND hWnd_LB;// Для ListenThread

WSAEVENT EventArray[MAX_SOCKET];
LPSOCKET_INFORMATION SocketArray[MAX_SOCKET];
PAIR_INFORMATION PairArray[5];// Массив с информацией о парах

// Диалоговое окно CTicTacToeServerDlg

CTicTacToeServerDlg::CTicTacToeServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TICTACTOESERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTicTacToeServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTBOX, m_ListBox);
}

BEGIN_MESSAGE_MAP(CTicTacToeServerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CTicTacToeServerDlg::OnBnClickedStart)
END_MESSAGE_MAP()


// Обработчики сообщений CTicTacToeServerDlg

BOOL CTicTacToeServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Задает значок для этого диалогового окна.  Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);			// Крупный значок
	SetIcon(m_hIcon, FALSE);		// Мелкий значок

	// TODO: добавьте дополнительную инициализацию
	char Str[128];
	sprintf_s(Str, sizeof(Str), "%d", iPort);
	GetDlgItem(IDC_PORT)->SetWindowText(Str);
	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

// При добавлении кнопки свертывания в диалоговое окно нужно воспользоваться приведенным ниже кодом,
//  чтобы нарисовать значок.  Для приложений MFC, использующих модель документов или представлений,
//  это автоматически выполняется рабочей областью.

void CTicTacToeServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // контекст устройства для рисования

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Выравнивание значка по центру клиентского прямоугольника
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Нарисуйте значок
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// Система вызывает эту функцию для получения отображения курсора при перемещении
//  свернутого окна.
HCURSOR CTicTacToeServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTicTacToeServerDlg::OnBnClickedStart()
{
	char Str[81];
	hWnd_LB = m_ListBox.m_hWnd;   // Для ListenThread
	GetDlgItem(IDC_PORT)->GetWindowText(Str, sizeof(Str));
	iPort = atoi(Str);
	if (iPort <= 0 || iPort >= 0x10000)
	{
		AfxMessageBox("Incorrect Port number");
		return;
	}
	AfxBeginThread(ListenThread, NULL);
	m_ListBox.AddString("Сервер успешно запущен");
	GetDlgItem(IDC_START)->EnableWindow(false);
}

UINT ListenThread(PVOID lpParam)
{
	//Сначала мы описываем необходи¬мые локальные переменные, создаем рабочее окно и загружаем библиоте¬ку Winsock
	SOCKET Listen;
	SOCKET Accept;
	SOCKADDR_IN InternetAddr;
	DWORD Event;
	WSANETWORKEVENTS NetworkEvents;
	WSADATA wsaData;
	DWORD Ret;
	DWORD Flags;
	DWORD RecvBytes;
	DWORD SendBytes;
	char  Str[200];
	CListBox* pLB =
		(CListBox*)(CListBox::FromHandle(hWnd_LB));

	if ((Ret = WSAStartup(0x0202, &wsaData)) != 0)
	{
		sprintf_s(Str, sizeof(Str),
			"WSAStartup() failed with error %d", Ret);
		pLB->AddString(Str);
		return 1;
	}
	//Затем создаем сокет, через вызов функции CreateSocketInformation() создаем для него событие и структуру для описания его состояния и записываем в соответствующие массивы.
	//Затем посредством вызова функции WSAEventSelect() связываем его с интересующими нас событиями FD_ACCEPT и FD_CLOSE.
	//Если указано значение 0, вызывающий абонент не желает указывать протокол, и поставщик услуг выберет протокол для использования.
	if ((Listen = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		sprintf_s(Str, sizeof(Str),
			"socket() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}
	CreateSocketInformation(Listen, Str, pLB);

	if (WSAEventSelect(Listen, EventArray[EventTotal - 1], FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		sprintf_s(Str, sizeof(Str),
			"WSAEventSelect() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}
	//Привязываем сокет к интерфейсу и выставляем его на прослушивание соединений.
	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);// Функция htonl преобразует u_long из хоста в сетевой порядок байтов TCP / IP (который является прямым порядком байтов).
	InternetAddr.sin_port = htons(iPort);// Функция htons преобразует u_short из хоста в сетевой порядок байтов TCP / IP (который является прямым порядком байтов).

	// Связывает стандартный адрес с сокетом
	if (bind(Listen, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		sprintf_s(Str, sizeof(Str),
			"bind() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}

	if (listen(Listen, 5))
	{
		sprintf_s(Str, sizeof(Str),
			"listen() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}
	//Начинаем в цикле обработку происходящих событий.Характер об - работки, разумеется, зависит от типа события.Сначала дождемся уве - домления о событии посредством функции WSAWaitForMultipleEvents().После этого информация о событии находится в структуре NetworkEvents.
	while (TRUE)
	{
		ScroolToLastItem(hWnd_LB);// Автоматическое пролистывание списка в конец
		// Дожидаемся уведомления о событии на любом сокете
		//WSAWaitForMultipleEvents Возврящает управление, когда один из заданных объектов событий 
		//перейдет в свободное состояние, либо исчезнет заданный тайм-аут
		if ((Event = WSAWaitForMultipleEvents(EventTotal,// Event - индекс события в списке событий
			EventArray, FALSE, WSA_INFINITE, FALSE)) ==
			WSA_WAIT_FAILED)
		{
			sprintf_s(Str, sizeof(Str),
				"WSAWaitForMultipleEvents failed"
				" with error %d", WSAGetLastError());
			pLB->AddString(Str);
			return 1;
		}

		// WSAEnumNetworkEvents Идентифицирует произошедшее событие и засовывает в NetworkEvents это событие
		if (WSAEnumNetworkEvents(
			SocketArray[Event - WSA_WAIT_EVENT_0]->Socket,
			EventArray[Event - WSA_WAIT_EVENT_0],
			&NetworkEvents) == SOCKET_ERROR)
		{
			sprintf_s(Str, sizeof(Str),
				"WSAEnumNetworkEvents failed"
				" with error %d", WSAGetLastError());
			pLB->AddString(Str);
			return 1;
		}
		//Далее мы рассматриваем возможные типы событий на сокете. Если произошло событие типа FD_ACCEPT, 
		//мы вызываем функцию установления соединения с клиентом accept(). 
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
		{
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_ACCEPT failed with error %d",
					NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
				pLB->AddString(Str);
				break;
			}

			// Прием нового соединения и добавление его в списки сокетов и событий
			Accept = accept(// Соединяет сокет прослушивания с сокетом, возвращенным ф-й accept
				SocketArray[Event - WSA_WAIT_EVENT_0]->Socket,
				NULL, NULL);
			if (Accept == INVALID_SOCKET)
			{
				sprintf_s(Str, sizeof(Str),
					"accept() failed with error %d",
					WSAGetLastError());
				pLB->AddString(Str);
				break;
			}

			if (EventTotal >= MAX_SOCKET)// Если сокетов набралось максимальное количество
			{
				sprintf_s(Str, sizeof(Str), "Too many connections - closing socket.");
				pLB->AddString(Str);
				closesocket(Accept);
				continue;
			}
			else
			{
				WSABUF msg;
				DWORD msgLengh = 1;
				msg.buf = "1";
				msg.len = 1;
				WSASend(Accept, &msg, 1, &msgLengh, 0, NULL, NULL);// Соединение прошло успешно
			}

			CreateSocketInformation(Accept, Str, pLB);

			// Создание пары
			int isHereFreeSlot = 0;
			for (int i = 0; i < PairTotal; i++)
			{
				if (PairArray[i].secondPerson == NULL)// Если есть свободное место в паре
				{
					isHereFreeSlot = 1;
					PairArray[i].secondPerson = SocketArray[EventTotal - 1];
					WSABUF msg;
					DWORD msgLengh = 1;
					msg.buf = "0";// Значит будет играть за нолики
					msg.len = 1;
					if (WSASend(SocketArray[EventTotal - 1]->Socket,
						&msg, 1,
						&msgLengh, 0, NULL, NULL) ==
						SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK)
						{
							sprintf_s(Str, sizeof(Str),
								"WSASend() failed with "
								"error %d", WSAGetLastError());
							pLB->AddString(Str);
							FreeSocketInformation(
								Event - WSA_WAIT_EVENT_0, Str, pLB);
							return 1;
						}
						// Произошла ошибка WSAEWOULDBLOCK. 
						// Событие FD_WRITE будет отправлено, когда
						// в буфере будет больше свободного места
					}

					// отправка сообщения первому игроку, что подключился второй игрок
					WSABUF msgCnct;
					DWORD msgCnctLengh = 1;
					msgCnct.buf = "9";// Признак подключения второго игрока
					msgCnct.len = 1;
					if (WSASend(PairArray[i].firstPerson->Socket,
						&msgCnct, 1,
						&msgCnctLengh, 0, NULL, NULL) ==
						SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK)
						{
							sprintf_s(Str, sizeof(Str),
								"WSASend() failed with "
								"error %d", WSAGetLastError());
							pLB->AddString(Str);
							FreeSocketInformation(
								Event - WSA_WAIT_EVENT_0, Str, pLB);
							return 1;
						}
						// Произошла ошибка WSAEWOULDBLOCK. 
						// Событие FD_WRITE будет отправлено, когда
						// в буфере будет больше свободного места				
					}
					break;
				}
			}

			if (!isHereFreeSlot)// Если свободных мест в парах не оказалось
			{
				Pair pair;
				pair.firstPerson = SocketArray[EventTotal - 1];
				pair.secondPerson = NULL;
				PairArray[PairTotal] = pair;
				PairTotal++;
				WSABUF msg;
				DWORD msgLengh = 1;
				msg.buf = "1";// Значит будет играть за крестики
				msg.len = 1;
				if (WSASend(SocketArray[EventTotal - 1]->Socket,
					&msg, 1,
					&msgLengh, 0, NULL, NULL) ==
					SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						sprintf_s(Str, sizeof(Str),
							"WSASend() failed with "
							"error %d", WSAGetLastError());
						pLB->AddString(Str);
						FreeSocketInformation(
							Event - WSA_WAIT_EVENT_0, Str, pLB);
						return 1;
					}
					// Произошла ошибка WSAEWOULDBLOCK. 
					// Событие FD_WRITE будет отправлено, когда
					// в буфере будет больше свободного места
				}
			}

			// Вызвав функцию WSAEventSelect(), мы связываем с сокетом события FD_READ, FD_WRITE и FD_CLOSE.
			if (WSAEventSelect(Accept,
				EventArray[EventTotal - 1],
				FD_READ | FD_WRITE | FD_CLOSE) ==
				SOCKET_ERROR)
			{
				sprintf_s(Str, sizeof(Str),
					"WSAEventSelect()failed"
					" with error %d", WSAGetLastError());
				pLB->AddString(Str);
				return 1;
			}
			sprintf_s(Str, sizeof(Str),
				"Socket %d connected", Accept);
			pLB->AddString(Str);
		}

		//Далее, если произошло событие типа FD_READ или FD_WRITE, выполняем соответственно прием или отправку данных
		// Пытаемся читать или писать данные, 
		// если произошло соответствующее событие

		if (NetworkEvents.lNetworkEvents & FD_READ || NetworkEvents.lNetworkEvents & FD_WRITE)
		{
			if (NetworkEvents.lNetworkEvents & FD_READ &&
				NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_READ failed with error %d",
					NetworkEvents.iErrorCode[FD_READ_BIT]);
				pLB->AddString(Str);
				break;
			}

			if (NetworkEvents.lNetworkEvents & FD_WRITE && NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_WRITE failed with error %d",
					NetworkEvents.iErrorCode[FD_WRITE_BIT]);
				pLB->AddString(Str);
				break;
			}

			LPSOCKET_INFORMATION SocketInfo = SocketArray[Event - WSA_WAIT_EVENT_0];

			// Читаем данные только если приемный буфер пуст
			if (SocketInfo->BytesRECV == 0)
			{
				SocketInfo->DataBuf.buf = SocketInfo->Buffer;
				SocketInfo->DataBuf.len = DATA_BUFSIZE;

				Flags = 0;
				if (WSARecv(SocketInfo->Socket,
					&(SocketInfo->DataBuf), 1, &RecvBytes,
					&Flags, NULL, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						sprintf_s(Str, sizeof(Str),
							"WSARecv()failed with "
							" error %d", WSAGetLastError());
						pLB->AddString(Str);
						FreeSocketInformation(
							Event - WSA_WAIT_EVENT_0, Str, pLB);
						return 1;
					}
				}
				else
				{
					SocketInfo->BytesRECV = RecvBytes;
					unsigned l = sizeof(Str) - 1;
					if (l > RecvBytes) l = RecvBytes;
					strncpy_s(Str, SocketInfo->Buffer, l);
					Str[l] = 0;
					pLB->AddString(Str);
					ScroolToLastItem(hWnd_LB);// Автоматическое пролистывание списка в конец
				}
			}

			// Отправка данных, если это возможно
			if (SocketInfo->BytesRECV > SocketInfo->BytesSEND)
			{
				SocketInfo->DataBuf.buf = SocketInfo->Buffer + SocketInfo->BytesSEND;
				SocketInfo->DataBuf.len = SocketInfo->BytesRECV - SocketInfo->BytesSEND;
				if (WSASend(SocketInfo->Socket,
					&(SocketInfo->DataBuf), 1,
					&SendBytes, 0, NULL, NULL) ==
					SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						sprintf_s(Str, sizeof(Str),
							"WSASend() failed with "
							"error %d", WSAGetLastError());
						pLB->AddString(Str);
						FreeSocketInformation(
							Event - WSA_WAIT_EVENT_0, Str, pLB);
						return 1;
					}
					// Произошла ошибка WSAEWOULDBLOCK. 
					// Событие FD_WRITE будет отправлено, когда
					// в буфере будет больше свободного места
				}
				else
				{
					SocketInfo->BytesSEND += SendBytes;
					if (SocketInfo->BytesSEND == SocketInfo->BytesRECV)
					{
						SocketInfo->BytesSEND = 0;
						SocketInfo->BytesRECV = 0;
					}
				}

				// Отправка сообщения второму игроку
				for (int i = 0; i < PairTotal; i++)
				{
					if (PairArray[i].firstPerson->Socket == SocketInfo->Socket)
					{
						if (WSASend(PairArray[i].secondPerson->Socket,
							&(SocketInfo->DataBuf), 1,
							&SendBytes, 0, NULL, NULL) ==
							SOCKET_ERROR)
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								sprintf_s(Str, sizeof(Str),
									"WSASend() failed with "
									"error %d", WSAGetLastError());
								pLB->AddString(Str);
								FreeSocketInformation(
									Event - WSA_WAIT_EVENT_0, Str, pLB);
								return 1;
							}
							// Произошла ошибка WSAEWOULDBLOCK. 
							// Событие FD_WRITE будет отправлено, когда
							// в буфере будет больше свободного места
						}
						break;
					}

					if (PairArray[i].secondPerson->Socket == SocketInfo->Socket)
					{
						if (WSASend(PairArray[i].firstPerson->Socket,
							&(SocketInfo->DataBuf), 1,
							&SendBytes, 0, NULL, NULL) ==
							SOCKET_ERROR)
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								sprintf_s(Str, sizeof(Str),
									"WSASend() failed with "
									"error %d", WSAGetLastError());
								pLB->AddString(Str);
								FreeSocketInformation(
									Event - WSA_WAIT_EVENT_0, Str, pLB);
								return 1;
							}
							// Произошла ошибка WSAEWOULDBLOCK. 
							// Событие FD_WRITE будет отправлено, когда
							// в буфере будет больше свободного места
						}
						break;
					}
				}
			}
		}

		//Наконец, если соединение разорвано, выполняем завершающую очистку :
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_CLOSE failed with error %d",
					NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
				pLB->AddString(Str);
			}

			sprintf_s(Str, sizeof(Str),
				"Closing socket information %d",
				SocketArray[Event - WSA_WAIT_EVENT_0]->Socket);
			pLB->AddString(Str);

			FreePairInformation(Event - WSA_WAIT_EVENT_0, Str, pLB);// Удаление одного из игроков из пары 
			FreeSocketInformation(Event - WSA_WAIT_EVENT_0, Str, pLB);
		}
	} // while
	return 0;
}

BOOL CreateSocketInformation(SOCKET s, char* Str, CListBox* pLB)
{
	LPSOCKET_INFORMATION SI;

	if ((EventArray[EventTotal] = WSACreateEvent()) ==
		WSA_INVALID_EVENT)
	{
		sprintf_s(Str, sizeof(Str),
			"WSACreateEvent() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return FALSE;
	}

	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR,
		sizeof(SOCKET_INFORMATION))) == NULL)
	{
		sprintf_s(Str, sizeof(Str),
			"GlobalAlloc() failed with error %d",
			GetLastError());
		pLB->AddString(Str);
		return FALSE;
	}

	// Подготовка структуры SocketInfo для использования.
	SI->Socket = s;
	SI->BytesSEND = 0;
	SI->BytesRECV = 0;

	SocketArray[EventTotal] = SI;
	EventTotal++;
	return(TRUE);
}

void FreeSocketInformation(DWORD Event, char* Str, CListBox* pLB)
{
	LPSOCKET_INFORMATION SI = SocketArray[Event];
	DWORD i;

	closesocket(SI->Socket);
	GlobalFree(SI);
	WSACloseEvent(EventArray[Event]);

	// Сжатие массивов сокетов и событий

	for (i = Event; i < EventTotal; i++)
	{
		EventArray[i] = EventArray[i + 1];
		SocketArray[i] = SocketArray[i + 1];
	}

	EventTotal--;
}

void FreePairInformation(DWORD Event, char* Str, CListBox* pLB)
{
	LPSOCKET_INFORMATION SI = SocketArray[Event];
	DWORD i;

	for (int i = 0; i < PairTotal; i++)
	{
		if (PairArray[i].firstPerson == SI)
		{
			PairArray[i].firstPerson = NULL;
			if (PairArray[i].secondPerson == NULL)// Если в паре больше никого не осталось
			{
				//Удалить пару и сдвинуть все остальные пары
				for (; i < PairTotal; i++)
				{
					PairArray[i] = PairArray[i + 1];
				}
				PairTotal--;
				return;
			}

			//Уведомить другого игрока, что противник отключился
			WSABUF msg;
			DWORD msgLengh = 1;
			msg.buf = "8";
			msg.len = 1;
			if (WSASend(PairArray[i].secondPerson->Socket,
				&msg, 1,
				&msgLengh, 0, NULL, NULL) ==
				SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					sprintf_s(Str, sizeof(Str),
						"WSASend() failed with "
						"error %d", WSAGetLastError());
					pLB->AddString(Str);
					FreeSocketInformation(
						Event - WSA_WAIT_EVENT_0, Str, pLB);
					//return 1;
				}
				// Произошла ошибка WSAEWOULDBLOCK. 
				// Событие FD_WRITE будет отправлено, когда
				// в буфере будет больше свободного места
			}
			PairArray[i].firstPerson = PairArray[i].secondPerson;//Второй игрок занял место первого игрока
			PairArray[i].secondPerson = NULL;
			break;
		}

		if (PairArray[i].secondPerson == SI)
		{
			PairArray[i].secondPerson = NULL;
			if (PairArray[i].firstPerson == NULL)// Если в паре больше никого не осталось
			{
				// Удалить пару и сдвинуть все остальные пары
				for (; i < PairTotal; i++)
				{
					PairArray[i] = PairArray[i + 1];
				}

				PairTotal--;
				return;
			}
			//Уведомить другого игрока, что противник отключился
			WSABUF msg;
			DWORD msgLengh = 1;
			msg.buf = "8";
			msg.len = 1;
			if (WSASend(PairArray[i].firstPerson->Socket,
				&msg, 1,
				&msgLengh, 0, NULL, NULL) ==
				SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					sprintf_s(Str, sizeof(Str),
						"WSASend() failed with "
						"error %d", WSAGetLastError());
					pLB->AddString(Str);
					FreeSocketInformation(
						Event - WSA_WAIT_EVENT_0, Str, pLB);
					//return 1;
				}
				// Произошла ошибка WSAEWOULDBLOCK. 
				// Событие FD_WRITE будет отправлено, когда
				// в буфере будет больше свободного места
			}
			break;
		}
	}
}

void ScroolToLastItem(HWND hWnd_LB)
{
	CListBox* pLB = (CListBox*)(CListBox::FromHandle(hWnd_LB));
	int nNumItems = pLB->GetCount();
	pLB->SetCurSel(nNumItems - 1);
	pLB->SetCurSel(-1);
}