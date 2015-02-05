#ifndef HANDLE_H
#define HANDLE_H

#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include "ApiMacro.h"
#include "MsgProcotol.h"
#include "NetModule.h"
#include "encrypt.h"

#define LOGIN_SUCCESS	0
#define LOGIN_FAIL		1
#define LOGIN_ING		2
#define LOGIN_TIMEOUT	3
#define LOGIN_CHAT		4
#define LOGIN_CANCEL	-1	//退出 

#define LOGIN_SUCCESS_ENCRYPT 0xff

//用于注册新用户的函数
BOOL RegisterNewUser(HWND hDlg);	


//客户端工作线程,负责收发消息
unsigned __stdcall WorkThread(void* param);


//////////////////////负责处理各种消息的函数组/////////////////////////////
/////////////////////原型:int XXX_HANDLE(PACKET*)//////////////////////////
//////////////////////////////XXX为消息名//////////////////////////////////
int CMD_MSG_DOWN_HANDLE (PACKET* packet);
int CMD_SYS_INFO_HANDLE (PACKET* packet);
int CMD_USER_LIST_HANDLE(PACKET* packet);
int CMD_MSG_PRIVATE_HANDLE(PACKET* packet);
int CMD_USER_BAN_HANDLE (PACKET* packet);
int CMD_KICK_OUT_HANDLE (PACKET* packet);


#endif