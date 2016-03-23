
// MFCSERIALTESTDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCSERIALTEST.h"
#include "MFCSERIALTESTDlg.h"
#include "afxdialogex.h"
#include "SERIALPORT.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCSERIALTESTDlg 对话框



CMFCSERIALTESTDlg::CMFCSERIALTESTDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCSERIALTESTDlg::IDD, pParent)
	, m_Receive(_T(""))
	, m_Send(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCSERIALTESTDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Receive);
	DDX_Text(pDX, IDC_EDIT2, m_Send);
	DDX_Control(pDX, IDC_COMBO2, m_ComPortNO);
	DDX_Control(pDX, IDC_COMBO1, m_PortBaud);
	DDX_Control(pDX, IDC_COMBO3, m_PortDataBits);
	DDX_Control(pDX, IDC_COMBO4, m_PortStopBits);
	DDX_Control(pDX, IDC_COMBO5, m_PortParity);
	DDX_Control(pDX, IDC_CHECK3, m_HexRcvCBtn);
	DDX_Control(pDX, IDC_CHECK4, m_CharRcvCBtn);
	DDX_Control(pDX, IDC_CHECK1, m_HexSendCBtn);
	DDX_Control(pDX, IDC_CHECK2, m_CharSendCBtn);
}

BEGIN_MESSAGE_MAP(CMFCSERIALTESTDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCSERIALTESTDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON3, &CMFCSERIALTESTDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON7, &CMFCSERIALTESTDlg::OnBnClicked_ClosePort)
	ON_MESSAGE(WM_COMM_RXCHAR, &CMFCSERIALTESTDlg::OnRecvData)
	ON_BN_CLICKED(IDC_CHECK3, &CMFCSERIALTESTDlg::OnBnClickedRecCBtn)
	ON_BN_CLICKED(IDC_CHECK4, &CMFCSERIALTESTDlg::OnBnClickedCharRecCBtn)
	ON_BN_CLICKED(IDC_CHECK1, &CMFCSERIALTESTDlg::OnBnClickedHexSendCBtn)
	ON_BN_CLICKED(IDC_CHECK2, &CMFCSERIALTESTDlg::OnBnClickedCharSendCBtn)
	ON_BN_CLICKED(IDC_BUTTON5, &CMFCSERIALTESTDlg::OnBnClickedClrSData)
	ON_BN_CLICKED(IDC_BUTTON2, &CMFCSERIALTESTDlg::OnBnClickedSendData)
END_MESSAGE_MAP()

// CMFCSERIALTESTDlg 消息处理程序

BOOL CMFCSERIALTESTDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	//m_SerialPort = new CSerialPort();
	//初始化串口配置
	m_SerialPort.Hkey2ComboBox(m_ComPortNO);
	m_PortBaud.SetCurSel(6);
	m_PortDataBits.SetCurSel(0);
	m_PortStopBits.SetCurSel(0);
	m_PortParity.SetCurSel(0);

	count = 0;
	//初始化发送/接收类型
	m_HexRcvCBtn.SetCheck(1);
	m_HexSendCBtn.SetCheck(1);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCSERIALTESTDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCSERIALTESTDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCSERIALTESTDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CMFCSERIALTESTDlg::OnRecvData(WPARAM wParam, LPARAM lParam)
{
	/*
	CString recvStr((char *)wParam);
	m_Receive += recvStr;
	for (int k = 0; k < lParam; k++)
	{
		m_Send += (char *)wParam + k;
	}
	UpdateData(false);
	return TRUE;
	*/
	count++;
	//unsigned char buffer[1024];
	//buffer[count] = (char)wParam;
	// m_Receive.Format(_T("%d"), count);
	if (m_CharRcvCBtn.GetCheck())
	{
		m_Receive = (char)wParam;
		UpdateData(FALSE);
	}
	else if (m_HexRcvCBtn.GetCheck())
	{
		//m_Receive = (char)wParam;
		m_Receive.Format(m_Receive + _T("%4x"), wParam);
		UpdateData(FALSE);
	}

	return TRUE;
}

//打开串口
void CMFCSERIALTESTDlg::OnBnClickedButton1()
{
	//int portnr = ;
	CString portnrStr;
	UINT portnr;
	UINT baud;
	//UINT parityNum;
	char parity;
	UINT databits;
	UINT stopbits;
	
	m_ComPortNO.GetLBText(m_ComPortNO.GetCurSel(), portnrStr);
	for (int i = 3; i < portnrStr.GetLength(); i++)
	{
		char ch = portnrStr[i];
		//portnr = (UINT)ch;
		m_Receive += ch;
		UpdateData(FALSE);
	}
	portnr = atoi(m_Receive);
	m_Receive = "";
	//110;300;600;1200;2400;4800;9600;14400;19200;28800;38400;56000;57600;115200;128000;256000;
	switch (m_PortBaud.GetCurSel())
	{
		case 0:
			baud = 110;
			break;
		case 1:
			baud = 300;
			break;
		case 2:
			baud = 600;
			break;
		case 3:
			baud = 1200;
			break;
		case 4:
			baud = 2400;
			break;
		case 5:
			baud = 4800;
			break;
		case 6:
			baud = 9600;
			break;
		case 7:
			baud = 14400;
			break;
		case 8:
			baud = 19200;
			break;
		case 9:
			baud = 28800;
			break;
		case 10:
			baud = 38400;
			break;
		case 11:
			baud = 56000;
			break;
		case 12:
			baud = 57600;
			break;
		case 13:
			baud = 115200;
			break;
		case 14:
			baud = 128000;
			break;
		case 15:
			baud = 256000;
			break;
		default:
			break;
	}
	//None无;Odd奇;Even偶;Mark 1;Space 0;
	switch (m_PortParity.GetCurSel())
	{
	case 0:
		parity = 'N';
		break;
	case 1:
		parity = 'O';
		break;
	case 2:
		parity = 'E';
		break;
	case 3:
		parity = 'M';
		break;
	case 4:
		parity = 'S';
		break;
	default:
		break;
	}

	stopbits = m_PortStopBits.GetCurSel();
	
	switch (m_PortDataBits.GetCurSel())
	{
	case 0:
		databits = 8;
		break;
	case 1:
		databits = 7;
		break;
	case 2:
		databits = 6;
		break;
	case 3:
		databits = 5;
		break;
	case 4:
		databits = 4;
		break;
	default:
		break;
	}

	BOOL IsOpen = m_SerialPort.InitPort(this, portnr, baud, parity, databits, stopbits, EV_RXCHAR | EV_RXFLAG, 1024, 1, 1, 1, 1, 1);
	//判断串口是否打开
	if (IsOpen)
	{
		AfxMessageBox(_T("成功打开串口"));
	}
	else
	{
		AfxMessageBox(_T("打开串口失败，请重新打开！"));
	}

	if (m_SerialPort.StartMonitoring())
	{
		AfxMessageBox(_T("串口线程已成功启动"));
	}
	else
	{
		AfxMessageBox(_T("串口线程启动失败，请检查原因！"));
	}
}

void CMFCSERIALTESTDlg::OnBnClickedButton3()
{
	m_Receive = "";
	UpdateData(false);
}


void CMFCSERIALTESTDlg::OnBnClicked_ClosePort()
{
	m_SerialPort.ClosePort();
}


void CMFCSERIALTESTDlg::OnBnClickedRecCBtn()
{
	m_HexRcvCBtn.SetCheck(1);
	m_CharRcvCBtn.SetCheck(0);
}


void CMFCSERIALTESTDlg::OnBnClickedCharRecCBtn()
{
	m_HexRcvCBtn.SetCheck(0);
	m_CharRcvCBtn.SetCheck(1);
}


void CMFCSERIALTESTDlg::OnBnClickedHexSendCBtn()
{
	m_HexSendCBtn.SetCheck(1);
	m_CharSendCBtn.SetCheck(0);
}


void CMFCSERIALTESTDlg::OnBnClickedCharSendCBtn()
{
	m_HexSendCBtn.SetCheck(0);
	m_CharSendCBtn.SetCheck(1);
}


void CMFCSERIALTESTDlg::OnBnClickedClrSData()
{
	m_Send = "";
	UpdateData(FALSE);
}


void CMFCSERIALTESTDlg::OnBnClickedSendData()
{
	if (m_HexSendCBtn.GetCheck())
	{
		UpdateData(TRUE);
		CByteArray hexdata;
		String2Hex(m_Send, hexdata);
		//m_SerialPort.WriteToPort(hexdata);
	}
	else if (true)
	{

	}
}

int CMFCSERIALTESTDlg::String2Hex(CString str, CByteArray &senddata)
{
	int hexdata, lowhexdata;
	int hexdatalen = 0;
	int len = str.GetLength();
	senddata.SetSize(len / 2);
	for (int i = 0; i<len;)
	{
		char lstr, hstr = str[i];
		if (hstr == ' ')
		{
			i++;
			continue;
		}
		i++;
		if (i >= len)
			break;
		lstr = str[i];
		hexdata = ConvertHexChar(hstr);
		lowhexdata = ConvertHexChar(lstr);
		if ((hexdata == 16) || (lowhexdata == 16))
			break;
		else
			hexdata = hexdata * 16 + lowhexdata;
		i++;
		senddata[hexdatalen] = (char)hexdata;
		hexdatalen++;
	}
	senddata.SetSize(hexdatalen);
	return hexdatalen;
}

char CMFCSERIALTESTDlg::ConvertHexChar(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - 0x30;
	else if ((ch >= 'A') && (ch <= 'F'))
		return ch - 'A' + 10;
	else if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	else return (-1);
}
