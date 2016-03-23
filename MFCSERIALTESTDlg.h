
// MFCSERIALTESTDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "SERIALPORT.h"

// CMFCSERIALTESTDlg 对话框
class CMFCSERIALTESTDlg : public CDialogEx
{
// 构造
public:
	CMFCSERIALTESTDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFCSERIALTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	afx_msg LRESULT OnRecvData(WPARAM wParam, LPARAM lParam);


// 实现
protected:
	HICON m_hIcon;
	CSerialPort m_SerialPort;
	//HWND hWnd;

	// 生成的消息映射函数
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
