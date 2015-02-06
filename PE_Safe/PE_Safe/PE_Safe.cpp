#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "RvaToOffset.h"
#include "ApiMacro.h"
#include "BrowseFolder.h"
#include "BindDirectory.h"
#include "PE_Lock.h"


//添加WIN7风格界面
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
															processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")


//主对话框窗口过程
BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//主对话框消息处理函数
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;

//帮助对话框
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
//密码锁设置对话框
BOOL CALLBACK PassDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);



HINSTANCE	hInst;
TCHAR		g_szPassWord[32];


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	hInst = hInstance;
	InitCommonControls();
	DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc, 0);

	return 0;
}


BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, Main_OnInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND	, Main_OnCommand	);

		case WM_CLOSE:
			EndDialog (hDlg, 0);
			return true;
	}
	return FALSE;
}

//对话框初始化
BOOL Main_OnInitDialog (HWND hDlg, HWND hWndFocus, LPARAM lParam) 
{
	chSETDLGICONS(hDlg, IDI_ICON1);
	PopFileInitialize(hDlg);

	return TRUE;
}


BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	static	TCHAR	szDirectory[MAX_PATH];
	static	TCHAR	szBindName[MAX_PATH];
	static	TCHAR	szText[32] = TEXT("请输入绑定后的文件名");
	static  TCHAR	szText2[32]= TEXT("请选择要加锁的文件");
	static  TCHAR	szFileName[MAX_PATH];

	switch(id)
	{
		case IDC_BIND:
			if ( BrowseFolder(hDlg, szDirectory) )
			{
				lstrcpy(szBindName, TEXT("Bind") );
				if ( PopFileSaveDlg(hDlg, szBindName, szText ) )
				{
					//进行目录绑定
					if (BindDirectory(szDirectory, szBindName) )
						chMB("目录绑定成功!");
					else
						chMB("绑定失败@_@");
				}
			}
			return TRUE;

		case IDC_LOCK:   //PE加锁
			if ( PopFileOpenDlg(hDlg, szFileName, szText2 ) )
			{
				if ( IsPeFile(szFileName) )
				{
					if (DialogBoxParam (hInst, MAKEINTRESOURCE(IDD_LOCK), hDlg, PassDlgProc, 0) )
					{
						if (PE_Lock(szFileName, g_szPassWord) )
							chMB("加锁成功!");
						else
							chMB("加锁失败#_#");
					}
				}
				else
					chMB("不是PE文件!");
			}
			return TRUE;

		case IDC_SET:
			return TRUE;

		case IDC_SHELL:
			return TRUE;

		case ID_ABOUT:
			DialogBoxParam (hInst, MAKEINTRESOURCE(IDD_ABOUT), hDlg, AboutDlgProc, 0);
			return TRUE;

		case IDCANCEL:
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}


BOOL CALLBACK PassDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND	hEdit;

	switch(message)
	{
		case WM_INITDIALOG:
			hEdit	= GetDlgItem(hDlg, IDC_PASS);
			Edit_LimitText(hEdit, sizeof(g_szPassWord)-1);
			return TRUE;

		case WM_COMMAND :
			switch (LOWORD (wParam))  		            
			{

				case IDOK :
					GetWindowText(hEdit, g_szPassWord, sizeof(g_szPassWord) );
					EndDialog (hDlg, TRUE) ;
					return TRUE ;

				case IDCANCEL:
					memset(g_szPassWord, 0, sizeof(g_szPassWord) );
					EndDialog(hDlg, FALSE);
					return TRUE;
			}
			return TRUE ;

		case WM_CLOSE:
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}


BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//实现拖拽效果
			return TRUE ;
    
		case WM_COMMAND :
			switch (LOWORD (wParam))  		            
			{

				case IDOK :
					EndDialog (hDlg, 0) ;
					return TRUE ;
			}
			return TRUE ;

		case WM_CLOSE:
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}