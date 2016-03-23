#include "stdafx.h"
#include "SERIALPORT.h"

int m_nComArray[20];

CSerialPort::CSerialPort()
{
	hCom = NULL;

	//��ʼ���첽�ṹ��
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

	//�¼�������»����趨���ȼ�
	m_hEventArray[0] = m_hShutdownEvent;
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;

	//��ʼ���ٽ���Դ
	InitializeCriticalSection(&m_csCommunicationSync);

	//���û�������С����������
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
	����̲߳�����ͬ������ʱ��һ������Ҫ��˳����ʵģ�����ᵼ�����ݴ��ң�
	�޷��������ݣ�����������Ϊ���������⣬����Ҫ���뻥���������ÿ��
	�̶߳���˳��ط��ʱ�������������Ҫʹ��EnterCriticalSection��
	LeaveCriticalSection����
	*/
	EnterCriticalSection(&m_csCommunicationSync);

	//��������Ѵ򿪾͹ر�
	if (hCom != NULL)
	{
		CloseHandle(hCom);
		hCom = NULL;
	}

	sprintf(szPort, "\\\\.\\COM%d", portnr);//������ʾCOM10���ϵĶ˿�
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
	parity = toupper(parity);// Сд��ĸת��Ϊ��д�ַ�
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
	ͨ�ų�����CreateFile��ָ�������豸����صĲ������ԣ��ٷ���һ�������
	�þ���������ں�����ͨ�Ų��������ᴩ����ͨ�Ź��̴��ڴ򿪺�������
	������ΪĬ��ֵ�����ݾ�����Ҫ��ͨ������GetCommState(hComm,&&dcb)��ȡ
	��ǰ�����豸���ƿ�DCB���ã��޸ĺ�ͨ��SetCommState(hComm,&&dcb)����д
	�롣����ReadFile()��WriteFile()������API����ʵ�ִ��ڶ�д��������Ϊ��
	��ͨ�ŷ�ʽ�������������һ������Ϊָ��OVERLAPPED�ṹ�ķǿ�ָ�룬�ڶ�
	д��������ֵΪFALSE������£�����GetLastError()����������ֵΪERROR_IO_PENDING��
	����I/O�������ң�������ת���̨����ִ�С���ʱ��������WaitForSingleObject()
	���ȴ������źŲ�������ȴ�ʱ��
	*/

	hCom = CreateFile(szPort,
					GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,
					0
		);
	//����ʧ��
	if (hCom == INVALID_HANDLE_VALUE)
	{
		delete[] szPort;
		delete[] szBaud;

		return FALSE;
	}

	//���ó�ʱ
	m_CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout * 1000;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant * 1000;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant * 1000;

	//���ô���
	if (SetCommTimeouts(hCom, &m_CommTimeouts))//���ó�ʱ
	{
		/*
		���Զ˿����ݵ���Ӧʱ��Ҫ����ϸ񣬿ɲ����¼�������ʽ��
		�¼�������ʽͨ�������¼�֪ͨ������ϣ�����¼�����ʱ��Windows
		�������¼��ѷ�����֪ͨ������DOS�����µ��жϷ�ʽ�����ơ�Windows
		������9�ִ���ͨ���¼����ϳ��õ����������֣�
		EV_RXCHAR:���յ�һ���ֽڣ����������뻺������
		EV_TXEMPTY:����������е����һ���ַ������ͳ�ȥ��
		EV_RXFLAG:���յ��¼��ַ�(DCB�ṹ��EvtChar��Ա)���������뻺����
		����SetCommMask()ָ�������õ��¼���Ӧ�ó���ɵ���WaitCommEvent()���ȴ���
		���ķ�����SetCommMask(hComm,0)��ʹWaitCommEvent()��ֹ
		*/
		if (SetCommMask(hCom, dwCommEvents))//����ͨ���¼�
		{
			if (GetCommState(hCom, &m_dcb))//��õ�ǰDCB����
			{
				m_dcb.EvtChar = 'q';
				m_dcb.fRtsControl = RTS_CONTROL_ENABLE;//��RTSλ����Ϊ��
				//m_dcb.BaudRate = baud;
				//m_dcb.Parity = myparity;
				//m_dcb.ByteSize = databits;
				//m_dcb.StopBits = mystop;

				if (BuildCommDCB(szBaud, &m_dcb)){//��дDCB�ṹ
				//if (BuildCommDCB("baud=9600 parity=N data=8 stop=1", &m_dcb)){//��дDCB�ṹ
					if (SetCommState(hCom, &m_dcb))//����DCB
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

	//��ֹ��д����ս��պͷ���
	PurgeComm(hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

	//�ͷ��ٽ���Դ
	LeaveCriticalSection(&m_csCommunicationSync);

	return TRUE;
}

//���������߳�
BOOL CSerialPort::StartMonitoring()
{
	if (!(m_Thread = ::CreateThread(NULL, 0, CommThread, this, 0, NULL)))
		return FALSE;
	else
	{
		return TRUE;
	}
}

//�̺߳���
//�����̵߳Ĵ������̣�
//��鴮��-->����ѭ��{WaitCommEvent(������ѯ��)ѯ���¼�-->������¼�����-->����Ӧ�����ر�\��\д��}
DWORD WINAPI CSerialPort::CommThread(LPVOID pParam)
{
	CSerialPort *port = (CSerialPort*)pParam;

	//TRUE�����߳���������
	port->m_bThreadAlive = TRUE;

	//��ʼ�����־ֲ�����
	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	BOOL bResult = TRUE;

	//��ʼ������ڻ���
	if (port->hCom)//��鴮���Ƿ��Ѵ�
		PurgeComm(port->hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

	//ֻҪ�̴߳��ھͲ�ͣ��ȡ����
	for (;;)
	{
		/*
		WaitCommEvent������3������1pOverlapped������һ��OVERLAPPED�ṹ�ı���ָ��
		��Ҳ������NULL������NULLʱ����ʾ�ú�����ͬ���ģ������ʾ�ú������첽�ġ�
		����WaitCommEventʱ������첽��������������ɣ�����������FALSE��ϵͳ��
		WaitCommEvent����ǰ��OVERLAPPED�ṹ��ԱhEvent��Ϊ���ź�״̬���ȵ�����ͨ��
		�¼�ʱ��ϵͳ���������ź�
		*/
		bResult = WaitCommEvent(port->hCom, &Event, &port->m_ov);

		if (!bResult)
		{
			///���WaitCommEvent����ErrorΪFALSE�����ѯ������Ϣ
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING://���������û���ַ��ɶ�
				break;
			case 87://ϵͳ����
				break;

			default:///�����������������д��ڶ�д�жϿ��������ӵĴ���
				port->ProcessErrorMessage("WaitCommEvent()");
				break;
			}
		}
		else//WaitCommEven����ȷ����
		{
			bResult = ClearCommError(port->hCom, &dwError, &comstat);
			if (comstat.cbInQue == 0)
				continue;
		}
		//���ȴ��������������߳�
		//�ȴ������¼����ض�\��\д����һ�������ͷ���
		Event = WaitForMultipleObjects(3,//�����¼�
			port->m_hEventArray,//�¼�����
			FALSE,//��һ�������ͷ���
			INFINITE);//��ʱʱ��

		switch (Event)
		{
		case 0:
			{
				  //�ض��¼�,�رմ���
				  CloseHandle(port->hCom);
				  port->hCom = NULL;
				  port->m_bThreadAlive = FALSE;
				  // Kill this thread.  break is not needed, but makes me feel better.
				  ExitThread(100);

				  break;
			}
		case 1://read event������ĸ�����Ϣ���ͳ�ȥ
			{
				   GetCommMask(port->hCom, &CommEvent);
				   if (CommEvent & EV_RXCHAR)//���յ��ַ������������뻺������
					   ReceiveChar(port);

				   if (CommEvent & EV_CTS)//CTS�ź�״̬�����ı�
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_CTS_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_RXFLAG)//���յ��¼��ַ������������뻺������
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RXFLAG_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_BREAK)//�����з����ж�
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_BREAK_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_ERR)//������·״̬������·״̬�������CE_FRAME,CE_OVERRUN��CE_RXPARITY
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_ERR_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
				   if (CommEvent & EV_RING)//��⵽����ָʾ
					   ::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RING_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);

				   break;
			}
		case 2://��������
			{
				   WriteChar(port);
				   break;
			}
		default:
			{
				   AfxMessageBox("���������⣡");
				   break;
			}
		}
	}
	return 0;
}

//��λ�����߳�
BOOL CSerialPort::RestartMonitoring()
{
	::ResumeThread(m_Thread);
	return TRUE;
}

//��������߳�
BOOL CSerialPort::StopMonitoring()
{
	::SuspendThread(m_Thread);
	return TRUE;
}

//����д��󣬸�����ʾ
//��������
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

//д��һ���ַ�
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

//���յ��ַ���֪ͨͨ���߳�
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
		//��ֹ����
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
			//���ڶ����������������е��ֽ�
			bResult = ReadFile(port->hCom,
								&RXBuff,
								1,
								&BytesRead,
								&port->m_ov);
			//������ش��󣬽��д�����
			if (!bResult)
			{
				switch (dwError = GetLastError())
				{
				case ERROR_IO_PENDING:
					{
						//�첽IO���ڽ���
						bRead = FALSE;
						break;
					}
				default:
					{
						port->ProcessErrorMessage("ReadFile()");
						break;
						//��ֹ��д����ʱ�����ڷ������Ͽ�������ѭ��һֱִ��
					}
				}
			}//end if(!bResult)
			else
			{
				bRead = TRUE;
			}
		}//end if(bRead)
		//�첽IO��������ִ�У���Ҫ����GetOverlappedResult��ѯ
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
//�رմ���
void CSerialPort::ClosePort()
{
	MSG message;
	//��ֹ����
	do
	{
		SetEvent(m_hShutdownEvent);
		if (::PeekMessage(&message, m_pOwner->m_hWnd, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
	} while (m_bThreadAlive);

	//������ڻ�����ر�
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

	//�ͷŽ��ջ������ڴ�
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
//��������
void CSerialPort::SendData(LPCTSTR lpszData, const int nLength)
{
		assert(hCom != 0);

		memset(m_szWriteBuffer, 0, nLength);
		strcpy(m_szWriteBuffer, lpszData);
		m_nWriteSize = nLength;
		SetEvent(m_hWriteEvent);
}
//��������
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

//��ѯע���Ĵ��ںţ���ֵ����������
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
	//��ʼ����Ŵ��ڶ˿ںŵ�����
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
		AfxMessageBox(_T("�Ҳ������ڣ�"));
	}

}

void CSerialPort::Hkey2ComboBox(CComboBox& m_PortNO)
{
	HKEY hTestKey;
	bool Flag = FALSE;
	//������XPϵͳ��ע���λ�ã�����ϵͳ����ʵ��������޸�
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hTestKey)){
		QueryKey(hTestKey);
	}
	RegCloseKey(hTestKey);

	int i = 0;
	m_PortNO.ResetContent();//ˢ��ʱ��������б�
	while (i < MaxSerialPortNum && -1 != m_nComArray[i])
	{
		CString szCom;
		szCom.Format(_T("COM%d"), m_nComArray[i]);
		m_PortNO.InsertString(i, szCom.GetBuffer(5));
		++i;
		Flag = TRUE;
		if (Flag)//����һ�����ֵĴ�����Ϊ�����б�Ĭ��ֵ
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
