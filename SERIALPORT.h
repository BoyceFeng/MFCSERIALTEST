#pragma once
#include "stdafx.h"

#ifndef WM_COMM_MSG_BASE 
#define WM_COMM_MSG_BASE		WM_USER + 617		//!< ��Ϣ��ŵĻ���  
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

#define MaxSerialPortNum 20   ///��Ч�Ĵ����ܸ��������Ǵ��ڵĺ�

class CSerialPort
{
public:
	//���캯��
	CSerialPort();
	//��������
	~CSerialPort();

	//��ô���
	void GetCOMPort(CString* str);

	//��ʼ�����򿪴���
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

	//���ƴ��ڼ����߳�
	BOOL StartMonitoring();//��ʼ����
	BOOL RestartMonitoring();//���¼���
	BOOL StopMonitoring();//ֹͣ����

	DWORD GetWriteBufferSize();//��ȡд��������С
	DWORD GetCommEvents();//��ȡ�¼�
	DCB GetDCB();//��ȡDCB

	//��������
	void WriteToPort(char* string);
	void WriteToPort(char* string, int n);
	void WriteToPort(LPCTSTR string);
	void WriteToPort(LPCTSTR string, int n);
	void WriteToPort(BYTE* Buffer, int n);
	void ClosePort();
	BOOL IsOpen();

	void SendData(LPCTSTR lpszData, const int nLength);
	BOOL RecvData(LPCTSTR lpszData, const int nSize);
	void QueryKey(HKEY hkey);//��ѯע���Ĵ��ںţ���ֵ����������
	void Hkey2ComboBox(CComboBox& m_PortNO);//����ѯ���Ĵ��ں���ӵ�CComboBox�ؼ���

protected:
	void ProcessErrorMessage(char* EvenText);//������
	static DWORD WINAPI CommThread(LPVOID pParam);//�̺߳���
	static void ReceiveChar(CSerialPort* port);
	static void WriteChar(CSerialPort* port);


	HANDLE m_Thread;	//�߳̾��
	BOOL m_bIsSusupened;//�����߳��Ƿ����	

	//synchronisation;ͬ��
	CRITICAL_SECTION m_csCommunicationSync;//�ٽ���Դ
	BOOL m_bThreadAlive;	//thread�����߳��Ƿ����
	
	
	HANDLE hCom;		//���ھ��
	HANDLE m_hWriteEvent;//д�¼�
	HANDLE m_hShutdownEvent;

	/*�¼�����*/
	//����д�¼��������¼����ر��¼�
	///һ��Ԫ������һ���¼����������¼��̴߳���˿ڡ�
	///д�¼��ͽ����ַ��¼�λ��overlapped�ṹ�壨m_ov.hEvent����
	///���˿ڹر�ʱ����һ��ͨ�õĹرա�
	HANDLE m_hEventArray[3];

	OVERLAPPED m_ov;//�첽I/O
	COMMTIMEOUTS m_CommTimeouts;//��ʱ����
	DCB m_dcb;//�豸����ģ��

	CWnd* m_pOwner;//������

	UINT m_nPortNr;
	char* m_szWriteBuffer;//д������
	DWORD m_dwCommEvents;
	DWORD m_nWriteBufferSize;//д��������С

	int	m_nWriteSize;//д���ֽ���

};

