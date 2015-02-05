		.386
		.model flat,stdcall
		option casemap:none
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; Include �ļ�����
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include		c:\masm32\include\windows.inc

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; ������������������ʹ��
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

PUBLIC	Appendcode_Start	;���Ӵ�����ʼ��	
PUBLIC	Appendcode_End		;���Ӵ��������
PUBLIC	Patch_Data		;�������ݴ�

		.code
Appendcode_Start LABEL		DWORD

	jmp	_NewEntry

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;��Ҫ�ĺ�������Ϊ����WIN7 kernelbase.dll��ʹ��LoadLibraryExA����
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
szLoadLibraryExA db	'LoadLibraryExA',0  
szGetProcAddress db	'GetProcAddress',0

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;�������ܴ�����Ҫ��DLL�����������ַ�����ȫ�ֱ�������,����Ϊ������
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;szUser32	 db	'user32',0
;szMessageBoxA	 db	'MessageBoxA',0
;szCaption	db	'��ϲ',0
;szText		db	'�������ɹ�!',0


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
;_GetKernel32Base proc uses edi esi ebx _dwEsp
;	call	@F
;	@@:
;	pop	ebx
;	sub	ebx,offset @B
;
;	;��װSEH
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
;��PEB��ȡ��ַ�ķ���,WIN7�л�õ�ʵ����kernelbase.dll�Ļ���ַ
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_GetKernel32Base proc
	local	@dwRet

	pushad
	
	assume fs:nothing
	mov eax,fs:[30h]	;��ȡPEB���ڵ�ַ
	mov eax,[eax+0ch]	;��ȡPEB_LDR_DATA �ṹָ��
	mov esi,[eax+1ch]	;��ȡInInitializationOrderModuleList ����ͷ
				;��һ��LDR_MODULE�ڵ�InInitializationOrderModuleList��Ա��ָ��
	lodsd			;��ȡ˫������ǰ�ڵ��̵�ָ��
	mov eax,[eax+8]		;��ȡkernel32.dll�Ļ���ַ
	mov @dwRet,eax
	popad
	
	mov eax,@dwRet
	ret
_GetKernel32Base endp

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;���ҵ������ȡ�ƶ�API��ַ
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

;��������Ҫ�ĺ�����ȫ�ֱ���
szCreateThread		db	'CreateThread',0
szGetTickCount		db	'GetTickCount',0
szVirtualProtect	db	'VirtualProtect',0
lpGetTickCount		dd	0
StartCount		dd	0

;���±�����Ҫ������������
Patch_Data	LABEL		DWORD
;dwTypeOfPatch	dd	0				;ָʾ��������
dwPatchNum	dd	0				;��������	
dwPatchAddress	dd	16 dup(0)			;������ַ
byOldData	db	16 dup(0)			;�����������ݺ�������
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
		cmp	eax,493e0h		;�����������ʱ�˳��߳�
		jg	_exit
		;��ʼ��ⲹ����ַ
		lea	esi,dword ptr [ebx+offset dwPatchAddress]  ;ָ�򲹶���ַ
		lea	edi,dword ptr [ebx+offset byOldData]	   ;������������
		lea	edx,dword ptr [ebx+offset byNewData]	   ;������������
		;������в������ֽ�
		mov	ecx,dword ptr [ebx+offset dwPatchNum]
		_peek:
			push	ecx
			mov	ecx,dword ptr [esi]	
			xor	eax,eax
			mov	al,byte  ptr [ecx]		;ȡ����������
			cmp	al,byte  ptr [edi]		;�������Ƿ����
			jne	_mismatch
			
			;����ҳ��Ϊ��дִ��,��ȷ��������ַ��ӵ�ж�дִ��Ȩ��
			pushad
			lea		eax,@temp
			push	eax
			push	40h
			push	100h
			push	ecx
			call	_lpVirtualProtect
			popad		
			mov	al,byte ptr [edx]	;���в���
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
;�������ܲ���
;_dwKernelBase:		kernel32.dll��ַ
;_lpGetProcAddress:	GetProcAddress��ַ
;_lpLoadLibraryA	LoadLibraryA��LoadLibraryExA��ַ
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_Patch	proc	_dwKernelBase,_lpGetProcAddress,_lpLoadLib
		local	@lpVirtualProtect		;local	@hUser32,@lpMessageBoxA
		local	@temp

		pushad
		;����ע�͵���Ϊ���Դ���
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
		call	@lpVirtualProtect		;ȷ��ȫ�ֱ���λ�ÿ�д
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
				push	@lpVirtualProtect				;�̲߳���ΪVirtualProtect�����ĵ�ַ
				lea	edx,dword ptr [ebx+offset _Thread]
				push	edx
				push	0
				push	0
				call	eax	;��������߳̽��в���
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
;PE�ļ������
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_NewEntry:
	call	@F
	@@:
	pop	ebx
	sub	ebx,@B
	call	_start
	;ret
	jmpToStart db 0E9h,0F0h,0FFh,0ffh,0ffh	;��Ҫ������������
	ret

Appendcode_End LABEL		DWORD
end