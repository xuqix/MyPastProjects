#include "handle.h"

extern HWND	hChat;

extern char	szErrInfo[64];
extern char	szUserName[20];
extern char	szPassword[20];
extern char	szServerIP[20];

extern Encrypt			encrypt;		//加密类，用于数据包加密
extern BOOL				need_encrypt;//是否能加密
extern unsigned char	salt[8];//加密盐
extern volatile DWORD	Status;				//记录登陆状态,以下为状态类型

extern DWORD	dwThreadID;
extern DWORD	dwLastTime;			//最后一次发送数据报的时间
extern DWORD	dwTimeCount;		//计时器，计算禁言时间

extern DWORD	NET_PORT;			//存储端口号	
extern SOCKET	hSocket;
extern BOOL		CantSpeak;			//禁言位

//////////////////注册新用户函数，成功返回TRUE，否则返回FALSE//////////////////////////////
///////////////////为方便起见，注册用户进行一次单独TCP的连接///////////////////////////////
BOOL RegisterNewUser(HWND hDlg)
{
	DWORD	count;
	BOOL	flag;

	lstrcpyA( szErrInfo, "抱歉，注册失败。发生了一些错误");
	count = GetDlgItemTextA(hDlg, IDC_USER, szUserName, sizeof(szUserName));
	if(count==0)	{ lstrcatA( szErrInfo, "\r\n错误信息：用户名或密码不能为空"); return FALSE; }
	count = GetDlgItemTextA(hDlg, IDC_PASS, szPassword, sizeof(szPassword));
	if(count==0)	{ lstrcatA( szErrInfo, "\r\n错误信息：用户名或密码不能为空"); return FALSE; }
	count = GetDlgItemTextA(hDlg, IDC_SERVER, szServerIP, sizeof(szServerIP));
	if(count==0)	{ lstrcatA( szErrInfo, "\r\n错误信息：请填入正确的服务器IP地址"); return FALSE; }
	NET_PORT	= GetDlgItemInt(hDlg, IDC_PORT, &flag, TRUE); 

	Status	= LOGIN_ING;	//借用登陆中状态


	PACKET		packet;		//自定义的数据包
	sockaddr_in	stSocket;	//网络连接需要的相关信息
	DWORD		ip_v4;		//32位ip地址

	memset( &packet, 0, sizeof(packet) );
	memset( &stSocket, 0, sizeof(stSocket) );
	if ( (ip_v4=inet_addr( szServerIP)) == INADDR_NONE )
	{
		//IP地址不合法，获得错误信息并置登陆状态为登陆失败（LOGIN_FAIL）
		lstrcatA(szErrInfo, "\r\n错误信息：无效的服务器IP地址，请检查IP设置");
		Status	= LOGIN_FAIL;
		return FALSE;
	}

	//填写地址结构并生成socket套接字
	stSocket.sin_addr.S_un.S_addr	= ip_v4;
	stSocket.sin_family = AF_INET;
	stSocket.sin_port	= htons( (u_short)NET_PORT);
	
	hSocket				= socket(AF_INET,SOCK_STREAM,0 );
	
	//尝试与服务器建立TCP连接
	if ( connect( hSocket, (SOCKADDR*)&stSocket, sizeof(stSocket)) == SOCKET_ERROR )
	{
		lstrcatA(szErrInfo, "\r\n错误信息：无法建立到服务器的连接，请重试");
		Status	= LOGIN_FAIL;
		if(hSocket)		//可能需要清理 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return 0;
	}

	//成功连接到服务器，开始组装注册数据包
	packet.Head.CMD_ID		= CMD_REGISTER;//暂时借用登陆数据包，等待服务器代码做出更新
	packet.Head.dwLength	= sizeof(packet.Head) + sizeof(packet.stRegister);
	packet.Head.Timestamp	= time(NULL);
	lstrcpyA(packet.stRegister.szNewUser, szUserName);
	lstrcpyA(packet.stRegister.szPassword, szPassword);

	//发送注册数据包,并一次性完成数据包应答的接收和注册结果的检测
	if( (send(hSocket, (char*)&packet, packet.Head.dwLength, 0) == SOCKET_ERROR) || (RecvPacket(hSocket, &packet,sizeof(packet) ) == FALSE)\
		|| (packet.stRegisterResp.bResult == LOGIN_FAIL) )
	{
		Status	= LOGIN_FAIL;
		if(hSocket)		//可能需要清理 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return FALSE;
	}

	//成功得到数据包应答,更新最后数据接发时间
	dwLastTime	= GetTickCount();
	//关闭连接
	closesocket(hSocket);	
	hSocket	= 0;
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////


/*
#define LOGIN_SUCCESS	0
#define LOGIN_FAIL		1
#define LOGIN_ING		2
#define LOGIN_TIMEOUT	3
#define LOGIN_CHAT		4
#define LOGIN_CANCEL	-1	//退出 
*/
//由于下面那个while(Status != LOGIN_CHAT)	;release版会被优化掉，所以这里使用#pragma指示来取消优化
//#pragma optimize("",off)
unsigned __stdcall WorkThread(void* param)
{
	PACKET		packet;		//自定义的数据包
	sockaddr_in	stSocket;	//网络连接需要的相关信息
	DWORD		ip_v4;		//32位ip地址

	memset( &packet, 0, sizeof(packet) );
	memset( &stSocket, 0, sizeof(stSocket) );
	if ( (ip_v4=inet_addr( szServerIP)) == INADDR_NONE )
	{
		//IP地址不合法，获得错误信息并置登陆状态为登陆失败（LOGIN_FAIL）
		lstrcpyA(szErrInfo, "无效的服务器IP地址，请检查IP设置");
		Status	= LOGIN_FAIL;
		return 0;
	}

	//填写地址结构并生成socket套接字
	stSocket.sin_addr.S_un.S_addr	= ip_v4;
	stSocket.sin_family = AF_INET;
	stSocket.sin_port	= htons( (u_short)NET_PORT);
	
	hSocket				= socket(AF_INET,SOCK_STREAM,0 );
	
	//尝试与服务器建立TCP连接
	if ( connect( hSocket, (SOCKADDR*)&stSocket, sizeof(stSocket)) == SOCKET_ERROR )
	{
		lstrcpyA(szErrInfo, "无法建立到服务器的连接，请重试");
		Status	= LOGIN_FAIL;
		if(hSocket)		//可能需要清理 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return 0;
	}

	//成功连接到服务器，开始组装登录数据包
	packet.Head.CMD_ID		= CMD_LOGIN;
	packet.Head.dwLength	= sizeof(packet.Head) + sizeof(packet.stLogin);
	packet.Head.Timestamp	= time(NULL);
	lstrcpyA(packet.stLogin.szUserName, szUserName);
	lstrcpyA(packet.stLogin.szPassword, szPassword);

	//发送登陆数据包,并一次性完成数据包应答的接收和登陆结果的检测
	if( (send(hSocket, (char*)&packet, packet.Head.dwLength, 0) == SOCKET_ERROR) || (RecvPacket(hSocket, &packet,sizeof(packet) ) == FALSE)\
																							|| (packet.stLoginResp.bResult == LOGIN_FAIL) )
	{
		lstrcpyA(szErrInfo, "无法登陆到服务器，可能是用户名或密码错误");
		Status	= LOGIN_FAIL;
		if(hSocket)		//可能需要清理 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return 0;
	}
	
	//成功得到数据包应答,更新最后数据接发时间
	dwLastTime	= GetTickCount();

	//检测是否需要消息加密并获得加密盐
	if( packet.stLoginResp.bResult == LOGIN_SUCCESS_ENCRYPT)
	{
		need_encrypt	= TRUE;
		memcpy( salt, packet.stLoginResp.salt, 8 );
		encrypt.init(salt);
	}

	//至此登陆成功，设置状态参数，并开始循环接收消息
	Status		= LOGIN_SUCCESS;
	int n;
	while(hSocket)
	{
		//检测超时(1min)
		if( (GetTickCount() - dwLastTime) > 60 * 1000)
			break;

		if( (n=WaitData(hSocket, 255 * 1000)) == SOCKET_ERROR)	//等待数据255ms
			break;

		if(n==0) continue;
		//接收数据包
		if(RecvPacket( hSocket, &packet, sizeof(packet) )==FALSE )
			break;

		//数据包解密
		if( need_encrypt )
		{
			unsigned char* lpData = (unsigned char*)&packet + sizeof(PACKET_HEAD);
			encrypt.decrypt( lpData, lpData, packet.Head.dwLength-sizeof(PACKET_HEAD) ) ;			
		}

		while(Status != LOGIN_CHAT)	;	//聊天对话框未完成初始化则一直等待

/////////////////对于不同的数据包进行分别处理////////////////
		switch(packet.Head.CMD_ID)
		{
			case CMD_MSG_DOWN:	//一般信息
				::CMD_MSG_DOWN_HANDLE(&packet);
				break;

			case CMD_SYS_INFO:	//系统信息
				::CMD_SYS_INFO_HANDLE(&packet);
				break;

			case CMD_USER_LIST:	//列表信息更新
				::CMD_USER_LIST_HANDLE(&packet);
				break;

			case CMD_MSG_PRIVATE://私聊信息
				::CMD_MSG_PRIVATE_HANDLE(&packet);
				break;

			case CMD_USER_BAN:	//用户禁言
				::CMD_USER_BAN_HANDLE(&packet);
				break;

			case CMD_KICK_OUT:	//踢出用户
				::CMD_KICK_OUT_HANDLE(&packet);
				break;
			
			default:
				break;
///////////////////////自定义消息编号////////////////////////
		}
		dwLastTime	= GetTickCount();
	}
	return 0;
}
//函数结束，恢复优化
//#pragma optimize("",on)


////////////////////////////////处理下发消息///////////////////////////////////
int CMD_MSG_DOWN_HANDLE(PACKET* lpPacket)
{
	char szBuffer[MAX_LENGTH+16] = { 0 } ;
	struct tm* timeinfo;
	lpPacket->Head.Timestamp	= time(NULL);
	timeinfo	= GetTimeInfo( lpPacket->Head.Timestamp);
	wsprintfA(szBuffer, "[%02d:%02d:%02d]  ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	lstrcatA( szBuffer, lpPacket->stMsgDown.szSender);
	lstrcatA( szBuffer, " 对所有人说：");
	lstrcatA( szBuffer, lpPacket->stMsgDown.szContent);
	lstrcatA( szBuffer, "\r\n");
	int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
	Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
	SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szBuffer);
	return 0;
}

////////////////////////////////处理系统消息//////////////////////////////////////
int CMD_SYS_INFO_HANDLE(PACKET* packet)
{
	char szBuffer[MAX_LENGTH+16] = { 0 } ;
	lstrcpyA( szBuffer, "★");
	lstrcatA( szBuffer, packet->stMsgDown.szContent);
	lstrcatA( szBuffer, "★");
	lstrcatA( szBuffer, "\r\n");
	int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
	Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
	SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szBuffer);

	//检查是否是新用户进入或者退出聊天室的消息，进行列表信息处理
	if( strstr(packet->stMsgDown.szContent, "进入聊天室") != NULL )
	{
		AddUserInfo( packet->stMsgDown.szSender );

		if(strcmp(szUserName, packet->stMsgDown.szSender)!=0)
			ComboBox_AddString(hCombobox, packet->stMsgDown.szSender);//下拉列表
	}
	else if( strstr(packet->stMsgDown.szContent, "退出了聊天室") != NULL )
	{
		//处理在线用户列表
		DelUserInfo( packet->stMsgDown.szSender );
		//处理下拉列表
		ComboBox_GetCurString(szBuffer);
		if(strcmp(packet->stMsgDown.szSender, szBuffer)==0)
			ComboBox_SetCurSel(hCombobox, 0);
		if(strcmp(szUserName, packet->stMsgDown.szSender)!=0)
			ComboBox_DelString(packet->stMsgDown.szSender);
	}

	return 0;
}


///////////////////////////处理列表更新消息////////////////////////////////////////
int CMD_USER_LIST_HANDLE(PACKET* packet)
{
	char	szUserName[20];
	char*	szList = packet->stUserList.szUserList;
	int  x=0;
	for(int i=0; packet->stUserList.szUserList[i]; i++)
		if(packet->stUserList.szUserList[i] == ',')
			packet->stUserList.szUserList[i] = 0;
	for(DWORD j=0; j<packet->stUserList.dwLength;  )
	{
		//在线用户列表的更新
		strcpy(szUserName, szList);
		AddUserInfo(szUserName);

		//下拉列表的更新
		ComboBox_AddString(hCombobox, szUserName);
		j = j + strlen(szList) + 1;
		szList = packet->stUserList.szUserList + j ;
	}
	return 0;
}

//////////////////////////处理来自其他用户的私聊信息////////////////////////////
int CMD_MSG_PRIVATE_HANDLE(PACKET* lpPacket)
{
	char szBuffer[MAX_LENGTH+16] = { 0 } ;
	char temp[32]={ 0 };

	struct tm* timeinfo;
	lpPacket->Head.Timestamp	= time(NULL);
	timeinfo	= GetTimeInfo( lpPacket->Head.Timestamp);
	//wsprintfA(szBuffer, "[%02d:%02d:%02d]  ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	wsprintf(temp, "☆【%s】悄悄地对你说：", lpPacket->stMsgDown.szSender);
	lstrcatA( szBuffer, temp);
	lstrcatA( szBuffer, lpPacket->stMsgDown.szContent);
	lstrcatA( szBuffer, "☆");
	lstrcatA( szBuffer, "\r\n");
	int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
	Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
	SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szBuffer);
	//私聊消息提示音
	PlaySound (TEXT ("msg.wav"), NULL, SND_FILENAME | SND_ASYNC) ;	//音乐播放API
	return 0;
}

//用户禁言命令
int CMD_USER_BAN_HANDLE (PACKET* packet)
{
	CantSpeak	= TRUE;
	dwTimeCount	= GetTickCount();
	return 0;
}

//踢出用户命令
int CMD_KICK_OUT_HANDLE (PACKET* packet)
{
	MessageBox(hChat, "对不起，你已被管理员请出聊天室", "提示", MB_ICONWARNING);
	SendMessage(hChat, WM_CLOSE, 0, 0);
	return 0;
}

