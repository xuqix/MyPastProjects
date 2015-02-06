
		.386
		.model flat,stdcall
		option casemap:none
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; Include �ļ�����
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include		windows.inc
include		user32.inc
includelib	user32.lib
include		kernel32.inc
includelib	kernel32.lib
;include		unicode.inc
;include		macros.inc


TOTAL_FILE_COUNT  equ   100        ;�����������ļ������������һ��ȫ0�ṹ��β

BinderFileStruct  STRUCT
	dwFileOffset	dword	0		;�������е���ʼƫ��
	dwFileSize      dword   0		;�ļ���С
	szFileName      db	MAX_PATH dup(0) ;�ļ���������Ŀ¼
BinderFileStruct  ENDS

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; ���ݶ�
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		.const
szCaption	db	'��ʾ',0
szText		db	'       �Ƿ��ͷ��ļ�?',0
szFail		db	'���ļ�ʧ��!',0
szSprit		db	'\',0

		.data
dwFlag          dd  0ffffffffh,0ffffffffh,0ffffffffh
dwTotalFile     dd  TOTAL_FILE_COUNT    ;�ļ�����
lpFileList      BinderFileStruct TOTAL_FILE_COUNT dup(<0>)
szMyName	db  MAX_PATH dup(0)   ;�Լ��ľ���·��
szBuffer	db  MAX_PATH dup(0)

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
; �����
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		.code
		

_Handler proc _lpExceptionRecord,_lpSEH,_lpContext,_lpDispatchertext
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
_Handler endp

_CreateDir	proc	_szFileName
	;local	@dwReturn
	local	@szNewDir[MAX_PATH]:byte,@szCurrentDir[MAX_PATH]:byte

	pushad
	invoke	GetCurrentDirectory, MAX_PATH, addr @szCurrentDir
	invoke  lstrcat,addr @szCurrentDir,offset szSprit
	lea	esi,@szCurrentDir
	invoke	lstrcpy,addr @szNewDir,addr @szCurrentDir
	invoke	lstrcat,addr @szNewDir,_szFileName
	lea	edi,@szNewDir
	mov	ecx,MAX_PATH
	repe	cmpsb
	mov	al,byte ptr [edi]
	.while	al
		cmp	al,5ch    ; '\'
		jne	_con
		mov	byte ptr [edi],0
		invoke	CreateDirectory,addr @szNewDir, NULL    ;����Ŀ¼()
		mov	byte ptr [edi],5ch
		_con:
		inc	edi
		mov	al,byte ptr [edi]
	.endw
	popad
	ret
_CreateDir	endp

;�ͷ��ļ�
_ReleaseFile	proc	
	local	@szBuffer[MAX_PATH]:byte
	local	@szPath[MAX_PATH]:byte ,@szCurrentDir[MAX_PATH]:byte
	local	@hFile,@hMap,@lpMemory, @hNewFile,@dwFileSize,@temp
	
	pushad
	invoke	GetModuleFileName,NULL,offset szMyName,MAX_PATH
	invoke	CreateFile,offset szMyName,GENERIC_READ,FILE_SHARE_READ ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL
	cmp	eax,0ffffffffh
	je	_ErrFile
	mov	@hFile,eax
        invoke	CreateFileMapping,@hFile, NULL,PAGE_READONLY,0,0,NULL
        cmp	eax,0
        je	_ErrFile
	mov	@hMap,eax
        invoke	MapViewOfFile,eax, FILE_MAP_READ,0,0,0
        cmp	eax,0
	je	_ErrFile
        mov	@lpMemory,eax              ;����ļ����ڴ��ӳ����ʼλ��
        ;��װSEH
	assume fs:nothing
        push	ebp
        push	offset _ErrForm
        push	offset _Handler
        push	fs:[0]
        mov fs:[0],esp
	
	invoke	GetCurrentDirectory, MAX_PATH, addr @szCurrentDir
	invoke  lstrcat,addr @szCurrentDir,offset szSprit
	invoke	lstrcpy,addr @szPath,addr @szCurrentDir
	mov	esi,offset lpFileList
	assume	esi:ptr BinderFileStruct
	.while	[esi].szFileName
		invoke	_CreateDir,addr [esi].szFileName
		invoke  lstrcat,addr @szCurrentDir,addr [esi].szFileName
		invoke	CreateFile,addr @szCurrentDir,GENERIC_READ or GENERIC_WRITE,FILE_SHARE_READ ,NULL, CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE, NULL
		cmp	eax,0ffffffffh
		je	_ErrForm
		mov	@hNewFile,eax
		mov	ebx,dword ptr [esi].dwFileOffset
		mov	edx,dword ptr [esi].dwFileSize
		mov	eax,@lpMemory
		add	eax,ebx
		lea	ecx,@temp
		invoke	WriteFile,@hNewFile,eax,edx,ecx,NULL
		invoke	CloseHandle,@hNewFile
		invoke	lstrcpy,addr @szCurrentDir,addr @szPath
		add	esi,sizeof BinderFileStruct
	.endw
	jmp	_ok
	_ErrForm:
	 invoke	CloseHandle,@hNewFile
	
	_ErrFile:
	invoke	MessageBox,NULL, offset szFail,offset szCaption, NULL
        _ok:
	invoke	UnmapViewOfFile,@lpMemory
        invoke	CloseHandle,@hMap
        invoke	CloseHandle,@hFile
	
	pop fs:[0]
        add esp,0ch
	popad
	ret
_ReleaseFile	endp

start:
	invoke	MessageBox,NULL,offset szText,offset szCaption,MB_YESNO
	cmp	eax,IDNO
	je	_exit
	invoke	_ReleaseFile
	_exit:
	invoke	ExitProcess,0
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		end	start