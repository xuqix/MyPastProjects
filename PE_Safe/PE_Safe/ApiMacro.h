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
inline void ListView_AddLine(HWND hwndCtl)
{
	RtlZeroMemory (&_stLVI,sizeof(LV_ITEM) );
	_stLVI.mask			= LVIF_TEXT;
	_stLVI.pszText		= TEXT("�����ݿ���ʾ");
	_stLVI.iSubItem		= 0;
	ListView_InsertItem(hwndCtl, &_stLVI);
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



//////////////////////////////////////////////�ļ��Ի�����/////////////////////////////////////

static OPENFILENAME _ofn ;

void PopFileInitialize (HWND hwnd)
{
     static TCHAR szFilter[] = TEXT ("PE Files (*.exe;*.dll)\0*.exe;*.dll;*.scr;*.fon;*.drv\0")  \
                               TEXT ("All Files (*.*)\0*.*\0\0") ;
     
     _ofn.lStructSize       = sizeof (OPENFILENAME) ;
     _ofn.hwndOwner         = hwnd ;
     _ofn.hInstance         = NULL ;
     _ofn.lpstrFilter       = szFilter ;
     _ofn.lpstrCustomFilter = NULL ;
     _ofn.nMaxCustFilter    = 0 ;
     _ofn.nFilterIndex      = 0 ;
     _ofn.lpstrFile         = NULL ;          // Set in Open and Close functions
     _ofn.nMaxFile          = MAX_PATH ;
     _ofn.lpstrFileTitle    = NULL ;          // Set in Open and Close functions
     _ofn.nMaxFileTitle     = MAX_PATH ;
     _ofn.lpstrInitialDir   = NULL ;
     _ofn.lpstrTitle        = NULL ;
     _ofn.Flags             = 0 ;             // Set in Open and Close functions
     _ofn.nFileOffset       = 0 ;
     _ofn.nFileExtension    = 0 ;
     _ofn.lpstrDefExt       = TEXT ("exe") ;
     _ofn.lCustData         = 0L ;
     _ofn.lpfnHook          = NULL ;
     _ofn.lpTemplateName    = NULL ;
}

BOOL PopFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
     _ofn.hwndOwner         = hwnd ;
     _ofn.lpstrFile         = pstrFileName ;
     _ofn.lpstrFileTitle    = pstrTitleName ;
     _ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
     
     return GetOpenFileName (&_ofn) ;
}

BOOL PopFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
     _ofn.hwndOwner         = hwnd ;
     _ofn.lpstrFile         = pstrFileName ;
     _ofn.lpstrFileTitle    = pstrTitleName ;
     _ofn.Flags             = OFN_OVERWRITEPROMPT ;
     
     return GetSaveFileName (&_ofn) ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif