#include <windows.h>
#include "resource.h"

//��̬��������
#define SLEEP_PATCH 1	
#define DEBUG_PATCH 2

//��̬���������֧��16��������ַ���ر𴴽�һ�������ڲ����������ɲ���ʱ��������

#pragma data_seg(".sdata")
DWORD	dwTypeOfPatch = 0;					//ָʾ��������
DWORD	dwPatchNum  = 0 ;						//��������
TCHAR	szFileName[MAX_PATH] = { 0 };

DWORD	dwPatchAddress[16] = { 0 };				//������ַ
BYTE	byOldData[16] = { 0 };					//�����������ݺ�������
BYTE	byNewData[16] = { 0 };	

#pragma data_seg()
#pragma comment(linker, "/SECTION:.sdata,ERW")

//ʱ���¼
DWORD	dwTickCount;

///////////////////////////����1,���ý��̶�д���Ʋ��ϲ��Բ���λ��///////////////////////////
////////////////�ô���������һЩ�ǣ��׶��ǽ�����ͣһ��ʱ����,����ʧ��,���ȶ�//////////////
void Patch1()
{
	STARTUPINFO     	si;
	PROCESS_INFORMATION pi;
	
	BYTE      ReadBuffer = 0;
	BOOL      bContinueRun=TRUE;
	
	ZeroMemory(&si, sizeof(STARTUPINFO)) ;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)) ;

    if( !CreateProcess(szFileName, NULL, NULL,  NULL,  FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi) ) 
	{
        MessageBox(NULL, TEXT("��ȷ�������Ƿ���Ŀ���ļ�Ŀ¼��."), TEXT("��ʾ"), MB_OK); 
        return ; 
    } 
	
	DWORD	i, dwCount=0;
	DWORD	dwCurrentTick, OldPro;
	while (bContinueRun) {
		
		ResumeThread(pi.hThread); 
		Sleep(10);
		SuspendThread(pi.hThread);
		for (i=0; dwPatchAddress[i] ; i++)
		{
			ReadProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i], &ReadBuffer, 1, NULL);
			
			if( byOldData[i] == ReadBuffer )    //�жϵ�ַ�����Ƿ���ȷ
			{	
				VirtualProtectEx(pi.hProcess, (LPVOID)dwPatchAddress[i], 1, PAGE_EXECUTE_READWRITE, &OldPro);
				WriteProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i],&byNewData[i], 1,NULL);
				dwCount++;
				ResumeThread(pi.hThread); 
			}	
		}
		_asm
		{
			mov	ebx,dwPatchNum
			cmp	dwCount,ebx
			jne	_exit
			mov	bContinueRun,0
			_exit:
		}
		//if (dwCount == dwPatchNum)  //���в�����ϣ��˳�ѭ��
		//	bContinueRun	= FALSE;
		dwCurrentTick	= GetTickCount();
		if ( (dwCurrentTick - dwTickCount) > 6*1000)  //���10�뻹û�˳�����ʱ
		{
			MessageBox( NULL, TEXT("��ʱ,����ʧ��,ָʾ�Ĳ������ݿ����д���"), 0, 0);
			TerminateProcess(pi.hProcess, 0);
			bContinueRun	= FALSE; ;
		}
	}

    CloseHandle(pi.hProcess); 
    CloseHandle(pi.hThread); 
}

/////////////////////////////////////////���õ��ԼĴ����򲹶�///////////////////////////////////////////////////////
/////////////����ಹ��Ӧ�ڲ�����ַ��һ����ַ����ϣ���жϵĵ�ַ��ȷ�����е�ַ�����ѽ��룬�Ա�֤������ȷ��///////////
void Patch2()
{
	STARTUPINFO				si ;
	PROCESS_INFORMATION		pi ;
	ZeroMemory(&si, sizeof(STARTUPINFO)) ;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)) ;
	si.cb = sizeof(STARTUPINFO) ;
	
	BOOL	WhileDoFlag=TRUE;
	BYTE    ReadBuffer=0;
	
	
	if( !CreateProcess(szFileName, NULL, NULL, NULL, FALSE, DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi) ) 
	{
        MessageBox(NULL, TEXT("��ȷ�������Ƿ���Ŀ���ļ�Ŀ¼��."), TEXT("��ʾ"), MB_OK);
        return ; 
    } 
	
	DEBUG_EVENT		DBEvent ;
	CONTEXT			Regs ;
	DWORD			dwSSCnt , dwNum;
	
	dwSSCnt = 0 ;

	Regs.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS ;

	//ʹ������Single Stepģʽ������,ÿִ��һ��ָ��ͻ�����Խ��̷���EXCEPTION_SINGLE_STEP
	//��Ҫע����ǣ��յ������Ϣ�������������ó���Single Step��ȥ������Ҫ��������һ��SFλ��
	GetThreadContext(pi.hThread,&Regs);
	Regs.EFlags|=0x100;
	SetThreadContext(pi.hThread,&Regs);

	ResumeThread(pi.hThread);

	DWORD	i,OldPro,dwCount=0;
	while (WhileDoFlag) {
		WaitForDebugEvent (&DBEvent, INFINITE);
		switch (DBEvent.dwDebugEventCode)
		{
		case	EXCEPTION_DEBUG_EVENT:

			switch (DBEvent.u.Exception.ExceptionRecord.ExceptionCode)
			{
			case	EXCEPTION_SINGLE_STEP :
				{
					++dwSSCnt ;
					if (dwSSCnt == 1)
					{   
						//���յ���һ��EXCEPTION_SINGLE_STEP�쳣�źţ���ʾ�ж��ڳ���ĵ�һ��ָ�����ڵ�
						//��Dr0���óɳ������ڵ�ַ
						
						GetThreadContext(pi.hThread,&Regs);
						
						Regs.Dr0=Regs.Eax;
						Regs.Dr7=0x101;
						
						SetThreadContext(pi.hThread,&Regs);
						
					}
					else if (dwSSCnt == 2)
					{
						//�ڶ����ж����������õĲ����㣬����Ӳ���ϵ㣬�������еĵ�ַ�������е��˴�ʱ����

						GetThreadContext(pi.hThread, &Regs) ;
						Regs.Dr0 = dwPatchAddress[0];
						Regs.Dr7 = 0x101 ;
						SetThreadContext(pi.hThread, &Regs) ;
					}
					else if (dwSSCnt == 3)
					{
						//�������жϣ���������ָʾ�ĵ�ַ
						GetThreadContext(pi.hThread, &Regs) ;
						Regs.Dr0 = Regs.Dr7 = 0 ;
					
						//���±�1��ʼ������Ҫ�򲹶��ĵ�ַ
						for (i=1; dwPatchAddress[i] ; i++)
						{
							ReadProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i], &ReadBuffer, 1, NULL);
			
							if( byOldData[i-1] == ReadBuffer )    //�жϵ�ַ�����Ƿ���ȷ
							{	
								VirtualProtectEx(pi.hProcess, (LPVOID)dwPatchAddress[i], 1, PAGE_EXECUTE_READWRITE, &OldPro);
								WriteProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i],&byNewData[i-1], 1,NULL); 
								dwCount++;
							}	
						}
						_asm
						{
							push eax
							mov	eax,dwPatchNum
							mov	dwNum,eax
							pop eax
						}
						if (dwCount != dwNum)   //�����ɹ�������Ҫ��������ͬ
							MessageBox(NULL, TEXT("�����ֽ�����һ��,��������ʧ��"), TEXT("��ʾ"), 0);
						
						SetThreadContext(pi.hThread, &Regs) ;
					}
					break ;
				}				 
			}
			break ;

		case	EXIT_PROCESS_DEBUG_EVENT :
				WhileDoFlag=FALSE;
				break ;
		}
		
		ContinueDebugEvent(DBEvent.dwProcessId, DBEvent.dwThreadId, DBG_CONTINUE) ;
	}
	
	CloseHandle(pi.hProcess) ;
	CloseHandle(pi.hThread)  ;
}

//////////////////////////////������ǰ����Ȩ�ޣ�����Ȩ�ޣ�///////////////////////////////////
BOOL EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable) {

   BOOL fOk = FALSE;    
   HANDLE hToken;

   if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, 
      &hToken)) {

      TOKEN_PRIVILEGES tp;
      tp.PrivilegeCount = 1;
      LookupPrivilegeValue(NULL, szPrivilege, &tp.Privileges[0].Luid);
      tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
      AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
      fOk = (GetLastError() == ERROR_SUCCESS);

      CloseHandle(hToken);
   }
   return(fOk);
}

//ȡ���Ż�������ת��Ч
#pragma optimize("",off)
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,PSTR szCmdLine, int iCmdShow)
{
	dwTickCount	= GetTickCount();
	EnablePrivilege(SE_DEBUG_NAME, TRUE);
	switch(dwTypeOfPatch)
	{
		case SLEEP_PATCH:
			Patch1();
			break;

		case DEBUG_PATCH:
			Patch2();
			break;
	}
	EnablePrivilege(SE_DEBUG_NAME, FALSE);

    return 0 ;
}
#pragma optimize("",on)


/*
void Patch2()
{
	STARTUPINFO				si ;
	PROCESS_INFORMATION		pi ;
	ZeroMemory(&si, sizeof(STARTUPINFO)) ;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)) ;
	si.cb = sizeof(STARTUPINFO) ;
	
	BOOL	WhileDoFlag=TRUE;
	BYTE    ReadBuffer={0};
	BYTE    dwINT3code[1]={0xCC};
	BYTE    dwOldbyte[1]={0};

	HANDLE	hFile;
	DWORD	dwEntry, dwRead;
	IMAGE_NT_HEADERS stNtHeaders;
	if (INVALID_HANDLE_VALUE != ( hFile = CreateFile (szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL) ) )
	{
		dwFileSize	= GetFileSize (hFile, NULL);
		if (dwFileSize)
		{
			SetFilePointer(hFile, 0x3c, 0, FILE_BEGIN);
			ReadFile(hFile, &dwEntry, 4, &dwRead, NULL);
			SetFilePointer(hFile, dwEntry, 0, FILE_BEGIN);
			ReadFile(hFile, &stNtHeaders, sizeof(IMAGE_NT_HEADERS), &dwRead, NULL);
			dwEntry	= stNtHeaders.OptionalHeader.AddressOfEntryPoint;
			dwEntry+= stNtHeaders.OptionalHeader.ImageBase;
			CloseHandle(hFile);
		}
	}
	else
	{
        MessageBox(NULL, TEXT("��ȷ�������Ƿ���Ŀ���ļ�Ŀ¼��."), NULL, MB_OK);
        return ; 
	}

	if( !CreateProcess(SZFILENAME, NULL, NULL, NULL, FALSE, DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi) ) 
	{
        MessageBox(NULL, TEXT("��ȷ�������Ƿ���Ŀ���ļ�Ŀ¼��."), NULL, MB_OK);
        return ; 
    } 
	
	DEBUG_EVENT		DBEvent ;
	CONTEXT			Regs ;
	DWORD			dwState,Oldpp;


	Regs.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS ;
	
	DWORD	i, dwCount=0;
	DWORD	dwCurrentTick, OldPro;

	while (WhileDoFlag) {
		WaitForDebugEvent (&DBEvent, INFINITE);
		dwState = DBG_EXCEPTION_NOT_HANDLED ;
		switch (DBEvent.dwDebugEventCode)
		{
			case CREATE_PROCESS_DEBUG_EVENT:
				//������̿�ʼ���򽫶ϵ��ַ�Ĵ����ΪINT3�ж�,ͬʱ����ԭ������
				ReadProcessMemory(pi.hProcess, (LPCVOID)(dwEntry), &dwOldbyte, sizeof(dwOldbyte), NULL) ;
				VirtualProtectEx(pi.hProcess, (LPVOID)dwEntry, 1, PAGE_EXECUTE_READWRITE, &Oldpp);
				WriteProcessMemory(pi.hProcess, (LPVOID)dwEntry,&dwINT3code, 1,NULL);	//�򲹶�
				dwState = DBG_CONTINUE ;
				break;			
				
			case	EXIT_PROCESS_DEBUG_EVENT :
				WhileDoFlag=FALSE;
				break ;
			
			case	EXCEPTION_DEBUG_EVENT:
				switch (DBEvent.u.Exception.ExceptionRecord.ExceptionCode)
				{
					case	EXCEPTION_BREAKPOINT:
					{
						GetThreadContext(pi.hThread, &Regs) ;
						if(Regs.Eip==dwEntry+1){
							//�жϴ����쳣�¼����ָ�ԭ�����룬����������
							Regs.Eip--;
							WriteProcessMemory(pi.hProcess, (LPVOID)dwEntry,&dwOldbyte, 1,NULL);
						
							for (i=0; dwPatchAddress[i] ; i++)
							{
								ReadProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i], &ReadBuffer, 1, NULL);
			
								if( byOldData[i] == ReadBuffer )    //�жϵ�ַ�����Ƿ���ȷ
								{	
									VirtualProtectEx(pi.hProcess, (LPVOID)dwPatchAddress[i], 1, PAGE_EXECUTE_READWRITE, &OldPro);
									WriteProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i],&byNewData[i], 1,NULL);
									dwCount++;
									ResumeThread(pi.hThread); 
								}	
							}
					
							SetThreadContext(pi.hThread, &Regs) ;
						}
						dwState = DBG_CONTINUE ;
						break;
					}
				}
				break;
		}
		
		ContinueDebugEvent(DBEvent.dwProcessId, DBEvent.dwThreadId, dwState) ;
	} //.end while
	
	CloseHandle(pi.hProcess) ;
	CloseHandle(pi.hThread)  ;
}*/