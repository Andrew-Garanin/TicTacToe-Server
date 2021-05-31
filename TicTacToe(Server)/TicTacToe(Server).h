
// TicTacToe(Server).h: главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "включить pch.h до включения этого файла в PCH"
#endif

#include "resource.h"		// основные символы


// CTicTacToeServerApp:
// Сведения о реализации этого класса: TicTacToe(Server).cpp
//

class CTicTacToeServerApp : public CWinApp
{
public:
	CTicTacToeServerApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern CTicTacToeServerApp theApp;
