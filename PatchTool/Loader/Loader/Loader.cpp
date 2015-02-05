#include <windows.h>
#include "resource.h"

//动态补丁类型
#define SLEEP_PATCH 1	
#define DEBUG_PATCH 2

//动态补丁，最多支持16个补丁地址，特别创建一个段用于补丁工具生成补丁时修正变量

#pragma data_seg(".sdata")
DWORD	dwTypeOfPatch = 0;					//指示补丁类型
DWORD	dwPatchNum  = 0 ;						//补丁数量
TCHAR	szFileName[MAX_PATH] = { 0 };

DWORD	dwPatchAddress[16] = { 0 };				//补丁地址
BYTE	byOldData[16] = { 0 };					//补丁处旧数据和新数据
BYTE	byNewData[16] = { 0 };	

#pragma data_seg()
#pragma comment(linker, "/SECTION:.sdata,ERW")

//时间记录
DWORD	dwTickCount;

///////////////////////////类型1,利用进程读写机制不断测试补丁位置///////////////////////////
////////////////好处是能无视一些壳，弊端是仅仅是停一段时间检测,可能失败,不稳定//////////////
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
        MessageBox(NULL, TEXT("请确定补丁是否在目标文件目录下."), TEXT("提示"), MB_OK); 
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
			
			if( byOldData[i] == ReadBuffer )    //判断地址数据是否正确
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
		//if (dwCount == dwPatchNum)  //所有补丁完毕，退出循环
		//	bContinueRun	= FALSE;
		dwCurrentTick	= GetTickCount();
		if ( (dwCurrentTick - dwTickCount) > 6*1000)  //如果10秒还没退出，超时
		{
			MessageBox( NULL, TEXT("超时,补丁失败,指示的补丁数据可能有错误"), 0, 0);
			TerminateProcess(pi.hProcess, 0);
			bContinueRun	= FALSE; ;
		}
	}

    CloseHandle(pi.hProcess); 
    CloseHandle(pi.hThread); 
}

/////////////////////////////////////////利用调试寄存器打补丁///////////////////////////////////////////////////////
/////////////打此类补丁应在补丁地址第一个地址填上希望中断的地址以确保所有地址数据已解码，以保证补丁正确性///////////
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
        MessageBox(NULL, TEXT("请确定补丁是否在目标文件目录下."), TEXT("提示"), MB_OK);
        return ; 
    } 
	
	DEBUG_EVENT		DBEvent ;
	CONTEXT			Regs ;
	DWORD			dwSSCnt , dwNum;
	
	dwSSCnt = 0 ;

	Regs.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS ;

	//使进程在Single Step模式下运行,每执行一条指令就会给调试进程发送EXCEPTION_SINGLE_STEP
	//需要注意的是，收到这个消息后，如果还想继续让程序Single Step下去，是需要重新设置一次SF位的
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
						//当收到第一个EXCEPTION_SINGLE_STEP异常信号，表示中断在程序的第一条指令，即入口点
						//把Dr0设置成程序的入口地址
						
						GetThreadContext(pi.hThread,&Regs);
						
						Regs.Dr0=Regs.Eax;
						Regs.Dr7=0x101;
						
						SetThreadContext(pi.hThread,&Regs);
						
					}
					else if (dwSSCnt == 2)
					{
						//第二次中断在起先设置的补丁点，设置硬件断点，后面所有的地址均在运行到此处时补丁

						GetThreadContext(pi.hThread, &Regs) ;
						Regs.Dr0 = dwPatchAddress[0];
						Regs.Dr7 = 0x101 ;
						SetThreadContext(pi.hThread, &Regs) ;
					}
					else if (dwSSCnt == 3)
					{
						//第三次中断，己到补丁指示的地址
						GetThreadContext(pi.hThread, &Regs) ;
						Regs.Dr0 = Regs.Dr7 = 0 ;
					
						//从下标1开始才是需要打补丁的地址
						for (i=1; dwPatchAddress[i] ; i++)
						{
							ReadProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i], &ReadBuffer, 1, NULL);
			
							if( byOldData[i-1] == ReadBuffer )    //判断地址数据是否正确
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
						if (dwCount != dwNum)   //补丁成功数与需要补丁数不同
							MessageBox(NULL, TEXT("补丁字节数不一致,补丁可能失败"), TEXT("提示"), 0);
						
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

//////////////////////////////提升当前进程权限（调试权限）///////////////////////////////////
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

//取消优化以免跳转无效
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
        MessageBox(NULL, TEXT("请确定补丁是否在目标文件目录下."), NULL, MB_OK);
        return ; 
	}

	if( !CreateProcess(SZFILENAME, NULL, NULL, NULL, FALSE, DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi) ) 
	{
        MessageBox(NULL, TEXT("请确定补丁是否在目标文件目录下."), NULL, MB_OK);
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
				//如果进程开始，则将断点地址的代码改为INT3中断,同时备份原机器码
				ReadProcessMemory(pi.hProcess, (LPCVOID)(dwEntry), &dwOldbyte, sizeof(dwOldbyte), NULL) ;
				VirtualProtectEx(pi.hProcess, (LPVOID)dwEntry, 1, PAGE_EXECUTE_READWRITE, &Oldpp);
				WriteProcessMemory(pi.hProcess, (LPVOID)dwEntry,&dwINT3code, 1,NULL);	//打补丁
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
							//中断触发异常事件，恢复原机器码，并读出数据
							Regs.Eip--;
							WriteProcessMemory(pi.hProcess, (LPVOID)dwEntry,&dwOldbyte, 1,NULL);
						
							for (i=0; dwPatchAddress[i] ; i++)
							{
								ReadProcessMemory(pi.hProcess, (LPVOID)dwPatchAddress[i], &ReadBuffer, 1, NULL);
			
								if( byOldData[i] == ReadBuffer )    //判断地址数据是否正确
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