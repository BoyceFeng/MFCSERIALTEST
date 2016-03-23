
// MFCSERIALTESTDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "SERIALPORT.h"

// CMFCSERIALTESTDlg �Ի���
class CMFCSERIALTESTDlg : public CDialogEx
{
// ����
public:
	CMFCSERIALTESTDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MFCSERIALTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	afx_msg LRESULT OnRecvData(WPARAM wParam, LPARAM lParam);


// ʵ��
protected:
	HICON m_hIcon;
	CSerialPort m_SerialPort;
	//HWND hWnd;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//afx_msg LRESULT OnComm(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CString m_Receive;
	CString m_Send;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton3();
	CComboBox m_ComPortNO;
	CComboBox m_PortBaud;
	CComboBox m_PortDataBits;
	CComboBox m_PortStopBits;
	CComboBox m_PortParity;
	afx_msg void OnBnClicked_ClosePort();

	long count;
};
