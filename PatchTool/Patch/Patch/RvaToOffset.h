#ifndef RVATOOFFSET_H
#define RVATOOFFSET_H

#include <windows.h>

char szNotFound[] = "无法查找";

///////////////////////////////////////将RVA偏移转换成文件偏移,失败返回-1////////////////////////////////////////////////

DWORD RvaToOffset (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva)
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



////////////////////////////////////将RVA偏移转成文件指针偏移,失败返回-1////////////////////////////////////////////////

DWORD RvaToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwRva)
{
	DWORD	Offset	= RvaToOffset(lpFileHead, dwRva);
	if(Offset == -1)
		return -1;
	return	(DWORD)(lpFileHead) + Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////将虚拟地址转成文件指针偏移,失败返回-1////////////////////////////////////////////////

DWORD VirtualAddressToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwVirtualAddress)
{
	::IMAGE_NT_HEADERS		*lpPEHead;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	
	return RvaToPointer(lpFileHead, dwVirtualAddress - lpPEHead->OptionalHeader.ImageBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////获得RVA偏移处的节区名称////////////////////////////////////////////////

PBYTE GetRvaSection (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva)
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
	return (PBYTE)szNotFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////获得指定RVA所处节区的节表头,失败返回NULL/////////////////////////////////////////

PIMAGE_SECTION_HEADER GetSectionOfRva (IMAGE_DOS_HEADER *lpFileHead, char* secName)
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

DWORD OffsetToRVA(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset)
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

DWORD OffsetToPointer(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset)
{
	DWORD	RVA	= OffsetToRVA(lpFileHead, dwOffset);
	if( RVA == -1)
		return -1;
	return	(DWORD)(lpFileHead) + RVA;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////按指定大小对齐//////////////////////////////////////////////////////////

DWORD Align(DWORD dwSize, DWORD dwAlignment)
{
	return (dwSize + dwAlignment - 1) /dwAlignment * dwAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////获得区块有效数据部分大小/////////////////////////////////////////////////

DWORD GetValidSize(PBYTE lpMemory, PIMAGE_SECTION_HEADER lpSection)
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

#endif