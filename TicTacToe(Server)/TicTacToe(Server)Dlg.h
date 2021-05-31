﻿
// TicTacToe(Server)Dlg.h: файл заголовка
//

#pragma once


// Диалоговое окно CTicTacToeServerDlg
class CTicTacToeServerDlg : public CDialog
{
// Создание
public:
	CTicTacToeServerDlg(CWnd* pParent = nullptr);	// стандартный конструктор

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TICTACTOESERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// поддержка DDX/DDV


// Реализация
protected:
	HICON m_hIcon;

	// Созданные функции схемы сообщений
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStart();
	CListBox m_ListBox;
};