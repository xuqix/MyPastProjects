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

//PlaySound (TEXT ("hellowin.wav"), NULL, SND_FILENAME | SND_ASYNC) ;���ֲ���API

HWND	hLogin, hChat;		//�洢��¼�Ի������������ҶԻ�����
HWND	hListView;			//�б���ͼ�ؼ����
HWND	hCombobox;			//�����б�ؼ����

HINSTANCE	hInst;			//ģ����

HBITMAP hBitmapLogin;		//��¼�Ի���λͼ���
HBITMAP	hBitmapChat;		//�����ҶԻ���λͼ���

Encrypt			encrypt;		//�����࣬�������ݰ�����
BOOL		need_encrypt = 0;	//�Ƿ��ܼ���
unsigned char	salt[8] = { 0 };//������

//����Ϊ��ʧ��������ֹ�Ż����ܵ��µ�bug
volatile DWORD	Status;				//��¼��½״̬,����Ϊ״̬����

/*
#define LOGIN_SUCCESS	0
#define LOGIN_FAIL		1
#define LOGIN_ING		2
#define LOGIN_TIMEOUT	3
#define LOGIN_CHAT		4
#define LOGIN_CANCEL	-1	//�˳� 

#define LOGIN_SUCCESS_ENCRYPT 0xff
*/

DWORD	dwThreadID;
DWORD	dwLastTime;			//���һ�η������ݱ���ʱ��
DWORD	dwTimeCount;		//��ʱ�����������ʱ��

#define TIME_LIMIT		10	//��½���ʱ��/��

//������������ѡ��ASCI�����Խ�ʡ�ռ�ӿ�Ч��
char	szErrInfo[64];
char	szUserName[20];
char	szPassword[20];
char	szServerIP[20];
DWORD	NET_PORT;			//�洢�˿ں�	
SOCKET	hSocket;
BOOL	CantSpeak;			//����λ


//��½�Ի��򴰿ڹ���
BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//��½�Ի�����Ϣ������
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;

//�����ҶԻ��򴰿ڹ���
BOOL CALLBACK ChatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
//�����ҶԻ�����Ϣ������
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
			MessageBoxA(0, szErrInfo, "��ʾ", MB_ICONWARNING);
			goto RELOGIN;
			break;

		case LOGIN_TIMEOUT:
			MessageBoxA(0, "��½��ʱ�����������Ƿ�ͨ","��ʾ", MB_OK);
			break;

		case LOGIN_CANCEL:
			break;
	}

	return 0;
}


//��½�Ի��򴰿ڹ���
BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC			hdc,hdcMem; 
	PAINTSTRUCT ps ;

	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Main_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	, Main_OnCommand	);
		
		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//ʵ����קЧ��
			return TRUE ;

		//��½������ͼ
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


//�Ի����ʼ��
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


//WM_COMMAND��Ϣ����
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
			if(count==0)	{ MessageBoxA(hDlg, "�û��������벻��Ϊ��", "����", MB_ICONWARNING); return TRUE; }
			count = GetDlgItemTextA(hDlg, IDC_PASS, szPassword, sizeof(szPassword));
			if(count==0)	{ MessageBoxA(hDlg, "�û��������벻��Ϊ��", "����", MB_ICONWARNING); return TRUE; }
			count = GetDlgItemTextA(hDlg, IDC_SERVER, szServerIP, sizeof(szServerIP));
			if(count==0)	{ MessageBoxA(hDlg, "��������ȷ�ķ�����IP��ַ", "����", MB_ICONWARNING); return TRUE; }
			NET_PORT	= GetDlgItemInt(hDlg, IDC_PORT, &flag, TRUE); 

			Status	= LOGIN_ING;	//����Ϊ��½��״̬
			//���������̣߳������������Ӳ���ͼ��¼
			chBEGINTHREADEX(NULL, 0, WorkThread, dwParam, 0, &dwThreadID);
			start = GetTickCount();
			while(Status==LOGIN_ING)
			{
				if( (GetTickCount()-start)/1000 > TIME_LIMIT)	//����Ƿ��¼��ʱ(��)
				{
					Status	= LOGIN_TIMEOUT;
					break;
				}
			}
			EndDialog (hDlg, Status) ;
			return TRUE;

		case ID_REGISTER:
			if( RegisterNewUser(hDlg) )
				MessageBoxA(hDlg, "��ϲ��ע��ɹ������ھͿ��Ե�¼�������˺��ˣ�", "��ʾ", MB_OK);
			else
				MessageBoxA(hDlg, szErrInfo, "��ʾ", MB_ICONWARNING); 
			return TRUE;

	}
	return FALSE;
}




//�����ҶԻ��򴰿ڹ���
BOOL CALLBACK ChatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{


	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Chat_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	,   Chat_OnCommand	 );
		
		
		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//ʵ����קЧ��
			return TRUE ;

		case WM_CLOSE:
			WSACleanup();
			CleanUserInfo();
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}

/////////////////////////////��Ϣ���Ϳ������Ϣ����//////////////////////////////////////////
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


//�����ҶԻ����ʼ��
BOOL Chat_OnInitDialog (HWND hDlg, HWND hWndFocus, LPARAM lParam) 
{		
	hChat		= hDlg;
	hListView	= GetDlgItem(hDlg, IDC_LIST);
	hCombobox	= GetDlgItem(hDlg, IDC_ACCEPT);
	chSETDLGICONS( hDlg, IDI_ICON1);
	SetFocus(GetDlgItem(hDlg, IDC_MESSAGE));
	//����״̬
	Status	= LOGIN_CHAT;
	OldWndProc = (WNDPROC)SetWindowLong( GetDlgItem(hDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG)NewWndProc);

    //���������
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
	//ListView_SetBkColor(g_hListView, 0xa0a0ff);����ɫ
    ShowWindow (hListView, SW_SHOW);
	
	ListView_InsertCaption(hListView, 0, 120, TEXT("�����û�") );
	ComboBox_AddString(hCombobox, "������");
	ComboBox_SetCurSel(hCombobox, 0);

	return FALSE;
}


//�����ҶԻ�����Ϣ������
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
				wsprintfA(szText, "����%d����ַ������ƣ�����������", MAX_LENGTH);
				MessageBoxA(hDlg, szText, "����", MB_ICONWARNING);
				return TRUE;
			}
			if( !hSocket ) 
			{
				MessageBoxA(hDlg, "δ֪�����׽��ֲ����ã��볢�����µ�¼", "����", MB_ICONERROR);
				Status	= LOGIN_FAIL;
				EndDialog (hDlg, Status) ;
			}

			//���ڹ�ˮ��Ϊ�Ĵ���(700ms)
			if( GetTickCount()-dwLastTime<700 )
			{
				char szSys[64] = { 0 } ;
				lstrcpyA( szSys, "��");
				lstrcatA( szSys, "��ѽ�������ٶ��е����~~��Ϣһ��~~");
				lstrcatA( szSys, "��");
				lstrcatA( szSys, "\r\n");
				int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
				Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
				SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szSys);
				return TRUE;
			}

			//��װ���ݰ�
			stMsg.Head.CMD_ID		= CMD_MSG_UP;
			stMsg.Head.dwLength		= sizeof(stMsg.Head) + count + 1 + 4 + sizeof(stMsg.stMsgUp.szAccepter);
			stMsg.Head.Timestamp	= time(NULL);
			stMsg.stMsgUp.dwLength	= count+1;
			lstrcpyA(stMsg.stMsgUp.szContent, szMessage);
			
			//�������ǽ���״̬��ȡ����Ϣ����
			if( CantSpeak )
			{
				if( (GetTickCount() - dwTimeCount) <= 60*1000 )//1�����ڽ���
				{
					MessageBox( hChat, "���ѱ�����Ա����1���ӣ����һ��ʱ����ٷ���", "��ʾ", MB_ICONWARNING);
					return TRUE;
				}
				else
					CantSpeak = FALSE; //ȡ������״̬
			}

			//��������˽����Ϣ���޸���ϢID����ӽ�������Ϣ
			ComboBox_GetCurString(szText);
			if( strcmp(szText, "������") != 0 )
			{
				strcpy(stMsg.stMsgUp.szAccepter, szText);
				stMsg.Head.CMD_ID	= CMD_MSG_PRIVATE;
			}

			tmpMsg	= stMsg;
			//�����ü��ܱ�־���������Ϣ����
			if(need_encrypt)
			{
				unsigned char* lpData = (unsigned char*)&tmpMsg + sizeof(PACKET_HEAD);
				encrypt.encrypt( lpData, lpData, tmpMsg.Head.dwLength-sizeof(PACKET_HEAD) ) ;
			}
			if(send(hSocket, (char*)&tmpMsg, tmpMsg.Head.dwLength, 0)==SOCKET_ERROR )
			{
				Status	= LOGIN_FAIL;
				lstrcpyA(szErrInfo, "����������Ӷ�ʧ");
				MessageBoxA(hDlg, szErrInfo, "��ʾ", MB_ICONERROR);
				EndDialog (hDlg, Status) ;
			}
			dwLastTime	= GetTickCount();
			SetDlgItemTextA(hDlg, IDC_MESSAGE, NULL);
			SetFocus(GetDlgItem(hDlg, IDC_MESSAGE));

			//���˷��ͳɹ�������˽����Ϣ�������Ϣ
			if(stMsg.Head.CMD_ID==CMD_MSG_PRIVATE)
			{
				int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
				Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
				wsprintf(szMessage, "�������ĵضԡ�%s��˵��%s��", stMsg.stMsgUp.szAccepter, stMsg.stMsgUp.szContent);
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


