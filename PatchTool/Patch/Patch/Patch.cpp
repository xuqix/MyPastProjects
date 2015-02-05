#include <windows.h>
#include "resource.h"
#include "AddCode.h"

#pragma comment(linker, "/SECTION:.text,ERW")    //���д���ԣ�����Ҫ��������

///////////////////////////���ö�ͼ��////////////////////////////////
inline void chSETDLGICONS(HWND hWnd, int idi) {
   SendMessage(hWnd, WM_SETICON, ICON_BIG,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

BOOL CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);	//�����ڹ���

BOOL IsFileModified( );  //����ļ��Ƿ��Ѳ�������
DWORD CRC32(BYTE* ptr,DWORD Size);  //��ȡĿ���ļ�CRC32У��ֵ���ͱ���������洢��У��ֵ���жԱ�(У��ֵΪPEͷǰ4���ֽ�)

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
				case IDC_BACKUP:	//����ѡ��
					if(SendDlgItemMessage(hDlg, IDC_BACKUP, BM_GETCHECK, 0, 0) == BST_CHECKED)
						bBackup	= TRUE;
					else
						bBackup = FALSE;
					break;

				case IDOK:
					if ( !IsFileModified() )
					{
						MessageBox(hDlg, TEXT("�Ѳ������ļ���,����"), TEXT("��ʾ"), 0);
						break;
					}
					if (bBackup )   //�����ļ�����
					{
						lstrcpy(szBakFileName,szFileName);
						lstrcat(szBakFileName,TEXT(".bak") );
						CopyFile(szFileName,szBakFileName,TRUE);
					}
					if (AddCode( ) )	//ΪĿ��PE�򲹶�(��PE��Ӵ���)
						MessageBox(hDlg, TEXT("�����ɹ�"), TEXT("��ʾ"), 0);
					else
						MessageBox(hDlg, TEXT("����ʧ��"), TEXT("��ʾ"), 0);
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
// ���ļ��ж�CRC32ֵ�Ƿ���ȷ
//

BOOL IsFileModified()
{
	PIMAGE_DOS_HEADER	    pDosHeader=NULL;
    PIMAGE_NT_HEADERS       pNtHeader=NULL;
    PIMAGE_SECTION_HEADER   pSecHeader=NULL;

	DWORD fileSize,OriginalCRC32,NumberOfBytesRW;
 	PBYTE  pBuffer ; 

	//���ļ�
	HANDLE hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) 
		 return FALSE;


	//����ļ����� :
	fileSize = GetFileSize(hFile,NULL);
	if (fileSize == 0xFFFFFFFF) 
		return FALSE;

	pBuffer = new BYTE[fileSize];     // �����ڴ�,Ҳ����VirtualAlloc�Ⱥ��������ڴ�
	ReadFile(hFile,pBuffer, fileSize, &NumberOfBytesRW, NULL);//��ȡ�ļ�����
	CloseHandle(hFile);  //�ر��ļ�

	pDosHeader=(PIMAGE_DOS_HEADER)pBuffer;

///////////////��λ�����ļ�PEͷǰ4���ֽڶ�ȡĿ��CRC32��ֵ/////////////////////////////
	TCHAR	szMyName[MAX_PATH];
	DWORD	dwRead;
	GetModuleFileName(NULL, szMyName,MAX_PATH);
	HANDLE hMyFile = CreateFile(szMyName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hMyFile == INVALID_HANDLE_VALUE ) 
	{
		delete pBuffer;
		return FALSE;
	}

	//��λ��PEͷǰ4���ֽڴ�����ȡ
	SetFilePointer(hMyFile, 0x3c, 0, FILE_BEGIN);	
	ReadFile(hMyFile, &OriginalCRC32, 4, &dwRead, NULL);
	SetFilePointer(hMyFile, OriginalCRC32-4, 0, FILE_BEGIN);
	ReadFile(hMyFile, &OriginalCRC32, 4, &dwRead, NULL); 	
	CloseHandle(hMyFile);
//////////////////////////////////////////////////////////////////////////////////////////

	fileSize=fileSize-DWORD(pDosHeader->e_lfanew);//��PE�ļ�ͷǰ�ǲ�������ȥ��

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
// �����ַ�����CRC32ֵ
// ������������CRC32ֵ�ַ������׵�ַ�ʹ�С
// ����ֵ: ����CRC32ֵ

DWORD CRC32(BYTE* ptr,DWORD Size)
{

	DWORD crcTable[256],crcTmp1;
	
	//��̬����CRC-32��
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
	//����CRC32ֵ
	DWORD crcTmp2= 0xFFFFFFFF;
	while(Size--)
	{
		crcTmp2 = ((crcTmp2>>8) & 0x00FFFFFF) ^ crcTable[ (crcTmp2^(*ptr)) & 0xFF ];
		ptr++;
	}
		
	return (crcTmp2^0xFFFFFFFF);
}