#include "StdAfx.h"
#include "SerialPortThread.h"

HANDLE hCom; //全局变量，串口句柄
HANDLE hCommThread; //全局变量，串口线程
DWORD dwThreadID;
OVERLAPPED m_osRead, m_osWrite;
CString strRec;

BOOL OpenSerialPort(LPCWSTR lpFileName)   //初始化串口
{
	//打开并设置COM1
	hCom = CreateFile(
						lpFileName,						//将要打开的串口逻辑名，如COM1 或COM2
						GENERIC_READ | GENERIC_WRITE,	//指定串口访问的类型，可以是读取、写入或两者并列
						0,								//指定共享属性，由于串口不能共享,该参数必须置为0
						NULL,							//引用安全性属性结构，缺省值为NULL
						OPEN_EXISTING,					//创建标志，对串口操作该参数必须置为OPEN_EXISTING
						FILE_FLAG_OVERLAPPED,			//属性描述，用于指定该串口是否可进行异步操作
														//FILE_FLAG_OVERLAPPED：可使用异步的I/O
						NULL);							//指向模板文件的句柄，对串口而言该参数必须置为NULL
	//判读是否成功打开串口
	if (hCom == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(L"打开COM9失败");
		return false;
	}
	
	//串口打开成功后，设置串口参数
	else
	{
		AfxMessageBox(L"打开COM9成功");
		COMMTIMEOUTS TimeOuts;
		//设定读超时
		TimeOuts.ReadIntervalTimeout = MAXDWORD;
		TimeOuts.ReadTotalTimeoutMultiplier = 0;
		TimeOuts.ReadTotalTimeoutConstant = 0;
		//在读一次输入缓冲区的内容后读操作就立即返回，
		//而不管是否读入了要求的字符。


		//设定写超时
		TimeOuts.WriteTotalTimeoutMultiplier = 100;
		TimeOuts.WriteTotalTimeoutConstant = 500;
		SetCommTimeouts(hCom, &TimeOuts); //设置超时
		SetupComm(hCom, 1024, 512); //输入缓冲区和输出缓冲区的大小都是1024
		
		/*************设置串口参数********************/
		DCB wdcb;
		GetCommState(hCom, &wdcb);
		wdcb.BaudRate = 9600;//波特率：9600，其他：不变
		wdcb.ByteSize = 8;   //每个字节8位
		wdcb.StopBits = ONESTOPBIT; //停止位1
		wdcb.fBinary = true;		//允许二进制传输
		wdcb.fParity = true;		//允许奇偶校检，具体看Parity的设置
		wdcb.Parity = NOPARITY;  //无奇偶校检位
		SetCommState(hCom, &wdcb);
		PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);//清楚接收/发送缓冲区
	}
	return true;
}

//以一个线程监控串口接收的数据
DWORD WINAPI CommWatchProc(HWND hWnd)//主窗口句柄
{
	/*
	//char str[1024];
	DWORD wCount = 0; //读取的字节数
	while (1)
	{
		ReadFile(hCom, str, 100, &wCount, NULL);
		if (wCount > 0) //收到数据
		{
			str[wCount] = '\0';
			AfxMessageBox(L"接收到数据");
			::PostMessage(hWnd, COM_RECVDATA, (unsigned int)str, wCount);
			//发送消息给对话框主窗口，以进行接收内容的显示
		}
	}
	*/
	COMSTAT commStat;
	OVERLAPPED os;//异步状态输入/输出
	os.hEvent = NULL;
	os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	os.Offset = 0;
	os.OffsetHigh = 0;
	m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	char czReceiveBuffer[512];
	DWORD dErrInformation;
	memset(czReceiveBuffer, 0, sizeof(czReceiveBuffer));
	//设定接收中断
	SetCommMask(hCom, EV_RXCHAR);
	if (hCom != NULL)
	{
		//清空错误信息
		ClearCommError(hCom, &dErrInformation, &commStat);
	}

	while (TRUE)
	{
		DWORD wEven;
		//等待事件的产生
		INT bResult = WaitCommEvent(hCom, &wEven, &os);
		//若返回0，调用GetLastError来查询错误类型
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
		//若返回1，则调用ClearCommError清除错误，并查询串口的状态，得到commstat的值
		else
		{
			ClearCommError(hCom, &dErrInformation, &commStat);
			//检查ClearCommError所查询的串口状态commstat.cbInQue的值，若不大于0说明错误，继续等待事件的触发
			if (commStat.cbInQue == 0)
			{
				//终止本次循环
				continue;
			}
		}
		//若大于0，说明buffer里面有字符
		//再使用WaitForSingleObject函数确认事件，并执行相应操作。
		//INFINITE表示函数将直到相应时间事件变成有信号状态才返回，否则就一直等待下去，直到WaitForSingleObject有返回直才执行后面的代码
		//有信号状态返回WAIT_OBJECT_0
		INT nEvent = WaitForSingleObject(os.hEvent, INFINITE);
		if (nEvent == WAIT_OBJECT_0)
		{
			DWORD CommEvent = 0;
			//获得串口事件
			GetCommMask(hCom, &CommEvent);
			if ((CommEvent&EV_RXCHAR) == EV_RXCHAR)//输入缓冲区接收到新字符
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
						//从接收缓冲区读取数据
						bresult = ReadFile(hCom, //设备句柄
											czReceiveBuffer, //保存接收字符的数组
											100, //读取的长度
											&dRBufferSize, //实际读取的字节长度
											&os	//OVERLAPPED结构体变量
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

