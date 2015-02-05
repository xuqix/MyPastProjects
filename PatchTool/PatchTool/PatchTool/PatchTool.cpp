#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include "ApiMacro.h"
#include "kernel.h"

//搞掉些烦人的警告
#pragma warning(disable:4244)
#pragma warning(disable:4996)

#define ITEM_NUM	16		//ListView最大项目数
#define CODE_SIZE	1024*32 //自定义补丁代码的最大长度
static  char	g_szBuffer[CODE_SIZE] = { 0 };	//32KB缓存
static  char	g_szUserCode[CODE_SIZE] = { 0 };//32KB用户代码缓存
BOOL	DIY_OK = FALSE;


//添加WIN7风格界面
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'\
															processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

////////////////////////////////////////界面辅助函数//////////////////////////////////////////////////////
void SuperClass();	//编辑控件超类化，生成16进制编辑控件
LONG WINAPI ProcEdit(HWND, UINT, WPARAM, LPARAM);	//新编辑控件窗口过程
void PopFileInitialize (HWND hwnd);  //文件对话框初始化
BOOL PopFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName); //打开对话框
BOOL PopFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName); //保存对话框
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//主对话框窗口过程
BOOL CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//主对话框消息处理函数
BOOL Main_OnNotify	(HWND hwnd, int wParam,	LPNMHDR pnm);
BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify);
BOOL Main_OnInitDialog (HWND hWnd, HWND hWndFocus, LPARAM lParam) ;

//高级设置对话框
BOOL CALLBACK SetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//帮助对话框
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
//地址输入对话框
BOOL CALLBACK AddressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//定制补丁对话框
BOOL CALLBACK DiyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void GetListData( );	//提取ListView的数据存到全局变量中
void GetPeHeader(TCHAR szFileName[]); //获取整个PE头部数据
BOOL CreateDiyPatch();	//定制补丁生成函数



HINSTANCE	hInst;
HWND		g_hWnd;			//主对话框窗口句柄
HWND		g_hListView;   //表格视图句柄
BOOL		g_bOffset = FALSE;  //记录补丁地址是否是用文件偏移，如果是则转换成虚拟地址	
DWORD		g_dwLineOfNum = 0;  //记录ListView数据行数
DWORD		g_dwTypeOfPatch = 1;//记录补丁类型,并赋值默认方法
DWORD		g_dwTypeOfLoader= 1;//记录Loader类型,并赋值默认方法
BOOL		g_bIsPeFile = FALSE;//指示目标文件是否是PE文件
PBYTE		g_lpPeHeader= NULL; //指向整个PE头部，用于RVA与offset之间的转换
DWORD		g_dwAddress = 0;	//地址对话框获得的地址

TCHAR		g_szFileName[MAX_PATH] = { 0 };		//目标文件名
TCHAR		g_szPatchName[MAX_PATH]= { 0 };		//补丁文件名

DWORD		g_pPatchAddress[ITEM_NUM] = { 0 };	//补丁地址
BYTE		g_pOldByte[ITEM_NUM] = { 0 };		//老数据
BYTE		g_pNewByte[ITEM_NUM] = { 0 };		//新数据


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	hInst = hInstance;
	InitCommonControls();
	SuperClass();//编辑控件超类化
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

    //定义表格外观
	g_hListView	= GetDlgItem(hDlg, IDC_LISTVIEW);
	ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
	//ListView_SetBkColor(g_hListView, 0xa0a0ff);背景色
    ShowWindow (g_hListView, SW_SHOW);

	ListView_InsertCaption(g_hListView, 0, 200, TEXT("虚拟地址") );
	ListView_InsertCaption(g_hListView, 1, 178, TEXT("原始字节") );
	ListView_InsertCaption(g_hListView, 2, 178, TEXT("新的字节") );
	//ListView_AddLine(g_hListView);

	return TRUE;
}


BOOL Main_OnCommand	(HWND hDlg, int id,HWND hCtrl, UINT codeNotify)
{
	static TCHAR	szStr[] = TEXT("选择目标PE文件");
	static TCHAR	szOffset[10];
	static TCHAR	szOldByte[3], szNewByte[3];
	static TCHAR	szStr2[]= TEXT("创建补丁文件");  
	
	TCHAR	szFormat[12] = TEXT("0x00000000");
	int	i;

DWORD a;
	switch(id)
	{
		//case IDC_LISTVIEW:			//列表
		//	return TRUE;

		//case IDC_PATH:				//目标文件路径
		//	return TRUE;

		case IDC_OPEN:				//浏览，打开文件
			if (PopFileOpenDlg(hDlg, g_szFileName, szStr ) )
				if ( IsPeFile(g_szFileName) )
				{
					g_bIsPeFile	= TRUE;
					SetDlgItemText(hDlg, IDC_PATH, g_szFileName);
					GetPeHeader(g_szFileName);   //读取PE头以备偏移转换用
				}
				else
				{
					g_bIsPeFile	= FALSE;
					MessageBox(hDlg, TEXT("文件格式错误"), TEXT("提示"), 0);
				}
			return TRUE;

		case IDC_FILE_OFFSET:		//文件偏移
			g_bOffset	= TRUE;
			ListView_SetCaption(g_hListView, 0, TEXT("文件偏移") );
			
			ListView_DeleteAllItems(g_hListView);
			g_dwLineOfNum = 0;
			return TRUE;

		case IDC_VIRTUALADDRESS:	//虚拟地址
			g_bOffset	= FALSE;
			ListView_SetCaption(g_hListView, 0, TEXT("虚拟地址") );
			
			ListView_DeleteAllItems(g_hListView);
			g_dwLineOfNum = 0;
			return TRUE;

/*		case IDC_OFFSET:	//偏移
			return TRUE;

		case IDC_OLDBYTE:	//老字节
			return TRUE;

		case IDC_NEWBYTE:	//新字节
			return TRUE;
*/
		case IDC_ADD:		//添加
			if (GetDlgItemText(hDlg, IDC_OFFSET, szOffset, sizeof(szOffset)) && GetDlgItemText(hDlg, IDC_OLDBYTE, szOldByte,\
											sizeof(szOldByte)) && GetDlgItemText(hDlg, IDC_NEWBYTE, szNewByte, sizeof(szNewByte)) )
			{
				if ( g_dwLineOfNum < ITEM_NUM)
				{
					szFormat[10-lstrlen(szOffset)] = TEXT('\0');
					lstrcat(szFormat, szOffset);
					i = ListView_AddLine(g_hListView);//添加一行
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

		case IDC_DELETE:	//删除选中行
			i = ListView_GetSelectionMark(g_hListView);  //获得选中行索引
			ListView_DeleteItem(g_hListView, i);
			g_dwLineOfNum--;
			return TRUE;

		case IDC_CLEAR:		//清空所有行
			ListView_DeleteAllItems(g_hListView);
			g_dwLineOfNum = 0;
			return TRUE;

		case IDC_SET:		//高级设置
			DialogBox (hInst, MAKEINTRESOURCE (IDD_SET), hDlg, SetDlgProc) ;
			return TRUE;

		case IDC_LOADER:	//创建Loader
			if (g_bIsPeFile )
			{
				lstrcpy(g_szPatchName, TEXT("Loader.exe") );
				if ( PopFileSaveDlg (hDlg, g_szPatchName, szStr2))
				{
					GetListData( );
					if (g_dwTypeOfLoader == DEBUG_PATCH)		//用调试寄存器补丁需要特别处理
					{
						DialogBoxParam (hInst, MAKEINTRESOURCE(IDD_ADDRESS), NULL, AddressDlgProc, 0);
						if (g_dwAddress == 0)   //如果没有指定进行补丁的地址
							break;
						else
						{
							for (int i=g_dwLineOfNum; i > 0; i--)
							{
								g_pPatchAddress[i] = g_pPatchAddress[i-1];
							}
							g_pPatchAddress[0] = g_dwAddress;  //存储指定地址，由补丁数据的存储方式决定
						}
					}
					if (CreatePatch(g_szPatchName, g_szFileName, g_pPatchAddress, g_pOldByte, g_pNewByte, g_dwTypeOfLoader,\
															g_dwLineOfNum, IDR_LOADER, FALSE)  )//ID为资源ID号
						MessageBox( hDlg, TEXT("创建补丁成功"), TEXT("恭喜"), 0);
					else
						MessageBox(hDlg, TEXT("失败，未知错误"), TEXT("悲剧"), 0);
				}
			}
			else
				MessageBox(hDlg, TEXT("请选择目标文件"), TEXT("创建补丁失败"), 0);
			return TRUE;

		case IDC_PATCH:		//创建补丁
			if (g_bIsPeFile )
			{
				lstrcpy(g_szPatchName, TEXT("Patch.exe") );
				if ( PopFileSaveDlg (hDlg, g_szPatchName, szStr2))
				{
					GetListData( );
					if (CreatePatch(g_szPatchName, g_szFileName, g_pPatchAddress, g_pOldByte, g_pNewByte, g_dwTypeOfPatch,\
															g_dwLineOfNum, IDR_PATCH, TRUE)  )//ID为资源ID号
						MessageBox( hDlg, TEXT("创建补丁成功"), TEXT("恭喜"), 0);
					else
						MessageBox(hDlg, TEXT("失败，未知错误"), TEXT("悲剧"), 0);
				}
			}
			else
				MessageBox(hDlg, TEXT("请选择目标文件"), TEXT("创建补丁失败"), 0);
			return TRUE;

		case IDC_DIY:		//定制补丁
			DialogBox (hInst, MAKEINTRESOURCE (IDD_DIY), hDlg, DiyDlgProc) ;
			return TRUE;

		case IDC_CREATE:	//生成定制补丁
			if (!CreateDiyPatch() )
				MessageBox(hDlg, TEXT("创建补丁失败"), TEXT("oh yea"), 0);
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


//高级设置对话框窗口过程
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

				case IDOK :	//保存设置
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
	static BOOL		bFirst = TRUE; //第一次进入？	
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
				MessageBox(g_hWnd,L"读取失败",L"未知错误", NULL);
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
	static TCHAR	szStr[]= TEXT("创建自定义补丁"); 
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
		lstrcpy(&(szCurrentDirectory[x]), L"\\template\\");  //获得当前目录
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
						memcpy(lpMemory, g_szUserCode, strlen( (char*)g_szUserCode) ); //复制自定义代码
						
						//下面代码用来隐藏控制台，要不一闪一闪比较难看
						TCHAR	szConsoleTitle[100];//控制台标题
						HWND	hCon;
						AllocConsole();
						GetConsoleTitle(szConsoleTitle, sizeof(szConsoleTitle));
						hCon=FindWindow(NULL, szConsoleTitle);
						ShowWindow(hCon, SW_HIDE);
						
						//设置工作目录并编译连接
						SetCurrentDirectory(szCurrentDirectory);
						system("ml /c /coff DiyPatch >> _1.txt");
						system("link /subsystem:windows DiyPatch.obj DiyPatch.res >> _1.txt");
						system("del DiyPatch.obj");
						MoveFile(L"DiyPatch.exe", g_szPatchName);
						memcpy(lpMemory, g_szBuffer, dwFileSize );//还原模版代码
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
							MessageBoxA(g_hWnd, szBuffer, "生成结果", 0);
							fclose(pFile);
						}
						system("del _1.txt");
						SetCurrentDirectory(szOriDirectory); //还原工作目录
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

//提取数据
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
		if (g_bOffset)   //统一转换成虚拟地址
			dwAddress	= OffsetToRva( (PIMAGE_DOS_HEADER)g_lpPeHeader, dwAddress);
		
		g_pPatchAddress[i]	= dwAddress;
		g_pOldByte[i]		= (BYTE)dwOldData;
		g_pNewByte[i]		= (BYTE)dwNewData;
	}
}

//读取目标文件PE头
void GetPeHeader(TCHAR szFileName[])
{
	DWORD	dwRead, dwHeaderSize;
	HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) 
		return ;

	if (g_lpPeHeader)
		delete g_lpPeHeader;

/////////////////////////////////读取整个PE头/////////////////////////////////////////////
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


///////////////////////////////////16进制编辑控件实现/////////////////////////////////////////

WNDPROC		g_lpOldProc;   //edit控件的老窗口过程地址

void SuperClass()
{
	WNDCLASSEX	stWC;
	stWC.cbSize	= sizeof(WNDCLASSEX);
	GetClassInfoEx(NULL, TEXT("Edit"), &stWC);
	g_lpOldProc			= stWC.lpfnWndProc;
	stWC.lpfnWndProc	= ProcEdit;
	stWC.hInstance		= hInst;
	stWC.lpszClassName	= TEXT("HexEdit");   //新类名
	RegisterClassEx(&stWC);
}

TCHAR	szHexChar[] = TEXT("0123456789abcdefABCDEF\b");	  //编辑控件允许显示的字符，包括退格键

LONG WINAPI ProcEdit(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)	//新编辑控件窗口过程
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



//////////////////////////////////////////////文件对话框函数/////////////////////////////////////

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