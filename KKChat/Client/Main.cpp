#include <windows.h>
#include <windowsx.h>
#include "resource.h"

#include "MsgProcotol.h"
#include "NetModule.h"
#include "encrypt.h"
#include "handle.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "WINMM.LIB")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
						processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

//PlaySound (TEXT ("hellowin.wav"), NULL, SND_FILENAME | SND_ASYNC) ;音乐播放API

HWND	hLogin, hChat;		//存储登录对话框句柄和聊天室对话框句柄
HWND	hListView;			//列表视图控件句柄
HWND	hCombobox;			//下拉列表控件句柄

HINSTANCE	hInst;			//模块句柄

HBITMAP hBitmapLogin;		//登录对话框位图句柄
HBITMAP	hBitmapChat;		//聊天室对话框位图句柄

Encrypt			encrypt;		//加密类，用于数据包加密
BOOL		need_encrypt = 0;	//是否能加密
unsigned char	salt[8] = { 0 };//加密盐

//声明为易失变量，防止优化可能导致的bug
volatile DWORD	Status;				//记录登陆状态,以下为状态类型

/*
#define LOGIN_SUCCESS	0
#define LOGIN_FAIL		1
#define LOGIN_ING		2
#define LOGIN_TIMEOUT	3
#define LOGIN_CHAT		4
#define LOGIN_CANCEL	-1	//退出 

#define LOGIN_SUCCESS_ENCRYPT 0xff
*/

DWORD	dwThreadID;
DWORD	dwLastTime;			//最后一次发送数据报的时间
DWORD	dwTimeCount;		//计时器，计算禁言时间

#define TIME_LIMIT		10	//登陆最大时间/秒

//对于网络数据选择ASCI编码以节省空间加快效率
char	szErrInfo[64];
char	szUserName[20];
char	szPassword[20];
char	szServerIP[20];
DWORD	NET_PORT;			//存储端口号	
SOCKET	hSocket;
BOOL	CantSpeak;			//禁言位


//登陆对话框窗口过程
BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//登陆对话框消息处理函数
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;

//聊天室对话框窗口过程
BOOL CALLBACK ChatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
//聊天室对话框消息处理函数
BOOL Chat_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Chat_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;




int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	hInst	= hInstance;
RELOGIN:
	switch(DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc, 0) )
	{
		case LOGIN_SUCCESS:
			if(DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_CHAT), NULL, ChatDlgProc, 0) == LOGIN_FAIL)
				goto RELOGIN;
			break;

		case LOGIN_FAIL:
			MessageBoxA(0, szErrInfo, "提示", MB_ICONWARNING);
			goto RELOGIN;
			break;

		case LOGIN_TIMEOUT:
			MessageBoxA(0, "登陆超时，请检查网络是否畅通","提示", MB_OK);
			break;

		case LOGIN_CANCEL:
			break;
	}

	return 0;
}


//登陆对话框窗口过程
BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC			hdc,hdcMem; 
	PAINTSTRUCT ps ;

	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Main_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	, Main_OnCommand	);
		
		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//实现拖拽效果
			return TRUE ;

		//登陆界面贴图
		case WM_PAINT:
			hdc = BeginPaint(hDlg, &ps);
			hdcMem = CreateCompatibleDC(hdc);
			SelectObject(hdcMem, hBitmapLogin);
			StretchBlt (hdc, 0, 0, 600, 480, hdcMem, 0, 0, 600, 480, MERGECOPY) ;
			//::BitBlt(hdc,0,0,500,400,hdcMem,0,0,MERGECOPY);
			DeleteDC(hdcMem);
			EndPaint (hDlg, &ps);
			return TRUE;

		case WM_CLOSE:
			EndDialog (hDlg, LOGIN_CANCEL);
			return TRUE;
	}
	return FALSE;
}


//对话框初始化
BOOL Main_OnInitDialog (HWND hDlg, HWND hWndFocus, LPARAM lParam) 
{
	WSADATA WSAData;
	chSETDLGICONS(hDlg, IDI_ICON1);
	WSAStartup(0x101, &WSAData);
	hLogin	= hDlg;
	SendDlgItemMessage(hDlg,IDC_USER,	EM_SETLIMITTEXT,15,0);
	SendDlgItemMessage(hDlg,IDC_PASS,	EM_SETLIMITTEXT,15,0);
	SendDlgItemMessage(hDlg,IDC_SERVER,	EM_SETLIMITTEXT,15,0);
	SendDlgItemMessage(hDlg,IDC_PORT,	EM_SETLIMITTEXT, 5,0);
	Edit_SetPasswordChar( GetDlgItem(hDlg, IDC_PASS),'*');
	SetDlgItemText(hDlg, IDC_PORT, TEXT("8888") );
	SetDlgItemText(hDlg, IDC_SERVER, TEXT("127.0.0.1"));
	SetFocus(GetDlgItem(hDlg, IDC_USER));
	hBitmapLogin = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGIN) );
	return FALSE;
}


//WM_COMMAND消息处理
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	DWORD	dwParam=0;
	DWORD	start;
	BOOL	flag;
	DWORD	count;

	switch(id)
	{
		case ID_LOGIN :
			//DialogBox (hInst, MAKEINTRESOURCE (IDD_ABOUT), hDlg, AboutDlgProc) ;

			count = GetDlgItemTextA(hDlg, IDC_USER, szUserName, sizeof(szUserName));
			if(count==0)	{ MessageBoxA(hDlg, "用户名或密码不能为空", "错误", MB_ICONWARNING); return TRUE; }
			count = GetDlgItemTextA(hDlg, IDC_PASS, szPassword, sizeof(szPassword));
			if(count==0)	{ MessageBoxA(hDlg, "用户名或密码不能为空", "错误", MB_ICONWARNING); return TRUE; }
			count = GetDlgItemTextA(hDlg, IDC_SERVER, szServerIP, sizeof(szServerIP));
			if(count==0)	{ MessageBoxA(hDlg, "请填入正确的服务器IP地址", "错误", MB_ICONWARNING); return TRUE; }
			NET_PORT	= GetDlgItemInt(hDlg, IDC_PORT, &flag, TRUE); 

			Status	= LOGIN_ING;	//设置为登陆中状态
			//启动工作线程，建立网络连接并试图登录
			chBEGINTHREADEX(NULL, 0, WorkThread, dwParam, 0, &dwThreadID);
			start = GetTickCount();
			while(Status==LOGIN_ING)
			{
				if( (GetTickCount()-start)/1000 > TIME_LIMIT)	//检测是否登录超时(秒)
				{
					Status	= LOGIN_TIMEOUT;
					break;
				}
			}
			EndDialog (hDlg, Status) ;
			return TRUE;

		case ID_REGISTER:
			if( RegisterNewUser(hDlg) )
				MessageBoxA(hDlg, "恭喜！注册成功！现在就可以登录您的新账号了！", "提示", MB_OK);
			else
				MessageBoxA(hDlg, szErrInfo, "提示", MB_ICONWARNING); 
			return TRUE;

	}
	return FALSE;
}




//聊天室对话框窗口过程
BOOL CALLBACK ChatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{


	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Chat_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	,   Chat_OnCommand	 );
		
		
		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//实现拖拽效果
			return TRUE ;

		case WM_CLOSE:
			WSACleanup();
			CleanUserInfo();
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}

/////////////////////////////消息发送框键盘消息处理//////////////////////////////////////////
static WNDPROC	OldWndProc;
LRESULT CALLBACK NewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BYTE	state[256];

	switch(message)
	{	
		case WM_KEYDOWN:
			if( wParam == VK_RETURN )
			{	
				GetKeyboardState(state);
				if ( (state[VK_RCONTROL] & 0x80) == 0x80 )
				{
					SendMessage( hChat, WM_COMMAND, (WPARAM)ID_SEND, 0);
					return FALSE;
				}
				//return FALSE;
			}
			break;

		default:
			break;
	}
	return CallWindowProc(OldWndProc, hWnd, message, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////////////////////


//聊天室对话框初始化
BOOL Chat_OnInitDialog (HWND hDlg, HWND hWndFocus, LPARAM lParam) 
{		
	hChat		= hDlg;
	hListView	= GetDlgItem(hDlg, IDC_LIST);
	hCombobox	= GetDlgItem(hDlg, IDC_ACCEPT);
	chSETDLGICONS( hDlg, IDI_ICON1);
	SetFocus(GetDlgItem(hDlg, IDC_MESSAGE));
	//设置状态
	Status	= LOGIN_CHAT;
	OldWndProc = (WNDPROC)SetWindowLong( GetDlgItem(hDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG)NewWndProc);

    //定义表格外观
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
	//ListView_SetBkColor(g_hListView, 0xa0a0ff);背景色
    ShowWindow (hListView, SW_SHOW);
	
	ListView_InsertCaption(hListView, 0, 120, TEXT("在线用户") );
	ComboBox_AddString(hCombobox, "所有人");
	ComboBox_SetCurSel(hCombobox, 0);

	return FALSE;
}


//聊天室对话框消息处理函数
BOOL Chat_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	static char szText[64] ;
	PACKET	stMsg,tmpMsg;
	char	szMessage[MAX_LENGTH];
	DWORD	count;

	switch(id)
	{		
		case ID_SEND :
			count	= GetDlgItemTextA(hDlg, IDC_MESSAGE, szMessage, MAX_LENGTH-1);
			if(count == 0)
				return TRUE;
			if(count >=MAX_LENGTH)
			{
				wsprintfA(szText, "超过%d最大字符数限制，请重新输入", MAX_LENGTH);
				MessageBoxA(hDlg, szText, "警告", MB_ICONWARNING);
				return TRUE;
			}
			if( !hSocket ) 
			{
				MessageBoxA(hDlg, "未知错误，套接字不可用，请尝试重新登录", "警告", MB_ICONERROR);
				Status	= LOGIN_FAIL;
				EndDialog (hDlg, Status) ;
			}

			//对于灌水行为的处理(700ms)
			if( GetTickCount()-dwLastTime<700 )
			{
				char szSys[64] = { 0 } ;
				lstrcpyA( szSys, "★");
				lstrcatA( szSys, "诶呀，发言速度有点快了~~休息一会~~");
				lstrcatA( szSys, "★");
				lstrcatA( szSys, "\r\n");
				int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
				Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
				SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szSys);
				return TRUE;
			}

			//组装数据包
			stMsg.Head.CMD_ID		= CMD_MSG_UP;
			stMsg.Head.dwLength		= sizeof(stMsg.Head) + count + 1 + 4 + sizeof(stMsg.stMsgUp.szAccepter);
			stMsg.Head.Timestamp	= time(NULL);
			stMsg.stMsgUp.dwLength	= count+1;
			lstrcpyA(stMsg.stMsgUp.szContent, szMessage);
			
			//若发现是禁言状态则取消消息发送
			if( CantSpeak )
			{
				if( (GetTickCount() - dwTimeCount) <= 60*1000 )//1分钟内禁言
				{
					MessageBox( hChat, "你已被管理员禁言1分钟，请过一段时间后再发言", "提示", MB_ICONWARNING);
					return TRUE;
				}
				else
					CantSpeak = FALSE; //取消禁言状态
			}

			//若发现是私聊消息则修改消息ID并添加接受者信息
			ComboBox_GetCurString(szText);
			if( strcmp(szText, "所有人") != 0 )
			{
				strcpy(stMsg.stMsgUp.szAccepter, szText);
				stMsg.Head.CMD_ID	= CMD_MSG_PRIVATE;
			}

			tmpMsg	= stMsg;
			//若设置加密标志，则进行消息加密
			if(need_encrypt)
			{
				unsigned char* lpData = (unsigned char*)&tmpMsg + sizeof(PACKET_HEAD);
				encrypt.encrypt( lpData, lpData, tmpMsg.Head.dwLength-sizeof(PACKET_HEAD) ) ;
			}
			if(send(hSocket, (char*)&tmpMsg, tmpMsg.Head.dwLength, 0)==SOCKET_ERROR )
			{
				Status	= LOGIN_FAIL;
				lstrcpyA(szErrInfo, "与服务器连接丢失");
				MessageBoxA(hDlg, szErrInfo, "提示", MB_ICONERROR);
				EndDialog (hDlg, Status) ;
			}
			dwLastTime	= GetTickCount();
			SetDlgItemTextA(hDlg, IDC_MESSAGE, NULL);
			SetFocus(GetDlgItem(hDlg, IDC_MESSAGE));

			//至此发送成功，若是私聊消息则添加信息
			if(stMsg.Head.CMD_ID==CMD_MSG_PRIVATE)
			{
				int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
				Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
				wsprintf(szMessage, "☆你悄悄地对【%s】说：%s☆", stMsg.stMsgUp.szAccepter, stMsg.stMsgUp.szContent);
				lstrcatA( szMessage, "\r\n");
				SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szMessage);
			}
			//SetDlgItemTextA(hDlg, IDC_RECORD,szMessage);
			return TRUE ;

		case IDCANCEL:
			EndDialog(hDlg,0);
			return TRUE;
	}
	return FALSE;
}


