#ifndef PE32_K_H
#define PE32_K_H

#ifdef __cplusplus
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif

//指定
#define ADD_LAST_SECTION 1	//添加代码到最后一个区段	
#define ADD_NEW_SECTION  2  //添加代码到一个新建的区段
#define ADD_PE_HEADER	 3  //添加代码到PE头部
#define ADD_LARGE_FILE	 4  //由于采用分配内存的方式添加代码，大文件会失败，所以特别弄一个针对大文件的添加代码方法(也是加入到最后一个节),待实现


//重置,重置后可继续用文件初始化此DLL
EXPORT BOOL _stdcall Reset(TCHAR szFileName[]);


//使用函数前必须调用此函数初始化
//参数---szFileName:文件名
EXPORT BOOL _stdcall InitPE32(TCHAR szFileName[]);


//取得原文件数据指针,只读属性
EXPORT CONST PBYTE _stdcall GetFileBuffer();


//取得文件名
EXPORT CONST PTCHAR _stdcall GetFileName();


//添加代码到目标文件
//参数---szNewFile:添加代码后新文件的名字	lpCOdeStart:指向代码起始处	
//		 dwCodeSize:添加代码的大小			dwTypeOfAdd:添加方法类型
EXPORT BOOL _stdcall AddCode(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize, DWORD dwTypeOfAdd);


//获得目标文件的CRC32校验值
EXPORT DWORD _stdcall GetCRC32(TCHAR szFileName[] );


//检测是否为PE文件
//参数---szFileName:文件名
EXPORT BOOL _stdcall IsPeFile(TCHAR szFileName[]);


//将RVA偏移转换成文件偏移,失败返回-1
EXPORT DWORD _stdcall RvaToOffset (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva);


//将RVA偏移转成文件指针偏移,失败返回NULL
EXPORT PBYTE _stdcall RvaToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwRva);


//将虚拟地址转成文件指针偏移,失败返回NULL
EXPORT PBYTE _stdcall VirtualAddressToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwVirtualAddress);


//获得RVA偏移处的节区名称，失败返回NULL
EXPORT CONST PBYTE _stdcall GetRvaSection (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva);


//获得指定RVA所处节区的节表头,失败返回NULL
EXPORT PIMAGE_SECTION_HEADER _stdcall GetSectionOfRva (IMAGE_DOS_HEADER *lpFileHead, char* secName);


//文件偏移转换成RVA，失败返回-1
EXPORT DWORD _stdcall OffsetToRVA(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset);


//文件偏移转换成内存指针，失败返回NULL
EXPORT PBYTE _stdcall OffsetToPointer(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset);


//提取图标数据，参数指定文件名
EXPORT BOOL _stdcall GetIcon (TCHAR	szIconFileName[]);

#endif