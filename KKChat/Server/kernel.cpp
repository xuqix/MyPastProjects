#include "kernel.h"
#include "resource.h"
#define LOGIN_SUCCESS_ENCRYPT 0xff

//数据库数据信息
extern SQL_OUTPUT	SQL_RESULT[MAX_BUF];

extern  HWND	hwnd;			//对话框句柄
extern  HWND	hListView;		//列表句柄
extern	volatile DWORD	Status;			//服务器状态
#define	RUNING			1	//运行中
#define STOP			0	//停止运行

Encrypt			encrypt;		//加密类
BOOL			need_encrypt=0;	//是否需要加密
unsigned char	salt[8] ={ 0 }; //加密盐

extern HWND	hwnd;
extern HWND hListView;

DWORD	NET_PORT	= 8888;		//存储端口号,默认9999	
SOCKET	hListenSocket;			//监听线程的套接字

DWORD		dwUserCount = 0;	//在线用户计数

//所有注册用户的信息结构，简单的做用户名到密码的映射，代替数据库
map<string, string>		UserData;

//在线用户信息
map<string, UserInfo>	OlInfo;		

//消息队列
MsgQueue				MsgQue;

/////////////////////////////本程序数据库所使用的表创建语句////////////////////////////////////////
//create table info(username varchar(18) PRIMARY KEY,passwd varchar(18) NOT NULL,reg_time datetime)
/////////////////////////////本程序数据库所使用的表插入语句////////////////////////////////////////
//////////////////insert into info(username,passwd,reg_time) values(1123,456,DATE())///////////////
/////////////////////////////本程序数据库所使用的表查询语句////////////////////////////////////////
//////////////////////////select * from info where username='xq'///////////////////////////////////
/////////////////////////////本程序数据库所使用的删除语句//////////////////////////////////////////
///////////////////////////delete from info where username='%s'////////////////////////////////////

//封装数据库的插入操作
void	DB_INSERT(char *username, char *password)
{
	static	char  defstr[] = "insert into info(username,passwd,reg_time) values('%s','%s',DATE()+TIME())";
	char	sql[256] = { 0 };
	//组装sql插入语句
	sprintf(sql, defstr, username, password);
	//执行sql语句
	Execute(sql);
}
//在数据库中查询指定用户的信息,调用后数据存储在SQL_RESULT中
void	DB_QUERY(char *username)
{
	static	char  defstr[] = "select * from info where username='%s'";
	char	sql[256] = { 0 };
	//组装sql插入语句
	sprintf(sql, defstr, username);
	//执行sql语句
	Execute(sql);
}
//在数据库中删除指定用户
void	DB_DELETE(char *username)
{
	static	char  defstr[] = "delete from info where username='%s'";
	char	sql[256] = { 0 };
	//组装sql插入语句
	sprintf(sql, defstr, username);
	//执行sql语句
	Execute(sql);
}

//发送队列中的消息
DWORD	SendMsg(SOCKET hSocket, PACKET& packet,CLIENT& client)
{
	Msg		msg;
	BOOL	OK;	//数据可以发送
	int		n;	//当前就绪连接数

	while(Status == RUNING)
	{
		msg	= MsgQue.get_msg(client.dwMessageID+1);
		if(msg.dwMessageID == 0)
			return TRUE;
		client.dwMessageID	= msg.dwMessageID;

		//开始组装下发数据包,对于不同数据包有不同的组装过程，可自行定义
		switch(msg.CMD_ID)
		{
			case CMD_MSG_DOWN:
			case CMD_SYS_INFO:
			case CMD_MSG_PRIVATE:
				if( msg.CMD_ID == CMD_MSG_PRIVATE )
					if( hSocket != OlInfo[string(msg.szAccepter)].hSocket )	
					{
						//是私聊信息但是不是接收者
						OK=FALSE;
						break;
					}
				packet.Head.CMD_ID			= msg.CMD_ID;
				packet.Head.Timestamp		= msg.Timestamp;
				lstrcpyA(packet.stMsgDown.szSender, msg.szSender);
				lstrcpyA(packet.stMsgDown.szContent, msg.szContent);
				packet.stMsgDown.dwLength	= lstrlenA(msg.szContent) + 1;
				packet.Head.dwLength		= sizeof(packet.Head) + packet.stMsgDown.dwLength + sizeof(packet.stMsgDown.szSender) + 4;
				OK=TRUE;;
				break;

			case CMD_USER_LIST:
				//测试是否是数据包的接收者，否则不发送
				if( hSocket == OlInfo[string(msg.szAccepter)].hSocket )
				{
					packet.Head.CMD_ID			= msg.CMD_ID;
					packet.Head.Timestamp		= msg.Timestamp;
					lstrcpyA(packet.stUserList.szUserList, msg.szContent);
					packet.stUserList.dwLength	= lstrlenA(msg.szContent) + 1;
					packet.Head.dwLength		= sizeof(packet.Head) + packet.stUserList.dwLength + 4;
					OK=TRUE;
				}
				else
					OK=FALSE;
				break;

			case CMD_USER_BAN:
			case CMD_KICK_OUT:
				//测试是否是数据包的接收者，否则不发送
				if( hSocket == OlInfo[string(msg.szAccepter)].hSocket )
				{
					packet.Head.CMD_ID			= msg.CMD_ID;
					packet.Head.Timestamp		= msg.Timestamp;
					packet.Head.dwLength		= sizeof(packet.Head);
					OK=TRUE;
				}
				else
					OK=FALSE;
				break;

			/*case CMD_MSG_PRIVATE:
				//测试是否是数据包的接收者，否则不发送
				if( hSocket == OlInfo[string(msg.szAccepter)].hSocket )
				{
					packet.Head.CMD_ID			= msg.CMD_ID;
					packet.Head.Timestamp		= msg.Timestamp;
					lstrcpyA(packet.stMsgDown.szContent, msg.szContent);
					lstrcpyA(packet.stMsgDown.szSender, msg.szSender);
					packet.stUserList.dwLength	= lstrlenA(msg.szContent) + 1;
					packet.Head.dwLength		= sizeof(packet.Head) + packet.stUserList.dwLength + sizeof(packet.stMsgDown.szSender) + 4;
					OK=TRUE;
				}
				else
					OK=FALSE;
				break;*/
			//////////定义更多的组装过程/////////////////
		}
		if(OK)
		{
			if ( need_encrypt )
			{
				//进行消息加密
				unsigned char* lpData = (unsigned char*)&packet + sizeof(PACKET_HEAD);
				encrypt.encrypt( lpData, lpData, packet.Head.dwLength-sizeof(PACKET_HEAD) ) ;
			}
			if ( send(hSocket, (char*)&packet, packet.Head.dwLength, 0) ==SOCKET_ERROR)
				return SOCKET_ERROR;
			client.dwLastTime	= GetTickCount();
		}
		if( ((n = WaitData(hSocket, 0)) == SOCKET_ERROR) )
			return SOCKET_ERROR;
		if( n )
			return TRUE;
	}
	return FALSE;
}

//服务线程，每个客户端连接将产生一个服务线程,参数为本连接的套接字
unsigned __stdcall ServerThread(void* Socket)
{
	CLIENT			client	= { 0 };
	PACKET			packet;
	sockaddr_in		stClientAddr;
	SOCKET			hSocket = (SOCKET)Socket;
	int				len = sizeof(stClientAddr);

	if(getpeername(hSocket,  (sockaddr*)&stClientAddr, &len))
		MessageBox( hwnd, "获取当前正在连接的用户信息失败","提示", 0);
	/*freopen("c:\\1.txt","a+",stdout);
	printf( "对方IP：%s ", inet_ntoa(stClientAddr.sin_addr));
	printf( "对方PORT：%d ", ntohs(stClientAddr.sin_port));*/
	client.dwMessageID	= MsgQue.ID;	//消息编号
	
/////////////开始接收新连接的第一个数据包并作相应处理//////////////

	if (RecvPacket( hSocket, &packet, sizeof(packet) ) == FALSE)
	{
		closesocket(hSocket);
		return 0;
	}
	////注册和登陆结果0均表示成功
	if (packet.Head.CMD_ID == CMD_REGISTER)		
	{
		//注册处理
		//没有数据库的老的处理方法
		//if( UserData.count( string(packet.stRegister.szNewUser) ) != 0)
		DB_QUERY(packet.stRegister.szNewUser );
		if( !strcmp(SQL_RESULT[0].user_name, packet.stRegister.szNewUser ) )	//如果用户名存在
		{
			closesocket(hSocket);
			return 0;
		}

/////////////////////////将用户信息添加进数据库/////////////////////////////////////
		::DB_INSERT(packet.stRegister.szNewUser, packet.stRegister.szPassword);
///////////////////////////提交操作后才会生效///////////////////////////////////////
		Commit();

		//未使用数据库时的用户信息保存方式
		UserData.insert( make_pair(string(packet.stRegister.szNewUser), string(packet.stRegister.szPassword) ) );	//用户信息添加进数据库
		
		packet.Head.CMD_ID				= CMD_REGISTER_RESP;
		packet.Head.Timestamp			= time(0);
		packet.Head.dwLength			= sizeof(packet.Head) + sizeof(packet.stRegisterResp);
		packet.stRegisterResp.bResult	= 0;	
		
		//注册结果不管怎样，都直接结束链接并返回
		send(hSocket, (char*)&packet, packet.Head.dwLength, 0);
		closesocket(hSocket);
		return 0;
	}
	else if(packet.Head.CMD_ID == CMD_LOGIN) //登陆处理
	{
		//检查登陆用户的账号密码
		BOOL	bResult = CheckUser(&packet);

		//记录用户信息
		lstrcpy( client.szUserName, packet.stLogin.szUserName);

		packet.Head.CMD_ID			= CMD_LOGIN_RESP;
		packet.Head.Timestamp		= time(0);
		packet.Head.dwLength		= sizeof(packet.Head) + sizeof(packet.stLoginResp);
		packet.stLoginResp.bResult	= bResult;
		
		//若登陆成功并且需要加密
		if ( (bResult==0) && need_encrypt )
		{
			packet.stLoginResp.bResult	= LOGIN_SUCCESS_ENCRYPT;
			memcpy(packet.stLoginResp.salt, salt, 8);
		}

		//如果发送数据失败或登录失败则处理
		if(  (send(hSocket, (char*)&packet, packet.Head.dwLength, 0) == SOCKET_ERROR ) || bResult)	
		{
			closesocket(hSocket);
			return 0;
		}
	}
	else
	{
		//数据包存在问题，没有这个命令
		closesocket(hSocket);
		return 0;
	}

///////////////////////////////////////////////////////////////////

	//至此登陆成功，进行在线用户信息的添加
	UserInfo	ui;
	ui.hSocket	= hSocket;
	ui.IP		= stClientAddr.sin_addr.S_un.S_addr;
	ui.Timestamp= packet.Head.Timestamp;
	OlInfo.insert( make_pair( string(client.szUserName), ui) );

	dwUserCount++;
	SetDlgItemInt(hwnd, IDC_COUNT, dwUserCount, 0);
/////////////////添加服务器在线用户列表//////////////////////////
	char IP[20], PORT[20], TIME[20];
	struct tm* timeinfo;
	wsprintfA(IP,   "%s", inet_ntoa(stClientAddr.sin_addr));
	wsprintfA(PORT, "%d", ntohs(stClientAddr.sin_port));
	//获取时间信息
	timeinfo	= GetTimeInfo( ui.Timestamp );
	wsprintfA(TIME, "[%02d:%02d:%02d]  ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	AddUserInfo( client.szUserName, IP, TIME);

	//更新客户端在线用户列表
	update(hSocket, &packet, &client);
	
	//系统广播用户的登陆
	char szBuffer[MAX_LENGTH];
	wsprintfA(szBuffer, "【系统消息】欢迎%s进入聊天室！", client.szUserName);
	MsgQue.push_back(  Msg(ui.Timestamp, client.szUserName, szBuffer, CMD_SYS_INFO)  );
	client.dwLastTime	= GetTickCount();

	int n;	//就绪连接数，即可以接受数据的连接
	//进入消息处理循环
	while( Status == RUNING )
	{
		//发送消息队列中的消息并检测链路工作是否正常
		if ( SendMsg(hSocket, packet, client)!=TRUE )
			break;
		if ( !LinkCheck(hSocket, &packet, &client) )
			break;
		if ( Status == STOP )
			break;
		if ( (n = WaitData(hSocket, 250*1000) ) == SOCKET_ERROR )
			break;
		if( n > 0)
		{
			if ( !RecvPacket(hSocket, &packet, sizeof(packet) ) )
				break;
			if ( need_encrypt )
			{
				unsigned char* lpData = (unsigned char*)&packet + sizeof(PACKET_HEAD);
				encrypt.decrypt( lpData, lpData, packet.Head.dwLength-sizeof(PACKET_HEAD) ) ;
			}
			client.dwLastTime	= GetTickCount();
			
////////////////////////对不同数据包进行处理///////////////////////////////
			switch(packet.Head.CMD_ID)
			{
				case CMD_MSG_UP:
					::CMD_MSG_UP_HANDLE(packet, client);
					break;

				case CMD_MSG_PRIVATE:
					::CMD_MSG_PRIVATE_HANDLE(packet, client);
					break;
			}
///////////////////////////////////////////////////////////////////////////

		}
	}
	wsprintfA(szBuffer, "【系统消息】%s退出了聊天室。", client.szUserName);
	MsgQue.push_back(  Msg( time(0), client.szUserName, szBuffer, CMD_SYS_INFO)  );
	//SendMsg(hSocket, packet, client);
	closesocket(hSocket);

	//删除在线用户信息
	OlInfo.erase(OlInfo.find( string(client.szUserName) ) ); 

	//列表中删除本用户信息
	DelUserInfo(client.szUserName );

	dwUserCount--;
	SetDlgItemInt(hwnd, IDC_COUNT, dwUserCount, 0);

	return 0;
}

//监听线程，监听来自XXXX端口的网络连接
unsigned __stdcall ListenThread(void* param)
{
	SOCKET		hSocket;
	sockaddr_in	stSocket;
	DWORD		dwThreadID;

//////////////////////////////加密初始化操作/////////////////////////////////		
	//获取8字节随机数据作为"加密盐"	
	::rng_get_bytes(salt,8,NULL);
	if ( encrypt.init(salt) )
			need_encrypt = TRUE;
/////////////////////////////////////////////////////////////////////////////

///////////////////////////数据库连接操作/////////////////////////////////////
	if(!Connect())
	{
		MessageBox(NULL, TEXT("数据库连接失败"), TEXT("错误"), MB_ICONWARNING);
		return 0;
	}
	
	
	memset(&stSocket, 0, sizeof(stSocket));
	hListenSocket		= socket(AF_INET,SOCK_STREAM,0);
	stSocket.sin_port	= htons(NET_PORT);
	stSocket.sin_family	= AF_INET;
	stSocket.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind( hListenSocket, (sockaddr*)&stSocket, sizeof(stSocket)) )
	{
		MessageBox(hwnd, TEXT("无法绑定TCP第%d号端口，请检查端口是否被其他程序占用"), TEXT("提示"), MB_ICONWARNING);
		ExitProcess(0);		//直接退出
	}

	//下面进入网络连接的监听
	listen(hListenSocket,4);
	while(Status == RUNING)
	{
		hSocket = accept(hListenSocket, NULL, 0);
		if (hSocket==INVALID_SOCKET)
			break;

		//启动通讯服务线程
		chBEGINTHREADEX(NULL, 0, ServerThread, hSocket, 0, &dwThreadID);
	}
	closesocket(hListenSocket);

//////////////////////////断开数据库连接/////////////////////////////
	DisConnect();
	return 0;
}


//列表中添加用户信息
void	AddUserInfo(char* szUserName, char* szIP, char* szLoginTime)
{
	int i;
	i = ListView_AddLine(hListView);//添加一行
	ListView_SetItemText(hListView, i, 0, szUserName);
	ListView_SetItemText(hListView, i, 1, szIP);
	ListView_SetItemText(hListView, i, 2, szLoginTime);
}

//删除列表中选中行的信息
void	DelUserInfo()
{
	int i;
	i = ListView_GetSelectionMark(hListView);  //获得选中行索引
	ListView_DeleteItem(hListView, i);
}

//删除列表中指定用户信息
void	DelUserInfo(char* szUserName)
{
	int		i,n;
	char	szBuffer[16];
	n	= ListView_GetItemCount(hListView);
	for(i=0; i < n; i++)
	{
		ListView_GetItemText(hListView, i, 0, szBuffer, sizeof(szBuffer) );
		if(strcmp(szUserName, szBuffer) == 0)
			break;
	}
	if(i!=n)
		ListView_DeleteItem(hListView, i);
}


void	CleanUserInfo(HWND hListView)
{
	int i;
	ListView_DeleteAllItems(hListView);
	i = ListView_GetItemCount(hListView);
	for( i--; i>=0; i--)
		ListView_DeleteColumn(hListView, i);
}


//通讯线路的检测，通过发送数据包测试连接是否可用
//返回值为0则无活动，释放连接
int		LinkCheck(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient)
{
	//如果超过30秒无活动，则发送探测数据检查连接
	if( (GetTickCount()-lpClient->dwLastTime)> 30*1000)
	{
		lpClient->dwLastTime	= GetTickCount();
		packet->Head.CMD_ID		= CMD_CHECK_LINK;
		packet->Head.dwLength	= sizeof(packet->Head);
		if( send( hSocket, (char*)packet, packet->Head.dwLength, 0) == SOCKET_ERROR )
			return FALSE;
	}

	return TRUE;
}

//登录用户检测,返回0表示成功
BOOL	CheckUser(PACKET* lpPacket)
{
	string name(lpPacket->stLogin.szUserName);
	//数据库中检查用户信息
	DB_QUERY(lpPacket->stLogin.szUserName);
	//密码验证和重复登陆验证
	if( (OlInfo.count( name ) == 0) && !strcmp(SQL_RESULT[0].passwd, lpPacket->stLogin.szPassword) )
		return 0;
	return 1;
////////////未使用数据库时的检测方法//////////////////
	//检测是否存在本用户
	if( UserData.count( name ) == 0 )
		return 1;
	//检测密码
	if( UserData[name] != string(lpPacket->stLogin.szPassword) )
		return 1;
	//检测是否存在重复登录
	if( OlInfo.count( name ) != 0)
		return 1;
	return 0;
}

//禁言选中用户
int		BanUser()
{
	char	szBuffer[64];
	int	idx = ListView_GetSelectionMark( hListView);
	if(idx==-1)	return idx;
	ListView_GetItemText(hListView, idx, 0, szBuffer, sizeof(szBuffer) );
	MsgQue.push_back(  Msg( time(0), NULL, NULL,CMD_USER_BAN,szBuffer )  );
	return TRUE;
}

//踢出选中用户
int		KickUser()
{
	char	szBuffer[64];
	int	idx = ListView_GetSelectionMark( hListView);
	if(idx==-1)	return idx;
	ListView_GetItemText(hListView, idx, 0, szBuffer, sizeof(szBuffer) );
	MsgQue.push_back(  Msg( time(0), NULL, NULL,CMD_KICK_OUT,szBuffer )  );
	return TRUE;
}

//发送对客户端进行用户列表更新的数据包
BOOL	update(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient)
{
	char szBuffer[MAX_LENGTH*2] = { 0 };
	for(map<string, UserInfo>::iterator iter=OlInfo.begin(); iter!=OlInfo.end(); iter++)
	{
		if(strcmp(lpClient->szUserName, iter->first.c_str() ) != 0)
		{
			strcat(szBuffer, iter->first.c_str() );
			strcat(szBuffer, ",");
		}
	}
	if(OlInfo.size()<=1)
		return FALSE;
	szBuffer[strlen(szBuffer)-1] = 0;
	MsgQue.push_back( Msg(time(0), lpClient->szUserName, szBuffer, CMD_USER_LIST, lpClient->szUserName) );
	return TRUE;
}

int CMD_MSG_UP_HANDLE  (PACKET& packet, CLIENT& client)
{
	MsgQue.push_back( Msg(packet.Head.Timestamp, client.szUserName, packet.stMsgUp.szContent) );
	return TRUE;
}

int CMD_MSG_PRIVATE_HANDLE(PACKET& packet, CLIENT& client)
{
	MsgQue.push_back( Msg(packet.Head.Timestamp, client.szUserName, packet.stMsgUp.szContent, CMD_MSG_PRIVATE, packet.stMsgUp.szAccepter ) );
	return TRUE;
}
