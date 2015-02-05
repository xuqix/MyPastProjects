		.386
		.model flat,stdcall
		option casemap:none
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; Include 文件定义
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include		c:\masm32\include\windows.inc

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; 导出变量供补丁工具使用
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

PUBLIC	Appendcode_Start	;附加代码起始处	
PUBLIC	Appendcode_End		;附加代码结束处
PUBLIC	Patch_Data		;补丁数据处

		.code
Appendcode_Start LABEL		DWORD

	jmp	_NewEntry

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;重要的函数名，为兼容WIN7 kernelbase.dll，使用LoadLibraryExA函数
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
szLoadLibraryExA db	'LoadLibraryExA',0  
szGetProcAddress db	'GetProcAddress',0

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;补丁功能代码需要的DLL，函数名，字符串等全局变量定义,以下为测试用
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;szUser32	 db	'user32',0
;szMessageBoxA	 db	'MessageBoxA',0
;szCaption	db	'恭喜',0
;szText		db	'代码插入成功!',0


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
;用PEB获取基址的方法,WIN7中获得的实际是kernelbase.dll的基地址
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

;补丁所需要的函数和全局变量
szCreateThread		db	'CreateThread',0
szGetTickCount		db	'GetTickCount',0
szVirtualProtect	db	'VirtualProtect',0
lpGetTickCount		dd	0
StartCount		dd	0

;以下变量需要补丁程序修正
Patch_Data	LABEL		DWORD
;dwTypeOfPatch	dd	0				;指示补丁类型
dwPatchNum	dd	0				;补丁数量	
dwPatchAddress	dd	16 dup(0)			;补丁地址
byOldData	db	16 dup(0)			;补丁处旧数据和新数据
byNewData	db	16 dup(0) 			

_Thread	proc	_lpVirtualProtect
	local	@lpGetTickCount,@temp,@StartCount,@num

	pushad
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	mov	edx,dword ptr [ebx+offset lpGetTickCount]
	mov	@lpGetTickCount,edx
	mov	edx,dword ptr [ebx+offset StartCount]
	mov	@StartCount,edx

	mov	ecx,dword ptr [ebx+offset dwPatchNum]
	mov	@num,ecx
	.while	TRUE
		call	@lpGetTickCount
		sub	eax,@StartCount
		cmp	eax,493e0h		;大于五分钟则超时退出线程
		jg	_exit
		;开始检测补丁地址
		lea	esi,dword ptr [ebx+offset dwPatchAddress]  ;指向补丁地址
		lea	edi,dword ptr [ebx+offset byOldData]	   ;补丁处旧数据
		lea	edx,dword ptr [ebx+offset byNewData]	   ;补丁处新数据
		;检测所有补丁处字节
		mov	ecx,dword ptr [ebx+offset dwPatchNum]
		_peek:
			push	ecx
			mov	ecx,dword ptr [esi]	
			xor	eax,eax
			mov	al,byte  ptr [ecx]		;取补丁处数据
			cmp	al,byte  ptr [edi]		;补丁处是否解码
			jne	_mismatch
			
			;更改页面为读写执行,以确保补丁地址处拥有读写执行权限
			pushad
			lea		eax,@temp
			push	eax
			push	40h
			push	100h
			push	ecx
			call	_lpVirtualProtect
			popad		
			mov	al,byte ptr [edx]	;进行补丁
			mov	byte ptr [ecx],al
			dec	@num
			_mismatch:	
			inc	edi
			inc	edx
			add	esi,4
			pop	ecx
			cmp	@num,0
			je	_exit
		loop	_peek
	.endw
	_exit:
	popad
	ret
_Thread	endp


;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;补丁功能部分
;_dwKernelBase:		kernel32.dll基址
;_lpGetProcAddress:	GetProcAddress地址
;_lpLoadLibraryA	LoadLibraryA或LoadLibraryExA地址
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_Patch	proc	_dwKernelBase,_lpGetProcAddress,_lpLoadLib
		local	@lpVirtualProtect		;local	@hUser32,@lpMessageBoxA
		local	@temp

		pushad
		;以下注释掉的为测试代码
		;lea	edx,dword ptr [ebx+offset szUser32]
		;push	0
		;push	0
		;push	edx
		;call	_lpLoadLib
		;.if	eax
		;	mov	@hUser32,eax
		;	lea	edx,dword ptr [ebx+offset szMessageBoxA]
		;	push	edx
		;	push	eax
		;	call	_lpGetProcAddress
		;	.if	eax
		;		mov	@lpMessageBoxA,eax
		;	.endif
		;.endif
		;.if	@lpMessageBoxA
		;push	MB_YESNO
		;lea	edx,dword ptr [ebx+offset szCaption]
		;push	edx
		;lea	edx,dword ptr [ebx+offset szText]
		;push	edx
		;push	NULL
		;call	@lpMessageBoxA
		;.endif
		lea	edx,dword ptr [ebx+offset szVirtualProtect]
		push	edx
		push	_dwKernelBase
		call	_lpGetProcAddress
		cmp	eax,0
		je	_exit
		mov	@lpVirtualProtect,eax
		lea	edx,@temp
		push	edx
		push	40h
		push	1000h
		lea	edx,dword ptr [ebx+offset lpGetTickCount]
		push	edx
		call	@lpVirtualProtect		;确保全局变量位置可写
		lea	edx,dword ptr [ebx+offset szGetTickCount]
		push	edx
		push	_dwKernelBase
		call	_lpGetProcAddress
		.if	eax
			mov	dword ptr [ebx+offset lpGetTickCount],eax
			call	eax
			mov	dword ptr [ebx+offset StartCount],eax
			lea	edx,dword ptr [ebx+offset szCreateThread]
			push	edx
			push	_dwKernelBase
			call	_lpGetProcAddress
			.if	eax	
				lea		edx,@temp
				push	edx
				push	0
				push	@lpVirtualProtect				;线程参数为VirtualProtect函数的地址
				lea	edx,dword ptr [ebx+offset _Thread]
				push	edx
				push	0
				push	0
				call	eax	;创建监测线程进行补丁
			.endif
		.endif
		_exit:
		popad
		ret
_Patch	endp

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
	jmpToStart db 0E9h,0F0h,0FFh,0ffh,0ffh	;需要补丁程序修正
	ret

Appendcode_End LABEL		DWORD
end