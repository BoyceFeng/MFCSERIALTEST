#ifndef _SERIAL_PORT_CONTROL_H
#define _SERIAL_PORT_CONTROL_H

#include "stdafx.h"

#define COM_RECVDATA WM_USER+999//�Զ�����Ϣ

extern HANDLE hCom; //ȫ�ֱ��������ھ��
extern HANDLE hCommThread; //ȫ�ֱ����������߳�
extern DWORD dwThreadID;
extern OVERLAPPED m_osRead, m_osWrite;

//���ڼ����߳̿��ƺ���
extern DWORD WINAPI CommWatchProc(HWND hWnd);
//�򿪲�����PC����1(COM1)
extern BOOL OpenSerialPort(LPCWSTR lpFileName);

#endif