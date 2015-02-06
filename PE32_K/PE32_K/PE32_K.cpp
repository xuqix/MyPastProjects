#include <windows.h>
#include "PE32_K.H"


TCHAR		g_szFileName[MAX_PATH] = { 0 };
HINSTANCE	g_hInstDll	= NULL;		//DLLģ����
PBYTE		g_lpBuffer	= NULL;		//�洢�ļ�����
DWORD		g_dwFileSize= 0;		//�ļ����ݴ�С
PBYTE		g_lpNewFile = NULL;		//�洢��Ӵ���������
BOOL		PE	= FALSE;  //ָʾ�Ƿ���PE�ļ�


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



///////////////////////////////////////��RVAƫ��ת�����ļ�ƫ��,ʧ�ܷ���-1////////////////////////////////////////////////

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



////////////////////////////////////��RVAƫ��ת���ļ�ָ��ƫ��,ʧ�ܷ���NULL////////////////////////////////////////////////

PBYTE _stdcall RvaToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwRva)
{
	DWORD	Offset	= RvaToOffset(lpFileHead, dwRva);
	if(Offset == -1)
		return NULL;
	return	(PBYTE)(lpFileHead) + Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////�������ַת���ļ�ָ��ƫ��,ʧ�ܷ���NULL////////////////////////////////////////////////

PBYTE _stdcall VirtualAddressToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwVirtualAddress)
{
	::IMAGE_NT_HEADERS		*lpPEHead;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	
	return (PBYTE)RvaToPointer(lpFileHead, dwVirtualAddress - lpPEHead->OptionalHeader.ImageBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////���RVAƫ�ƴ��Ľ�������////////////////////////////////////////////////

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



///////////////////////////////////////���ָ��RVA���������Ľڱ�ͷ,ʧ�ܷ���NULL/////////////////////////////////////////

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



////////////////////////////////////////////�ļ�ƫ��ת����RVA///////////////////////////////////////////////////////////

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



////////////////////////////////////////////�ļ�ƫ��ת�����ڴ�ָ��///////////////////////////////////////////////////////////

PBYTE _stdcall OffsetToPointer(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset)
{
	DWORD	RVA	= OffsetToRVA(lpFileHead, dwOffset);
	if( RVA == -1)
		return NULL;
	return	(PBYTE)(lpFileHead) + RVA;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////��ָ����С����//////////////////////////////////////////////////////////

DWORD _stdcall Align(DWORD dwSize, DWORD dwAlignment)
{
	return (dwSize + dwAlignment - 1) /dwAlignment * dwAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////���������Ч���ݲ��ִ�С/////////////////////////////////////////////////

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
	dwSize -= 8;  //��ȥ8���ֽڷ�ֹ���ַ�����ĳ�ṹ�Ľ�β
	if (dwSize > 0)
		return lpSection->SizeOfRawData - dwSize;
	else
		return lpSection->SizeOfRawData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//����Ƿ���PE�ļ�
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



//����ļ�����ָ��
CONST PBYTE _stdcall GetFileBuffer()
{
	return g_lpBuffer;
}


//����ļ���
EXPORT CONST PTCHAR _stdcall GetFileName()
{
	return g_szFileName;
}


//DLLж��ʱ����
void _stdcall DllClean()
{
	if (g_lpBuffer)
		VirtualFree(g_lpBuffer, g_dwFileSize, MEM_RELEASE);
}


//����DLL��Ȼ�����³�ʼ��
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


//��ʼ�������ļ�����copy������Ļ�������
BOOL _stdcall InitPE32(TCHAR szFileName[])
{
	DWORD	dwFileSize;	//�ļ���С
	PBYTE	lpMemory;	//�ڴ�ӳ��ָ��
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


//����CRC32ֵ��DLL�ڲ�����
//����---ptr:����ָ��	dwSize:���ݴ�С
static DWORD _stdcall CalCRC32(BYTE* ptr,DWORD dwSize)
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
	while(dwSize--)
	{
		crcTmp2 = ((crcTmp2>>8) & 0x00FFFFFF) ^ crcTable[ (crcTmp2^(*ptr)) & 0xFF ];
		ptr++;
	}
		
	return (crcTmp2^0xFFFFFFFF);
}


//���ָ���ļ���CRC32ֵ�����󷵻�0
DWORD _stdcall GetCRC32(TCHAR szFileName[] )
{
	if (!g_lpBuffer)
		return FALSE;
	//�����Ҫ���Ǳ��ļ���CRC32��ֱ�ӵ���
	if (!lstrcmp(szFileName, g_szFileName) )
		return CalCRC32(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew, g_dwFileSize);
	else
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
		if (!pBuffer)	return FALSE;
		ReadFile(hFile,pBuffer, fileSize, &NumberOfBytesRW, NULL);//��ȡ�ļ�����
		CloseHandle(hFile);  //�ر��ļ�

		pDosHeader	= (PIMAGE_DOS_HEADER)pBuffer;
		fileSize	= fileSize - pDosHeader->e_lfanew;
		CRC32	= CalCRC32(pBuffer + pDosHeader->e_lfanew, fileSize);
		delete	pBuffer;
		return CRC32;
	}
}



//��Ӵ��뵽���һ������,DLL�ڲ�����
//����---lpCOdeStart:ָ�������ʼ��	dwCodeSize:��Ӵ���Ĵ�С
static BOOL _stdcall AddToLastSection(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//�����ļ���С
	DWORD					dwFileAlignSize;	//ԭ�ļ�������С
	DWORD					dwLastSectionAlignSize; //��������ڴ������С
	DWORD					dwFileAlign;		//�ļ���������
	DWORD					dwSectionAlign;		//�ڴ��������
	//PBYTE					g_lpNewFile;		//�����ļ�����
	DWORD					dwSectionNum;

	lpNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	
	dwSectionNum = lpNtHeaders->FileHeader.NumberOfSections ;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwFileAlignSize	= Align(g_dwFileSize, dwFileAlign);	//��ԭ�ļ������С

	dwNewFileSize	= Align(dwFileAlignSize + dwCodeSize, dwFileAlign); //��������ļ�������С
	dwLastSectionAlignSize	= Align(lpLastSectionHeader->Misc.VirtualSize + dwCodeSize, dwSectionAlign); //����ڴ���������δ�С

	g_lpNewFile		= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if ( !g_lpNewFile )  //�����ڴ�ʧ��
		return FALSE;

	//����ԭ�ļ�����
	memset(g_lpNewFile, 0, dwNewFileSize);
	memcpy(g_lpNewFile, g_lpBuffer, g_dwFileSize);

	//����ָ������
	try {
		memcpy(g_lpNewFile + dwFileAlignSize, lpCodeStart, dwCodeSize); 
	}
	catch(...)
	{
		return FALSE;
	}

	//����PEͷ����
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//ָ������ڴ�
	DWORD					OldEntry;
	
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpNewFile + ((PIMAGE_DOS_HEADER)g_lpNewFile)->e_lfanew );
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum - 1;

 	
	//�����������Ӷ�дִ������
	lpNewLastSection->Characteristics  |= 0xC0000020;
	
	//�������һ�����ε�ƫ����
	lpNewLastSection->SizeOfRawData		= dwNewFileSize - lpNewLastSection->PointerToRawData;  
	lpNewLastSection->Misc.VirtualSize	= Align( GetValidSize(g_lpNewFile, lpNewLastSection), dwSectionAlign);//Align(lpNewLastSection->Misc.VirtualSize + dwCodeSize, dwSectionAlign) ;
	
	//���������С
	lpNewNtHeaders->OptionalHeader.SizeOfImage	= Align(lpNewLastSection->VirtualAddress + lpNewLastSection->Misc.VirtualSize, dwSectionAlign);

	//������ڵ�ַ
	OldEntry	= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= OffsetToRVA( (IMAGE_DOS_HEADER *)g_lpNewFile, dwFileAlignSize) ;
	
	//����������������OEP�Ĳ���
	lpNewEntry			= (DWORD*)(g_lpNewFile + dwFileAlignSize + dwCodeSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwCodeSize - 1); 

	//������ϣ�д���ļ�
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

	//�ͷ��ڴ�
	VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
	g_lpNewFile	= NULL;
	return TRUE;
}


//��Ӵ��뵽һ��������,DLL�ڲ�����
//����---lpCOdeStart:ָ�������ʼ��	dwCodeSize:��Ӵ���Ĵ�С
static BOOL _stdcall AddToNewSection(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//�����ļ���С
	DWORD					dwPatchSectionSize; //�ڴ��в�������Ĵ�С
	DWORD					dwPatchFileSize;	//���������ļ��д�С
	DWORD					dwFileAlign;		//�ļ���������
	DWORD					dwSectionAlign;		//�ڴ��������
	//PBYTE					g_lpNewFile;			//�����ļ�����
	DWORD					dwSectionNum;
	DWORD					dwNewHeaderSize;	//��ͷ����С
	DWORD					dwOldHeaderSize;	//��ͷ����С
	DWORD					dwSectionSize;		//�ڴ������������ܴ�С
	BOOL					bChange = FALSE;	//ָʾ�½ڱ�ļ����Ƿ�Ӱ�쵽�ļ�ͷ��С������Ӱ�죬��������������ƫ��

	lpNtHeaders			= (PIMAGE_NT_HEADERS)(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew);
	lpSectionHeader		= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum		= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	dwFileAlign			= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign		= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwOldHeaderSize		= lpSectionHeader->PointerToRawData;

	dwSectionSize		= Align(lpLastSectionHeader->VirtualAddress + lpLastSectionHeader->Misc.VirtualSize - Align(dwOldHeaderSize, dwSectionAlign), dwSectionAlign);  //�ڴ��������ܴ�С

	//��ò����������
	dwPatchSectionSize	= Align(dwCodeSize, dwSectionAlign);	//�ڴ��������ζ����С
	dwPatchFileSize		= Align(dwCodeSize, dwFileAlign);		//�ļ��������ζ����С
	
	//ͷ���Ƿ�������һ���ڱ�
	DWORD	dwValidSize;	//ͷ����ǰ��Ч��С
	dwValidSize			= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew ;
	if ( Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign) > 0x1000 )  
		return FALSE;

	//�Ƿ�Ҫ����ͷ����С
	dwNewHeaderSize		= Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign); //��ͷ����С
	if (dwNewHeaderSize > Align(dwValidSize, dwFileAlign) )
		bChange	= TRUE;   //��¼ͷ�����ӣ�������Ҫ�޸����нڱ�ƫ��

	dwNewFileSize		= Align (Align(g_dwFileSize, dwFileAlign) + dwPatchFileSize, dwFileAlign);
	g_lpNewFile			= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);

	if ( !g_lpNewFile )  //�����ڴ�ʧ��
		return FALSE;

	//����ԭ�ļ�����
	memset(g_lpNewFile, 0, dwNewFileSize);
	memcpy(g_lpNewFile, g_lpBuffer, dwValidSize);	//ͷ�����ݸ���

	//�������ݸ���
	DWORD	dwSize		= lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize; //�ļ������������ܴ�С
	memcpy(g_lpNewFile + dwNewHeaderSize, g_lpBuffer + lpSectionHeader->PointerToRawData, dwSize);


	//�������ݸ���
	try {
		memcpy(g_lpNewFile + dwNewHeaderSize + dwSize, lpCodeStart, dwCodeSize);
	}
	catch(...)
	{
		return FALSE;
	}


	//��ʼ����PEͷ
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//ָ������ڴ�
	DWORD					OldEntry;

	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpNewFile + ((PIMAGE_DOS_HEADER)g_lpNewFile)->e_lfanew);
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER) (lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum; //�˴���ָ��Ҫ������������
	
	lpNewNtHeaders->FileHeader.NumberOfSections	   += 1; //���һ������
	lpNewNtHeaders->OptionalHeader.SizeOfHeaders	= dwNewHeaderSize;
	lpNewNtHeaders->OptionalHeader.SizeOfImage		= dwNewHeaderSize + dwSectionSize + dwPatchSectionSize; //�����ܴ�С
	
	//������Ҫ�����ڱ�ƫ��
	if (bChange)
	{
		PIMAGE_SECTION_HEADER	lpTempSection = lpNewSectionHeader;
		DWORD	dwOffset	= dwNewHeaderSize - dwOldHeaderSize;
		for (DWORD i=0; i < dwSectionNum; i++, lpTempSection++ )
		{
			lpTempSection->PointerToRawData += dwOffset;
		}
	}

	//����һ���½ڱ�
	lpNewLastSection->Characteristics	= 0xC0000020; //��дִ���������
	lpNewLastSection->VirtualAddress	= Align(dwNewHeaderSize, dwSectionAlign) + dwSectionSize;
	lpNewLastSection->Misc.VirtualSize	= dwPatchSectionSize;
	lpNewLastSection->PointerToRawData	= dwNewHeaderSize + dwSize;
	lpNewLastSection->SizeOfRawData		= dwPatchFileSize;
	lstrcpyA( (LPSTR)(lpNewLastSection->Name), ".crk");


	//������ڵ�ַ
	OldEntry		= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= Align(dwNewHeaderSize, dwSectionAlign) + dwSectionSize ;
	
	//����������������OEP�Ĳ���
	lpNewEntry			= (DWORD*)(g_lpNewFile + dwNewHeaderSize + dwSize + dwCodeSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwCodeSize - 1); 

	//������ϣ�д���ļ�
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

	//�ͷ��ڴ�
	VirtualFree(g_lpNewFile, dwNewFileSize, MEM_RELEASE);
	g_lpNewFile	= NULL;
	return TRUE;
}


//��Ӵ��뵽PEͷ����DLL�ڲ�����
//����---lpCOdeStart:ָ�������ʼ��	dwCodeSize:��Ӵ���Ĵ�С
static BOOL _stdcall AddToHeaderSection(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//�����ļ���С
	DWORD					dwFileAlign;		//�ļ���������
	DWORD					dwSectionAlign;		//�ڴ��������
	//PBYTE					g_lpNewFile;			//�����ļ�����
	DWORD					dwSectionNum;
	DWORD					dwPatchOffset;		//�������븴�Ƶ��ļ���ƫ�ƣ�Ҳ����ͷ����Ч���ݵĴ�С
	DWORD					dwNewHeaderSize;	//��ͷ����С
	DWORD					dwOldHeaderSize;	//��ͷ����С
	DWORD					dwSectionSize;		//���������ܴ�С


	lpNtHeaders		= (PIMAGE_NT_HEADERS)( g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum	= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;      //������һ������Ľڱ�
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwOldHeaderSize	= lpSectionHeader->PointerToRawData;					
	
	//dwPatchOffset	= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew;  //��PEͷ��Ч���ݴ�С�����Ӵ���Ӵ˷���
	//ԭ����ʹ���������ַ�ʽ����Ч��С�������ǵ����PE���в�������µļ��ݣ�����ֱ�Ӵ�ͷ�������ǰ������ȡ�ô�С
	
	PBYTE					lpTemp = g_lpBuffer + lpSectionHeader->PointerToRawData - 1;
	DWORD					dwSize = 0;
	while(*lpTemp == 0)  { lpTemp--; dwSize++; }	//��ÿ�������ݵĴ�С
	dwSize		   -= sizeof(IMAGE_SECTION_HEADER);	//��Ϊ�����и�ȫ��Ľڱ����Լ�ȥ 
	dwPatchOffset	= lpSectionHeader->PointerToRawData - dwSize ; //��PEͷ��Ч���ݴ�С�����Ӵ���Ӵ˷���


	dwNewHeaderSize	= Align(dwPatchOffset + dwCodeSize, dwFileAlign);  //�����ͷ��������С
	if (dwNewHeaderSize > 0x1000)
	{
		MessageBox (GetActiveWindow() , TEXT("PEͷ��С��������ѡ������������ʽ"), NULL, MB_OK);
		return FALSE;
	}

	//�������ƫ�Ƽ��ϴ�С�ټ�ȥ��PEͷ��С�͵���ԭ�ļ��������ε��ļ���С���ټ�����ͷ����С���������������Ҫ�����ļ���С
	dwSectionSize	= Align(lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize, dwFileAlign);
	dwNewFileSize	= Align(dwNewHeaderSize + dwSectionSize, dwFileAlign);  

	g_lpNewFile		= (PBYTE)VirtualAlloc(NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if (!g_lpNewFile) //�����ڴ�ʧ��
		return FALSE;

	//����ԭ�ļ�����
	memset(g_lpNewFile, 0, dwNewFileSize);
	memcpy(g_lpNewFile, g_lpBuffer, dwPatchOffset);		//����ԭʼPEͷ
	memcpy(g_lpNewFile + dwNewHeaderSize, g_lpBuffer + dwOldHeaderSize, dwSectionSize);		//������������ 


	//���Ʋ�������
	memcpy(g_lpNewFile + dwPatchOffset, lpCodeStart, dwCodeSize);

	//��ʼ��ͷ�����ݽ����޸�
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)(g_lpNewFile + ((PIMAGE_DOS_HEADER)g_lpNewFile)->e_lfanew);
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNewNtHeaders + 1);

	//�����ļ�ͷ����С���ӣ��ڱ����е��ļ�ƫ�ƶ���Ҫ�޸���RVA�򲻱�
	DWORD	dwOffset;
	PIMAGE_SECTION_HEADER	lpTempSectionHeader = lpNewSectionHeader;
	dwOffset	= dwNewHeaderSize - dwOldHeaderSize; //�������Ҫ���ϵ��ļ�ƫ��
	for (DWORD i=0; i < dwSectionNum; i++, lpTempSectionHeader++)
	{
		lpTempSectionHeader->PointerToRawData += dwOffset;
	}


	//����ͷ����С
	lpNewNtHeaders->OptionalHeader.SizeOfHeaders	= dwNewHeaderSize;

	//�������
	DWORD	dwOldEntry;
	dwOldEntry	= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= dwPatchOffset;

	//����������������һ����ת����
	DWORD	*lpNewEntry;
	lpNewEntry	= (DWORD*)(g_lpNewFile + dwPatchOffset + dwCodeSize - 5);   //ָ��Ҫ�����Ĳ���
	*lpNewEntry	= dwOldEntry - (dwPatchOffset + dwCodeSize - 1); 

	//������ϣ�д���ļ�
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

	//�ͷ��ڴ�
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
		//ʹ��ָ��������Ӵ���
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




//////////////////////////////////////////////////��ȡͼ������///////////////////////////////////////////////////////

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
	//�洢����Ŀ¼�Ĳ���
	PIMAGE_RESOURCE_DIRECTORY	lpResDir1;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry1;
	DWORD	dwResNum1;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir2;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry2;
	DWORD	dwResNum2;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir3;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry3;

	PIMAGE_RESOURCE_DATA_ENTRY	lpDataEntry;
	BYTE	*lpResourceData;//ָ����������Դ����
	DWORD	dwResSize, temp;
	DWORD	dwNum=0;

	lpResDir1	=	lpResource;
	dwResNum1	=	lpResDir1->NumberOfIdEntries + lpResDir1->NumberOfNamedEntries;
	lpResEntry1	=	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir1);
	

	//��Ŀ¼��Ѱ��ͼ��������
	for (DWORD i=0; i < dwResNum1; i++)
	{
			//�����ͼ����Դ
			if ( lpResEntry1->Id == 0x3)
			{
				//�ڶ���,����ÿ��ѭ������һ��ͼ���鲢����һ��ͼ���ļ�
				lpResDir2	= (PIMAGE_RESOURCE_DIRECTORY)( (DWORD)lpResource + lpResEntry1->OffsetToDirectory);
				dwResNum2	= lpResDir2->NumberOfIdEntries + lpResDir2->NumberOfNamedEntries;
				lpResEntry2	= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir2);
				
				for (DWORD j=0; j < dwResNum2; j++)
				{
					dwNum++; //ͼ��ID
					if (dwNum == dwOrder)   // ��ҪѰ�ҵ��Ǹ�ͼ��ID
					{
					//������,����ͼ����ֻ��һ����Ŀ
						lpResDir3		= (PIMAGE_RESOURCE_DIRECTORY)( (DWORD)lpResource + lpResEntry2->OffsetToDirectory);
						lpResEntry3		= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir3);
						lpDataEntry		= (PIMAGE_RESOURCE_DATA_ENTRY)( (DWORD)lpResource + lpResEntry3->OffsetToData);
						dwResSize		= lpDataEntry->Size;
						lpResourceData	= g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataEntry->OffsetToData);
					
						//��ʼдͼ������
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
	DWORD	dwIconNum;	//ͼ����
	ICON_DIR_ENTRY *lpIconEntry;
	DWORD	dwHeadSize;

	if ( INVALID_HANDLE_VALUE == (hIconFile = CreateFile( szIconFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,0)) )
	{
		return ;
	}
	
	//д��ͼ���ļ�ͷ
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
	//д��ͼ������
	for (DWORD i=0; i < dwIconNum; i++)
	{
		WriteIconData(szIconFileName, lpResource, lpPeIcon->Order);
		lpPeIcon++;
	}

	CloseHandle(hIconFile);
}

//��ȡͼ������,����ָ��ͼ����
BOOL _stdcall GetIcon (TCHAR	szIconFileName[])
{
	if ( (!g_lpBuffer) && !PE )
		return FALSE;
	PIMAGE_DATA_DIRECTORY		lpDataDir;
	PIMAGE_NT_HEADERS			lpPeHead = (PIMAGE_NT_HEADERS)(g_lpBuffer + ((PIMAGE_DOS_HEADER)g_lpBuffer)->e_lfanew);
	BYTE						*lpResource;

	//�洢����Ŀ¼�Ĳ���
	PIMAGE_RESOURCE_DIRECTORY	lpResDir1;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry1;
	DWORD	dwResNum1;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir2;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry2;
	DWORD	dwResNum2;

	PIMAGE_RESOURCE_DIRECTORY	lpResDir3;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	lpResEntry3;

	PIMAGE_RESOURCE_DATA_ENTRY	lpDataEntry;
	BYTE	*lpResourceData;//ָ����������Դ����
	DWORD	dwResSize;
	DWORD	dwNum=0;//ͼ������

	lpDataDir	=	(PIMAGE_DATA_DIRECTORY)( &lpPeHead->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
	lpResource	=   ( g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataDir->VirtualAddress) );
	lpResDir1	=	(PIMAGE_RESOURCE_DIRECTORY)lpResource;
	dwResNum1	=	lpResDir1->NumberOfIdEntries + lpResDir1->NumberOfNamedEntries;
	lpResEntry1	=	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir1);
	

	//��Ŀ¼��Ѱ��ͼ��������
	for (DWORD i=0; i < dwResNum1; i++)
	{
		//�����ID��Դ
		if ( !lpResEntry1->NameIsString )
		{
			//�����ͼ������Դ
			if ( lpResEntry1->Id == 0xe)
			{
				//�ڶ���,����ÿ��ѭ������һ��ͼ���鲢����һ��ͼ���ļ�
				lpResDir2	= (PIMAGE_RESOURCE_DIRECTORY)(lpResource + lpResEntry1->OffsetToDirectory);
				dwResNum2	= lpResDir2->NumberOfIdEntries + lpResDir2->NumberOfNamedEntries;
				lpResEntry2	= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir2);
				for (DWORD j=0; j < dwResNum2; j++)
				{
					//������,����ͼ����ֻ��һ����Ŀ
					lpResDir3		= (PIMAGE_RESOURCE_DIRECTORY)(lpResource + lpResEntry2->OffsetToDirectory);
					lpResEntry3		= (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(++lpResDir3);
					lpDataEntry		= (PIMAGE_RESOURCE_DATA_ENTRY)(lpResource + lpResEntry3->OffsetToData);
					dwResSize		= lpDataEntry->Size;
					lpResourceData	= g_lpBuffer + RvaToOffset( (IMAGE_DOS_HEADER*)g_lpBuffer, lpDataEntry->OffsetToData);
					
					//��ʼ����ͼ������
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