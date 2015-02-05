#ifndef APIMACRO_H
#define APIMACRO_H

////////////////ͨ�ÿؼ�ͷ�ļ������ӿ�////////////
#include <process.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
//////////////////////////////////////////////////


/////////////////���öԻ���ͼ��///////////////////
inline void chSETDLGICONS(HWND hWnd, int idi) {
   SendMessage(hWnd, WM_SETICON, ICON_BIG,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

/////////////////�����Ի���///////////////////////
inline void chMB(PCSTR szMsg) {
   char szTitle[MAX_PATH];
   GetModuleFileName(NULL, szTitle, MAX_PATH);
   MessageBox(GetActiveWindow(), szMsg, szTitle, MB_OK);
}

/////////////////����һ���߳�/////////////////////
#define chBEGINTHREADEX(psa, cbStackSize, pfnStartAddr, \
   pvParam, dwCreateFlags, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                          \
         (void *)        (psa),                         \
         (unsigned)      (cbStackSize),                 \
						 (pfnStartAddr),                \
         (void *)        (pvParam),                     \
         (unsigned)      (dwCreateFlags),               \
         (unsigned *)    (pdwThreadId)))


//ͨ�ÿؼ�ʹ��ǰ���InitCommonControls��ʼ��
//List_View �ؼ���       ����������������0��ʼ
static LV_ITEM		_stLVI;
static LV_COLUMN	_stLVC;

//////////////////////////////////��ListView������һ��///////////////////////////////////
inline int ListView_AddLine(HWND hwndCtl)
{
	RtlZeroMemory (&_stLVI,sizeof(LV_ITEM) );
	_stLVI.mask			= LVIF_TEXT;
	_stLVI.pszText		= TEXT("�����ݿ���ʾ");
	_stLVI.iSubItem		= 0;
	return ListView_InsertItem(hwndCtl, &_stLVI);
}



////////////////////////////////��ListView������һ��������///////////////////////////////////
inline void ListView_InsertCaption(HWND hwndCtl, int iColumn, int iWidth, LPTSTR lpszCaption)
{
	RtlZeroMemory (&_stLVC,sizeof(LV_COLUMN) );
	_stLVC.mask			= LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	_stLVC.fmt			= LVCFMT_LEFT;
	_stLVC.pszText		= lpszCaption;
	_stLVC.cx			= iWidth;
	_stLVC.iSubItem		= iColumn;
	ListView_InsertColumn(hwndCtl, iColumn, &_stLVC);
}


////////////////////////////////��ListView�и���һ��������///////////////////////////////////
inline void ListView_SetCaption(HWND hwndCtl, int iColumn, LPTSTR lpszCaption)
{
	RtlZeroMemory (&_stLVC,sizeof(LV_COLUMN) );
	_stLVC.mask			= LVCF_TEXT | LVCF_FMT;
	_stLVC.fmt			= LVCFMT_LEFT;
	_stLVC.pszText		= lpszCaption;
	_stLVC.iSubItem		= iColumn;
	ListView_SetColumn(hwndCtl, iColumn, &_stLVC);
}


////////////////////////////////////���Ƶĸ�������///////////////////////////////////////////
extern HWND hListView;
//�б�������û���Ϣ
inline void	AddUserInfo(char* szUserName)
{
	int i;
	i = ListView_AddLine(hListView);//���һ��
	ListView_SetItemText(hListView, i, 0, szUserName);
}

//ɾ���б���ѡ���е���Ϣ
inline void	DelUserInfo()
{
	int i;
	i = ListView_GetSelectionMark(hListView);  //���ѡ��������
	ListView_DeleteItem(hListView, i);
}

//ɾ���б���ָ���û���Ϣ
inline void	DelUserInfo(char* szUserName)
{
	int		i,n;
	char	szBuffer[16];
	n	= ListView_GetItemCount(hListView);
	for(i=0; i < n; i++)
	{
		ListView_GetItemText(hListView, i, 0, szBuffer, sizeof(szBuffer) );
		if(strcmp(szUserName, szBuffer) == 0)
			break;
	}
	if(i!=n)
		ListView_DeleteItem(hListView, i);
}


inline void	CleanUserInfo()
{
	int i;
	ListView_DeleteAllItems(hListView);
	i = ListView_GetItemCount(hListView);
	for( i--; i>=0; i--)
		ListView_DeleteColumn(hListView, i);
}

extern HWND hCombobox;
inline void	ComboBox_DelString(char* szUserName)
{
	char szBuffer[32] = { 0 };
	int idx = ComboBox_FindStringExact(hCombobox, 1, szUserName);
	if(idx==CB_ERR)
		return;
	ComboBox_DeleteString(hCombobox, idx);
}

inline void ComboBox_GetCurString(char* szUserName)
{
	ComboBox_GetLBText(hCombobox, ComboBox_GetCurSel(hCombobox), szUserName);
}


#endif