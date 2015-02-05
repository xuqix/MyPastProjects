#ifndef PACKET_H
#define PACKET_H

//�Զ����������ݴ���Э�飬Ϊ���ֹ����ṩ�������ݰ��ṹ����
#include <windows.h>

#define MAX_LENGTH	512			//�û�������Ϣ���ݵ���󳤶�(�ֽ�)


//���Զ�������ݰ�����
//�ͻ���->��������Ϣ�����ŷ�Χ[0x1,0x80)
//������->�ͻ�����Ϣ�����ŷ�Χ[0x80,0xff)

/////////////////////�ͻ���-->������/////////////////////////
#define	CMD_LOGIN		0x1		//�û���½
#define CMD_MSG_UP		0x2		//�ϴ�������Ϣ
#define CMD_REGISTER	0x3		//ע�����û�
/////////////////////////////////////////////////////////////

#define CMD_MSG_PRIVATE	0xff	//˽������ID���ͻ��˺ͷ���������

/////////////////////������-->�ͻ���/////////////////////////
#define CMD_LOGIN_RESP  0x81	//��½Ӧ��
#define CMD_MSG_DOWN	0x82	//�·���Ϣ
#define CMD_CHECK_LINK	0x83	//��·���
#define CMD_REGISTER_RESP 0x84	//��½Ӧ��
#define CMD_SYS_INFO	0x85	//ϵͳ��Ϣ
#define CMD_USER_LIST	0x86	//�û��б���Ϣ�����ڸ��ͻ��˸��������б�
#define CMD_USER_BAN	0x87	//����ָ���û�
#define	CMD_KICK_OUT	0x88	//��ָ���û��߳�������
/////////////////////////////////////////////////////////////

//////////////////////���ݰ��ṹ/////////////////////////////
////////////PACKET = PACKET_HEAD + PACKET_XXXX///////////////

////////////���ݰ�ͷ�����κ����ݰ����Դ˽ṹ��ʼ/////////////
struct PACKET_HEAD
{
	DWORD	CMD_ID;				//����ID
	DWORD	dwLength;			//���ݰ�����=���ݰ�ͷ+���ݰ���
	time_t	Timestamp;			//ʱ�����Ϣ����Ҫ����ת���ɿɶ�ʱ��
};


//////////////��¼���ݽṹ(�ͻ���-->������)//////////////////
struct PACKET_LOGIN
{
	char szUserName[16];		//�û���
	char szPassword[16];		//����
};

////////////��½Ӧ�����ݽṹ(������-->�ͻ���)////////////////
//��¼Ӧ����
//#define LOGIN_SUCCESS		0
//#define LOGIN_FAIL		1
struct PACKET_LOGIN_RESP
{
	BYTE bResult;				//��½Ӧ��,�ɹ������Ǽ���������Ϊ2
	unsigned char salt[8];		//������
};


//////////////ע���û����ݽṹ(�ͻ���-->������)//////////////
struct PACKET_REGISTER
{
	char szNewUser[16];			//�û���
	char szPassword[16];		//����
};


//////////ע���û�Ӧ�����ݽṹ(������-->�ͻ���///////////////
//�ɹ�Ϊ0��ʧ��Ϊ1
struct PACKET_REGISTER_RESP
{
	BYTE bResult;				//ע��Ӧ��
};

////////�û��ϴ�������Ϣ���ݽṹ(�ͻ���-->������)////////////
struct PACKET_UP
{
	char  szAccepter[16];		//��Ϣ�Ľ�����
	DWORD dwLength;				//��Ϣ���ݵĳ���
	char  szContent[MAX_LENGTH];//�洢��Ϣ����
};

//////////�������ַ���Ϣ���ݽṹ(������-->�ͻ���)///////////
struct PACKET_DOWN
{
	char  szSender[16];			//��Ϣ������
	DWORD dwLength;				//��Ϣ���ݳ���
	char  szContent[MAX_LENGTH];//�洢��Ϣ����
};


//////////�û��б���Ϣ���ݽṹ(������-->�ͻ���)//////////////
///////////�洢�û�����ÿ���û�������','�ָ�/////////////////
struct PACKET_USER_LIST
{
	//�����������ߵ��û���
	DWORD	dwLength;		//�û����б���
	char	szUserList[MAX_LENGTH*2];
};


/////////////////////////���������ݰ�����////////////////////////////
struct PACKET
{
	//���ݰ�ͷ
	PACKET_HEAD				Head;
	
	//���ݰ���
	union	
	{
//////////////////ϵͳĬ�����ݰ��ṹ����////////////////
		PACKET_LOGIN			stLogin;
		PACKET_LOGIN_RESP		stLoginResp;
		PACKET_UP				stMsgUp;
		PACKET_DOWN				stMsgDown;
		PACKET_REGISTER			stRegister;
		PACKET_REGISTER_RESP	stRegisterResp;
		PACKET_USER_LIST		stUserList;
////////////////////////////////////////////////////////


//////////////////�Զ������ݰ��ṹ����//////////////////

////////////////////////////////////////////////////////

	};
};
/////////////////////////////////////////////////////////////////////


#endif