#include <windows.h>
#include "resource.h"
#include "AddCode.h"

#pragma comment(linker, "/SECTION:.text,ERW")    //添加写属性，后面要修正参数

///////////////////////////设置对图标////////////////////////////////
inline void chSETDLGICONS(HWND hWnd, int idi) {
   SendMessage(hWnd, WM_SETICON, ICON_BIG,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);	//主窗口过程

BOOL IsFileModified( );  //检测文件是否已补丁或损坏
DWORD CRC32(BYTE* ptr,DWORD Size);  //获取目标文件CRC32校验值，和本补丁程序存储的校验值进行对比(校验值为PE头前4个字节)

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,PSTR szCmdLine, int iCmdShow)
{

	DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc, 0);

    return 0 ;
}

BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static	BOOL	bBackup = TRUE;
	static  TCHAR	szBakFileName[MAX_PATH] ;  

	switch (message)
	{
		case WM_INITDIALOG:
			chSETDLGICONS(hDlg, IDI_ICON1);
			CheckDlgButton(hDlg, IDC_BACKUP, BST_CHECKED);
			return TRUE;

		case WM_PAINT:
			return FALSE;

		case WM_COMMAND:
			switch (LOWORD (wParam))
			{
				case IDC_BACKUP:	//备份选项
					if(SendDlgItemMessage(hDlg, IDC_BACKUP, BM_GETCHECK, 0, 0) == BST_CHECKED)
						bBackup	= TRUE;
					else
						bBackup = FALSE;
					break;

				case IDOK:
					if ( !IsFileModified() )
					{
						MessageBox(hDlg, TEXT("已补丁或文件损坏,放弃"), TEXT("提示"), 0);
						break;
					}
					if (bBackup )   //进行文件备份
					{
						lstrcpy(szBakFileName,szFileName);
						lstrcat(szBakFileName,TEXT(".bak") );
						CopyFile(szFileName,szBakFileName,TRUE);
					}
					if (AddCode( ) )	//为目标PE打补丁(给PE添加代码)
						MessageBox(hDlg, TEXT("补丁成功"), TEXT("提示"), 0);
					else
						MessageBox(hDlg, TEXT("补丁失败"), TEXT("提示"), 0);
					break;
				
				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;
			}
			return TRUE;
	}

	return FALSE;
}


////////////////////////////////////////////////////////////////
// 打开文件判断CRC32值是否正确
//

BOOL IsFileModified()
{
	PIMAGE_DOS_HEADER	    pDosHeader=NULL;
    PIMAGE_NT_HEADERS       pNtHeader=NULL;
    PIMAGE_SECTION_HEADER   pSecHeader=NULL;

	DWORD fileSize,OriginalCRC32,NumberOfBytesRW;
 	PBYTE  pBuffer ; 

	//打开文件
	HANDLE hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) 
		 return FALSE;


	//获得文件长度 :
	fileSize = GetFileSize(hFile,NULL);
	if (fileSize == 0xFFFFFFFF) 
		return FALSE;

	pBuffer = new BYTE[fileSize];     // 申请内存,也可用VirtualAlloc等函数申请内存
	ReadFile(hFile,pBuffer, fileSize, &NumberOfBytesRW, NULL);//读取文件内容
	CloseHandle(hFile);  //关闭文件

	pDosHeader=(PIMAGE_DOS_HEADER)pBuffer;

///////////////定位到本文件PE头前4个字节读取目标CRC32的值/////////////////////////////
	TCHAR	szMyName[MAX_PATH];
	DWORD	dwRead;
	GetModuleFileName(NULL, szMyName,MAX_PATH);
	HANDLE hMyFile = CreateFile(szMyName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hMyFile == INVALID_HANDLE_VALUE ) 
	{
		delete pBuffer;
		return FALSE;
	}

	//定位到PE头前4个字节处并读取
	SetFilePointer(hMyFile, 0x3c, 0, FILE_BEGIN);	
	ReadFile(hMyFile, &OriginalCRC32, 4, &dwRead, NULL);
	SetFilePointer(hMyFile, OriginalCRC32-4, 0, FILE_BEGIN);
	ReadFile(hMyFile, &OriginalCRC32, 4, &dwRead, NULL); 	
	CloseHandle(hMyFile);
//////////////////////////////////////////////////////////////////////////////////////////

	fileSize=fileSize-DWORD(pDosHeader->e_lfanew);//将PE文件头前那部分数据去除

	if (CRC32((BYTE*)(pBuffer+pDosHeader->e_lfanew),fileSize) == OriginalCRC32 )
	{
		delete pBuffer;
		return TRUE;
	}
	else
	{
		delete pBuffer;
		return FALSE;
	}

}
////////////////////////////////////////////////////////////////
// 计算字符串的CRC32值
// 参数：欲计算CRC32值字符串的首地址和大小
// 返回值: 返回CRC32值

DWORD CRC32(BYTE* ptr,DWORD Size)
{

	DWORD crcTable[256],crcTmp1;
	
	//动态生成CRC-32表
	for (int i=0; i<256; i++)
	 {
		crcTmp1 = i;
		for (int j=8; j>0; j--)
		 {
			if (crcTmp1&1) crcTmp1 = (crcTmp1 >> 1) ^ 0xEDB88320L;
			 else crcTmp1 >>= 1;
		}

		 crcTable[i] = crcTmp1;
	 }
	//计算CRC32值
	DWORD crcTmp2= 0xFFFFFFFF;
	while(Size--)
	{
		crcTmp2 = ((crcTmp2>>8) & 0x00FFFFFF) ^ crcTable[ (crcTmp2^(*ptr)) & 0xFF ];
		ptr++;
	}
		
	return (crcTmp2^0xFFFFFFFF);
}