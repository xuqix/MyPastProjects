#ifndef RVATOOFFSET_H
#define RVATOOFFSET_H

#include <windows.h>

char szNotFound[] = "�޷�����";

///////////////////////////////////////��RVAƫ��ת�����ļ�ƫ��,ʧ�ܷ���-1////////////////////////////////////////////////

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



////////////////////////////////////��RVAƫ��ת���ļ�ָ��ƫ��,ʧ�ܷ���-1////////////////////////////////////////////////

DWORD RvaToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwRva)
{
	DWORD	Offset	= RvaToOffset(lpFileHead, dwRva);
	if(Offset == -1)
		return -1;
	return	(DWORD)(lpFileHead) + Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////�������ַת���ļ�ָ��ƫ��,ʧ�ܷ���-1////////////////////////////////////////////////

DWORD VirtualAddressToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwVirtualAddress)
{
	::IMAGE_NT_HEADERS		*lpPEHead;
	lpPEHead		= (IMAGE_NT_HEADERS*)( (BYTE*)lpFileHead + lpFileHead->e_lfanew);
	
	return RvaToPointer(lpFileHead, dwVirtualAddress - lpPEHead->OptionalHeader.ImageBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////���RVAƫ�ƴ��Ľ�������////////////////////////////////////////////////

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



///////////////////////////////////////���ָ��RVA���������Ľڱ�ͷ,ʧ�ܷ���NULL/////////////////////////////////////////

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



////////////////////////////////////////////�ļ�ƫ��ת����RVA///////////////////////////////////////////////////////////

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



////////////////////////////////////////////�ļ�ƫ��ת�����ڴ�ָ��///////////////////////////////////////////////////////////

DWORD OffsetToPointer(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset)
{
	DWORD	RVA	= OffsetToRVA(lpFileHead, dwOffset);
	if( RVA == -1)
		return -1;
	return	(DWORD)(lpFileHead) + RVA;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////��ָ����С����//////////////////////////////////////////////////////////

DWORD Align(DWORD dwSize, DWORD dwAlignment)
{
	return (dwSize + dwAlignment - 1) /dwAlignment * dwAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////���������Ч���ݲ��ִ�С/////////////////////////////////////////////////

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
	dwSize -= 8;  //��ȥ8���ֽڷ�ֹ���ַ�����ĳ�ṹ�Ľ�β
	if (dwSize > 0)
		return lpSection->SizeOfRawData - dwSize;
	else
		return lpSection->SizeOfRawData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif