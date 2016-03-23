#ifndef _SERIAL_PORT_CONTROL_H
#define _SERIAL_PORT_CONTROL_H

#include "stdafx.h"

#define COM_RECVDATA WM_USER+999//自定义消息

extern HANDLE hCom; //全局变量，串口句柄
extern HANDLE hCommThread; //全局变量，串口线程
extern DWORD dwThreadID;
extern OVERLAPPED m_osRead, m_osWrite;

//串口监视线程控制函数
extern DWORD WINAPI CommWatchProc(HWND hWnd);
//打开并设置PC串口1(COM1)
extern BOOL OpenSerialPort(LPCWSTR lpFileName);

#endif