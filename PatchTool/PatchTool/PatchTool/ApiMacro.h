#ifndef APIMACRO_H
#define APIMACRO_H

////////////////ͨ�ÿؼ�ͷ�ļ������ӿ�////////////
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
//////////////////////////////////////////////////


///////////Sets the dialog box icons//////////////
inline void chSETDLGICONS(HWND hWnd, int idi) {
   SendMessage(hWnd, WM_SETICON, ICON_BIG,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

inline void chMB(PCSTR szMsg) {
   char szTitle[MAX_PATH];
   GetModuleFileNameA(NULL, szTitle, _countof(szTitle));
   MessageBoxA(GetActiveWindow(), szMsg, szTitle, MB_OK);
}

#define chBEGINTHREADEX(psa, cbStackSize, pfnStartAddr, \
   pvParam, dwCreateFlags, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                          \
         (void *)        (psa),                         \
         (unsigned)      (cbStackSize),                 \
         (PTHREAD_START) (pfnStartAddr),                \
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


#endif