#ifndef KERNEL_H
#define KERNEL_H

#include <windows.h>
#include <windowsx.h>
#include "ApiMacro.h"
#include "MsgQueue.h"
#include "MsgProcotol.h"
#include "NetModule.h"
#include "encrypt.h"
#include "ODBCModule.h"



//每个连接将产生一个如下客户端信息结构
struct CLIENT
{
	DWORD	dwMessageID;	//已发送的消息编号
	char	szUserName[16];	//用户名
	DWORD	dwLastTime;		//通讯线路最近活动时间
};

//通讯线路的检测，以保证正常连接和释放状态
//返回值为0则无活动，释放连接
int		LinkCheck(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient);

//禁言选中用户
int		BanUser();

//踢出选中用户
int		KickUser();

//列表中添加用户信息
void	AddUserInfo(char* szUserName, char* szIP, char* szLoginTime);

//删除列表中选中行的信息
void	DelUserInfo();

//删除列表中指定用户信息
void	DelUserInfo(char* szUserName);

//清空列表
void	CleanUserInfo(HWND hListView);

//用户登录检测
BOOL	CheckUser(PACKET* lpPacket);

//发送对客户端进行用户列表更新的数据包
BOOL	update(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient);

//服务线程，每个客户端连接将产生一个服务线程,参数为本连接的套接字
unsigned __stdcall ServerThread(void* param);

//监听线程
unsigned __stdcall ListenThread(void* param);


//////////////////////负责处理各种消息的函数组/////////////////////////////
///////////////////函数原型:int XXX_HANDLE(PACKET*)//////////////////////////
//////////////////////////////XXX为消息名//////////////////////////////////
int CMD_MSG_UP_HANDLE	  (PACKET& packet, CLIENT& client);
int CMD_MSG_PRIVATE_HANDLE(PACKET& packet, CLIENT& client);

#endif