


Appendcode_Start LABEL		DWORD

	jmp	_NewEntry
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;重要的函数名，为兼容WIN7 kernelbase.dll，增加LoadLibraryExA函数
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
szLoadLibraryExA db	'LoadLibraryExA',0
;szLoadLibraryA   db	'LoadLibraryA',0       
szGetProcAddress db	'GetProcAddress',0

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;SEH错误Handler
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
;获取kernel32.dll基地址,2种获取方法自行选择
;PS:用PEB获取最好使用LoadLibraryExA函数以兼容WIN7
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;_GetKernel32Base proc uses edi esi ebx _dwEsp
;	call	@F
;	@@:
;	pop	ebx
;	sub	ebx,offset @B
;
;	;安装SEH
;	assume	fs:nothing
;	push	ebp
;	lea	eax, [ebx+offset _safeplace]
;	push	eax
;	lea	eax,[ebx + offset _SEHHandler]
;	push	eax
;	push	fs:[0]
;	mov	fs:[0],esp
;
;	mov	eax,_dwEsp
;	and	eax,0ffff0000h
;
;	.while	eax>=70000000h
;		.if word ptr [eax] == IMAGE_DOS_SIGNATURE
;			mov	edi,eax
;			add	edi,[eax+03ch]
;			.if word ptr [edi] == IMAGE_NT_SIGNATURE
;				jmp	find
;			.endif
;		.endif
;		_safeplace:
;		sub	eax,10000h
;	.endw
;	mov	eax,0
;	find:
;	pop	fs:[0]
;	add	esp,0ch
;	ret
;_GetKernel32Base endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;用PEB获取基址的方法
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_GetKernel32Base proc
	local	@dwRet

	pushad
	
	assume fs:nothing
	mov eax,fs:[30h]	;获取PEB所在地址
	mov eax,[eax+0ch]	;获取PEB_LDR_DATA 结构指针
	mov esi,[eax+1ch]	;获取InInitializationOrderModuleList 链表头
				;第一个LDR_MODULE节点InInitializationOrderModuleList成员的指针
	lodsd			;获取双向链表当前节点后继的指针
	mov eax,[eax+8]		;获取kernel32.dll的基地址
	mov @dwRet,eax
	popad
	
	mov eax,@dwRet
	ret
_GetKernel32Base endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;查找导出表获取制定API地址
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
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

include	Patch.asm

_start	proc
	local	@dwKernel32Base
	local	@lpGetProcAddress,@lpLoadLibraryExA
	
	pushad

	call	_GetKernel32Base
	.if	eax
		mov	@dwKernel32Base,eax
		lea	edx,dword ptr [ebx+offset szGetProcAddress]
		push	edx
		push	eax
		call	_GetApi
		mov	@lpGetProcAddress,eax
	.endif
	.if	@lpGetProcAddress
		lea	edx,dword ptr [ebx+offset szLoadLibraryExA]
		push	edx
		push	@dwKernel32Base
		call	@lpGetProcAddress
		.if	eax
			mov	@lpLoadLibraryExA,eax
			push	eax
			push	@lpGetProcAddress
			push	@dwKernel32Base
			call	_Patch
		.endif
	.endif

	popad
	xor	eax,eax
	ret
_start	endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;PE文件新入口
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_NewEntry:
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	call	_start
	;ret
	jmpToStart db 0E9h,0F0h,0FFh,0ffh,0ffh	;需要修正
	ret

Appendcode_End LABEL		DWORD