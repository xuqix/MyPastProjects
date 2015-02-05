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

//��Ϣ���ж��壬��װdeque������֤�̰߳�ȫ
//��Ϣ�洢���ݽṹ����Ϣ��š������ߡ������ߡ�����
struct Msg
{
	Msg() { dwMessageID = 0; Timestamp = 0; memset(szSender,0,sizeof(szSender)); memset(szAccepter, 0, sizeof(szAccepter)); }
	//������Ĭ��Ϊ�ձ�ʾ�㲥���͸�������
	Msg( /*DWORD dwMsgID,*/time_t t, char* szSend, char* szConten, DWORD cmd = CMD_MSG_DOWN, char* szAccept=NULL)
	{
		//dwMessageID	= dwMsgID;
		Timestamp	= t;
		CMD_ID		= cmd;
		if (szSend)		strcpy(szSender, szSend);
		if (szAccept)	strcpy(szAccepter, szAccept);
		if (szConten)	strcpy(szContent, szConten);
	}
	DWORD	dwMessageID;			//��ǰ��Ϣ����Ϣ���(��������)
	time_t	Timestamp;				//ʱ���
	char	szSender[16];			//��ǰ��Ϣ�ķ�����
	char	szAccepter[16];			//��ǰ��Ϣ�Ľ�����
	char	szContent[MAX_LENGTH*2];//��Ϣ����
	DWORD	CMD_ID;					//��Ϣ���ͣ��粻�ر�ָ��Ĭ��ΪCMD_MSG_DOWN
};


//�洢��Ϣ�Ķ��У�Ϊ������й����Ӵ��������ƶ��д�С���Ϊ128�����ƴ�С��Ҫ�Զ�ͷ��β���д�������ѡ��˫�˶���
//���ڶ����ڶ��߳��в�����ѡ��ʹ���ٽ�����������ɶ��е��̰߳�ȫ
#define MAX_SIZE	128
class MsgQueue
{
public:
	DWORD	ID;					//��Ϣ���

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

	//���ָ����Ϣ��ŵ���Ϣ�����С����С��ţ��򷵻���С���Ǹ�(˵�����������ӳٵ��µ���Ϣ��ʧ)�������򷵻�һ����ϢIDΪ0�Ľṹ
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
	deque<Msg>	q;				//˫�˶���
	CRITICAL_SECTION	stCS;	//�ٽ�������
};

struct UserInfo
{
	SOCKET	hSocket;	//���û���ʹ�����ӵ��׽���
	//accept( client , (sockaddr*)&client_addr, ....); 
	DWORD	IP;			//IP��ַ��32λ��ʾ
	time_t	Timestamp;	//���һ�ε�¼ʱ��
};


#endif
