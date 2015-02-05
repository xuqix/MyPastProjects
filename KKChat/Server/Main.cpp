//���Щ���˵ľ���
#pragma warning(disable:4244)
#pragma warning(disable:4996)

#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include "resource.h"
#include "MsgQueue.h"
#include "ApiMacro.h"
#include "ODBCModule.h"
#include "MsgProcotol.h"
#include "NetModule.h"
#include "kernel.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
						processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define LOGIN_SUCCESS	0
#define LOGIN_FAIL		1
#define LOGIN_ING		2
#define LOGIN_TIMEOUT	3
#define LOGIN_CHAT		4
#define LOGIN_CANCEL	-1	//�˳� 

#define LOGIN_SUCCESS_ENCRYPT 0xff

HWND	hwnd;			//�Ի�����
HWND	hListView;		//�б���
//����Ϊ��ʧ��������ֹ�Ż����ܵ��µ�bug
volatile DWORD	Status;			//������״̬

#define	RUNING			1	//������
#define STOP			0	//ֹͣ����
//������������ѡ��ASCI�����Խ�ʡ�ռ�ӿ�Ч��
char	szErrInfo[64];
extern  DWORD	NET_PORT;			//�洢�˿ں�
extern	SOCKET	hListenSocket;		//�����׽���
extern DWORD	dwUserCount;		//�����û�����
extern MsgQueue	MsgQue;				//��Ϣ����
//SOCKET	hSocket;

//TAB�������Ի�����
HWND	ChildhWnd1;
HWND	ChildhWnd2;
HINSTANCE hInst;

//���Ի��򴰿ڹ���
BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//���Ի�����Ϣ������
BOOL Main_OnNotify	(HWND hwnd, int wParam,	LPNMHDR pnm);
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;

//����������ҳ�Ի�����
BOOL CALLBACK ServerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//����������ҳ��Ϣ������
BOOL Server_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Server_OnInitDialog(HWND hDlg, HWND hWndFocus, LPARAM lParam) ;

//���ݿ����ҳ�Ի�����
BOOL CALLBACK DatabaseDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//����TAB�ؼ���Ŀ
void Tab_InsertItem(HWND hwnd,int index, LPWSTR szText);


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow) 
{
	hInst	= hInstance;
	::DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc, NULL);
     return 0 ;
}

BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Main_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	, Main_OnCommand	);
		HANDLE_MSG(hDlg, WM_NOTIFY	, Main_OnNotify	);

		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//ʵ����קЧ��
			return TRUE ;

		case WM_CLOSE:
			if(Status == RUNING)
				return TRUE;
			WSACleanup();
			DisConnect();
			EndDialog (hDlg, 0);
			return true;
	}
	return FALSE;
}

void Tab_InsertItem(HWND hwnd,int index, LPSTR szText)
{
	static TC_ITEMA	itemTab;//TAB�ؼ����ݽṹ
	HWND	hTab = GetDlgItem(hwnd, IDC_TAB);

	itemTab.mask	= TCIF_TEXT;
	itemTab.iImage		= 0;
	itemTab.lParam		= 0;
	itemTab.pszText		= szText;
	itemTab.cchTextMax	= 4;//lstrlen(szText);

	SendMessage(hTab, TCM_INSERTITEM, index, (LPARAM)&itemTab);
}

//�����б���
void ListViewClear(HWND hListView)
{
	SendMessage(hListView,LVM_DELETEALLITEMS,0,0);	
	while(1) 
	{  
		if(!SendMessage(hListView,LVM_DELETECOLUMN,0,0)) 
			break; 
	}
}

BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) 
{
	HWND	hTab;
	hTab	= GetDlgItem(hWnd, IDC_TAB);

	WSADATA WSAData;
	chSETDLGICONS(hWnd, IDI_ICON1);
	WSAStartup(0x101, &WSAData);


	InitCommonControls();
	Tab_InsertItem(hWnd, 0, TEXT("����������") );
	Tab_InsertItem(hWnd, 1, TEXT("���ݿ����") );

	ChildhWnd1 = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hTab, ServerDlgProc, 0);
	ChildhWnd2 = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hTab, DatabaseDlgProc , 0);

	ShowWindow(ChildhWnd1, SW_SHOWDEFAULT);
	return TRUE;
}

BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	switch(id)
	{
		/*case IDM_HELP_ABOUT :
			DialogBox (hInst, MAKEINTRESOURCE (IDD_ABOUT), hDlg, AboutDlgProc) ;
			return TRUE;

		case ID_MENU_OPEN:
			SendMessage(ChildhWnd1, WM_COMMAND, (WPARAM)IDC_OPENFILE, 0);
			return TRUE;

		case ID_MENU_EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;*/
	}
	return FALSE;
}

BOOL Main_OnNotify	(HWND hwnd, int wParam,	LPNMHDR pnm)
{
	if(pnm->code == TCN_SELCHANGE)
	{
		//���������ӶԻ���
		ShowWindow(ChildhWnd1, SW_HIDE);
		ShowWindow(ChildhWnd2, SW_HIDE);
		
		HWND	hTab = GetDlgItem(hwnd, IDC_TAB);
		int		index= SendMessage(hTab, TCM_GETCURSEL,0 ,0);
		if (index == 0)
			ShowWindow(ChildhWnd1,SW_SHOWDEFAULT);
		else if (index == 1)
			ShowWindow(ChildhWnd2,SW_SHOWDEFAULT);

	}
	return FALSE;
}

//����������Ի��򴰿ڹ���
BOOL CALLBACK ServerDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Server_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	, Server_OnCommand	);

		//case  WM_LBUTTONDOWN:
		//	PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//ʵ����קЧ��
		//	return TRUE ;

		case WM_CLOSE:
			if(Status == RUNING)
				return TRUE;
			MessageBox(0,"","",0);
			//WSACleanup();
			DisConnect();
			EndDialog (hDlg, LOGIN_CANCEL);
			return TRUE;
	}
	return FALSE;
}


//�Ի����ʼ��
BOOL Server_OnInitDialog (HWND hDlg, HWND hWndFocus, LPARAM lParam) 
{

	hwnd	= hDlg;
	hListView	= GetDlgItem(hDlg, IDC_LIST);

	Status	= STOP;
    //���������
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
	//ListView_SetBkColor(g_hListView, 0xa0a0ff);����ɫ
    ShowWindow (hListView, SW_SHOW);
	
	ListView_InsertCaption(hListView, 0, 120, TEXT("�û���") );
	ListView_InsertCaption(hListView, 1, 150, TEXT("IP��ַ") );
	ListView_InsertCaption(hListView, 2, 105, TEXT("��½ʱ��") );
	return FALSE;
}


//WM_COMMAND��Ϣ����
BOOL Server_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	static int flag=0;
	DWORD	dwParam = 0, dwThreadID, count=0;
	int		ret;
	char	szBuffer[MAX_LENGTH*2];
	char	szMessage[MAX_LENGTH];

	switch(id)
	{
		case ID_BAN :
			if (Status!=RUNING)	return TRUE;
			if( ret = BanUser() )
			{
				if(ret==-1)	
					MessageBox(hwnd, "��ѡ��Ҫ���Ե��û�", "��ʾ",	MB_OK);
				else
					MessageBox(hwnd, "�ѽ���ѡ���û�1����", "��ʾ", MB_OK);
			}
			else
				MessageBox(hwnd, "δ֪����,����ʧ��","��ʾ",MB_ICONWARNING);
			return TRUE;

		case ID_OUT:
			if (Status!=RUNING)	return TRUE;
			if( ret = KickUser() )
			{
				if(ret==-1)	
					MessageBox(hwnd, "��ѡ��Ҫ�߳����û�", "��ʾ",	MB_OK);
				else
					MessageBox(hwnd, "�ѽ�ѡ���û��߳�������", "��ʾ", MB_OK);
			}
			return TRUE;

		case IDC_SEND:
			if(dwUserCount > 0)	//����������û�
			{
				count	= GetDlgItemTextA(hDlg, IDC_MESSAGE, szMessage, MAX_LENGTH-1);
				if(count == 0)
					return TRUE;
				if(count >=MAX_LENGTH)
				{
					wsprintfA(szMessage, "����%d����ַ������ƣ�����������", MAX_LENGTH);
					MessageBoxA(hDlg, szMessage, "����", MB_ICONWARNING);
					return TRUE;
				}
				wsprintfA(szBuffer, "��ϵͳ�㲥�� ");
				lstrcatA(szBuffer, szMessage);
				MsgQue.push_back(  Msg( time(0), 0, szBuffer, CMD_SYS_INFO)  );
				SetDlgItemTextA(hDlg, IDC_MESSAGE, NULL);
				SetFocus(GetDlgItem(hDlg, IDC_MESSAGE));
				MessageBox(hDlg, "�㲥�ɹ���", "��ʾ", MB_OK);
			}
			return TRUE;

		case IDC_SERVER:
			if(flag)
			{
				flag  = !flag;
				Status= STOP;
				CleanUserInfo(hListView);
				
				SetDlgItemText(hDlg, IDC_SERVER, "����������");
				if(hListenSocket)
					closesocket(hListenSocket);
			}
			else
			{
				flag  = !flag;
				Status= RUNING;
				CleanUserInfo(hListView);

				//���������������߳�
				chBEGINTHREADEX(NULL, 0, ListenThread, dwParam, 0, &dwThreadID);
				
				SetDlgItemText(hDlg, IDC_SERVER, "�رշ�����");
			}
			return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK DatabaseDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char	sql[1024] = { 0 };

	switch(message)
	{
		case WM_INITDIALOG:
			    //���������
				ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_LIST1), LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
				//ListView_SetBkColor(g_hListView, 0xa0a0ff);����ɫ
				 ShowWindow (GetDlgItem(hDlg, IDC_LIST1), SW_SHOW);
				return TRUE;

		case WM_COMMAND :
			::SetDlgItemText(hDlg, IDC_STATIC1, 0);
			switch (LOWORD (wParam))  		            
			{
				case IDC_EXEC :
					if(Status != RUNING)
					{
						MessageBox(hDlg, "�޷�ִ��sql��䣬��δ����������", "����", MB_ICONWARNING);
						return TRUE;
					}
					if(GetDlgItemText(hDlg, IDC_SQL, sql, sizeof(sql)) <= 0 )
						return TRUE;
					ListViewClear(GetDlgItem(hDlg, IDC_LIST1) );
					Execute(sql, 1); 
					SendDlgItemMessage(hDlg,IDC_SQL,EM_SETSEL,0,-1);
					return TRUE;

				case IDC_COMMIT :
					if(Status != RUNING)
					{
						MessageBox(hDlg, "����ʧ�ܣ���δ����������", "����", MB_ICONWARNING);
						return TRUE;
					}
					::Commit();
					return TRUE;

				case IDC_ROLLBACK:
					if(Status != RUNING)
					{
						MessageBox(hDlg, "����ʧ�ܣ���δ����������", "����", MB_ICONWARNING);
						return TRUE;
					}
					ListViewClear(GetDlgItem(hDlg, IDC_LIST1) );
					::Rollback();
					return TRUE ;
			}
			break;
	}
	return FALSE;
}