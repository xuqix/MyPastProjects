#include <windows.h>
#include "PE32_K.H"


TCHAR		g_szFileName[MAX_PATH] = { 0 };
HINSTANCE	g_hInstDll	= NULL;		//DLL模块句柄
PBYTE		g_lpBuffer	= NULL;		//存储文件数据
DWORD		g_dwFileSize= 0;		//文件数据大小
PBYTE		g_lpNewFile = NULL;		//存储添加代码后的数据
BOOL		PE	= FALSE;  //指示是否是PE文件


void _stdcall DllClean();

///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, PVOID fImpLoad) {

   switch (fdwReason) {

      case DLL_PROCESS_ATTACH:
         // DLL is attaching to the address space of the current process.
         g_hInstDll = hInstDll;
         break;

      case DLL_THREAD_ATTACH:
         // A new thread is being created in the current process.
         break;

      case DLL_THREAD_DETACH:
         // A thread is exiting cleanly.
         break;

      case DLL_PROCESS_DETACH:
         // The calling process is detaching the DLL from its address space.
         DllClean();
		 break;
   }
   return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////将RVA偏移转换成文件偏移,失败返回-1////////////////////////////////////////////////

DWORD _stdcall RvaToOffset (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva)
{
	::IMAGE_NT_HEADERS		*lpPEHead;
	::IMAGE_SECTION_HEADER	*lpSectionHead;
	DWORD i;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	i	= lpPEHead->FileHeader.NumberOfSections;
	lpSectionHead	= (IMAGE_SECTION_HEADER*)(++lpPEHead); 
	for ( ; i > 0 ; i--, lpSectionHead++)
	{
		if ( (dwRva >= lpSectionHead->VirtualAddress) && (dwRva < (lpSectionHead->VirtualAddress + lpSectionHead->SizeOfRawData) ) )
		{
			dwRva	= dwRva - lpSectionHead->VirtualAddress + lpSectionHead->PointerToRawData;
			return dwRva;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////将RVA偏移转成文件指针偏移,失败返回NULL////////////////////////////////////////////////

PBYTE _stdcall RvaToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwRva)
{
	DWORD	Offset	= RvaToOffset(lpFileHead, dwRva);
	if(Offset == -1)
		return NULL;
	return	(PBYTE)(lpFileHead) + Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////将虚拟地址转成文件指针偏移,失败返回NULL////////////////////////////////////////////////

PBYTE _stdcall VirtualAddressToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwVirtualAddress)
{
	::IMAGE_NT_HEADERS		*lpPEHead;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	
	return (PBYTE)RvaToPointer(lpFileHead, dwVirtualAddress - lpPEHead->OptionalHeader.ImageBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////获得RVA偏移处的节区名称////////////////////////////////////////////////

CONST PBYTE _stdcall GetRvaSection (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva)
{
	IMAGE_NT_HEADERS		*lpPEHead;
	IMAGE_SECTION_HEADER	*lpSectionHead;
	DWORD i;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	i	= lpPEHead->FileHeader.NumberOfSections;
	lpSectionHead	= (IMAGE_SECTION_HEADER*)(++lpPEHead); 
	for ( ; i > 0 ; i--, lpSectionHead++)
	{
		if ( (dwRva >= lpSectionHead->VirtualAddress) && (dwRva < (lpSectionHead->VirtualAddress + lpSectionHead->SizeOfRawData) ) )
		{
			return (PBYTE)lpSectionHead;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////获得指定RVA所处节区的节表头,失败返回NULL/////////////////////////////////////////

PIMAGE_SECTION_HEADER _stdcall GetSectionOfRva (IMAGE_DOS_HEADER *lpFileHead, char* secName)
{
	::PIMAGE_NT_HEADERS lpNtHead		= (PIMAGE_NT_HEADERS)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	DWORD dwSec = lpNtHead->FileHeader.NumberOfSections;
	IMAGE_SECTION_HEADER* lpSection	= (PIMAGE_SECTION_HEADER) (lpNtHead + 1);
	
	for (DWORD i=0; i < dwSec; i++)
	{
		if(!strncmp((char*)lpSection->Name, secName, IMAGE_SIZEOF_SHORT_NAME) )
			return lpSection;
		lpSection++;
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////文件偏移转换成RVA///////////////////////////////////////////////////////////

DWORD _stdcall OffsetToRVA(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset)
{
	::IMAGE_NT_HEADERS		*lpPEHead;
	::IMAGE_SECTION_HEADER	*lpSectionHead;
	DWORD i;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	i	= lpPEHead->FileHeader.NumberOfSections;
	lpSectionHead	= (IMAGE_SECTION_HEADER*)(++lpPEHead); 

	for ( ; i > 0; i--, lpSectionHead++)
	{
		if ( (dwOffset >= lpSectionHead->PointerToRawData) && (dwOffset < (lpSectionHead->PointerToRawData + lpSectionHead->SizeOfRawData) ) )
		{
			dwOffset	= dwOffset - lpSectionHead->PointerToRawData + lpSectionHead->VirtualAddress;
			return dwOffset;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////文件偏移转换成内存指针///////////////////////////////////////////////////////////

PBYTE _stdcall OffsetToPointer(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset)
{
	DWORD	RVA	= OffsetToRVA(lpFileHead, dwOffset);
	if( RVA == -1)
		return NULL;
	return	(PBYTE)(lpFileHead) + RVA;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////按指定大小对齐//////////////////////////////////////////////////////////

DWORD _stdcall Align(DWORD dwSize, DWORD dwAlignment)
{
	return (dwSize + dwAlignment - 1) /dwAlignment * dwAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////获得区块有效数据部分大小/////////////////////////////////////////////////

DWORD _stdcall GetValidSize(PBYTE lpMemory, PIMAGE_SECTION_HEADER lpSection)
{
	PBYTE	lpData;
	DWORD	dwSize=0;

	lpData	= (PBYTE)( lpMemory + lpSection->PointerToRawData + lpSection->SizeOfRawData - 1);
	while (*lpData == 0)
	{
		lpData--;
		dwSize++;
	}
	dwSize -= 8;  //减去8个字节防止是字符串或某结构的结尾
	if (dwSize > 0)
		return lpSection->SizeOfRawData - dwSize;
	else
		return lpSection->SizeOfRawData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//检测是否是PE文件
BOOL _stdcall IsPeFile(TCHAR szFileName[])
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
			{
				CloseHandle(hFile);
				return TRUE;
			}
		}
		CloseHandle(hFile);
	}
	return FALSE;
}



//获得文件数据指针
CONST PBYTE _stdcall GetFileBuffer()
{
	return g_lpBuffer;
}


//获得文件名
EXPORT CONST PTCHAR _stdcall GetFileName()
{
	return g_szFileName;
}


//DLL卸载时清理
void _stdcall DllClean()
{
	if (g_lpBuffer)
		VirtualFree(g_lpBuffer, g_dwFileSize, MEM_RELEASE);
}


//重置DLL，然后重新初始化
BOOL _stdcall Reset(TCHAR szFileName[])
{
	if (g_lpBuffer)
		VirtualFree(g_lpBuffer, g_dwFileSize, MEM_RELEASE);
	memset(g_szFileName, 0, sizeof(g_szFileName) ) ;
	g_lpBuffer	 = NULL;	
	g_dwFileSize = 0;		
	g_lpNewFile  = NULL;	
	PE	= FALSE;  
	return InitPE32(szFileName);
}


//初始化，将文件数据copy到分配的缓冲区中
BOOL _stdcall InitPE32(TCHAR szFileName[])
{
	DWORD	dwFileSize;	//文件大小
	PBYTE	lpMemory;	//内存映射指针
	HANDLE	hFile, hMap;
	if (!IsPeFile(szFileName) )
		return FALSE;
	
	if (INVALID_HANDLE_VALUE != ( hFile = CreateFile (szFileName, GENERIC_READ , FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		dwFileSize	= GetFileSize (hFile, NULL);
		if (dwFileSize)
		{
			hMap	= CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (hMap)
			{
				lpMemory	= (BYTE *)MapViewOfFile (hMap, FILE_MAP_READ, 0, 0, 0);
				if (lpMemory)
				{
					lstrcpy(g_szFileName, szFileName);
					g_lpBuffer	= (PBYTE)VirtualAlloc(NULL, dwFileSize, MEM_COMMIT, PAGE_READWRITE);
					if (g_lpBuffer)
					{
						PE	= TRUE;
						g_dwFileSize	= dwFileSize;
						memcpy(g_lpBuffer, lpMemory, dwFileSize);
						UnmapViewOfFile(lpMemory);
						CloseHandle(hMap);
						CloseHandle(hFile);
						return TRUE;
					}
				}
			}
		}
	}

	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);
	return FALSE;
}


//计算CRC32值，DLL内部函数
//参数---ptr:数据指针	dwSize:数据大小
static DWORD _stdcall CalCRC32(BYTE* ptr,DWORD dwSize)
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
	while(dwSize--)
	{
		crcTmp2 = ((crcTmp2>>8) & 0x00FFFFFF) ^ crcTable[ (crcTmp2^(*ptr)) & 0xFF ];
		ptr++;
	}
		
	return (crcTmp2^0xFFFFFFFF);
}


//获得指定文件的CRC32值，错误返回0
DWORD _stdcall GetCRC32(TCHAR szFileName[] )
{
	if (!g_lpBuffer)
		return FALSE;
	//如果需要的是本文件的CRC32则直接调用
	if (!lstrcmp(szFileName, g_szFileName) )
		return CalCRC32(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew, g_dwFileSize);
	else
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
		if (!pBuffer)	return FALSE;
		ReadFile(hFile,pBuffer, fileSize, &NumberOfBytesRW, NULL);//读取文件内容
		CloseHandle(hFile);  //关闭文件

		pDosHeader	= (PIMAGE_DOS_HEADER)pBuffer;
		fileSize	= fileSize - pDosHeader->e_lfanew;
		CRC32	= CalCRC32(pBuffer + pDosHeader->e_lfanew, fileSize);
		delete	pBuffer;
		return CRC32;
	}
}



//添加代码到最后一个区块,DLL内部函数
//参数---lpCOdeStart:指向代码起始处	dwCodeSize:添加代码的大小
static BOOL _stdcall AddToLastSection(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//最终文件大小
	DWORD					dwFileAlignSize;	//原文件对齐后大小
	DWORD					dwLastSectionAlignSize; //最后区段内存对齐后大小
	DWORD					dwFileAlign;		//文件对齐粒度
	DWORD					dwSectionAlign;		//内存对齐粒度
	//PBYTE					g_lpNewFile;		//最终文件缓存
	DWORD					dwSectionNum;

	lpNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	
	dwSectionNum = lpNtHeaders->FileHeader.NumberOfSections ;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwFileAlignSize	= Align(g_dwFileSize, dwFileAlign);	//求原文件对齐大小

	dwNewFileSize	= Align(dwFileAlignSize + dwCodeSize, dwFileAlign); //获得最终文件对齐后大小
	dwLastSectionAlignSize	= Align(lpLastSectionHeader->Misc.VirtualSize + dwCodeSize, dwSectionAlign); //获得内存中最后区段大小

	g_lpNewFile		= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if ( !g_lpNewFile )  //分配内存失败
		return FALSE;

	//复制原文件数据
	memset(g_lpNewFile, 0, dwNewFileSize);
	memcpy(g_lpNewFile, g_lpBuffer, g_dwFileSize);

	//复制指定代码
	try {
		memcpy(g_lpNewFile + dwFileAlignSize, lpCodeStart, dwCodeSize); 
	}
	catch(...)
	{
		return FALSE;
	}

	//修正PE头数据
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//指向新入口处
	DWORD					OldEntry;
	
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpNewFile + ((PIMAGE_DOS_HEADER)g_lpNewFile)->e_lfanew );
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum - 1;

 	
	//给最后区段添加读写执行属性
	lpNewLastSection->Characteristics  |= 0xC0000020;
	
	//修正最后一个区段的偏移量
	lpNewLastSection->SizeOfRawData		= dwNewFileSize - lpNewLastSection->PointerToRawData;  
	lpNewLastSection->Misc.VirtualSize	= Align( GetValidSize(g_lpNewFile, lpNewLastSection), dwSectionAlign);//Align(lpNewLastSection->Misc.VirtualSize + dwCodeSize, dwSectionAlign) ;
	
	//修正镜像大小
	lpNewNtHeaders->OptionalHeader.SizeOfImage	= Align(lpNewLastSection->VirtualAddress + lpNewLastSection->Misc.VirtualSize, dwSectionAlign);

	//修正入口地址
	OldEntry	= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= OffsetToRVA( (IMAGE_DOS_HEADER *)g_lpNewFile, dwFileAlignSize) ;
	
	//修正补丁代码跳回OEP的参数
	lpNewEntry			= (DWORD*)(g_lpNewFile + dwFileAlignSize + dwCodeSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwCodeSize - 1); 

	//补丁完毕，写回文件
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szNewFile, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
		g_lpNewFile	= NULL;
		return FALSE;
	}

	WriteFile(hNewFile, g_lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//释放内存
	VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
	g_lpNewFile	= NULL;
	return TRUE;
}


//添加代码到一个新区块,DLL内部函数
//参数---lpCOdeStart:指向代码起始处	dwCodeSize:添加代码的大小
static BOOL _stdcall AddToNewSection(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//最终文件大小
	DWORD					dwPatchSectionSize; //内存中补丁区块的大小
	DWORD					dwPatchFileSize;	//补丁区段文件中大小
	DWORD					dwFileAlign;		//文件对齐粒度
	DWORD					dwSectionAlign;		//内存对齐粒度
	//PBYTE					g_lpNewFile;			//最终文件缓存
	DWORD					dwSectionNum;
	DWORD					dwNewHeaderSize;	//新头部大小
	DWORD					dwOldHeaderSize;	//老头部大小
	DWORD					dwSectionSize;		//内存中所有区块总大小
	BOOL					bChange = FALSE;	//指示新节表的加入是否影响到文件头大小，如有影响，则需修正各区段偏移

	lpNtHeaders			= (PIMAGE_NT_HEADERS)(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew);
	lpSectionHeader		= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum		= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	dwFileAlign			= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign		= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwOldHeaderSize		= lpSectionHeader->PointerToRawData;

	dwSectionSize		= Align(lpLastSectionHeader->VirtualAddress + lpLastSectionHeader->Misc.VirtualSize - Align(dwOldHeaderSize, dwSectionAlign), dwSectionAlign);  //内存中区段总大小

	//获得补丁相关数据
	dwPatchSectionSize	= Align(dwCodeSize, dwSectionAlign);	//内存中新区段对齐大小
	dwPatchFileSize		= Align(dwCodeSize, dwFileAlign);		//文件中新区段对齐大小
	
	//头部是否能增加一个节表
	DWORD	dwValidSize;	//头部当前有效大小
	dwValidSize			= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew ;
	if ( Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign) > 0x1000 )  
		return FALSE;

	//是否要增加头部大小
	dwNewHeaderSize		= Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign); //新头部大小
	if (dwNewHeaderSize > Align(dwValidSize, dwFileAlign) )
		bChange	= TRUE;   //记录头部增加，后面需要修改所有节表偏移

	dwNewFileSize		= Align (Align(g_dwFileSize, dwFileAlign) + dwPatchFileSize, dwFileAlign);
	g_lpNewFile			= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);

	if ( !g_lpNewFile )  //分配内存失败
		return FALSE;

	//复制原文件数据
	memset(g_lpNewFile, 0, dwNewFileSize);
	memcpy(g_lpNewFile, g_lpBuffer, dwValidSize);	//头部数据复制

	//区段数据复制
	DWORD	dwSize		= lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize; //文件中所有区段总大小
	memcpy(g_lpNewFile + dwNewHeaderSize, g_lpBuffer + lpSectionHeader->PointerToRawData, dwSize);


	//补丁数据复制
	try {
		memcpy(g_lpNewFile + dwNewHeaderSize + dwSize, lpCodeStart, dwCodeSize);
	}
	catch(...)
	{
		return FALSE;
	}


	//开始修正PE头
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//指向新入口处
	DWORD					OldEntry;

	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpNewFile + ((PIMAGE_DOS_HEADER)g_lpNewFile)->e_lfanew);
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER) (lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum; //此处即指向要创建的新区段
	
	lpNewNtHeaders->FileHeader.NumberOfSections	   += 1; //添加一个区段
	lpNewNtHeaders->OptionalHeader.SizeOfHeaders	= dwNewHeaderSize;
	lpNewNtHeaders->OptionalHeader.SizeOfImage		= dwNewHeaderSize + dwSectionSize + dwPatchSectionSize; //镜像总大小
	
	//根据需要修正节表偏移
	if (bChange)
	{
		PIMAGE_SECTION_HEADER	lpTempSection = lpNewSectionHeader;
		DWORD	dwOffset	= dwNewHeaderSize - dwOldHeaderSize;
		for (DWORD i=0; i < dwSectionNum; i++, lpTempSection++ )
		{
			lpTempSection->PointerToRawData += dwOffset;
		}
	}

	//建立一个新节表
	lpNewLastSection->Characteristics	= 0xC0000020; //读写执行属性添加
	lpNewLastSection->VirtualAddress	= Align(dwNewHeaderSize, dwSectionAlign) + dwSectionSize;
	lpNewLastSection->Misc.VirtualSize	= dwPatchSectionSize;
	lpNewLastSection->PointerToRawData	= dwNewHeaderSize + dwSize;
	lpNewLastSection->SizeOfRawData		= dwPatchFileSize;
	lstrcpyA( (LPSTR)(lpNewLastSection->Name), ".crk");


	//修正入口地址
	OldEntry		= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= Align(dwNewHeaderSize, dwSectionAlign) + dwSectionSize ;
	
	//修正补丁代码跳回OEP的参数
	lpNewEntry			= (DWORD*)(g_lpNewFile + dwNewHeaderSize + dwSize + dwCodeSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwCodeSize - 1); 

	//补丁完毕，写回文件
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szNewFile, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
		g_lpNewFile	= NULL;
		return FALSE;
	}

	WriteFile(hNewFile, g_lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//释放内存
	VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
	g_lpNewFile	= NULL;
	return TRUE;
}


//添加代码到PE头部，DLL内部函数
//参数---lpCOdeStart:指向代码起始处	dwCodeSize:添加代码的大小
static BOOL _stdcall AddToHeaderSection(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//最终文件大小
	DWORD					dwFileAlign;		//文件对齐粒度
	DWORD					dwSectionAlign;		//内存对齐粒度
	//PBYTE					g_lpNewFile;			//最终文件缓存
	DWORD					dwSectionNum;
	DWORD					dwPatchOffset;		//补丁代码复制到文件的偏移，也即老头部有效数据的大小
	DWORD					dwNewHeaderSize;	//新头部大小
	DWORD					dwOldHeaderSize;	//老头部大小
	DWORD					dwSectionSize;		//所有区块总大小


	lpNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum	= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;      //获得最后一个区块的节表
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwOldHeaderSize	= lpSectionHeader->PointerToRawData;					
	
	//dwPatchOffset	= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew;  //老PE头有效数据大小，附加代码从此放置
	//原本是使用上面这种方式求有效大小，但考虑到如果PE已有补丁情况下的兼容，所有直接从头最后面往前搜索以取得大小
	
	PBYTE					lpTemp = g_lpBuffer + lpSectionHeader->PointerToRawData - 1;
	DWORD					dwSize = 0;
	while(*lpTemp == 0)  { lpTemp--; dwSize++; }	//获得可填充数据的大小
	dwSize		   -= sizeof(IMAGE_SECTION_HEADER);	//因为可能有个全零的节表，所以减去 
	dwPatchOffset	= lpSectionHeader->PointerToRawData - dwSize ; //老PE头有效数据大小，附加代码从此放置


	dwNewHeaderSize	= Align(dwPatchOffset + dwCodeSize, dwFileAlign);  //获得新头部对齐后大小
	if (dwNewHeaderSize > 0x1000)
	{
		MessageBox (GetActiveWindow() , TEXT("PE头大小不够，请选择其他补丁方式"), NULL, MB_OK);
		return FALSE;
	}

	//最后区段偏移加上大小再减去老PE头大小就等于原文件所有区段的文件大小，再加上新头部大小并对齐就是我们需要的新文件大小
	dwSectionSize	= Align(lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize, dwFileAlign);
	dwNewFileSize	= Align(dwNewHeaderSize + dwSectionSize, dwFileAlign);  

	g_lpNewFile		= (PBYTE)VirtualAlloc(NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if (!g_lpNewFile) //分配内存失败
		return FALSE;

	//复制原文件数据
	memset(g_lpNewFile, 0, dwNewFileSize);
	memcpy(g_lpNewFile, g_lpBuffer, dwPatchOffset);		//复制原始PE头
	memcpy(g_lpNewFile + dwNewHeaderSize, g_lpBuffer + dwOldHeaderSize, dwSectionSize);		//复制区块数据 


	//复制补丁代码
	memcpy(g_lpNewFile + dwPatchOffset, lpCodeStart, dwCodeSize);

	//开始对头部数据进行修复
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)(g_lpNewFile + ((PIMAGE_DOS_HEADER)g_lpNewFile)->e_lfanew);
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNewNtHeaders + 1);

	//由于文件头部大小增加，节表所有的文件偏移都需要修复，RVA则不变
	DWORD	dwOffset;
	PIMAGE_SECTION_HEADER	lpTempSectionHeader = lpNewSectionHeader;
	dwOffset	= dwNewHeaderSize - dwOldHeaderSize; //计算出需要加上的文件偏移
	for (DWORD i=0; i < dwSectionNum; i++, lpTempSectionHeader++)
	{
		lpTempSectionHeader->PointerToRawData += dwOffset;
	}


	//修正头部大小
	lpNewNtHeaders->OptionalHeader.SizeOfHeaders	= dwNewHeaderSize;

	//修正入口
	DWORD	dwOldEntry;
	dwOldEntry	= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= dwPatchOffset;

	//修正补丁代码的最后一个跳转参数
	DWORD	*lpNewEntry;
	lpNewEntry	= (DWORD*)(g_lpNewFile + dwPatchOffset + dwCodeSize - 5);   //指向要修正的参数
	*lpNewEntry	= dwOldEntry - (dwPatchOffset + dwCodeSize - 1); 

	//补丁完毕，写回文件
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szNewFile, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{	
		VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
		g_lpNewFile	= NULL;
		return FALSE;
	}

	WriteFile(hNewFile, g_lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//释放内存
	VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
	g_lpNewFile	= NULL;
	return TRUE;
}


BOOL _stdcall AddCode(TCHAR szNewFileName[], PBYTE lpCodeStart, DWORD dwCodeSize, DWORD dwTypeOfAdd)
{
	if ( (!g_lpBuffer) && !PE )
		return FALSE;
	try
	{
		//使用指定方法添加代码
		switch(dwTypeOfAdd)
		{
			case ADD_LAST_SECTION:
				if (!AddToLastSection(szNewFileName, lpCodeStart, dwCodeSize) )
					return FALSE;
				break;
	
			case ADD_NEW_SECTION:
				if (!AddToNewSection(szNewFileName, lpCodeStart, dwCodeSize) )
					return FALSE;
				break;
	
			case ADD_PE_HEADER:
				if (!AddToHeaderSection(szNewFileName, lpCodeStart, dwCodeSize) )
					return FALSE;
				break;
	
			default:
				return FALSE;
		}
	}
	catch(...)
	{
		return FALSE;
	}

	return TRUE;
}




//////////////////////////////////////////////////提取图标数据///////////////////////////////////////////////////////

HANDLE	hIconFile;

typedef struct _ICON
{
	BYTE	bWidth;
	BYTE	bHeight;
	BYTE	bColorCount;
	BYTE	bReserved;
	WORD	wPlanes;
	WORD	wBitCount;
	DWORD	dwBytesInRes;
	DWORD	dwImageOffset;
}ICON_DIR_ENTRY;

typedef struct _ION
{
	BYTE	bWidth;
	BYTE	bHeight;
	BYTE	bColorCount;
	BYTE	bReserved;
	WORD	wPlanes;
	WORD	wBitCount;
	DWORD	dwBytesInRes;
	WORD	Order;
}PE_ICON_DIR_ENTRY;

typedef struct ICON
{
	WORD	idReserved;
	WORD	idType;
	WORD	idCount;  
}ICON_DIR;

void _stdcall WriteIconData(TCHAR szIconFileName[],PIMAGE_RESOURCE_DIRECTORY lpResource,DWORD dwOrder)
{
	//存储三层目录的参数
	PIMAGE_RESOURCE_DIRECTORY	lpResDir1;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry1;
	DWORD	dwResNum1;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir2;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry2;
	DWORD	dwResNum2;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir3;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry3;

	PIMAGE_RESOURCE_DATA_ENTRY	lpDataEntry;
	BYTE	*lpResourceData;//指向真正的资源数据
	DWORD	dwResSize, temp;
	DWORD	dwNum=0;

	lpResDir1	=	lpResource;
	dwResNum1	=	lpResDir1->NumberOfIdEntries + lpResDir1->NumberOfNamedEntries;
	lpResEntry1	=	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir1);
	

	//在目录中寻找图标组数据
	for (DWORD i=0; i < dwResNum1; i++)
	{
			//如果是图标资源
			if ( lpResEntry1->Id == 0x3)
			{
				//第二层,下面每次循环都是一个图标组并产生一个图标文件
				lpResDir2	= (PIMAGE_RESOURCE_DIRECTORY)( (DWORD)lpResource + lpResEntry1->OffsetToDirectory);
				dwResNum2	= lpResDir2->NumberOfIdEntries + lpResDir2->NumberOfNamedEntries;
				lpResEntry2	= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir2);
				
				for (DWORD j=0; j < dwResNum2; j++)
				{
					dwNum++; //图标ID
					if (dwNum == dwOrder)   // 是要寻找的那个图标ID
					{
					//第三层,假设图标组只有一个项目
						lpResDir3		= (PIMAGE_RESOURCE_DIRECTORY)( (DWORD)lpResource + lpResEntry2->OffsetToDirectory);
						lpResEntry3		= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir3);
						lpDataEntry		= (PIMAGE_RESOURCE_DATA_ENTRY)( (DWORD)lpResource + lpResEntry3->OffsetToData);
						dwResSize		= lpDataEntry->Size;
						lpResourceData	= g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataEntry->OffsetToData);
					
						//开始写图标数据
						WriteFile(hIconFile, lpResourceData, dwResSize, &temp, NULL);
					}

					lpResEntry2++;
				}
				
			}

		lpResEntry1++;
	}
}

void _stdcall GetIconHeader(TCHAR szIconFileName[],BYTE *lpResourceData,DWORD dwSize, DWORD dwNum)
{
	BYTE	*lpData;
	DWORD	temp;
	DWORD	dwIconNum;	//图标数
	ICON_DIR_ENTRY *lpIconEntry;
	DWORD	dwHeadSize;

	if ( INVALID_HANDLE_VALUE == (hIconFile = CreateFile( szIconFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,0)) )
	{
		return ;
	}
	
	//写入图标文件头
	DWORD	prev=0;
	ICON_DIR_ENTRY icon;
	lpData		= lpResourceData;
	dwIconNum	= ((ICON_DIR*)lpData)->idCount; 
	WriteFile(hIconFile, lpData, 6, &temp, NULL);
	lpData += 6;
	lpIconEntry = (ICON_DIR_ENTRY*)lpData;
	for (DWORD i=0; i < dwIconNum; i++)
	{
		memcpy(&icon, lpIconEntry, sizeof(ICON_DIR_ENTRY) );
		icon.dwImageOffset	= prev + dwIconNum * sizeof(ICON_DIR_ENTRY) + 6;
		WriteFile(hIconFile, &icon, sizeof(ICON_DIR_ENTRY), &temp, NULL);
		prev	+= icon.dwBytesInRes;
		lpIconEntry = (ICON_DIR_ENTRY*)((DWORD)lpIconEntry + sizeof(PE_ICON_DIR_ENTRY) );
	}

	PE_ICON_DIR_ENTRY	*lpPeIcon;
	dwHeadSize	= (BYTE*)lpIconEntry - lpResourceData;

	lpPeIcon	= (PE_ICON_DIR_ENTRY*)(lpResourceData + 6);
	PIMAGE_DATA_DIRECTORY		lpDataDir = (PIMAGE_DATA_DIRECTORY)( &((PIMAGE_NT_HEADERS)(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
	PIMAGE_RESOURCE_DIRECTORY	lpResource= (PIMAGE_RESOURCE_DIRECTORY)(g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataDir->VirtualAddress) );
	//写入图标数据
	for (DWORD i=0; i < dwIconNum; i++)
	{
		WriteIconData(szIconFileName, lpResource, lpPeIcon->Order);
		lpPeIcon++;
	}

	CloseHandle(hIconFile);
}

//提取图标数据,参数指定图标名
BOOL _stdcall GetIcon (TCHAR	szIconFileName[])
{
	if ( (!g_lpBuffer) && !PE )
		return FALSE;
	PIMAGE_DATA_DIRECTORY		lpDataDir;
	PIMAGE_NT_HEADERS			lpPeHead = (PIMAGE_NT_HEADERS)(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew);
	BYTE						*lpResource;

	//存储三层目录的参数
	PIMAGE_RESOURCE_DIRECTORY	lpResDir1;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry1;
	DWORD	dwResNum1;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir2;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry2;
	DWORD	dwResNum2;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir3;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry3;

	PIMAGE_RESOURCE_DATA_ENTRY	lpDataEntry;
	BYTE	*lpResourceData;//指向真正的资源数据
	DWORD	dwResSize;
	DWORD	dwNum=0;//图标组编号

	lpDataDir	=	(PIMAGE_DATA_DIRECTORY)( &lpPeHead->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
	lpResource	=   ( g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataDir->VirtualAddress) );
	lpResDir1	=	(PIMAGE_RESOURCE_DIRECTORY)lpResource;
	dwResNum1	=	lpResDir1->NumberOfIdEntries + lpResDir1->NumberOfNamedEntries;
	lpResEntry1	=	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir1);
	

	//在目录中寻找图标组数据
	for (DWORD i=0; i < dwResNum1; i++)
	{
		//如果是ID资源
		if ( !lpResEntry1->NameIsString )
		{
			//如果是图标组资源
			if ( lpResEntry1->Id == 0xe)
			{
				//第二层,下面每次循环都是一个图标组并产生一个图标文件
				lpResDir2	= (PIMAGE_RESOURCE_DIRECTORY)(lpResource + lpResEntry1->OffsetToDirectory);
				dwResNum2	= lpResDir2->NumberOfIdEntries + lpResDir2->NumberOfNamedEntries;
				lpResEntry2	= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir2);
				for (DWORD j=0; j < dwResNum2; j++)
				{
					//第三层,假设图标组只有一个项目
					lpResDir3		= (PIMAGE_RESOURCE_DIRECTORY)(lpResource + lpResEntry2->OffsetToDirectory);
					lpResEntry3		= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir3);
					lpDataEntry		= (PIMAGE_RESOURCE_DATA_ENTRY)(lpResource + lpResEntry3->OffsetToData);
					dwResSize		= lpDataEntry->Size;
					lpResourceData	= g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataEntry->OffsetToData);
					
					//开始处理图标数据
					GetIconHeader(szIconFileName,lpResourceData, dwResSize,++dwNum);
				
					lpResEntry2++;
				}
			}
		}

		lpResEntry1++;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////