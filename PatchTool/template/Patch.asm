;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;�������ܴ�����Ҫ��DLL�����������ַ�����ȫ�ֱ�������
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
szUser32	 db	'user32',0

szMessageBoxA	 db	'MessageBoxA',0

szCaption	db	'��ϲ',0
szText		db	'�ǵ�������óɹ�!',0
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;�������ܲ���
;_dwKernelBase:		kernel32.dll��ַ
;_lpGetProcAddress:	GetProcAddress��ַ
;_lpLoadLibraryA	LoadLibraryA��LoadLibraryExA��ַ
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
_Patch	proc	_dwKernelBase,_lpGetProcAddress,_lpLoadLibraryExA
		local	@hUser32,@lpMessageBoxA
		
		pushad
		lea	edx,dword ptr [ebx+offset szUser32]
		push	0
		push	0
		push	edx
		call	_lpLoadLibraryExA
		.if	eax
			mov	@hUser32,eax
			lea	edx,dword ptr [ebx+offset szMessageBoxA]
			push	edx
			push	eax
			call	_lpGetProcAddress
			.if	eax
				mov	@lpMessageBoxA,eax
			.endif
		.endif
		.if	@lpMessageBoxA
		push	MB_YESNO
		lea	edx,dword ptr [ebx+offset szCaption]
		push	edx
		lea	edx,dword ptr [ebx+offset szText]
		push	edx
		push	NULL
		call	@lpMessageBoxA
		.endif
		popad
		ret
_Patch	endp