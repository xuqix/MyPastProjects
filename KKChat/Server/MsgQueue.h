#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <windows.h>
#include <iostream>
#include <map>
#include <utility>
#include <string>
#include <queue>
#include "MsgProcotol.h"

using namespace std;

//消息队列定义，包装deque，并保证线程安全
//消息存储内容结构：消息编号、发送者、接收者、内容
struct Msg
{
	Msg() { dwMessageID = 0; Timestamp = 0; memset(szSender,0,sizeof(szSender)); memset(szAccepter, 0, sizeof(szAccepter)); }
	//接收者默认为空表示广播发送给所有人
	Msg( /*DWORD dwMsgID,*/time_t t, char* szSend, char* szConten, DWORD cmd = CMD_MSG_DOWN, char* szAccept=NULL)
	{
		//dwMessageID	= dwMsgID;
		Timestamp	= t;
		CMD_ID		= cmd;
		if (szSend)		strcpy(szSender, szSend);
		if (szAccept)	strcpy(szAccepter, szAccept);
		if (szConten)	strcpy(szContent, szConten);
	}
	DWORD	dwMessageID;			//当前消息的消息编号(不断增加)
	time_t	Timestamp;				//时间戳
	char	szSender[16];			//当前消息的发送者
	char	szAccepter[16];			//当前消息的接收者
	char	szContent[MAX_LENGTH*2];//消息内容
	DWORD	CMD_ID;					//消息类型，如不特别指定默认为CMD_MSG_DOWN
};


//存储消息的队列，为避免队列过于庞大设置限制队列大小最大为128，限制大小需要对队头队尾进行处理，所以选择双端队列
//由于队列在多线程中操作，选择使用临界区对象来完成队列的线程安全
#define MAX_SIZE	128
class MsgQueue
{
public:
	DWORD	ID;					//消息编号

	MsgQueue()	{	InitializeCriticalSection(&stCS); ID = 1; }
	int		size()  {	return q.size();	}
	bool	empty() {	return q.empty();	}
	void	push_back(Msg	msg)
	{
		EnterCriticalSection(&stCS);

		if(size()==MAX_SIZE)
		{
			q.pop_front();
		}
		msg.dwMessageID	= ++ID;
		q.push_back(msg);

		LeaveCriticalSection(&stCS);
	}

	//获得指定消息编号的消息，如果小于最小编号，则返回最小的那个(说明存在网络延迟导致的消息丢失)，大于则返回一个消息ID为0的结构
	Msg		get_msg(DWORD	dwMsgID)	
	{
		EnterCriticalSection(&stCS);
		
		Msg	m;
		if(dwMsgID<q[0].dwMessageID)
			m = q[0];
		else if(dwMsgID>q[q.size()-1].dwMessageID)
		{
			m = Msg(0, 0, 0);
			m.dwMessageID	= 0;
		}
		else
			m = q[dwMsgID - q[0].dwMessageID];

		LeaveCriticalSection(&stCS);

		return m;
	}
	~MsgQueue()	{ DeleteCriticalSection(&stCS); }
private:
	deque<Msg>	q;				//双端队列
	CRITICAL_SECTION	stCS;	//临界区对象
};

struct UserInfo
{
	SOCKET	hSocket;	//此用户所使用连接的套接字
	//accept( client , (sockaddr*)&client_addr, ....); 
	DWORD	IP;			//IP地址的32位表示
	time_t	Timestamp;	//最后一次登录时间
};


#endif
