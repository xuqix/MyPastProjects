#ifndef KERNEL_H
#define KERNEL_H
#include "RvaToOffset.h"

#define ITEM_NUM 16
//静态补丁类型
#define ADD_LAST_SECTION 1	//添加代码到最后一个区段	
#define ADD_NEW_SECTION  2  //添加代码到一个新建的区段
#define ADD_TO_HEADER	 3  //添加代码到PE头部
#define	BYTE_PATCH		 4  //这里再加一种字节补丁，针对一些简单的程序

//动态补丁类型
#define SLEEP_PATCH 1	
#define DEBUG_PATCH 2

extern HINSTANCE	hInst;   //此变量在主文件中定义

//主要功能函数实现
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
// 计算字符串的CRC32值
// 参数：欲计算CRC32值字符串的首地址和大小
// 返回值: 返回CRC32值

DWORD CalCRC32(BYTE* ptr,DWORD Size)
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

DWORD GetCRC32(TCHAR szFileName[])
{
	PIMAGE_DOS_HEADER	    pDosHeader=NULL;
    PIMAGE_NT_HEADERS       pNtHeader=NULL;
    PIMAGE_SECTION_HEADER   pSecHeader=NULL;

	DWORD fileSize, CRC32, NumberOfBytesRW;
 	PBYTE  pBuffer ; 

	//打开文件
	HANDLE hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) 
		 return FALSE;

	//获得文件长度 :
	fileSize = GetFileSize(hFile,NULL);
	if (fileSize == 0xFFFFFFFF) 
		return FALSE;

	pBuffer = new BYTE[fileSize];     // 申请内存
	ReadFile(hFile,pBuffer, fileSize, &NumberOfBytesRW, NULL);//读取文件内容
	CloseHandle(hFile);  //关闭文件

	pDosHeader	= (PIMAGE_DOS_HEADER)pBuffer;
	fileSize	= fileSize - pDosHeader->e_lfanew;
	CRC32	= CalCRC32(pBuffer + pDosHeader->e_lfanew, fileSize);
	delete	pBuffer;
	return CRC32;
}


/*需要工具修正的节为.sdata节
#pragma data_seg(".sdata")
DWORD	dwTypeOfPatch = 0;			/指示补丁类型
DWORD	dwPatchNum = 2;				//补丁数量
//偏移8
TCHAR	szFileName[MAX_PATH] = { 0 };
//偏移528
DWORD	dwPatchAddress[16] = { 0}  //////////////////////利用调试寄存器打丁///////////////////////////////////////////////////////
/////////////打此类补丁应在补丁地址第一个地址填上希望中断的地址以确保所有地址数据已解码，以保证补丁正确性///////////
//偏移592
BYTE	byOldData[16] = { 0};		//补丁处旧数据和新数据
//偏移608
BYTE	byNewData[16] = { 0};
#pragma data_seg()
根据需要加入了CRC32验证，需要补丁工具在PE头前4个字节写上目标文件的CRC32*/



//创建补丁文件，补丁模版以资源的形式存储在程序中
//参数:szPatchName:创建的补丁文件名     szFileName:目标文件名     lpPatchAddress:补丁地址数组  
//	   lpNewByte:补丁原始数据数组       lpNewByte:补丁新数据数组  dwTypeOfPatch:补丁类型
//	   dwPatchNum:补丁数量				ID:补丁模版的资源ID		  bCRC32:是否加入CRC32文件验证
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
			MessageBox(NULL, TEXT("CRC32提取出错"), NULL, 0);
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

///////////////////////////////////开始写入文件并修正补丁中的参数//////////////////////////////////////////

				HANDLE					hFile, hMap;
				PBYTE					lpMemory;
				PIMAGE_NT_HEADERS		lpNtHeaders;
				PIMAGE_SECTION_HEADER	lpSectionHeader;
				PBYTE					lpSectionData;
				DWORD*					lpCRC32;
				DWORD					dwFileSize, dwRead, dwSectionNum;

				if (INVALID_HANDLE_VALUE != ( hFile = CreateFile (szPatchName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
				{
					WriteFile(hFile, lpResData, dwResSize, &dwRead, NULL);    //写入文件
					dwFileSize	= GetFileSize (hFile, NULL);
					//修正数据
					if (dwFileSize)
					{
						hMap	= CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
						if (hMap)
						{
							lpMemory	= (BYTE *)MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
							if (lpMemory)
							{
								lpNtHeaders		= (PIMAGE_NT_HEADERS)(lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew);
								if (bCRC32)    //写CRC32值
								{
									lpCRC32			= (DWORD*)((PBYTE)(lpNtHeaders)-4);
									*lpCRC32		= CRC32;
								}
								dwSectionNum	= lpNtHeaders->FileHeader.NumberOfSections;
								lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
								
								//查找需修正变量所在区段
								for (DWORD i=0; i < dwSectionNum; i++, lpSectionHeader++)
								{
									if ( !lstrcmpiA( (LPCSTR)lpSectionHeader->Name, secName) )
										break;
								}
								lpSectionData			= lpMemory + RvaToOffset( (PIMAGE_DOS_HEADER)lpMemory, lpSectionHeader->VirtualAddress);
////////////////////////////////////////////////////////////修正变量///////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////修正完毕////////////////////////////////////////////////////////////////////////////
								
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