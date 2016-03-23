#include "StdAfx.h"
#include "SerialPortThread.h"

HANDLE hCom; //ȫ�ֱ��������ھ��
HANDLE hCommThread; //ȫ�ֱ����������߳�
DWORD dwThreadID;
OVERLAPPED m_osRead, m_osWrite;
CString strRec;

BOOL OpenSerialPort(LPCWSTR lpFileName)   //��ʼ������
{
	//�򿪲�����COM1
	hCom = CreateFile(
						lpFileName,						//��Ҫ�򿪵Ĵ����߼�������COM1 ��COM2
						GENERIC_READ | GENERIC_WRITE,	//ָ�����ڷ��ʵ����ͣ������Ƕ�ȡ��д������߲���
						0,								//ָ���������ԣ����ڴ��ڲ��ܹ���,�ò���������Ϊ0
						NULL,							//���ð�ȫ�����Խṹ��ȱʡֵΪNULL
						OPEN_EXISTING,					//������־���Դ��ڲ����ò���������ΪOPEN_EXISTING
						FILE_FLAG_OVERLAPPED,			//��������������ָ���ô����Ƿ�ɽ����첽����
														//FILE_FLAG_OVERLAPPED����ʹ���첽��I/O
						NULL);							//ָ��ģ���ļ��ľ�����Դ��ڶ��Ըò���������ΪNULL
	//�ж��Ƿ�ɹ��򿪴���
	if (hCom == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(L"��COM9ʧ��");
		return false;
	}
	
	//���ڴ򿪳ɹ������ô��ڲ���
	else
	{
		AfxMessageBox(L"��COM9�ɹ�");
		COMMTIMEOUTS TimeOuts;
		//�趨����ʱ
		TimeOuts.ReadIntervalTimeout = MAXDWORD;
		TimeOuts.ReadTotalTimeoutMultiplier = 0;
		TimeOuts.ReadTotalTimeoutConstant = 0;
		//�ڶ�һ�����뻺���������ݺ���������������أ�
		//�������Ƿ������Ҫ����ַ���


		//�趨д��ʱ
		TimeOuts.WriteTotalTimeoutMultiplier = 100;
		TimeOuts.WriteTotalTimeoutConstant = 500;
		SetCommTimeouts(hCom, &TimeOuts); //���ó�ʱ
		SetupComm(hCom, 1024, 512); //���뻺����������������Ĵ�С����1024
		
		/*************���ô��ڲ���********************/
		DCB wdcb;
		GetCommState(hCom, &wdcb);
		wdcb.BaudRate = 9600;//�����ʣ�9600������������
		wdcb.ByteSize = 8;   //ÿ���ֽ�8λ
		wdcb.StopBits = ONESTOPBIT; //ֹͣλ1
		wdcb.fBinary = true;		//��������ƴ���
		wdcb.fParity = true;		//������żУ�죬���忴Parity������
		wdcb.Parity = NOPARITY;  //����żУ��λ
		SetCommState(hCom, &wdcb);
		PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);//�������/���ͻ�����
	}
	return true;
}

//��һ���̼߳�ش��ڽ��յ�����
DWORD WINAPI CommWatchProc(HWND hWnd)//�����ھ��
{
	/*
	//char str[1024];
	DWORD wCount = 0; //��ȡ���ֽ���
	while (1)
	{
		ReadFile(hCom, str, 100, &wCount, NULL);
		if (wCount > 0) //�յ�����
		{
			str[wCount] = '\0';
			AfxMessageBox(L"���յ�����");
			::PostMessage(hWnd, COM_RECVDATA, (unsigned int)str, wCount);
			//������Ϣ���Ի��������ڣ��Խ��н������ݵ���ʾ
		}
	}
	*/
	COMSTAT commStat;
	OVERLAPPED os;//�첽״̬����/���
	os.hEvent = NULL;
	os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	os.Offset = 0;
	os.OffsetHigh = 0;
	m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	char czReceiveBuffer[512];
	DWORD dErrInformation;
	memset(czReceiveBuffer, 0, sizeof(czReceiveBuffer));
	//�趨�����ж�
	SetCommMask(hCom, EV_RXCHAR);
	if (hCom != NULL)
	{
		//��մ�����Ϣ
		ClearCommError(hCom, &dErrInformation, &commStat);
	}

	while (TRUE)
	{
		DWORD wEven;
		//�ȴ��¼��Ĳ���
		INT bResult = WaitCommEvent(hCom, &wEven, &os);
		//������0������GetLastError����ѯ��������
		if (!bResult)
		{
			switch (GetLastError())
			{
			case ERROR_IO_PENDING:
				break;
			case 87:
				break;
			default:
				break;
			}
		}
		//������1�������ClearCommError������󣬲���ѯ���ڵ�״̬���õ�commstat��ֵ
		else
		{
			ClearCommError(hCom, &dErrInformation, &commStat);
			//���ClearCommError����ѯ�Ĵ���״̬commstat.cbInQue��ֵ����������0˵�����󣬼����ȴ��¼��Ĵ���
			if (commStat.cbInQue == 0)
			{
				//��ֹ����ѭ��
				continue;
			}
		}
		//������0��˵��buffer�������ַ�
		//��ʹ��WaitForSingleObject����ȷ���¼�����ִ����Ӧ������
		//INFINITE��ʾ������ֱ����Ӧʱ���¼�������ź�״̬�ŷ��أ������һֱ�ȴ���ȥ��ֱ��WaitForSingleObject�з���ֱ��ִ�к���Ĵ���
		//���ź�״̬����WAIT_OBJECT_0
		INT nEvent = WaitForSingleObject(os.hEvent, INFINITE);
		if (nEvent == WAIT_OBJECT_0)
		{
			DWORD CommEvent = 0;
			//��ô����¼�
			GetCommMask(hCom, &CommEvent);
			if ((CommEvent&EV_RXCHAR) == EV_RXCHAR)//���뻺�������յ����ַ�
			{
				BOOL bread = TRUE;
				BOOL bresult = TRUE;
				DWORD deError = 0;
				DWORD BytesRead = 0;

				COMSTAT comstat;
				for (;;)
				{
					DWORD dRBufferSize = 100;
					INT bResult = ClearCommError(hCom, &deError, &comstat);
					if (comstat.cbInQue == 0)
					{
						break;
					}
					if (bread)
					{
						strRec = "";
						//�ӽ��ջ�������ȡ����
						bresult = ReadFile(hCom, //�豸���
											czReceiveBuffer, //��������ַ�������
											100, //��ȡ�ĳ���
											&dRBufferSize, //ʵ�ʶ�ȡ���ֽڳ���
											&os	//OVERLAPPED�ṹ�����
											); 
						if (!bresult)
						{
							switch (deError = GetLastError())
							{
								case ERROR_IO_PENDING:
								{
									bread = FALSE;
									break;
								}
								default:
								{
									break;
								}
							}
						}
						else
						{
							bread = TRUE;
						}
					}
					if (!bread)
					{
						bread = TRUE;
						bresult = GetOverlappedResult
							(
							hCom,
							&os,
							&BytesRead,
							TRUE
							);
						if (!bresult)
						{
						}
					}
					PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
					for (DWORD k = 0; k < dRBufferSize; k++)
					{
						strRec += czReceiveBuffer[k];
					}
					::PostMessage(hWnd, COM_RECVDATA, (unsigned int)czReceiveBuffer, dRBufferSize);
				}
			}
		}
		if (nEvent != WAIT_OBJECT_0)
		{
			GetLastError();
		}
		//return TRUE;
	}
}

