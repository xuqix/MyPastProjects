#ifndef PACKET_H
#define PACKET_H

//自定义网络数据传输协议，为各种功能提供各种数据包结构定义
#include <windows.h>

#define MAX_LENGTH	512			//用户发送消息内容的最大长度(字节)


//可自定义的数据包种类
//客户端->服务器消息命令编号范围[0x1,0x80)
//服务器->客户端消息命令编号范围[0x80,0xff)

/////////////////////客户端-->服务器/////////////////////////
#define	CMD_LOGIN		0x1		//用户登陆
#define CMD_MSG_UP		0x2		//上传发送消息
#define CMD_REGISTER	0x3		//注册新用户
/////////////////////////////////////////////////////////////

#define CMD_MSG_PRIVATE	0xff	//私聊命令ID，客户端和服务器公用

/////////////////////服务器-->客户端/////////////////////////
#define CMD_LOGIN_RESP  0x81	//登陆应答
#define CMD_MSG_DOWN	0x82	//下发消息
#define CMD_CHECK_LINK	0x83	//链路检测
#define CMD_REGISTER_RESP 0x84	//登陆应答
#define CMD_SYS_INFO	0x85	//系统消息
#define CMD_USER_LIST	0x86	//用户列表信息，用于给客户端更新在线列表
#define CMD_USER_BAN	0x87	//禁言指定用户
#define	CMD_KICK_OUT	0x88	//将指定用户踢出聊天室
/////////////////////////////////////////////////////////////

//////////////////////数据包结构/////////////////////////////
////////////PACKET = PACKET_HEAD + PACKET_XXXX///////////////

////////////数据包头部，任何数据包均以此结构起始/////////////
struct PACKET_HEAD
{
	DWORD	CMD_ID;				//命令ID
	DWORD	dwLength;			//数据包长度=数据包头+数据包体
	time_t	Timestamp;			//时间戳信息，需要自行转换成可读时间
};


//////////////登录数据结构(客户端-->服务器)//////////////////
struct PACKET_LOGIN
{
	char szUserName[16];		//用户名
	char szPassword[16];		//密码
};

////////////登陆应答数据结构(服务器-->客户端)////////////////
//登录应答结果
//#define LOGIN_SUCCESS		0
//#define LOGIN_FAIL		1
struct PACKET_LOGIN_RESP
{
	BYTE bResult;				//登陆应答,成功并且是加密数据则为2
	unsigned char salt[8];		//加密盐
};


//////////////注册用户数据结构(客户端-->服务器)//////////////
struct PACKET_REGISTER
{
	char szNewUser[16];			//用户名
	char szPassword[16];		//密码
};


//////////注册用户应答数据结构(服务器-->客户端///////////////
//成功为0，失败为1
struct PACKET_REGISTER_RESP
{
	BYTE bResult;				//注册应答
};

////////用户上传发送信息数据结构(客户端-->服务器)////////////
struct PACKET_UP
{
	char  szAccepter[16];		//消息的接收者
	DWORD dwLength;				//消息内容的长度
	char  szContent[MAX_LENGTH];//存储消息内容
};

//////////服务器分发信息数据结构(服务器-->客户端)///////////
struct PACKET_DOWN
{
	char  szSender[16];			//消息发送者
	DWORD dwLength;				//消息内容长度
	char  szContent[MAX_LENGTH];//存储消息内容
};


//////////用户列表信息数据结构(服务器-->客户端)//////////////
///////////存储用户名，每个用户名间用','分割/////////////////
struct PACKET_USER_LIST
{
	//不包括接收者的用户名
	DWORD	dwLength;		//用户名列表长度
	char	szUserList[MAX_LENGTH*2];
};


/////////////////////////完整的数据包定义////////////////////////////
struct PACKET
{
	//数据包头
	PACKET_HEAD				Head;
	
	//数据包体
	union	
	{
//////////////////系统默认数据包结构类型////////////////
		PACKET_LOGIN			stLogin;
		PACKET_LOGIN_RESP		stLoginResp;
		PACKET_UP				stMsgUp;
		PACKET_DOWN				stMsgDown;
		PACKET_REGISTER			stRegister;
		PACKET_REGISTER_RESP	stRegisterResp;
		PACKET_USER_LIST		stUserList;
////////////////////////////////////////////////////////


//////////////////自定义数据包结构类型//////////////////

////////////////////////////////////////////////////////

	};
};
/////////////////////////////////////////////////////////////////////


#endif