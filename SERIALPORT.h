#pragma once
#include "stdafx.h"

#ifndef WM_COMM_MSG_BASE 
#define WM_COMM_MSG_BASE		WM_USER + 617		//!< 消息编号的基点  
#endif

#define WM_COMM_BREAK_DETECTED		WM_COMM_MSG_BASE + 1	// A break was detected on input.
#define WM_COMM_CTS_DETECTED		WM_COMM_MSG_BASE + 2	// The CTS (clear-to-send) signal changed state. 
#define WM_COMM_DSR_DETECTED		WM_COMM_MSG_BASE + 3	// The DSR (data-set-ready) signal changed state. 
#define WM_COMM_ERR_DETECTED		WM_COMM_MSG_BASE + 4	// A line-status error occurred. Line-status errors are CE_FRAME, CE_OVERRUN, and CE_RXPARITY. 
#define WM_COMM_RING_DETECTED		WM_COMM_MSG_BASE + 5	// A ring indicator was detected. 
#define WM_COMM_RLSD_DETECTED		WM_COMM_MSG_BASE + 6	// The RLSD (receive-line-signal-detect) signal changed state. 
#define WM_COMM_RXCHAR				WM_COMM_MSG_BASE + 7	// A character was received and placed in the input buffer. 
#define WM_COMM_RXFLAG_DETECTED		WM_COMM_MSG_BASE + 8	// The event character was received and placed in the input buffer.  
#define WM_COMM_TXEMPTY_DETECTED	WM_COMM_MSG_BASE + 9	// The last character in the output buffer was sent.  

#define MaxSerialPortNum 20   ///有效的串口总个数，不是串口的号

class CSerialPort
{
public:
	//构造函数
	CSerialPort();
	//析构函数
	~CSerialPort();

	//获得串口
	void GetCOMPort(CString* str);

	//初始化并打开串口
	//bool InitCOMPort(LPCWSTR lpFileName, int baudRate, byte byteSize, byte stopBit, byte parity);

	BOOL InitPort(CWnd* pPortOwner, UINT portnr = 1, UINT baud = 9600,
		char parity = 'N', UINT databits = 8, UINT stopbits = 1,
		DWORD dwCommEvents = EV_RXCHAR | EV_CTS, UINT nBufferSize = 512,

		DWORD ReadIntervalTimeout = 1000,
		DWORD ReadTotalTimeoutMultiplier = 1000,
		DWORD ReadTotalTimeoutConstant = 1000,
		DWORD WriteTotalTimeoutMultiplier = 1000,
		DWORD WriteTotalTimeoutConstant = 1000
		);

	//控制串口监视线程
	BOOL StartMonitoring();//开始监听
	BOOL RestartMonitoring();//重新监听
	BOOL StopMonitoring();//停止监听

	DWORD GetWriteBufferSize();//获取写缓冲区大小
	DWORD GetCommEvents();//获取事件
	DCB GetDCB();//获取DCB

	//发送数据
	void WriteToPort(char* string);
	void WriteToPort(char* string, int n);
	void WriteToPort(LPCTSTR string);
	void WriteToPort(LPCTSTR string, int n);
	void WriteToPort(BYTE* Buffer, int n);
	void ClosePort();
	BOOL IsOpen();

	void SendData(LPCTSTR lpszData, const int nLength);
	BOOL RecvData(LPCTSTR lpszData, const int nSize);
	void QueryKey(HKEY hkey);//查询注册表的串口号，将值存于数组中
	void Hkey2ComboBox(CComboBox& m_PortNO);//将查询到的串口号添加到CComboBox控件中

protected:
	void ProcessErrorMessage(char* EvenText);//错误处理
	static DWORD WINAPI CommThread(LPVOID pParam);//线程函数
	static void ReceiveChar(CSerialPort* port);
	static void WriteChar(CSerialPort* port);


	HANDLE m_Thread;	//线程句柄
	BOOL m_bIsSusupened;//监视线程是否挂起	

	//synchronisation;同步
	CRITICAL_SECTION m_csCommunicationSync;//临界资源
	BOOL m_bThreadAlive;	//thread监视线程是否挂起
	
	
	HANDLE hCom;		//串口句柄
	HANDLE m_hWriteEvent;//写事件
	HANDLE m_hShutdownEvent;

	/*事件数组*/
	//包括写事件、接收事件、关闭事件
	///一个元素用于一个事件。有两个事件线程处理端口。
	///写事件和接收字符事件位于overlapped结构体（m_ov.hEvent）中
	///当端口关闭时，有一个通用的关闭。
	HANDLE m_hEventArray[3];

	OVERLAPPED m_ov;//异步I/O
	COMMTIMEOUTS m_CommTimeouts;//超时设置
	DCB m_dcb;//设备控制模块

	CWnd* m_pOwner;//窗体句柄

	UINT m_nPortNr;
	char* m_szWriteBuffer;//写缓冲区
	DWORD m_dwCommEvents;
	DWORD m_nWriteBufferSize;//写缓冲区大小

	int	m_nWriteSize;//写入字节数

};

