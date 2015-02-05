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



//ÿ�����ӽ�����һ�����¿ͻ�����Ϣ�ṹ
struct CLIENT
{
	DWORD	dwMessageID;	//�ѷ��͵���Ϣ���
	char	szUserName[16];	//�û���
	DWORD	dwLastTime;		//ͨѶ��·����ʱ��
};

//ͨѶ��·�ļ�⣬�Ա�֤�������Ӻ��ͷ�״̬
//����ֵΪ0���޻���ͷ�����
int		LinkCheck(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient);

//����ѡ���û�
int		BanUser();

//�߳�ѡ���û�
int		KickUser();

//�б�������û���Ϣ
void	AddUserInfo(char* szUserName, char* szIP, char* szLoginTime);

//ɾ���б���ѡ���е���Ϣ
void	DelUserInfo();

//ɾ���б���ָ���û���Ϣ
void	DelUserInfo(char* szUserName);

//����б�
void	CleanUserInfo(HWND hListView);

//�û���¼���
BOOL	CheckUser(PACKET* lpPacket);

//���ͶԿͻ��˽����û��б���µ����ݰ�
BOOL	update(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient);

//�����̣߳�ÿ���ͻ������ӽ�����һ�������߳�,����Ϊ�����ӵ��׽���
unsigned __stdcall ServerThread(void* param);

//�����߳�
unsigned __stdcall ListenThread(void* param);


//////////////////////�����������Ϣ�ĺ�����/////////////////////////////
///////////////////����ԭ��:int XXX_HANDLE(PACKET*)//////////////////////////
//////////////////////////////XXXΪ��Ϣ��//////////////////////////////////
int CMD_MSG_UP_HANDLE	  (PACKET& packet, CLIENT& client);
int CMD_MSG_PRIVATE_HANDLE(PACKET& packet, CLIENT& client);

#endif