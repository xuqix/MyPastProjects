#ifndef ADDCODE_H
#define ADDCODE_H

#include <windows.h>
#include "RvaToOffset.h"

/////////////////////////���漸�ָ��Ӵ���Ĳ���Ӧ�����ܺܺõĻ������(��ʾ�Ҳ��Թ���һ�ļ�����20�����Ȼ����#_#)///////////////////////////////////

//��̬��������
#define ADD_LAST_SECTION 1	//��Ӵ��뵽���һ������	
#define ADD_NEW_SECTION  2  //��Ӵ��뵽һ���½�������
#define ADD_TO_HEADER	 3  //��Ӵ��뵽PEͷ��
//��������3���⻹��һ����Ѱ���Ѵ������ο��д���������룬���ֲ���PEͷ���޸ĸ��򵥣�ֻ�ǱȽ�����ʧ�ܣ�����Ͳ�ʵ����
//�����ټ�һ���ֽڲ��������һЩ�򵥵ĳ���
#define	BYTE_PATCH		 4


///////////////////���ļ��������֧��16��������ַ���ر𴴽�һ�������ڲ����������ɲ���ʱ��������//////////////////////

#pragma data_seg(".sdata")
DWORD	dwTypeOfPatch = 0 ;					//ָʾ��������
DWORD	dwPatchNum = 0;						//��������
TCHAR	szFileName[MAX_PATH] = { L"load" };

DWORD	dwPatchAddress[16] = { 0 };				//������ַ
BYTE	byOldData[16] = { 0 };					//�����������ݺ�������
BYTE	byNewData[16] = { 0 };	
#pragma data_seg()
#pragma comment(linker, "/SECTION:.sdata,ERW")

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////�Ӹ��Ӵ���������ı�������Ҫ��������ʹ�ú�����//////////////////////

extern "C" DWORD Appendcode_Start;		//���Ӵ������ʼ
extern "C" DWORD Appendcode_End;		//���Ӵ���ν���
extern "C" DWORD Patch_Data;			//��Ҫ����������ת�浽���Ӵ�����

/////////////////////////////////////////////////////////////////////////////////////////

HANDLE	hFile ;//�ļ����
HANDLE	hMap; //ӳ���ļ����

//////////////////////////////��Ӵ��뵽���һ������/////////////////////////////////////

BOOL AddToLastSection(PBYTE lpMemory, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//�����ļ���С
	DWORD					dwFileAlignSize;	//ԭ�ļ�������С
	DWORD					dwLastSectionAlignSize; //��������ڴ������С
	DWORD					dwPatchSize;		//������С
	DWORD					dwFileAlign;		//�ļ���������
	DWORD					dwSectionAlign;		//�ڴ��������
	//DWORD					dwLastSectionSize;
	//DWORD					dwPatchStart;	//ָ������Ҫ���Ƶ����ļ�ƫ����ʼ
	PBYTE					lpNewFile;		//�����ļ�����
	DWORD					dwSectionNum;

	lpNtHeaders		= (PIMAGE_NT_HEADERS)( lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	
	dwSectionNum = lpNtHeaders->FileHeader.NumberOfSections ;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwFileAlignSize	= Align(dwFileSize, dwFileAlign);	//��ԭ�ļ������С
	
	dwPatchSize		= ((DWORD)&Appendcode_End ) - ( (DWORD)&Appendcode_Start );	//��ò��������С

	dwNewFileSize	= Align(dwFileAlignSize + dwPatchSize, dwFileAlign); //��������ļ�������С
	dwLastSectionAlignSize	= Align(lpLastSectionHeader->Misc.VirtualSize + dwPatchSize, dwSectionAlign); //����ڴ���������δ�С

	lpNewFile		= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if ( !lpNewFile )  //�����ڴ�ʧ��
		return FALSE;

	//����ԭ�ļ�����
	memset(lpNewFile, 0, dwNewFileSize);
	memcpy(lpNewFile, lpMemory, dwFileSize);
	//������ϣ��ر�ӳ���ļ��;�� (���رպ�����޷��������ļ�����������AddCode�رգ�����������쳣ʲô�Ļ���û�ã�ֻ��hMap���ó�ȫ�ֱ���Ȼ��������ˣ�ʵ�ڲ��Ź۰���_��)
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	//���Ʋ�������ǰ��ת����������
	PBYTE	pBuffer		= (PBYTE)(&Patch_Data);   //ָ�򸽼Ӵ��벹�����ݴ�
	//(*(DWORD*)pBuffer)	= dwPatchNum; ԭ������䣬���Ǳ��Ż�����ͱ����mov xxx,0  ���Զ��ڱ����Ķ�д��Ҫ�ر�ע�⣬һ��С�ľ��Ż����ˣ�Ҳ��������Ч�ʣ�����ͻ��ɻ�����Ȼ��̫�ÿ�
	_asm
	{
		pushad
		mov	 eax,dwPatchNum
		mov	 ebx,pBuffer
		mov	 dword ptr [ebx], eax
		popad
	}
	memcpy(pBuffer + 4, dwPatchAddress, 16*sizeof(DWORD) );
	pBuffer	+= 4 + 16*sizeof(DWORD);
	memcpy(pBuffer, byOldData, 16);
	memcpy(pBuffer+16, byNewData, 16);

	//���Ʋ�������
	memcpy(lpNewFile + dwFileAlignSize, &Appendcode_Start, dwPatchSize); 

	//����PEͷ����
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//ָ������ڴ�
	DWORD					OldEntry;
	
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( lpNewFile + ((PIMAGE_DOS_HEADER)lpNewFile)->e_lfanew );
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum - 1;

 	
	//�����������Ӷ�дִ������
	lpNewLastSection->Characteristics  |= 0xC0000020;
	
	//�������һ�����ε�ƫ����
	lpNewLastSection->SizeOfRawData		= dwNewFileSize - lpNewLastSection->PointerToRawData;  
	lpNewLastSection->Misc.VirtualSize	= Align( GetValidSize(lpNewFile, lpNewLastSection), dwSectionAlign);//Align(lpNewLastSection->Misc.VirtualSize + dwPatchSize, dwSectionAlign) ;
	
	//���������С
	lpNewNtHeaders->OptionalHeader.SizeOfImage	= Align(lpNewLastSection->VirtualAddress + lpNewLastSection->Misc.VirtualSize, dwSectionAlign);

	//������ڵ�ַ
	OldEntry	= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= OffsetToRVA( (IMAGE_DOS_HEADER *)lpNewFile, dwFileAlignSize) ;
	
	//����������������OEP�Ĳ���
	lpNewEntry			= (DWORD*)(lpNewFile + dwFileAlignSize + dwPatchSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwPatchSize - 1); 

	//������ϣ�д���ļ�
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
		return FALSE;
	}

	WriteFile(hNewFile, lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//�ͷ��ڴ�
	VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////�½�һ�����β��������///////////////////////////////////
//����ڱ�û�ж���Ŀռ�

BOOL AddToNewSection(PBYTE lpMemory, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//�����ļ���С
	DWORD					dwPatchSize;		//������С
	DWORD					dwPatchSectionSize; //�ڴ��в�������Ĵ�С
	DWORD					dwPatchFileSize;	//���������ļ��д�С
	DWORD					dwFileAlign;		//�ļ���������
	DWORD					dwSectionAlign;		//�ڴ��������
	PBYTE					lpNewFile;			//�����ļ�����
	DWORD					dwSectionNum;
	DWORD					dwNewHeaderSize;	//��ͷ����С
	DWORD					dwOldHeaderSize;	//��ͷ����С
	DWORD					dwSectionSize;		//�ڴ������������ܴ�С
	BOOL					bChange = FALSE;	//ָʾ�½ڱ�ļ����Ƿ�Ӱ�쵽�ļ�ͷ��С������Ӱ�죬��������������ƫ��

	lpNtHeaders			= (PIMAGE_NT_HEADERS)(lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew);
	lpSectionHeader		= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum		= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	dwFileAlign			= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign		= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwOldHeaderSize		= lpSectionHeader->PointerToRawData;

	dwSectionSize		= Align(lpLastSectionHeader->VirtualAddress + lpLastSectionHeader->Misc.VirtualSize - Align(dwOldHeaderSize, dwSectionAlign), dwSectionAlign);  //�ڴ��������ܴ�С

	//��ò����������
	dwPatchSize			= ((DWORD)&Appendcode_End ) - ( (DWORD)&Appendcode_Start );	//��ò��������С
	dwPatchSectionSize	= Align(dwPatchSize, dwSectionAlign);	//�ڴ��������ζ����С
	dwPatchFileSize		= Align(dwPatchSize, dwFileAlign);		//�ļ��������ζ����С
	
	//ͷ���Ƿ�������һ���ڱ�
	DWORD	dwValidSize;	//ͷ����ǰ��Ч��С
	dwValidSize			= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew ;
	if ( Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign) > 0x1000 )  
		return FALSE;

	//�Ƿ�Ҫ����ͷ����С
	dwNewHeaderSize		= Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign); //��ͷ����С
	if (dwNewHeaderSize > Align(dwValidSize, dwFileAlign) )
		bChange	= TRUE;   //��¼ͷ�����ӣ�������Ҫ�޸����нڱ�ƫ��

	dwNewFileSize		= Align (Align(dwFileSize, dwFileAlign) + dwPatchFileSize, dwFileAlign);
	lpNewFile			= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);

	if ( !lpNewFile )  //�����ڴ�ʧ��
		return FALSE;

	//����ԭ�ļ�����
	memset(lpNewFile, 0, dwNewFileSize);
	memcpy(lpNewFile, lpMemory, dwValidSize);	//ͷ�����ݸ���

	//�������ݸ���
	DWORD	dwSize		= lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize; //�ļ������������ܴ�С
	memcpy(lpNewFile + dwNewHeaderSize, lpMemory + lpSectionHeader->PointerToRawData, dwSize);

	//���Ʋ�������ǰ��ת����������
	PBYTE	pBuffer		= (PBYTE)(&Patch_Data);   //ָ�򸽼Ӵ��벹�����ݴ�
	_asm
	{
		pushad
		mov	 eax,dwPatchNum
		mov	 ebx,pBuffer
		mov	 dword ptr [ebx], eax
		popad
	}
	memcpy(pBuffer + 4, dwPatchAddress, 16*sizeof(DWORD) );
	pBuffer	+= 4 + 16*sizeof(DWORD);
	memcpy(pBuffer, byOldData, 16);
	memcpy(pBuffer+16, byNewData, 16);

	//�������ݸ���
	memcpy(lpNewFile + dwNewHeaderSize + dwSize, &Appendcode_Start, dwPatchSize);

	//������ϣ��ر�ӳ���ļ��;��
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	//��ʼ����PEͷ
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//ָ������ڴ�
	DWORD					OldEntry;

	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( lpNewFile + ((PIMAGE_DOS_HEADER)lpNewFile)->e_lfanew);
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER) (lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum; //�˴���ָ��Ҫ������������
	
	lpNewNtHeaders->FileHeader.NumberOfSections += 1; //���һ������
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
	lpNewEntry			= (DWORD*)(lpNewFile + dwNewHeaderSize + dwSize + dwPatchSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwPatchSize - 1); 

	//������ϣ�д���ļ�
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
		return FALSE;
	}

	WriteFile(hNewFile, lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//�ͷ��ڴ�
	VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////��Ӵ��뵽PEͷ��///////////////////////////////////////////////
//����PEͷ���ڴ������Ϊ0x1000�ֽ�,���Դ˷������ܻ����ʧ��

BOOL AddToHeaderSection(PBYTE lpMemory, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//�����ļ���С
	DWORD					dwPatchSize;		//������С
	DWORD					dwFileAlign;		//�ļ���������
	DWORD					dwSectionAlign;		//�ڴ��������
	PBYTE					lpNewFile;			//�����ļ�����
	DWORD					dwSectionNum;
	DWORD					dwPatchOffset;		//�������븴�Ƶ��ļ���ƫ�ƣ�Ҳ����ͷ����Ч���ݵĴ�С
	DWORD					dwNewHeaderSize;	//��ͷ����С
	DWORD					dwOldHeaderSize;	//��ͷ����С
	DWORD					dwSectionSize;		//���������ܴ�С


	lpNtHeaders		= (PIMAGE_NT_HEADERS)( lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum	= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;      //������һ������Ľڱ�
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwPatchSize		= ((DWORD)&Appendcode_End ) - ( (DWORD)&Appendcode_Start );	//��ò��������С
	dwOldHeaderSize	= lpSectionHeader->PointerToRawData;					
	
	//dwPatchOffset	= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew;  //��PEͷ��Ч���ݴ�С�����Ӵ���Ӵ˷���
	//ԭ����ʹ���������ַ�ʽ����Ч��С�������ǵ����PE���в�������µļ��ݣ�����ֱ�Ӵ�ͷ�������ǰ������ȡ�ô�С
	
	PBYTE					lpTemp = lpMemory + lpSectionHeader->PointerToRawData - 1;
	DWORD					dwSize = 0;
	while(*lpTemp == 0)  { lpTemp--; dwSize++; }	//��ÿ�������ݵĴ�С
	dwSize		   -= sizeof(IMAGE_SECTION_HEADER);	//��Ϊ�����и�ȫ��Ľڱ����Լ�ȥ 
	dwPatchOffset	= lpSectionHeader->PointerToRawData - dwSize ; //��PEͷ��Ч���ݴ�С�����Ӵ���Ӵ˷���


	dwNewHeaderSize	= Align(dwPatchOffset + dwPatchSize, dwFileAlign);  //�����ͷ��������С
	if (dwNewHeaderSize > 0x1000)
	{
		MessageBox (GetActiveWindow() , TEXT("PEͷ��С��������ѡ������������ʽ"), NULL, MB_OK);
		return FALSE;
	}

	//�������ƫ�Ƽ��ϴ�С�ټ�ȥ��PEͷ��С�͵���ԭ�ļ��������ε��ļ���С���ټ�����ͷ����С���������������Ҫ�����ļ���С
	dwSectionSize	= Align(lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize, dwFileAlign);
	dwNewFileSize	= Align(dwNewHeaderSize + dwSectionSize, dwFileAlign);  

	lpNewFile		= (PBYTE)VirtualAlloc(NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if (!lpNewFile) //�����ڴ�ʧ��
		return FALSE;

	//����ԭ�ļ�����
	memset(lpNewFile, 0, dwNewFileSize);
	memcpy(lpNewFile, lpMemory, dwPatchOffset);		//����ԭʼPEͷ
	memcpy(lpNewFile + dwNewHeaderSize, lpMemory + dwOldHeaderSize, dwSectionSize);		//������������ 

	//������ϣ��ر�ӳ���ļ��;�� (���رպ�����޷��������ļ�����������AddCode�رգ�����������쳣ʲô�Ļ���û�ã�ֻ��hMap���ó�ȫ�ֱ���Ȼ��������ˣ�ʵ�ڲ��Ź۰���_��)
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	//���Ʋ�������ǰ��ת����������
	PBYTE	pBuffer		= (PBYTE)(&Patch_Data);   //ָ�򸽼Ӵ��벹�����ݴ�
	_asm
	{
		pushad
		mov	 eax,dwPatchNum
		mov	 ebx,pBuffer
		mov	 dword ptr [ebx], eax
		popad
	}
	memcpy(pBuffer + 4, dwPatchAddress, 16*sizeof(DWORD) );
	pBuffer	+= 4 + 16*sizeof(DWORD);
	memcpy(pBuffer, byOldData, 16);
	memcpy(pBuffer+16, byNewData, 16);

	//���Ʋ�������
	memcpy(lpNewFile + dwPatchOffset, &Appendcode_Start, dwPatchSize);

	//��ʼ��ͷ�����ݽ����޸�
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)(lpNewFile + ((PIMAGE_DOS_HEADER)lpNewFile)->e_lfanew);
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
	lpNewEntry	= (DWORD*)(lpNewFile + dwPatchOffset + dwPatchSize - 5);   //ָ��Ҫ�����Ĳ���
	*lpNewEntry	= dwOldEntry - (dwPatchOffset + dwPatchSize - 1); 

	//������ϣ�д���ļ�
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{	
		VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
		return FALSE;
	}

	WriteFile(hNewFile, lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//�ͷ��ڴ�
	VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////

BOOL BytePatch(PBYTE lpMemory,DWORD dwFileSize)
{
	//����Ͳ���Ҫԭ���ľ���ˣ����ص�(��ʵ����Ϊ������ֻ�����ԣ����ֲ��� ��д���ԣ�ֻ��������ʵ�ڲ��Ź۰�����+..+)
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	HANDLE  hNewFile, hNewMap;
	PBYTE	lpNewMemory;
	DWORD	num=0;		//��¼�ɹ������Ĵ���
	DWORD	dwNum;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
		return FALSE;
	//��ʼ����
	hNewMap	= CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hMap)
	{
		lpNewMemory	= (BYTE *)MapViewOfFile (hNewMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (lpNewMemory)
		{
			PBYTE   pData=0;
			_asm     //Ҳû�뵽ʲô�ð취��ֻ�����õ�dwPatchNum�ĵط������������ʵ����
			{
				pushad
				mov	 eax,dwPatchNum
				mov	 dwNum, eax
				popad
			}
			for (DWORD i = 0; i < dwNum; i++)
			{
				pData = (PBYTE)VirtualAddressToPointer( (PIMAGE_DOS_HEADER)lpNewMemory, dwPatchAddress[i]);   //�������ַת������Ե��ļ�ָ��
				if ( *pData == byOldData[i])
				{
					*pData = byNewData[i];
					num++;
				}
			}
		}
	}
	UnmapViewOfFile(lpNewMemory);
	CloseHandle(hNewMap);
	CloseHandle(hNewFile);
	if (num != dwNum)   //����Ҫ�����������Ƚ�
	{
		MessageBox (GetActiveWindow() , TEXT("��������һ��,���鲹����ַ"), NULL, MB_OK);
		return FALSE;
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////
//���������Ǹ�switch��ת����Ϊ����Ϊ���������Ż�������������ʹ��#pragmaָʾ��ȡ���Ż�
#pragma optimize("",off)
BOOL AddCode( )
{
	DWORD	dwFileSize;	//�ļ���С
	PBYTE	lpMemory;	//�ڴ�ӳ��ָ��
	
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
					//ʹ��ָ�������򲹶�
					switch(dwTypeOfPatch)
					{
						case ADD_LAST_SECTION:
							if (!AddToLastSection(lpMemory, dwFileSize) )
								return FALSE;
							break;

						case ADD_NEW_SECTION:
							if (!AddToNewSection(lpMemory, dwFileSize) )
								return FALSE;
							break;

						case ADD_TO_HEADER:
							if (!AddToHeaderSection(lpMemory, dwFileSize) )
								return FALSE;
							break;

						case BYTE_PATCH:
							if (!BytePatch(lpMemory, dwFileSize))
								return FALSE;
							break;

					}
					
					return TRUE;
				}
				else
					MessageBox (GetActiveWindow() , TEXT("Ŀ���ļ���ʧ��"), NULL, MB_OK);
			}
			else
				MessageBox ( GetActiveWindow() , TEXT("Ŀ���ļ���ʧ��"), NULL, MB_OK);
		}
	}
	else
		MessageBox (GetActiveWindow() , TEXT("Ŀ���ļ���ʧ��"), NULL, MB_OK);
	
	

	return FALSE;
}
//�����������ָ��Ż�
#pragma optimize("",on)

#endif 