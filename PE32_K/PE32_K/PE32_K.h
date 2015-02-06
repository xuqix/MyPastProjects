#ifndef PE32_K_H
#define PE32_K_H

#ifdef __cplusplus
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif

//ָ��
#define ADD_LAST_SECTION 1	//��Ӵ��뵽���һ������	
#define ADD_NEW_SECTION  2  //��Ӵ��뵽һ���½�������
#define ADD_PE_HEADER	 3  //��Ӵ��뵽PEͷ��
#define ADD_LARGE_FILE	 4  //���ڲ��÷����ڴ�ķ�ʽ��Ӵ��룬���ļ���ʧ�ܣ������ر�Ūһ����Դ��ļ�����Ӵ��뷽��(Ҳ�Ǽ��뵽���һ����),��ʵ��


//����,���ú�ɼ������ļ���ʼ����DLL
EXPORT BOOL _stdcall Reset(TCHAR szFileName[]);


//ʹ�ú���ǰ������ô˺�����ʼ��
//����---szFileName:�ļ���
EXPORT BOOL _stdcall InitPE32(TCHAR szFileName[]);


//ȡ��ԭ�ļ�����ָ��,ֻ������
EXPORT CONST PBYTE _stdcall GetFileBuffer();


//ȡ���ļ���
EXPORT CONST PTCHAR _stdcall GetFileName();


//��Ӵ��뵽Ŀ���ļ�
//����---szNewFile:��Ӵ�������ļ�������	lpCOdeStart:ָ�������ʼ��	
//		 dwCodeSize:��Ӵ���Ĵ�С			dwTypeOfAdd:��ӷ�������
EXPORT BOOL _stdcall AddCode(TCHAR szNewFile[], PBYTE lpCodeStart, DWORD dwCodeSize, DWORD dwTypeOfAdd);


//���Ŀ���ļ���CRC32У��ֵ
EXPORT DWORD _stdcall GetCRC32(TCHAR szFileName[] );


//����Ƿ�ΪPE�ļ�
//����---szFileName:�ļ���
EXPORT BOOL _stdcall IsPeFile(TCHAR szFileName[]);


//��RVAƫ��ת�����ļ�ƫ��,ʧ�ܷ���-1
EXPORT DWORD _stdcall RvaToOffset (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva);


//��RVAƫ��ת���ļ�ָ��ƫ��,ʧ�ܷ���NULL
EXPORT PBYTE _stdcall RvaToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwRva);


//�������ַת���ļ�ָ��ƫ��,ʧ�ܷ���NULL
EXPORT PBYTE _stdcall VirtualAddressToPointer(IMAGE_DOS_HEADER *lpFileHead,DWORD dwVirtualAddress);


//���RVAƫ�ƴ��Ľ������ƣ�ʧ�ܷ���NULL
EXPORT CONST PBYTE _stdcall GetRvaSection (IMAGE_DOS_HEADER *lpFileHead, DWORD dwRva);


//���ָ��RVA���������Ľڱ�ͷ,ʧ�ܷ���NULL
EXPORT PIMAGE_SECTION_HEADER _stdcall GetSectionOfRva (IMAGE_DOS_HEADER *lpFileHead, char* secName);


//�ļ�ƫ��ת����RVA��ʧ�ܷ���-1
EXPORT DWORD _stdcall OffsetToRVA(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset);


//�ļ�ƫ��ת�����ڴ�ָ�룬ʧ�ܷ���NULL
EXPORT PBYTE _stdcall OffsetToPointer(IMAGE_DOS_HEADER *lpFileHead, DWORD dwOffset);


//��ȡͼ�����ݣ�����ָ���ļ���
EXPORT BOOL _stdcall GetIcon (TCHAR	szIconFileName[]);

#endif