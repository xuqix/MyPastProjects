#ifndef ADDCODE_H
#define ADDCODE_H

#include <windows.h>
#include "RvaToOffset.h"

/////////////////////////下面几种附加代码的补丁应该是能很好的互相兼容(表示我测试过给一文件加了20多个依然正常#_#)///////////////////////////////////

//静态补丁类型
#define ADD_LAST_SECTION 1	//添加代码到最后一个区段	
#define ADD_NEW_SECTION  2  //添加代码到一个新建的区段
#define ADD_TO_HEADER	 3  //添加代码到PE头部
//除了以上3种外还有一种是寻找已存在区段空闲处并插入代码，这种插入PE头的修改更简单，只是比较容易失败，这里就不实现了
//这里再加一种字节补丁，针对一些简单的程序
#define	BYTE_PATCH		 4


///////////////////此文件补丁最多支持16个补丁地址，特别创建一个段用于补丁工具生成补丁时修正变量//////////////////////

#pragma data_seg(".sdata")
DWORD	dwTypeOfPatch = 0 ;					//指示补丁类型
DWORD	dwPatchNum = 0;						//补丁数量
TCHAR	szFileName[MAX_PATH] = { L"load" };

DWORD	dwPatchAddress[16] = { 0 };				//补丁地址
BYTE	byOldData[16] = { 0 };					//补丁处旧数据和新数据
BYTE	byNewData[16] = { 0 };	
#pragma data_seg()
#pragma comment(linker, "/SECTION:.sdata,ERW")

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////从附加代码中引入的变量，需要补丁程序使用和修正//////////////////////

extern "C" DWORD Appendcode_Start;		//附加代码段起始
extern "C" DWORD Appendcode_End;		//附加代码段结束
extern "C" DWORD Patch_Data;			//需要将补丁数据转存到附加代码中

/////////////////////////////////////////////////////////////////////////////////////////

HANDLE	hFile ;//文件句柄
HANDLE	hMap; //映射文件句柄

//////////////////////////////添加代码到最后一个区块/////////////////////////////////////

BOOL AddToLastSection(PBYTE lpMemory, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//最终文件大小
	DWORD					dwFileAlignSize;	//原文件对齐后大小
	DWORD					dwLastSectionAlignSize; //最后区段内存对齐后大小
	DWORD					dwPatchSize;		//补丁大小
	DWORD					dwFileAlign;		//文件对齐粒度
	DWORD					dwSectionAlign;		//内存对齐粒度
	//DWORD					dwLastSectionSize;
	//DWORD					dwPatchStart;	//指定补丁要复制到的文件偏移起始
	PBYTE					lpNewFile;		//最终文件缓存
	DWORD					dwSectionNum;

	lpNtHeaders		= (PIMAGE_NT_HEADERS)( lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	
	dwSectionNum = lpNtHeaders->FileHeader.NumberOfSections ;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwFileAlignSize	= Align(dwFileSize, dwFileAlign);	//求原文件对齐大小
	
	dwPatchSize		= ((DWORD)&Appendcode_End ) - ( (DWORD)&Appendcode_Start );	//获得补丁代码大小

	dwNewFileSize	= Align(dwFileAlignSize + dwPatchSize, dwFileAlign); //获得最终文件对齐后大小
	dwLastSectionAlignSize	= Align(lpLastSectionHeader->Misc.VirtualSize + dwPatchSize, dwSectionAlign); //获得内存中最后区段大小

	lpNewFile		= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if ( !lpNewFile )  //分配内存失败
		return FALSE;

	//复制原文件数据
	memset(lpNewFile, 0, dwNewFileSize);
	memcpy(lpNewFile, lpMemory, dwFileSize);
	//复制完毕，关闭映射文件和句柄 (不关闭后面就无法创建新文件，本来想在AddCode关闭，结果试了抛异常什么的还是没用，只能hMap设置成全局变量然后在这关了，实在不雅观啊‘_’)
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	//复制补丁代码前先转储补丁数据
	PBYTE	pBuffer		= (PBYTE)(&Patch_Data);   //指向附加代码补丁数据处
	//(*(DWORD*)pBuffer)	= dwPatchNum; 原本是这句，但是被优化掉后就变成了mov xxx,0  所以对于变量的读写需要特别注意，一不小心就优化掉了，也不想牺牲效率，这里就换成汇编吧虽然不太好看
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

	//复制补丁代码
	memcpy(lpNewFile + dwFileAlignSize, &Appendcode_Start, dwPatchSize); 

	//修正PE头数据
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//指向新入口处
	DWORD					OldEntry;
	
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( lpNewFile + ((PIMAGE_DOS_HEADER)lpNewFile)->e_lfanew );
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum - 1;

 	
	//给最后区段添加读写执行属性
	lpNewLastSection->Characteristics  |= 0xC0000020;
	
	//修正最后一个区段的偏移量
	lpNewLastSection->SizeOfRawData		= dwNewFileSize - lpNewLastSection->PointerToRawData;  
	lpNewLastSection->Misc.VirtualSize	= Align( GetValidSize(lpNewFile, lpNewLastSection), dwSectionAlign);//Align(lpNewLastSection->Misc.VirtualSize + dwPatchSize, dwSectionAlign) ;
	
	//修正镜像大小
	lpNewNtHeaders->OptionalHeader.SizeOfImage	= Align(lpNewLastSection->VirtualAddress + lpNewLastSection->Misc.VirtualSize, dwSectionAlign);

	//修正入口地址
	OldEntry	= lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint;
	lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint	= OffsetToRVA( (IMAGE_DOS_HEADER *)lpNewFile, dwFileAlignSize) ;
	
	//修正补丁代码跳回OEP的参数
	lpNewEntry			= (DWORD*)(lpNewFile + dwFileAlignSize + dwPatchSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwPatchSize - 1); 

	//补丁完毕，写回文件
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
		return FALSE;
	}

	WriteFile(hNewFile, lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//释放内存
	VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////新建一个区段并插入代码///////////////////////////////////
//如果节表没有多余的空间

BOOL AddToNewSection(PBYTE lpMemory, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//最终文件大小
	DWORD					dwPatchSize;		//补丁大小
	DWORD					dwPatchSectionSize; //内存中补丁区块的大小
	DWORD					dwPatchFileSize;	//补丁区段文件中大小
	DWORD					dwFileAlign;		//文件对齐粒度
	DWORD					dwSectionAlign;		//内存对齐粒度
	PBYTE					lpNewFile;			//最终文件缓存
	DWORD					dwSectionNum;
	DWORD					dwNewHeaderSize;	//新头部大小
	DWORD					dwOldHeaderSize;	//老头部大小
	DWORD					dwSectionSize;		//内存中所有区块总大小
	BOOL					bChange = FALSE;	//指示新节表的加入是否影响到文件头大小，如有影响，则需修正各区段偏移

	lpNtHeaders			= (PIMAGE_NT_HEADERS)(lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew);
	lpSectionHeader		= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum		= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;
	dwFileAlign			= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign		= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwOldHeaderSize		= lpSectionHeader->PointerToRawData;

	dwSectionSize		= Align(lpLastSectionHeader->VirtualAddress + lpLastSectionHeader->Misc.VirtualSize - Align(dwOldHeaderSize, dwSectionAlign), dwSectionAlign);  //内存中区段总大小

	//获得补丁相关数据
	dwPatchSize			= ((DWORD)&Appendcode_End ) - ( (DWORD)&Appendcode_Start );	//获得补丁代码大小
	dwPatchSectionSize	= Align(dwPatchSize, dwSectionAlign);	//内存中新区段对齐大小
	dwPatchFileSize		= Align(dwPatchSize, dwFileAlign);		//文件中新区段对齐大小
	
	//头部是否能增加一个节表
	DWORD	dwValidSize;	//头部当前有效大小
	dwValidSize			= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew ;
	if ( Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign) > 0x1000 )  
		return FALSE;

	//是否要增加头部大小
	dwNewHeaderSize		= Align(dwValidSize + sizeof(IMAGE_SECTION_HEADER), dwFileAlign); //新头部大小
	if (dwNewHeaderSize > Align(dwValidSize, dwFileAlign) )
		bChange	= TRUE;   //记录头部增加，后面需要修改所有节表偏移

	dwNewFileSize		= Align (Align(dwFileSize, dwFileAlign) + dwPatchFileSize, dwFileAlign);
	lpNewFile			= (PBYTE)VirtualAlloc (NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);

	if ( !lpNewFile )  //分配内存失败
		return FALSE;

	//复制原文件数据
	memset(lpNewFile, 0, dwNewFileSize);
	memcpy(lpNewFile, lpMemory, dwValidSize);	//头部数据复制

	//区段数据复制
	DWORD	dwSize		= lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize; //文件中所有区段总大小
	memcpy(lpNewFile + dwNewHeaderSize, lpMemory + lpSectionHeader->PointerToRawData, dwSize);

	//复制补丁代码前先转储补丁数据
	PBYTE	pBuffer		= (PBYTE)(&Patch_Data);   //指向附加代码补丁数据处
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

	//补丁数据复制
	memcpy(lpNewFile + dwNewHeaderSize + dwSize, &Appendcode_Start, dwPatchSize);

	//复制完毕，关闭映射文件和句柄
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	//开始修正PE头
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	PIMAGE_SECTION_HEADER	lpNewLastSection;
	DWORD*					lpNewEntry;			//指向新入口处
	DWORD					OldEntry;

	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)( lpNewFile + ((PIMAGE_DOS_HEADER)lpNewFile)->e_lfanew);
	lpNewSectionHeader	= (PIMAGE_SECTION_HEADER) (lpNewNtHeaders + 1);
	lpNewLastSection	= lpNewSectionHeader + dwSectionNum; //此处即指向要创建的新区段
	
	lpNewNtHeaders->FileHeader.NumberOfSections += 1; //添加一个区段
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
	lpNewEntry			= (DWORD*)(lpNewFile + dwNewHeaderSize + dwSize + dwPatchSize - 5);
	*lpNewEntry			= OldEntry - (lpNewNtHeaders->OptionalHeader.AddressOfEntryPoint + dwPatchSize - 1); 

	//补丁完毕，写回文件
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
		return FALSE;
	}

	WriteFile(hNewFile, lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//释放内存
	VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////添加代码到PE头部///////////////////////////////////////////////
//由于PE头部内存中最大为0x1000字节,所以此方法可能会添加失败

BOOL AddToHeaderSection(PBYTE lpMemory, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS		lpNtHeaders;
	PIMAGE_SECTION_HEADER	lpSectionHeader;
	PIMAGE_SECTION_HEADER	lpLastSectionHeader;
	DWORD					dwNewFileSize;		//最终文件大小
	DWORD					dwPatchSize;		//补丁大小
	DWORD					dwFileAlign;		//文件对齐粒度
	DWORD					dwSectionAlign;		//内存对齐粒度
	PBYTE					lpNewFile;			//最终文件缓存
	DWORD					dwSectionNum;
	DWORD					dwPatchOffset;		//补丁代码复制到文件的偏移，也即老头部有效数据的大小
	DWORD					dwNewHeaderSize;	//新头部大小
	DWORD					dwOldHeaderSize;	//老头部大小
	DWORD					dwSectionSize;		//所有区块总大小


	lpNtHeaders		= (PIMAGE_NT_HEADERS)( lpMemory + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew );
	lpSectionHeader	= (PIMAGE_SECTION_HEADER)(lpNtHeaders + 1);
	dwSectionNum	= lpNtHeaders->FileHeader.NumberOfSections;
	lpLastSectionHeader	= lpSectionHeader + dwSectionNum - 1;      //获得最后一个区块的节表
	dwFileAlign		= lpNtHeaders->OptionalHeader.FileAlignment;
	dwSectionAlign	= lpNtHeaders->OptionalHeader.SectionAlignment;
	dwPatchSize		= ((DWORD)&Appendcode_End ) - ( (DWORD)&Appendcode_Start );	//获得补丁代码大小
	dwOldHeaderSize	= lpSectionHeader->PointerToRawData;					
	
	//dwPatchOffset	= sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER)*(dwSectionNum + 1) + ((PIMAGE_DOS_HEADER)lpMemory)->e_lfanew;  //老PE头有效数据大小，附加代码从此放置
	//原本是使用上面这种方式求有效大小，但考虑到如果PE已有补丁情况下的兼容，所有直接从头最后面往前搜索以取得大小
	
	PBYTE					lpTemp = lpMemory + lpSectionHeader->PointerToRawData - 1;
	DWORD					dwSize = 0;
	while(*lpTemp == 0)  { lpTemp--; dwSize++; }	//获得可填充数据的大小
	dwSize		   -= sizeof(IMAGE_SECTION_HEADER);	//因为可能有个全零的节表，所以减去 
	dwPatchOffset	= lpSectionHeader->PointerToRawData - dwSize ; //老PE头有效数据大小，附加代码从此放置


	dwNewHeaderSize	= Align(dwPatchOffset + dwPatchSize, dwFileAlign);  //获得新头部对齐后大小
	if (dwNewHeaderSize > 0x1000)
	{
		MessageBox (GetActiveWindow() , TEXT("PE头大小不够，请选择其他补丁方式"), NULL, MB_OK);
		return FALSE;
	}

	//最后区段偏移加上大小再减去老PE头大小就等于原文件所有区段的文件大小，再加上新头部大小并对齐就是我们需要的新文件大小
	dwSectionSize	= Align(lpLastSectionHeader->PointerToRawData + lpLastSectionHeader->SizeOfRawData - dwOldHeaderSize, dwFileAlign);
	dwNewFileSize	= Align(dwNewHeaderSize + dwSectionSize, dwFileAlign);  

	lpNewFile		= (PBYTE)VirtualAlloc(NULL, dwNewFileSize, MEM_COMMIT, PAGE_READWRITE);
	if (!lpNewFile) //分配内存失败
		return FALSE;

	//复制原文件数据
	memset(lpNewFile, 0, dwNewFileSize);
	memcpy(lpNewFile, lpMemory, dwPatchOffset);		//复制原始PE头
	memcpy(lpNewFile + dwNewHeaderSize, lpMemory + dwOldHeaderSize, dwSectionSize);		//复制区块数据 

	//复制完毕，关闭映射文件和句柄 (不关闭后面就无法创建新文件，本来想在AddCode关闭，结果试了抛异常什么的还是没用，只能hMap设置成全局变量然后在这关了，实在不雅观啊‘_’)
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	//复制补丁代码前先转储补丁数据
	PBYTE	pBuffer		= (PBYTE)(&Patch_Data);   //指向附加代码补丁数据处
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

	//复制补丁代码
	memcpy(lpNewFile + dwPatchOffset, &Appendcode_Start, dwPatchSize);

	//开始对头部数据进行修复
	PIMAGE_NT_HEADERS		lpNewNtHeaders;
	PIMAGE_SECTION_HEADER	lpNewSectionHeader;
	lpNewNtHeaders		= (PIMAGE_NT_HEADERS)(lpNewFile + ((PIMAGE_DOS_HEADER)lpNewFile)->e_lfanew);
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
	lpNewEntry	= (DWORD*)(lpNewFile + dwPatchOffset + dwPatchSize - 5);   //指向要修正的参数
	*lpNewEntry	= dwOldEntry - (dwPatchOffset + dwPatchSize - 1); 

	//补丁完毕，写回文件
	HANDLE  hNewFile;
	DWORD	dwRead;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{	
		VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
		return FALSE;
	}

	WriteFile(hNewFile, lpNewFile, dwNewFileSize, &dwRead, NULL);
	CloseHandle(hNewFile);

	//释放内存
	VirtualFree(lpNewFile, dwNewFileSize, MEM_RELEASE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////

BOOL BytePatch(PBYTE lpMemory,DWORD dwFileSize)
{
	//这里就不需要原来的句柄了，都关掉(其实是因为设置了只读属性，但又不好 加写属性，只好这样，实在不雅观啊啊啊+..+)
	UnmapViewOfFile(lpMemory);
	CloseHandle(hMap);
	CloseHandle(hFile);

	HANDLE  hNewFile, hNewMap;
	PBYTE	lpNewMemory;
	DWORD	num=0;		//记录成功补丁的次数
	DWORD	dwNum;
	if (INVALID_HANDLE_VALUE == ( hNewFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE , FILE_SHARE_READ , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
		return FALSE;
	//开始补丁
	hNewMap	= CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hMap)
	{
		lpNewMemory	= (BYTE *)MapViewOfFile (hNewMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (lpNewMemory)
		{
			PBYTE   pData=0;
			_asm     //也没想到什么好办法，只有在用到dwPatchNum的地方都用内联汇编实现了
			{
				pushad
				mov	 eax,dwPatchNum
				mov	 dwNum, eax
				popad
			}
			for (DWORD i = 0; i < dwNum; i++)
			{
				pData = (PBYTE)VirtualAddressToPointer( (PIMAGE_DOS_HEADER)lpNewMemory, dwPatchAddress[i]);   //将虚拟地址转换成相对的文件指针
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
	if (num != dwNum)   //与需要补丁的数量比较
	{
		MessageBox (GetActiveWindow() , TEXT("补丁数不一致,请检查补丁地址"), NULL, MB_OK);
		return FALSE;
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////
//由于下面那个switch跳转会因为变量为常量而被优化掉，所以这里使用#pragma指示来取消优化
#pragma optimize("",off)
BOOL AddCode( )
{
	DWORD	dwFileSize;	//文件大小
	PBYTE	lpMemory;	//内存映射指针
	
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
					//使用指定方法打补丁
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
					MessageBox (GetActiveWindow() , TEXT("目标文件打开失败"), NULL, MB_OK);
			}
			else
				MessageBox ( GetActiveWindow() , TEXT("目标文件打开失败"), NULL, MB_OK);
		}
	}
	else
		MessageBox (GetActiveWindow() , TEXT("目标文件打开失败"), NULL, MB_OK);
	
	

	return FALSE;
}
//函数结束，恢复优化
#pragma optimize("",on)

#endif 