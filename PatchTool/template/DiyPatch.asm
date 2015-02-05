
		.386
		.model flat,stdcall
		option casemap:none
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; Include 文件定义
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include		windows.inc
include		user32.inc
includelib	user32.lib
include		kernel32.inc
includelib	kernel32.lib
include		comdlg32.inc
includelib	comdlg32.lib


IDD_DIALOG      equ               101
IDB_BITMAP1      equ               102
ICO_MAIN        equ               103
IDC_CHECK1       equ               1001
IDC_BACKUP       equ               1001
ID_OK		 equ		   1005
ID_CANCEL	 equ		   2005


		.data?
hInstance	dd	?
hRichEdit	dd	?
hWinMain	dd	?
hWinEdit	dd	?
szFileName	db	MAX_PATH dup (?)
szBackupName	db	MAX_PATH dup (?)
hFile		dd	?
hMap		dd	?

		.const
szExtPe		db	'PE Files',0,'*.exe;*.dll;*.scr;*.fon;*.drv',0
		db	'All Files(*.*)',0,'*.*',0,0
szErr		db	'文件格式错误!',0
szErrFormat	db	'这个文件不是PE格式的文件!',0
szBak		db	'.bak',0
sz		db	'恭喜',0
szSuccessful	db	'补丁成功！',0
szFail		db	'补丁失败',0




;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; 代码段
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		.code
include	addcode.asm


;文件偏移转为RVA
_offsetToRva	proc	_lpMemory, _dwOffset
		local	@lpNtHeaders,@lpSectionHeader
		local	@dwReturn,@dwSectionNum
		pushad
		mov	esi,_lpMemory
		mov	@lpNtHeaders,esi
		add	esi,3ch
		mov	eax,dword ptr [esi]
		add	@lpNtHeaders,eax
		mov	esi,@lpNtHeaders
		assume	esi:ptr IMAGE_NT_HEADERS
		movzx	eax,[esi].FileHeader.NumberOfSections
		mov	@dwSectionNum,eax
		add	esi,sizeof IMAGE_NT_HEADERS
		;mov	@lpSectionHeader,esi
		mov	ecx,@dwSectionNum
		;mov	esi,@lpSectionHeader
		assume	esi:ptr IMAGE_SECTION_HEADER
		_loop:
		mov	eax,[esi].PointerToRawData
		mov	ebx,[esi].SizeOfRawData
		add	ebx,eax
		.if	_dwOffset >= eax && _dwOffset < ebx
			mov	edx,[esi].VirtualAddress
			mov	ebx,_dwOffset
			sub	ebx,eax
			add	ebx,edx
			mov	@dwReturn,ebx
			jmp	_find
		.endif
		add	esi,sizeof IMAGE_SECTION_HEADER
		loop	_loop
		mov	@dwReturn,0ffffffffh
		_find:
		popad
		mov	eax,@dwReturn
		ret
_offsetToRva	endp

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
; 计算按照指定值对齐后的数值
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
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;将代码加入文件最后区段，因为这种方式基本不会失败
_AddCode	proc	_lpFile,_lpPeHead,_dwSize
		local	@dwPatchSize,@dwFileAlignSize,@dwFinalSize,@lpMemory,@dwOldEntry,@dwRead,@dwReturn
		local	@dwSectionAlign,@dwFileAlign,@dwSectionNum,@lpSectionHeader
		pushad
		mov	@dwPatchSize,offset Appendcode_End - offset Appendcode_Start
		mov	esi,_lpPeHead
		assume	esi:ptr IMAGE_NT_HEADERS
		movzx	eax,[esi].FileHeader.NumberOfSections
		mov	@dwSectionNum,eax
		mov	eax,[esi].OptionalHeader.FileAlignment
		mov	@dwFileAlign,eax
		mov	eax,[esi].OptionalHeader.SectionAlignment
		mov	@dwSectionAlign,eax
		invoke	_Align,_dwSize,@dwFileAlign
		mov	@dwFileAlignSize,eax
		add	eax,@dwPatchSize
		invoke	_Align,eax,@dwFileAlign
		mov	@dwFinalSize,eax
		invoke	VirtualAlloc,NULL,eax,MEM_COMMIT,PAGE_READWRITE
		cmp	eax,0
		je	_exit
		;复制数据
		mov	@lpMemory,eax
		mov	edi,eax
		mov	esi,_lpFile
		mov	ecx,_dwSize
		rep	movsb
		mov	edi,@lpMemory
		add	edi,@dwFileAlignSize
		mov	esi,offset Appendcode_Start
		mov	ecx,@dwPatchSize
		rep	movsb
		;复制完毕，开始修正PE头
		mov	eax,_lpPeHead
		sub	eax,_lpFile
		mov	esi,@lpMemory
		add	esi,eax
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
		invoke	UnmapViewOfFile,_lpFile
		invoke	CloseHandle,hMap
		invoke	CloseHandle,hFile
		invoke	CreateFile,offset szFileName, GENERIC_READ or GENERIC_WRITE, FILE_SHARE_READ, NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,0
		cmp	eax,0ffffffffh
		je	_exit
		mov	@dwRead,eax
		invoke	WriteFile,@dwRead,@lpMemory,@dwFinalSize,addr @dwRead,NULL
		mov	@dwReturn,eax
		jmp	_ok
		_exit:
		mov	@dwReturn,0
		_ok:
		popad
		mov	eax,@dwReturn
		ret
_AddCode	endp

;SEH错误处理
_Handler	proc	_lpExceptionRecord,_lpSEH,_lpContext,_lpDispatcherContext

		pushad
		mov	esi,_lpExceptionRecord
		mov	edi,_lpContext
		assume	esi:ptr EXCEPTION_RECORD,edi:ptr CONTEXT
		mov	eax,_lpSEH
		push	[eax + 0ch]
		pop	[edi].regEbp
		push	[eax + 8]
		pop	[edi].regEip
		push	eax
		pop	[edi].regEsp
		assume	esi:nothing,edi:nothing
		popad
		mov	eax,ExceptionContinueExecution
		ret

_Handler	endp
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_kernel		proc
		local	@stOF:OPENFILENAME
		local	@hFile,@dwFileSize,@hMapFile,@lpMemory

		invoke	RtlZeroMemory,addr @stOF,sizeof @stOF
		mov	@stOF.lStructSize,sizeof @stOF
		push	hWinMain
		pop	@stOF.hwndOwner
		mov	@stOF.lpstrFilter,offset szExtPe
		mov	@stOF.lpstrFile,offset szFileName
		mov	@stOF.nMaxFile,MAX_PATH
		mov	@stOF.Flags,OFN_PATHMUSTEXIST or OFN_FILEMUSTEXIST
		invoke	GetOpenFileName,addr @stOF
		.if	! eax
			jmp	@F
		.endif
;********************************************************************
; 打开文件并建立文件 Mapping
;********************************************************************
		invoke	CreateFile,addr szFileName,GENERIC_READ,FILE_SHARE_READ or \
			FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL
		.if	eax !=	INVALID_HANDLE_VALUE
			mov	@hFile,eax
			invoke	GetFileSize,eax,NULL
			mov	@dwFileSize,eax
			.if	eax
				invoke	CreateFileMapping,@hFile,NULL,PAGE_READONLY,0,0,NULL
				.if	eax
					mov	@hMapFile,eax
					invoke	MapViewOfFile,eax,FILE_MAP_READ,0,0,0
					.if	eax
						mov	@lpMemory,eax
;********************************************************************
; 创建用于错误处理的 SEH 结构
;********************************************************************
						assume	fs:nothing
						push	ebp
						push	offset _ErrFormat
						push	offset _Handler
						push	fs:[0]
						mov	fs:[0],esp
;********************************************************************
; 检测 PE 文件是否有效
;********************************************************************
						mov	esi,@lpMemory
						assume	esi:ptr IMAGE_DOS_HEADER
						.if	[esi].e_magic != IMAGE_DOS_SIGNATURE
							jmp	_ErrFormat
						.endif
						add	esi,[esi].e_lfanew
						assume	esi:ptr IMAGE_NT_HEADERS
						.if	[esi].Signature != IMAGE_NT_SIGNATURE
							jmp	_ErrFormat
						.endif
						invoke	SendDlgItemMessage,hWinMain, IDC_BACKUP, BM_GETCHECK, 0, 0 
						cmp	eax,BST_CHECKED
						jne	_addcode
						invoke  lstrcpy,offset szBackupName, offset szFileName
						invoke  lstrcat,offset szBackupName, offset szBak
						invoke  CopyFile,offset szFileName,offset szBackupName,TRUE
						_addcode:
						mov	eax,@hFile
						mov	hFile,eax
						mov	eax,@hMapFile
						mov	hMap,eax
						invoke	_AddCode,@lpMemory,esi,@dwFileSize
						cmp	eax,0
						jne	_ErrorExit
						invoke	MessageBox,hWinMain,offset szFail,NULL,MB_OK
						jmp	_e
						_ErrFormat:
						invoke	MessageBox,hWinMain,addr szErrFormat,NULL,MB_OK
						jmp	_e
						_ErrorExit:
						invoke	MessageBox,hWinMain,offset szSuccessful,offset sz,MB_OK
						jmp	_o
						_e:
						pop	fs:[0]
						add	esp,0ch
						
					.endif
				.endif
			.endif
		.endif
		_o:
@@:
		invoke	UnmapViewOfFile,@lpMemory
		invoke	CloseHandle,@hMapFile
		invoke	CloseHandle,@hFile
		ret

_kernel		endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_ProcDlgMain	proc	uses ebx edi esi hWnd,wMsg,wParam,lParam

		mov	eax,wMsg
		.if	eax == WM_CLOSE
			invoke	EndDialog,hWnd,NULL
		.elseif	eax == WM_INITDIALOG
			push	hWnd
			pop	hWinMain
			invoke	CheckDlgButton,hWinMain, IDC_BACKUP, BST_CHECKED
		.elseif	eax == WM_COMMAND
			mov	eax,wParam
			.if	ax ==	ID_OK
				call	_kernel
			.elseif	ax ==	ID_CANCEL
				invoke	EndDialog,hWnd,NULL
				
			.endif
		.else
			mov	eax,FALSE
			ret
		.endif
		mov	eax,TRUE
		ret

_ProcDlgMain	endp
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
start:
		invoke	GetModuleHandle,NULL
		mov	hInstance,eax
		invoke	DialogBoxParam,hInstance,IDD_DIALOG,NULL,offset _ProcDlgMain,NULL
		invoke	ExitProcess,NULL
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		end	start

