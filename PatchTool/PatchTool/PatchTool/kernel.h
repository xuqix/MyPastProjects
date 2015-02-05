#ifndef KERNEL_H
#define KERNEL_H
#include "RvaToOffset.h"

#define ITEM_NUM 16
//��̬��������
#define ADD_LAST_SECTION 1	//��Ӵ��뵽���һ������	
#define ADD_NEW_SECTION  2  //��Ӵ��뵽һ���½�������
#define ADD_TO_HEADER	 3  //��Ӵ��뵽PEͷ��
#define	BYTE_PATCH		 4  //�����ټ�һ���ֽڲ��������һЩ�򵥵ĳ���

//��̬��������
#define SLEEP_PATCH 1	
#define DEBUG_PATCH 2

extern HINSTANCE	hInst;   //�˱��������ļ��ж���

//��Ҫ���ܺ���ʵ��
BOOL IsPeFile(TCHAR szFileName[])
{
	HANDLE	hFile;
	WORD	wMagic;
	DWORD   dwRead,dw;
	if (INVALID_HANDLE_VALUE != ( hFile = CreateFile (szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, \
																		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		ReadFile(hFile , &wMagic, 2, &dwRead, NULL);
		if ( wMagic == 0x5A4D)
		{
			SetFilePointer(hFile, 0x3C, 0, FILE_BEGIN);
			ReadFile(hFile , &dw, 4, &dwRead, NULL);
			SetFilePointer(hFile, dw, 0, FILE_BEGIN);
			ReadFile(hFile , &wMagic, 2, &dwRead, NULL);
			if (wMagic == 0x4550)
				return TRUE;
		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////
// �����ַ�����CRC32ֵ
// ������������CRC32ֵ�ַ������׵�ַ�ʹ�С
// ����ֵ: ����CRC32ֵ

DWORD CalCRC32(BYTE* ptr,DWORD Size)
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

DWORD GetCRC32(TCHAR szFileName[])
{
	PIMAGE_DOS_HEADER	    pDosHeader=NULL;
    PIMAGE_NT_HEADERS       pNtHeader=NULL;
    PIMAGE_SECTION_HEADER   pSecHeader=NULL;

	DWORD fileSize, CRC32, NumberOfBytesRW;
 	PBYTE  pBuffer ; 

	//���ļ�
	HANDLE hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) 
		 return FALSE;

	//����ļ����� :
	fileSize = GetFileSize(hFile,NULL);
	if (fileSize == 0xFFFFFFFF) 
		return FALSE;

	pBuffer = new BYTE[fileSize];     // �����ڴ�
	ReadFile(hFile,pBuffer, fileSize, &NumberOfBytesRW, NULL);//��ȡ�ļ�����
	CloseHandle(hFile);  //�ر��ļ�

	pDosHeader	= (PIMAGE_DOS_HEADER)pBuffer;
	fileSize	= fileSize - pDosHeader->e_lfanew;
	CRC32	= CalCRC32(pBuffer + pDosHeader->e_lfanew, fileSize);
	delete	pBuffer;
	return CRC32;
}


/*��Ҫ���������Ľ�Ϊ.sdata��
#pragma data_seg(".sdata")
DWORD	dwTypeOfPatch = 0;			/ָʾ��������
DWORD	dwPatchNum = 2;				//��������
//ƫ��8
TCHAR	szFileName[MAX_PATH] = { 0 };
//ƫ��528
DWORD	dwPatchAddress[16] = { 0}  //////////////////////���õ��ԼĴ�����///////////////////////////////////////////////////////
/////////////����ಹ��Ӧ�ڲ�����ַ��һ����ַ����ϣ���жϵĵ�ַ��ȷ�����е�ַ�����ѽ��룬�Ա�֤������ȷ��///////////
//ƫ��592
BYTE	byOldData[16] = { 0};		//�����������ݺ�������
//ƫ��608
BYTE	byNewData[16] = { 0};
#pragma data_seg()
������Ҫ������CRC32��֤����Ҫ����������PEͷǰ4���ֽ�д��Ŀ���ļ���CRC32*/



//���������ļ�������ģ������Դ����ʽ�洢�ڳ�����
//����:szPatchName:�����Ĳ����ļ���     szFileName:Ŀ���ļ���     lpPatchAddress:������ַ����  
//	   lpNewByte:����ԭʼ��������       lpNewByte:��������������  dwTypeOfPatch:��������
//	   dwPatchNum:��������				ID:����ģ�����ԴID		  bCRC32:�Ƿ����CRC32�ļ���֤
BOOL CreatePatch(TCHAR szPatchName[], TCHAR szFileName[], DWORD lpPatchAddress[],BYTE lpOldByte[],BYTE lpNewByte[],\
				 DWORD	dwTypeOfPatch, DWORD dwPatchNum, DWORD ID, BOOL bCRC32 )
{
	static	char	secName[8] = ".sdata";
	DWORD	CRC32;
	if (bCRC32)		
	{
		CRC32 = GetCRC32(szFileName );
		if (!CRC32)
		{
			MessageBox(NULL, TEXT("CRC32��ȡ����"), NULL, 0);
			return FALSE;
		}
	}
	
	DWORD	dwResSize;
	PBYTE	lpResData;
	HGLOBAL	hGlobal;
	HRSRC hRes	= FindResource(hInst, MAKEINTRESOURCE(ID), L"PETYPE" );
	if (hRes)
	{
		dwResSize	= SizeofResource(hInst, hRes);
		hGlobal		= LoadResource(hInst, hRes);
		if (hGlobal )
		{
			lpResData	= (PBYTE)LockResource(hGlobal);
			if (lpResData )
			{

///////////////////////////////////��ʼд���ļ������������еĲ���//////////////////////////////////////////

				HANDLE					hFile, hMap;
				PBYTE					lpMemory;
				PIMAGE_NT_HEADERS		lpNtHeaders;
				PIMAGE_SECTION_HEADER	lpSectionHeader;
				PBYTE					lpSectionData;
				DWORD*					lpCRC32;
				DWORD					dwFileSize, dwRead, dwSectionNum;

				if (INVALID_HANDLE_VALUE != ( hFile = CreateFile (szPatchName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
				{
					WriteFile(hFile, lpResData, dwResSize, &dwRead, NULL);    //д���ļ�
					dwFileSize	= GetFileSize (hFile, NULL);
					//��������
					if (dwFileSize)
					{
						hMap	= CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
						if (hMap)
						{
							lpMemory	= (BYTE *)MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
							if (lpMemory)
							{
								lpNtHeaders		= (PIMAGE_NT_HEADERS)(lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew);
								if (bCRC32)    //дCRC32ֵ
								{
									lpCRC32			= (DWORD*)((PBYTE)(lpNtHeaders)-4);
									*lpCRC32		= CRC32;
								}
								dwSectionNum	= lpNtHeaders->FileHeader.NumberOfSections;
								lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
								
								//����������������������
								for (DWORD i=0; i < dwSectionNum; i++, lpSectionHeader++)
								{
									if ( !lstrcmpiA( (LPCSTR)lpSectionHeader->Name, secName) )
										break;
								}
								lpSectionData			= lpMemory + RvaToOffset( (PIMAGE_DOS_HEADER)lpMemory, lpSectionHeader->VirtualAddress);
////////////////////////////////////////////////////////////��������///////////////////////////////////////////////////////////////////////////
								int x;
								*(DWORD*)lpSectionData	= dwTypeOfPatch;
								*(DWORD*)(lpSectionData+4) = dwPatchNum;
								
								for(x=lstrlen(szFileName); x > 0; x--)
									if(szFileName[x] == TEXT('\\') )
										break;
								
								lstrcpy( (LPWSTR)(lpSectionData+8), &(szFileName[x]) );
								memcpy(lpSectionData+528, lpPatchAddress, ITEM_NUM*sizeof(DWORD));
								memcpy(lpSectionData+592, lpOldByte, ITEM_NUM*sizeof(BYTE));
								memcpy(lpSectionData+608, lpNewByte, ITEM_NUM*sizeof(BYTE));
////////////////////////////////////////////////////////////�������////////////////////////////////////////////////////////////////////////////
								
								UnmapViewOfFile (lpMemory);
								CloseHandle (hMap);
								CloseHandle (hFile);
								return TRUE;
							}

						}

					}

				}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

				return FALSE;
			}
		}
	}
	return FALSE;
}


#endif