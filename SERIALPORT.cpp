#include "stdafx.h"
#include "SERIALPORT.h"

int m_nComArray[20];

CSerialPort::CSerialPort()
{
	hCom = NULL;

	//初始化异步结构体
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	//create events
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_szWriteBuffer = NULL;

	m_bThreadAlive = FALSE;
	m_nWriteSize = 1;
	m_bIsSusupened = FALSE;

}

BOOL CSerialPort::InitPort(CWnd* pPortOwner, 
						UINT portnr, 
						UINT baud,
						char parity, 
						UINT databits, 
						UINT stopbits,
						DWORD dwCommEvents, 
						UINT writebuffersize,

						DWORD ReadIntervalTimeout,
						DWORD ReadTotalTimeoutMultiplier,
						DWORD ReadTotalTimeoutConstant,
						DWORD WriteTotalTimeoutMultiplier,
						DWORD WriteTotalTimeoutConstant)
{
	assert(portnr > 0 && portnr <200);
	assert(pPortOwner != NULL);

	//if The Thread is alived : kill
	if (m_bThreadAlive)
	{
		do
		{
			SetEvent(m_hShutdownEvent);
		} while (m_bThreadAlive);
	}
	//create Events
	if (m_ov.hEvent != NULL )
		ResetEvent(m_ov.hEvent);
	else
		m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);
	else
		m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	else
		m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//事件数组出事化，设定优先级
	m_hEventArray[0] = m_hShutdownEvent;
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;

	//初始化临界资源
	InitializeCriticalSection(&m_csCommunicationSync);

	//设置缓冲区大小，并保存句柄
	m_pOwner = pPortOwner;

	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	m_szWriteBuffer = new char[writebuffersize];
	
	m_nPortNr = portnr;
	
	m_nWriteBufferSize = writebuffersize;
	m_dwCommEvents = dwCommEvents;

	BOOL bResult = FALSE;
	char *szPort = new char[50];
	char *szBaud = new char[50];

	/*
	多个线程操作相同的数据时，一般是需要按顺序访问的，否则会导致数据错乱，
	无法控制数据，变成随机量。为解决这个问题，就需要引入互斥变量，让每个
	线程都按顺序地访问变量。这样就需要使用EnterCriticalSection和
	LeaveCriticalSection函数
	*/
	EnterCriticalSection(&m_csCommunicationSync);

	//如果串口已打开就关闭
	if (hCom != NULL)
	{
		CloseHandle(hCom);
		hCom = NULL;
	}

	sprintf(szPort, "\\\\.\\COM%d", portnr);//可以显示COM10以上的端口
	int myparity;

	switch (stopbits)
	{
	case 0:
		//mystop = ONESTOPBIT;
		stopbits += 1;
		break;
	case 1:
		//mystop = ONE5STOPBITS;
		stopbits += 0.5;
		break;
	case 2:
		//mystop = TWOSTOPBITS;
		break;
	default:
		break;
	}
	myparity = 0;
	parity = toupper(parity);// 小写字母转换为大写字符
	switch (parity)
	{
	case 'N':
		myparity = 0;
		break;
	case 'O':
		myparity = 1;
		break;
	case 'E':
		myparity = 2;
		break;
	case 'M':
		myparity = 3;
		break;
	case 'S':
		myparity = 4;
		break;
	default:
		break;
	}

	sprintf(szBaud, "baud=%d  parity=%c  data=%d  stop=%d", baud, parity, databits, stopbits);

	/*
	通信程序在CreateFile处指定串口设备及相关的操作属性，再返回一个句柄，
	该句柄将被用于后续的通信操作，并贯穿整个通信过程串口打开后，其属性
	被设置为默认值，根据具体需要，通过调用GetCommState(hComm,&&dcb)读取
	当前串口设备控制块DCB设置，修改后通过SetCommState(hComm,&&dcb)将其写
	入。运用ReadFile()与WriteFile()这两个API函数实现串口读写操作，若为异
	步通信方式，两函数中最后一个参数为指向OVERLAPPED结构的非空指针，在读
	写函数返回值为FALSE的情况下，调用GetLastError()函数，返回值为ERROR_IO_PENDING，
	表明I/O操作悬挂，即操作转入后台继续执行。此时，可以用WaitForSingleObject()
	来等待结束信号并设置最长等待时间
	*/

	hCom = CreateFile(szPort,
					GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,
					0
		);
	//创建失败
	if (hCom == INVALID_HANDLE_VALUE)
	{
		delete[] szPort;
		delete[] szBaud;

		return FALSE;
	}

	//设置超时
	m_CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout * 1000;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant * 1000;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant * 1000;

	//配置串口
	if (SetCommTimeouts(hCom, &m_CommTimeouts))//设置超时
	{
		/*
		若对端口数据的响应时间要求较严格，可采用事件驱动方式。
		事件驱动方式通过设置事件通知，当所希望的事件发生时，Windows
		发出该事件已发生的通知，这与DOS环境下的中断方式很相似。Windows
		定义了9种串口通信事件，较常用的有以下三种：
		EV_RXCHAR:接收到一个字节，并放入输入缓冲区；
		EV_TXEMPTY:输出缓冲区中的最后一个字符，发送出去；
		EV_RXFLAG:接收到事件字符(DCB结构中EvtChar成员)，放入输入缓冲区
		在用SetCommMask()指定了有用的事件后，应用程序可调用WaitCommEvent()来等待事
		件的发生。SetCommMask(hComm,0)可使WaitCommEvent()中止
		*/
		if (SetCommMask(hCom, dwCommEvents))//设置通信事件
		{
			if (GetCommState(hCom, &m_dcb))//获得当前DCB参数
			{
				m_dcb.EvtChar = 'q';
				m_dcb.fRtsControl = RTS_CONTROL_ENABLE;//将RTS位设置为高
				//m_dcb.BaudRate = baud;
				//m_dcb.Parity = myparity;
				//m_dcb.ByteSize = databits;
				//m_dcb.StopBits = mystop;

				if (BuildCommDCB(szBaud, &m_dcb)){//填写DCB结构
				//if (BuildCommDCB("baud=9600 parity=N data=8 stop=1", &m_dcb)){//填写DCB结构
					if (SetCommState(hCom, &m_dcb))//配置DCB
						;
					else

						ProcessErrorMessage("SetCommState()");
				}else
					ProcessErrorMessage("BuildCommDCB()");
			}
			else
				ProcessErrorMessage("GetCommState()");
		}
		else
			ProcessErrorMessage("SetCommMask()");
	}
	else
		ProcessErrorMessage("SetCommTimeouts()");

	delete[] szPort;
	delete[] szBaud;

	//终止读写并清空接收和发送
	PurgeComm(hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

	//释放临界资源
	LeaveCriticalSection(&m_csCommunicationSync);

	return TRUE;
}

//开启监视线程
BOOL CSerialPort::StartMonitoring()
{
	if (!(m_Thread = ::CreateThread(NULL, 0, CommThread, this, 0, NULL)))
		return FALSE;
	else
	{
		return TRUE;
	}
}

//线程函数
//监视线程的大致流程：
//检查串口-->进入循环{WaitCommEvent(不阻塞询问)询问事件-->如果有事件来到-->到相应处理（关闭\读\写）}
DWORD WINAPI CSerialPort::CommThread(LPVOID pParam)
{
	CSerialPort *port = (CSerialPort*)pParam;

	//TRUE代表线程正在运行
	port->m_bThreadAlive = TRUE;

	//初始化各种局部变量
	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	BOOL bResult = TRUE;

	//开始清楚串口缓冲
	if (port->hCom)//检查串口是否已打开
		PurgeComm(port->hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

	//只要线程存在就不停读取数据
	for (;;)
	{
		/*
		WaitCommEvent函数第3个参数1pOverlapped可以是一个OVERLAPPED结构的变量指针
		，也可以是NULL，当用NULL时，表示该函数是同步的，否则表示该函数是异步的。
		调用WaitCommEvent时，如果异步操作不能立即完成，会立即返回FALSE，系统在
		WaitCommEvent返回前将OVERLAPPED结构成员hEvent设为无信号状态，等到产生通信
		事件时，系统将其置有信号
		*/
		bResult = WaitCommEvent(port->hCom, &Event, &port->m_ov);

		if (!bResult)
		{
			///如果WaitCommEvent返回Error为FALSE，则查询错误信息
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING://正常情况，没有字符可读
				break;
			case 87://系统错误
				break;

			default:///发生其他错误，其中有串口读写中断开串口连接的错误
				port->ProcessErrorMessage("WaitCommEvent()");
				break;
			}
		}
		else//WaitCommEven能正确返回
		{
			bResult = ClearCommError(port->hCom, &dwError, &comstat);
			if (comstat.cbInQue == 0)
				continue;
		}
		//主等待函数，会阻塞线程
		//等待三个事件：关断\读\写，有一个发生就返回
		Event = WaitForMultipleObjects(3,//三个事件
			port->m_hEventArray,//事件数组
			FALSE,//有一个发生就返回
			INFINITE);//超时时间

		switch (Event)
		{
		case 0:
			{
				  //关断事件,关闭串口
				  CloseHandle(port->hCom);
				  port->hCom = NULL;
				  port->m_bThreadAlive = FALSE;
				  // Kill this thread.  break is not needed, but makes me feel better.
				  ExitThread(100);

				  break;
			}
		case 1://read event将定义的各种消息发送出去
			{
				   GetCommMask(port->hCom, &CommEvent);
				   if (CommEvent & EV_RXCHAR)//接收到字符，并置于输入缓冲区中
					   ReceiveChar(port);

				   if (CommEvent & EV_CTS)//CTS信号状态发生改变
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_CTS_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_RXFLAG)//接收到事件字符，并置入输入缓冲区中
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RXFLAG_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_BREAK)//输入中发生中断
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_BREAK_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_ERR)//发生线路状态错误，线路状态错误包括CE_FRAME,CE_OVERRUN和CE_RXPARITY
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_ERR_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_RING)//检测到振铃指示
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RING_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);

				   break;
			}
		case 2://发送数据
			{
				   WriteChar(port);
				   break;
			}
		default:
			{
				   AfxMessageBox("接收有问题！");
				   break;
			}
		}
	}
	return 0;
}

//复位监视线程
BOOL CSerialPort::RestartMonitoring()
{
	::ResumeThread(m_Thread);
	return TRUE;
}

//挂起监视线程
BOOL CSerialPort::StopMonitoring()
{
	::SuspendThread(m_Thread);
	return TRUE;
}

//如果有错误，给出提示
//错误处理函数
void CSerialPort::ProcessErrorMessage(char* ErrorText)
{
	char *Temp = new char[200];
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	sprintf(Temp, "WARNING:: %s Failed with the following error:\n%s\nPort:%d\n", (char*)ErrorText, lpMsgBuf, m_nPortNr);
	MessageBox(NULL, Temp, "Application Error", MB_ICONSTOP);

	LocalFree(lpMsgBuf);
	delete[]Temp;
}

//写入一个字符
void CSerialPort::WriteChar(CSerialPort* port)
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;

	DWORD BytesSent = 0;
	DWORD SendLen = port->m_nWriteSize;
	ResetEvent(port->m_hWriteEvent);

	EnterCriticalSection(&port->m_csCommunicationSync);

	if (bWrite)
	{
		port->m_ov.Offset = 0;
		port->m_ov.OffsetHigh = 0;

		PurgeComm(port->hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

		bResult = WriteFile(port->hCom,
							port->m_szWriteBuffer,
							SendLen,
							&BytesSent,
							&port->m_ov);
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
				{
						BytesSent = 0;
						bWrite = FALSE;
						break;
				}
			default:
				{
					   port->ProcessErrorMessage("WriteFile()");
				}
			}
		}//end if(!Result)
		else
		{
			LeaveCriticalSection(&port->m_csCommunicationSync);
		}
	}//end if(bWrite)
	if (!bWrite)
	{
		bWrite = TRUE;

		bResult = GetOverlappedResult(port->hCom, &port->m_ov, &BytesSent, TRUE);

		LeaveCriticalSection(&port->m_csCommunicationSync);

		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in WriteFile()");
		}
	}//end if(!bWrite)
}

//接收到字符，通知通信线程
void CSerialPort::ReceiveChar(CSerialPort* port)
{
	BOOL bRead = TRUE;
	BOOL bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	COMSTAT comstat;
	unsigned char RXBuff;

	for (;;)
	{
		//防止死锁
		if (WaitForSingleObject(port->m_hShutdownEvent, 0) == WAIT_OBJECT_0)
			return;

		EnterCriticalSection(&port->m_csCommunicationSync);

		bResult = ClearCommError(port->hCom, &dwError, &comstat);

		LeaveCriticalSection(&port->m_csCommunicationSync);

		if (comstat.cbInQue == 0)
			break;

		EnterCriticalSection(&port->m_csCommunicationSync);

		if (bRead)
		{
			//串口读出，读出缓冲区中的字节
			bResult = ReadFile(port->hCom,
								&RXBuff,
								1,
								&BytesRead,
								&port->m_ov);
			//如果返回错误，进行错误处理
			if (!bResult)
			{
				switch (dwError = GetLastError())
				{
				case ERROR_IO_PENDING:
					{
						//异步IO仍在进行
						bRead = FALSE;
						break;
					}
				default:
					{
						port->ProcessErrorMessage("ReadFile()");
						break;
						//防止读写数据时，串口非正常断开导致死循环一直执行
					}
				}
			}//end if(!bResult)
			else
			{
				bRead = TRUE;
			}
		}//end if(bRead)
		//异步IO操作仍在执行，需要调用GetOverlappedResult查询
		if (!bRead)
		{
			bRead = TRUE;
			bResult = GetOverlappedResult(port->hCom, &port->m_ov, &BytesRead, TRUE);
			if (!bResult)
				port->ProcessErrorMessage("GetOverlappedResults() in ReadFile()");
		}//end if(!Read)

		LeaveCriticalSection(&port->m_csCommunicationSync);

		::SendMessage((port->m_pOwner->m_hWnd), WM_COMM_RXCHAR, (WPARAM)RXBuff, (LPARAM)port->m_nPortNr);
	}
}

// Write a string to the port
void CSerialPort::WriteToPort(char* string)
{
	assert(hCom != 0);

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	strcpy(m_szWriteBuffer, string);
	m_nWriteSize = strlen(string);
	SetEvent(m_hWriteEvent);
}

DCB CSerialPort::GetDCB()
{
	return m_dcb;
}

DWORD CSerialPort::GetCommEvents()
{
	return m_dwCommEvents;
}

DWORD CSerialPort::GetWriteBufferSize()
{
	return m_nWriteBufferSize;
}

BOOL CSerialPort::IsOpen()
{
	return hCom != NULL;
}
//关闭串口
void CSerialPort::ClosePort()
{
	MSG message;
	//防止死锁
	do
	{
		SetEvent(m_hShutdownEvent);
		if (::PeekMessage(&message, m_pOwner->m_hWnd, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
	} while (m_bThreadAlive);

	//如果串口还打开则关闭
	if (hCom != NULL)
	{
		CloseHandle(hCom);
		hCom = NULL;
	}
	//close handle
	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);

	//释放接收缓冲区内存
	if (m_szWriteBuffer != NULL)
	{
		delete[] m_szWriteBuffer;
		m_szWriteBuffer = NULL;
	}

}

void CSerialPort::WriteToPort(char* string, int n)
{
	assert(hCom != 0);

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	//strcpy(m_szWriteBuffer, string);
	memcpy(m_szWriteBuffer, string, n);
	m_nWriteSize = n;
	SetEvent(m_hWriteEvent);
}

void CSerialPort::WriteToPort(LPCTSTR string)
{
	assert(hCom != 0);

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	strcpy(m_szWriteBuffer, string);
	m_nWriteSize = strlen(string);
	SetEvent(m_hWriteEvent);
}

void CSerialPort::WriteToPort(BYTE* Buffer, int n)
{
	assert(hCom != 0);

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	int i;
	for (i = 0; i < n; i++)
	{
		m_szWriteBuffer[i] = Buffer[i];
	}
	m_nWriteSize = n;
	SetEvent(m_hWriteEvent);
}
//发送数据
void CSerialPort::SendData(LPCTSTR lpszData, const int nLength)
{
		assert(hCom != 0);

		memset(m_szWriteBuffer, 0, nLength);
		strcpy(m_szWriteBuffer, lpszData);
		m_nWriteSize = nLength;
		SetEvent(m_hWriteEvent);
}
//接收数据
BOOL CSerialPort::RecvData(LPCTSTR lpszData, const int nSize)
{
	assert(hCom != 0);
	memset(m_szWriteBuffer, 0, nSize);
	DWORD mylen = 0;
	DWORD mylen2 = 0;
	while (mylen < nSize)
	{
		if (!ReadFile(hCom, (LPVOID)lpszData, nSize, &mylen2, NULL))
			return FALSE;
		mylen += mylen2;
	}
	return TRUE;
}

//查询注册表的串口号，将值存于数组中
void CSerialPort::QueryKey(HKEY hKey)
{
#define	MAX_KEY_LENGTH 255;
#define MAX_VALUE_NAME 16383

	TCHAR	achClass[MAX_PATH] = TEXT("");
	DWORD	cchClassName = MAX_PATH;
	DWORD	cSubKeys = 0;
	DWORD	cbMaxSubKey;
	DWORD	cchMaxClass;
	DWORD	cValues;
	DWORD	cchMaxValue;
	DWORD	cbMaxValueData;
	DWORD	cbSecurityDescriptor;
	FILETIME	ftLastWriteTime;

	DWORD	i, retCode;

	TCHAR	achValue[MAX_VALUE_NAME];
	DWORD	cchValue = MAX_VALUE_NAME;

	retCode = RegQueryInfoKey(
		hKey,
		achClass,
		&cchClassName,
		NULL,
		&cSubKeys,
		&cbMaxSubKey,
		&cchMaxClass,
		&cValues,
		&cchMaxValue,
		&cbMaxValueData,
		&cbSecurityDescriptor,
		&ftLastWriteTime);
	//初始化存放串口端口号的数组
	for (i = 0; i < 20; i++)
	{
		m_nComArray[i] = -1;
	}

	if (cValues > 0){
		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++){
			cchValue = MAX_VALUE_NAME;	achValue[0] = '\0';
			if (ERROR_SUCCESS == RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL)){
				CString szName(achValue);
				if (-1 != szName.Find(_T("Serial")) || -1 != szName.Find(_T("VSerial"))){
				//if (-1 != szName.Find(_T("VSerial7_"))){
					BYTE strDSName[10];		memset(strDSName, 0, 10);
					DWORD nValueType = 0, nBuffLen = 10;
					if (ERROR_SUCCESS == RegQueryValueEx(hKey, (LPCTSTR)achValue, NULL, &nValueType, strDSName, &nBuffLen)){
						int nIndex = -1;
						while (++nIndex < MaxSerialPortNum){
							if (-1 == m_nComArray[nIndex]){
								m_nComArray[nIndex] = atoi((char*)(strDSName + 3));
								break;
							}
						}
					}
				}
			}
		}
	}
	else{
		AfxMessageBox(_T("找不到串口！"));
	}

}

void CSerialPort::Hkey2ComboBox(CComboBox& m_PortNO)
{
	HKEY hTestKey;
	bool Flag = FALSE;
	//仅仅是XP系统的注册表位置，其他系统根据实际情况做修改
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hTestKey)){
		QueryKey(hTestKey);
	}
	RegCloseKey(hTestKey);

	int i = 0;
	m_PortNO.ResetContent();//刷新时清空下拉列表
	while (i < MaxSerialPortNum && -1 != m_nComArray[i])
	{
		CString szCom;
		szCom.Format(_T("COM%d"), m_nComArray[i]);
		m_PortNO.InsertString(i, szCom.GetBuffer(5));
		++i;
		Flag = TRUE;
		if (Flag)//将第一个发现的串口设为下拉列表默认值
			m_PortNO.SetCurSel(0);
	}
}

CSerialPort::~CSerialPort()
{
	do{
		SetEvent(m_hShutdownEvent);
	} while (m_bThreadAlive);

	if (hCom != NULL){
		CloseHandle(hCom);
		hCom = NULL;
	}

	if (m_hShutdownEvent != NULL)
		CloseHandle(m_hShutdownEvent);
	if (m_ov.hEvent != NULL)
		CloseHandle(m_ov.hEvent);
	if (m_hWriteEvent != NULL)
		CloseHandle(m_hWriteEvent);

	delete[] m_szWriteBuffer;
}
