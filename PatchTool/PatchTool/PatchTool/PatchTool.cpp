#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include "ApiMacro.h"
#include "kernel.h"

//���Щ���˵ľ���
#pragma warning(disable:4244)
#pragma warning(disable:4996)

#define ITEM_NUM	16		//ListView�����Ŀ��
#define CODE_SIZE	1024*32 //�Զ��岹���������󳤶�
static  char	g_szBuffer[CODE_SIZE] = { 0 };	//32KB����
static  char	g_szUserCode[CODE_SIZE] = { 0 };//32KB�û����뻺��
BOOL	DIY_OK = FALSE;


//���WIN7������
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
															processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

////////////////////////////////////////���渨������//////////////////////////////////////////////////////
void SuperClass();	//�༭�ؼ����໯������16���Ʊ༭�ؼ�
LONG WINAPI ProcEdit(HWND, UINT, WPARAM, LPARAM);	//�±༭�ؼ����ڹ���
void PopFileInitialize (HWND hwnd);  //�ļ��Ի����ʼ��
BOOL PopFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName); //�򿪶Ի���
BOOL PopFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName); //����Ի���
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//���Ի��򴰿ڹ���
BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//���Ի�����Ϣ������
BOOL Main_OnNotify	(HWND hwnd, int wParam,	LPNMHDR pnm);
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;

//�߼����öԻ���
BOOL CALLBACK SetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//�����Ի���
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
//��ַ����Ի���
BOOL CALLBACK AddressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//���Ʋ����Ի���
BOOL CALLBACK DiyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void GetListData( );	//��ȡListView�����ݴ浽ȫ�ֱ�����
void GetPeHeader(TCHAR szFileName[]); //��ȡ����PEͷ������
BOOL CreateDiyPatch();	//���Ʋ������ɺ���



HINSTANCE	hInst;
HWND		g_hWnd;			//���Ի��򴰿ھ��
HWND		g_hListView;   //�����ͼ���
BOOL		g_bOffset = FALSE;  //��¼������ַ�Ƿ������ļ�ƫ�ƣ��������ת���������ַ	
DWORD		g_dwLineOfNum = 0;  //��¼ListView��������
DWORD		g_dwTypeOfPatch = 1;//��¼��������,����ֵĬ�Ϸ���
DWORD		g_dwTypeOfLoader= 1;//��¼Loader����,����ֵĬ�Ϸ���
BOOL		g_bIsPeFile = FALSE;//ָʾĿ���ļ��Ƿ���PE�ļ�
PBYTE		g_lpPeHeader= NULL; //ָ������PEͷ��������RVA��offset֮���ת��
DWORD		g_dwAddress = 0;	//��ַ�Ի����õĵ�ַ

TCHAR		g_szFileName[MAX_PATH] = { 0 };		//Ŀ���ļ���
TCHAR		g_szPatchName[MAX_PATH]= { 0 };		//�����ļ���

DWORD		g_pPatchAddress[ITEM_NUM] = { 0 };	//������ַ
BYTE		g_pOldByte[ITEM_NUM] = { 0 };		//������
BYTE		g_pNewByte[ITEM_NUM] = { 0 };		//������


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	hInst = hInstance;
	InitCommonControls();
	SuperClass();//�༭�ؼ����໯
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

//�Ի����ʼ��
BOOL Main_OnInitDialog (HWND hDlg, HWND hWndFocus, LPARAM lParam) 
{
	chSETDLGICONS(hDlg, IDI_ICON1);
	HWND	hEdit1,hEdit2,hEdit3;
	hEdit1	= GetDlgItem(hDlg, IDC_OFFSET);
	hEdit2	= GetDlgItem(hDlg, IDC_OLDBYTE);
	hEdit3	= GetDlgItem(hDlg, IDC_NEWBYTE);
	Edit_LimitText(hEdit1, 8);
	Edit_LimitText(hEdit2, 2);
	Edit_LimitText(hEdit3, 2);

	Edit_LimitText(GetDlgItem(hDlg, IDC_PATH), MAX_PATH);

	g_hWnd	= hDlg;
	PopFileInitialize (hDlg);
	CheckDlgButton(hDlg, IDC_VIRTUALADDRESS, BST_CHECKED);

    //���������
	g_hListView	= GetDlgItem(hDlg, IDC_LISTVIEW);
	ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
	//ListView_SetBkColor(g_hListView, 0xa0a0ff);����ɫ
    ShowWindow (g_hListView, SW_SHOW);

	ListView_InsertCaption(g_hListView, 0, 200, TEXT("�����ַ") );
	ListView_InsertCaption(g_hListView, 1, 178, TEXT("ԭʼ�ֽ�") );
	ListView_InsertCaption(g_hListView, 2, 178, TEXT("�µ��ֽ�") );
	//ListView_AddLine(g_hListView);

	return TRUE;
}


BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	static TCHAR	szStr[] = TEXT("ѡ��Ŀ��PE�ļ�");
	static TCHAR	szOffset[10];
	static TCHAR	szOldByte[3], szNewByte[3];
	static TCHAR	szStr2[]= TEXT("���������ļ�");  
	
	TCHAR	szFormat[12] = TEXT("0x00000000");
	int	i;

DWORD a;
	switch(id)
	{
		//case IDC_LISTVIEW:			//�б�
		//	return TRUE;

		//case IDC_PATH:				//Ŀ���ļ�·��
		//	return TRUE;

		case IDC_OPEN:				//��������ļ�
			if (PopFileOpenDlg(hDlg, g_szFileName, szStr ) )
				if ( IsPeFile(g_szFileName) )
				{
					g_bIsPeFile	= TRUE;
					SetDlgItemText(hDlg, IDC_PATH, g_szFileName);
					GetPeHeader(g_szFileName);   //��ȡPEͷ�Ա�ƫ��ת����
				}
				else
				{
					g_bIsPeFile	= FALSE;
					MessageBox(hDlg, TEXT("�ļ���ʽ����"), TEXT("��ʾ"), 0);
				}
			return TRUE;

		case IDC_FILE_OFFSET:		//�ļ�ƫ��
			g_bOffset	= TRUE;
			ListView_SetCaption(g_hListView, 0, TEXT("�ļ�ƫ��") );
			
			ListView_DeleteAllItems(g_hListView);
			g_dwLineOfNum = 0;
			return TRUE;

		case IDC_VIRTUALADDRESS:	//�����ַ
			g_bOffset	= FALSE;
			ListView_SetCaption(g_hListView, 0, TEXT("�����ַ") );
			
			ListView_DeleteAllItems(g_hListView);
			g_dwLineOfNum = 0;
			return TRUE;

/*		case IDC_OFFSET:	//ƫ��
			return TRUE;

		case IDC_OLDBYTE:	//���ֽ�
			return TRUE;

		case IDC_NEWBYTE:	//���ֽ�
			return TRUE;
*/
		case IDC_ADD:		//���
			if (GetDlgItemText(hDlg, IDC_OFFSET, szOffset, sizeof(szOffset)) && GetDlgItemText(hDlg, IDC_OLDBYTE, szOldByte,\
											sizeof(szOldByte)) && GetDlgItemText(hDlg, IDC_NEWBYTE, szNewByte, sizeof(szNewByte)) )
			{
				if ( g_dwLineOfNum < ITEM_NUM)
				{
					szFormat[10-lstrlen(szOffset)] = TEXT('\0');
					lstrcat(szFormat, szOffset);
					i = ListView_AddLine(g_hListView);//���һ��
					ListView_SetItemText(g_hListView, i, 0, szFormat);
					ListView_SetItemText(g_hListView, i, 1, szOldByte);
					ListView_SetItemText(g_hListView, i, 2, szNewByte);
					SetDlgItemText(hDlg, IDC_OFFSET, NULL);
					SetDlgItemText(hDlg, IDC_OLDBYTE, NULL);
					SetDlgItemText(hDlg, IDC_NEWBYTE, NULL);
					g_dwLineOfNum++;
				}
			}
			return TRUE;

		case IDC_DELETE:	//ɾ��ѡ����
			i = ListView_GetSelectionMark(g_hListView);  //���ѡ��������
			ListView_DeleteItem(g_hListView, i);
			g_dwLineOfNum--;
			return TRUE;

		case IDC_CLEAR:		//���������
			ListView_DeleteAllItems(g_hListView);
			g_dwLineOfNum = 0;
			return TRUE;

		case IDC_SET:		//�߼�����
			DialogBox (hInst, MAKEINTRESOURCE (IDD_SET), hDlg, SetDlgProc) ;
			return TRUE;

		case IDC_LOADER:	//����Loader
			if (g_bIsPeFile )
			{
				lstrcpy(g_szPatchName, TEXT("Loader.exe") );
				if ( PopFileSaveDlg (hDlg, g_szPatchName, szStr2))
				{
					GetListData( );
					if (g_dwTypeOfLoader == DEBUG_PATCH)		//�õ��ԼĴ���������Ҫ�ر���
					{
						DialogBoxParam (hInst, MAKEINTRESOURCE(IDD_ADDRESS), NULL, AddressDlgProc, 0);
						if (g_dwAddress == 0)   //���û��ָ�����в����ĵ�ַ
							break;
						else
						{
							for (int i=g_dwLineOfNum; i > 0; i--)
							{
								g_pPatchAddress[i] = g_pPatchAddress[i-1];
							}
							g_pPatchAddress[0] = g_dwAddress;  //�洢ָ����ַ���ɲ������ݵĴ洢��ʽ����
						}
					}
					if (CreatePatch(g_szPatchName, g_szFileName, g_pPatchAddress, g_pOldByte, g_pNewByte, g_dwTypeOfLoader,\
															g_dwLineOfNum, IDR_LOADER, FALSE)  )//IDΪ��ԴID��
						MessageBox( hDlg, TEXT("���������ɹ�"), TEXT("��ϲ"), 0);
					else
						MessageBox(hDlg, TEXT("ʧ�ܣ�δ֪����"), TEXT("����"), 0);
				}
			}
			else
				MessageBox(hDlg, TEXT("��ѡ��Ŀ���ļ�"), TEXT("��������ʧ��"), 0);
			return TRUE;

		case IDC_PATCH:		//��������
			if (g_bIsPeFile )
			{
				lstrcpy(g_szPatchName, TEXT("Patch.exe") );
				if ( PopFileSaveDlg (hDlg, g_szPatchName, szStr2))
				{
					GetListData( );
					if (CreatePatch(g_szPatchName, g_szFileName, g_pPatchAddress, g_pOldByte, g_pNewByte, g_dwTypeOfPatch,\
															g_dwLineOfNum, IDR_PATCH, TRUE)  )//IDΪ��ԴID��
						MessageBox( hDlg, TEXT("���������ɹ�"), TEXT("��ϲ"), 0);
					else
						MessageBox(hDlg, TEXT("ʧ�ܣ�δ֪����"), TEXT("����"), 0);
				}
			}
			else
				MessageBox(hDlg, TEXT("��ѡ��Ŀ���ļ�"), TEXT("��������ʧ��"), 0);
			return TRUE;

		case IDC_DIY:		//���Ʋ���
			DialogBox (hInst, MAKEINTRESOURCE (IDD_DIY), hDlg, DiyDlgProc) ;
			return TRUE;

		case IDC_CREATE:	//���ɶ��Ʋ���
			if (!CreateDiyPatch() )
				MessageBox(hDlg, TEXT("��������ʧ��"), TEXT("oh yea"), 0);
			return TRUE;

		case IDM_HELP_ABOUT :
			DialogBox (hInst, MAKEINTRESOURCE (IDD_ABOUT), hDlg, AboutDlgProc) ;
			return TRUE;

		case IDCANCEL:
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}


//�߼����öԻ��򴰿ڹ���
BOOL CALLBACK SetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static DWORD	dwTmpPatch = 1;
	static DWORD    dwTmpLoader= 1;

	switch(message)
	{
		case WM_INITDIALOG:
			switch(g_dwTypeOfPatch)
			{
				case ADD_LAST_SECTION:
					CheckDlgButton(hDlg, IDC_ADDLAST, BST_CHECKED);
					break;
					
				case ADD_NEW_SECTION:
					CheckDlgButton(hDlg, IDC_ADDNEW, BST_CHECKED);
					break;
					
				case ADD_TO_HEADER:
					CheckDlgButton(hDlg, IDC_ADDHEADER, BST_CHECKED);
					break;

				case BYTE_PATCH:
					CheckDlgButton(hDlg, IDC_ADDFILE, BST_CHECKED);
					break;
			}
			switch(g_dwTypeOfLoader)
			{
				case SLEEP_PATCH:
					CheckDlgButton(hDlg, IDC_THREAD, BST_CHECKED);
					break;

				case DEBUG_PATCH:
					CheckDlgButton(hDlg, IDC_DEBUG, BST_CHECKED);
					break;
			}
			
			return TRUE;
    
		case WM_COMMAND :
			switch (LOWORD (wParam))  		            
			{
				case IDC_ADDLAST:
					dwTmpPatch	= ADD_LAST_SECTION;
					return TRUE;

				case IDC_ADDHEADER:
					dwTmpPatch	= ADD_TO_HEADER;
					return TRUE;

				case IDC_ADDNEW:
					dwTmpPatch	= ADD_NEW_SECTION;
					return TRUE;

				case IDC_ADDFILE:
					dwTmpPatch	= BYTE_PATCH;
					return TRUE;

				case IDC_THREAD:
					dwTmpLoader = SLEEP_PATCH;
					return TRUE;

				case IDC_DEBUG:
					dwTmpLoader = DEBUG_PATCH;
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;

				case IDOK :	//��������
					g_dwTypeOfPatch	= dwTmpPatch;
					g_dwTypeOfLoader= dwTmpLoader;
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



BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case  WM_LBUTTONDOWN:
			PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);//ʵ����קЧ��
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

BOOL CALLBACK AddressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR	szBuffer[12];
	static HWND		hEdit;

	switch(message)
	{
		case WM_INITDIALOG:
			hEdit	= GetDlgItem(hDlg, ID_ADDRESS);
			Edit_LimitText(hEdit, 8);
			return TRUE;

		case WM_COMMAND :
			switch (LOWORD (wParam))  		            
			{
				
				case IDOK :
					GetDlgItemText(hDlg, ID_ADDRESS, szBuffer, sizeof(szBuffer) );
					swscanf(szBuffer, L"%x", &g_dwAddress);
					EndDialog (hDlg, 0) ;
					return TRUE ;
			}
			return TRUE ;

		case WM_CLOSE:
			g_dwAddress	= 0;
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK DiyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND		hEdit;
	static FILE		*pFile;
	static long		fileSize;
	static BOOL		bFirst = TRUE; //��һ�ν��룿	
	char	ch;
	int		i;

	switch(message)
	{
		case WM_INITDIALOG:
			hEdit	= GetDlgItem(hDlg, IDC_CODE);
			if (bFirst)
			{
				bFirst = FALSE;
				pFile = fopen(".\\template\\patch", "r");
				if (pFile != NULL)
				{
					i=0;
					while((ch = fgetc(pFile) )!= EOF) 
					{
						if (ch == '\n')
							g_szBuffer[i++]='\r';
						g_szBuffer[i++]=ch;
					}
					SetWindowTextA(hEdit, g_szBuffer);
					DIY_OK = TRUE;
					fclose(pFile);
				}
				ShowWindow(g_hWnd, SW_HIDE);
				return TRUE;
			}
			if (DIY_OK)
			{
				ShowWindow(g_hWnd, SW_HIDE);
				SetWindowTextA(hEdit, g_szBuffer);
			}
			else
			{
				MessageBox(g_hWnd,L"��ȡʧ��",L"δ֪����", NULL);
				EndDialog (hDlg, 0) ;
			}
			return TRUE;

		case WM_COMMAND :
			switch (LOWORD (wParam))  		            
			{
				
				case IDOK :
					GetDlgItemTextA(hDlg, IDC_CODE, g_szUserCode, CODE_SIZE );
					EnableWindow(GetDlgItem(g_hWnd, IDC_CREATE), TRUE);
					ShowWindow(g_hWnd, SW_SHOW);
					EndDialog (hDlg, 0) ;
					return TRUE ;

				case IDCANCEL:
					EnableWindow(GetDlgItem(g_hWnd, IDC_CREATE), FALSE);
					ShowWindow(g_hWnd, SW_SHOW);
					EndDialog (hDlg, 0) ;
					return TRUE;
			}
			return TRUE ;

		case WM_CLOSE:
			ShowWindow(g_hWnd, SW_SHOW);
			EndDialog (hDlg, 0) ;
			return TRUE;
	}
	return FALSE;
}

BOOL CreateDiyPatch()
{
	static TCHAR	szStr[]= TEXT("�����Զ��岹��"); 
	static char		szBuffer[1024];
	static TCHAR	szPatch[MAX_PATH];	
	static TCHAR	szCurrentDirectory[MAX_PATH];
	static TCHAR	szOriDirectory[MAX_PATH];
	HANDLE	hAsmFile, hAsmMap;
	PBYTE	lpMemory;
	DWORD	dwFileSize;
	int x;
	if (DIY_OK == FALSE)
		return FALSE;
	if ( PopFileSaveDlg (g_hWnd, g_szPatchName, szStr) )
	{
		GetCurrentDirectory(sizeof(szOriDirectory), szOriDirectory);
		GetModuleFileName(hInst, szPatch, sizeof(szPatch) );
		for(x=lstrlen(szPatch); x > 0; x--)
			if(szPatch[x] == TEXT('\\') )
				break;
		lstrcpy(szCurrentDirectory, szPatch);
		lstrcpy(&(szCurrentDirectory[x]), L"\\template\\");  //��õ�ǰĿ¼
		lstrcpy( &(szPatch[x]), TEXT("\\template\\patch"));
		if (INVALID_HANDLE_VALUE != ( hAsmFile = CreateFile ( szPatch, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
		{
			dwFileSize	= GetFileSize (hAsmFile, NULL);
			if (dwFileSize)
			{
				hAsmMap	= CreateFileMapping (hAsmFile, NULL, PAGE_READWRITE, 0, 0, NULL);
				if (hAsmMap)
				{
					lpMemory	= (BYTE *)MapViewOfFile (hAsmMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
					if (lpMemory)
					{
						memcpy(lpMemory, g_szUserCode, strlen( (char*)g_szUserCode) ); //�����Զ������
						
						//��������������ؿ���̨��Ҫ��һ��һ���Ƚ��ѿ�
						TCHAR	szConsoleTitle[100];//����̨����
						HWND	hCon;
						AllocConsole();
						GetConsoleTitle(szConsoleTitle, sizeof(szConsoleTitle));
						hCon=FindWindow(NULL, szConsoleTitle);
						ShowWindow(hCon, SW_HIDE);
						
						//���ù���Ŀ¼����������
						SetCurrentDirectory(szCurrentDirectory);
						system("ml /c /coff DiyPatch >> _1.txt");
						system("link /subsystem:windows DiyPatch.obj DiyPatch.res >> _1.txt");
						system("del DiyPatch.obj");
						MoveFile(L"DiyPatch.exe", g_szPatchName);
						memcpy(lpMemory, g_szBuffer, dwFileSize );//��ԭģ�����
						UnmapViewOfFile(lpMemory);
						CloseHandle(hAsmMap);
						CloseHandle(hAsmFile);

						FILE *pFile = fopen("_1.txt", "r+");
						int	ch,i=0;
						if (pFile != NULL)
						{
							while((ch = fgetc(pFile) )!= EOF) 
							{
								if (ch == '\n')
									szBuffer[i++]='\r';
								szBuffer[i++]=ch;
							}
							MessageBoxA(g_hWnd, szBuffer, "���ɽ��", 0);
							fclose(pFile);
						}
						system("del _1.txt");
						SetCurrentDirectory(szOriDirectory); //��ԭ����Ŀ¼
						return TRUE;
					}
					else	UnmapViewOfFile(lpMemory);
				}
				else	CloseHandle(hAsmMap);
			}
			else	CloseHandle(hAsmFile);
		}
	}
	return FALSE;
}

//��ȡ����
void GetListData( )
{
	TCHAR	szBuffer[12] = { 0 };
	DWORD	dwAddress;
	DWORD	dwOldData;
	DWORD	dwNewData;
	for ( DWORD i=0; i < g_dwLineOfNum; i++)
	{
		ListView_GetItemText(g_hListView, i, 0, szBuffer, sizeof(szBuffer) ); 
		swscanf(szBuffer, L"%x", &dwAddress);
		ListView_GetItemText(g_hListView, i, 1, szBuffer, sizeof(szBuffer) );
		swscanf(szBuffer, L"%x", &dwOldData);
		ListView_GetItemText(g_hListView, i, 2, szBuffer, sizeof(szBuffer) );
		swscanf(szBuffer, L"%x", &dwNewData);
		if (g_bOffset)   //ͳһת���������ַ
			dwAddress	= OffsetToRva( (PIMAGE_DOS_HEADER)g_lpPeHeader, dwAddress);
		
		g_pPatchAddress[i]	= dwAddress;
		g_pOldByte[i]		= (BYTE)dwOldData;
		g_pNewByte[i]		= (BYTE)dwNewData;
	}
}

//��ȡĿ���ļ�PEͷ
void GetPeHeader(TCHAR szFileName[])
{
	DWORD	dwRead, dwHeaderSize;
	HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) 
		return ;

	if (g_lpPeHeader)
		delete g_lpPeHeader;

/////////////////////////////////��ȡ����PEͷ/////////////////////////////////////////////
	IMAGE_NT_HEADERS	stNtHeaders;
	SetFilePointer(hFile, 0x3c, 0, FILE_BEGIN);	
	ReadFile(hFile, &dwHeaderSize, 4, &dwRead, NULL);
	SetFilePointer(hFile, dwHeaderSize, 0, FILE_BEGIN);
	ReadFile(hFile, &stNtHeaders, sizeof(IMAGE_NT_HEADERS), &dwRead, NULL);
	dwHeaderSize	= stNtHeaders.OptionalHeader.SizeOfHeaders;
	g_lpPeHeader	= new BYTE[dwHeaderSize];
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	ReadFile(hFile, g_lpPeHeader, dwHeaderSize, &dwRead, NULL);
	CloseHandle(hFile);
//////////////////////////////////////////////////////////////////////////////////////////
}


///////////////////////////////////16���Ʊ༭�ؼ�ʵ��/////////////////////////////////////////

WNDPROC		g_lpOldProc;   //edit�ؼ����ϴ��ڹ��̵�ַ

void SuperClass()
{
	WNDCLASSEX	stWC;
	stWC.cbSize	= sizeof(WNDCLASSEX);
	GetClassInfoEx(NULL, TEXT("Edit"), &stWC);
	g_lpOldProc			= stWC.lpfnWndProc;
	stWC.lpfnWndProc	= ProcEdit;
	stWC.hInstance		= hInst;
	stWC.lpszClassName	= TEXT("HexEdit");   //������
	RegisterClassEx(&stWC);
}

TCHAR	szHexChar[] = TEXT("0123456789abcdefABCDEF\b");	  //�༭�ؼ�������ʾ���ַ��������˸��

LONG WINAPI ProcEdit(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)	//�±༭�ؼ����ڹ���
{
	int len=lstrlen(szHexChar);
	if (message == WM_CHAR)
	{
		for (int i=0; i < len; i++)
		{
			if (szHexChar[i] == (TCHAR)wParam)
			{
				if (wParam > TEXT('9') )
					wParam &= ~0x20;
				return CallWindowProc(g_lpOldProc, hwnd, message, wParam, lParam);
			}
		}
		return TRUE;
	}

	return CallWindowProc(g_lpOldProc, hwnd, message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////�ļ��Ի�����/////////////////////////////////////

static OPENFILENAME ofn ;

void PopFileInitialize (HWND hwnd)
{
     static TCHAR szFilter[] = TEXT ("PE Files (*.exe;*.dll)\0*.exe;*.dll;*.scr;*.fon;*.drv\0")  \
                               TEXT ("All Files (*.*)\0*.*\0\0") ;
     
     ofn.lStructSize       = sizeof (OPENFILENAME) ;
     ofn.hwndOwner         = hwnd ;
     ofn.hInstance         = NULL ;
     ofn.lpstrFilter       = szFilter ;
     ofn.lpstrCustomFilter = NULL ;
     ofn.nMaxCustFilter    = 0 ;
     ofn.nFilterIndex      = 0 ;
     ofn.lpstrFile         = NULL ;          // Set in Open and Close functions
     ofn.nMaxFile          = MAX_PATH ;
     ofn.lpstrFileTitle    = NULL ;          // Set in Open and Close functions
     ofn.nMaxFileTitle     = MAX_PATH ;
     ofn.lpstrInitialDir   = NULL ;
     ofn.lpstrTitle        = NULL ;
     ofn.Flags             = 0 ;             // Set in Open and Close functions
     ofn.nFileOffset       = 0 ;
     ofn.nFileExtension    = 0 ;
     ofn.lpstrDefExt       = NULL; //TEXT ("exe") ;
     ofn.lCustData         = 0L ;
     ofn.lpfnHook          = NULL ;
     ofn.lpTemplateName    = NULL ;
}

BOOL PopFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
     ofn.hwndOwner         = hwnd ;
     ofn.lpstrFile         = pstrFileName ;
     ofn.lpstrFileTitle    = pstrTitleName ;
     ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
     
     return GetOpenFileName (&ofn) ;
}

BOOL PopFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
     ofn.hwndOwner         = hwnd ;
     ofn.lpstrFile         = pstrFileName ;
     ofn.lpstrFileTitle    = pstrTitleName ;
     ofn.Flags             = OFN_OVERWRITEPROMPT ;
     
     return GetSaveFileName (&ofn) ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////