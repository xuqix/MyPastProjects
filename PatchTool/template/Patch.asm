;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;补丁功能代码需要的DLL，函数名，字符串等全局变量定义
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
szUser32	 db	'user32',0

szMessageBoxA	 db	'MessageBoxA',0

szCaption	db	'恭喜',0
szText		db	'非导入表调用成功!',0
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;补丁功能部分
;_dwKernelBase:		kernel32.dll基址
;_lpGetProcAddress:	GetProcAddress地址
;_lpLoadLibraryA	LoadLibraryA或LoadLibraryExA地址
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