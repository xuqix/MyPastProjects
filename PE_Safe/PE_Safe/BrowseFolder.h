#ifndef BROWSEFOLDER_H
#define BROWSEFOLDER_H

#include <shlobj.h>

TCHAR 	szDirInfo[] = L"��ѡ��Ҫ�󶨵�Ŀ¼:";
TCHAR*  BrowseFolderTmp;

int CALLBACK BrowseFolderCallBack(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)//����Ի���ص�����
{
	TCHAR szBuffer[130];
	if (msg	== BFFM_INITIALIZED)
		SendMessage (hwnd, BFFM_SETSELECTION, true, (LPARAM)BrowseFolderTmp);
	else
		if (msg	== BFFM_SELCHANGED)
		{
			SHGetPathFromIDList ((LPCITEMIDLIST)lParam, szBuffer);
			SendMessage(hwnd,BFFM_SETSTATUSTEXT,0, (LPARAM)szBuffer);
		}
	return 0;
}

int BrowseFolder(HWND hwnd, TCHAR szBuffer[])//����Ի���ģ��
{
	BROWSEINFO	  stBrowseInfo;
	IMalloc		  *stMalloc;
	bool		  dwReturn;
	LPCITEMIDLIST pidlParent;

	CoInitialize (NULL);
	if ( E_FAIL == SHGetMalloc (&stMalloc) )
	{
		CoUninitialize();
		return false;
	}
	
	RtlZeroMemory (&stBrowseInfo, sizeof (stBrowseInfo) );
	stBrowseInfo.hwndOwner	= hwnd;
	BrowseFolderTmp	= szBuffer;
	stBrowseInfo.lpfn	= BrowseFolderCallBack;
	stBrowseInfo.lpszTitle	= szDirInfo;
	stBrowseInfo.ulFlags	= BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
	if (NULL == ( pidlParent	= SHBrowseForFolder (&stBrowseInfo) ))
		dwReturn	= FALSE;
	else
	{
		dwReturn	= TRUE;
		SHGetPathFromIDList (pidlParent, szBuffer);
	}
	stMalloc->Free ((void *)pidlParent);
	stMalloc->Release();
	CoUninitialize();
	return dwReturn;
}


#endif