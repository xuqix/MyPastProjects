#include "kernel.h"
#include "resource.h"
#define LOGIN_SUCCESS_ENCRYPT 0xff

//���ݿ�������Ϣ
extern SQL_OUTPUT	SQL_RESULT[MAX_BUF];

extern  HWND	hwnd;			//�Ի�����
extern  HWND	hListView;		//�б���
extern	volatile DWORD	Status;			//������״̬
#define	RUNING			1	//������
#define STOP			0	//ֹͣ����

Encrypt			encrypt;		//������
BOOL			need_encrypt=0;	//�Ƿ���Ҫ����
unsigned char	salt[8] ={ 0 }; //������

extern HWND	hwnd;
extern HWND hListView;

DWORD	NET_PORT	= 8888;		//�洢�˿ں�,Ĭ��9999	
SOCKET	hListenSocket;			//�����̵߳��׽���

DWORD		dwUserCount = 0;	//�����û�����

//����ע���û�����Ϣ�ṹ���򵥵����û����������ӳ�䣬�������ݿ�
map<string, string>		UserData;

//�����û���Ϣ
map<string, UserInfo>	OlInfo;		

//��Ϣ����
MsgQueue				MsgQue;

/////////////////////////////���������ݿ���ʹ�õı������////////////////////////////////////////
//create table info(username varchar(18) PRIMARY KEY,passwd varchar(18) NOT NULL,reg_time datetime)
/////////////////////////////���������ݿ���ʹ�õı�������////////////////////////////////////////
//////////////////insert into info(username,passwd,reg_time) values(1123,456,DATE())///////////////
/////////////////////////////���������ݿ���ʹ�õı��ѯ���////////////////////////////////////////
//////////////////////////select * from info where username='xq'///////////////////////////////////
/////////////////////////////���������ݿ���ʹ�õ�ɾ�����//////////////////////////////////////////
///////////////////////////delete from info where username='%s'////////////////////////////////////

//��װ���ݿ�Ĳ������
void	DB_INSERT(char *username, char *password)
{
	static	char  defstr[] = "insert into info(username,passwd,reg_time) values('%s','%s',DATE()+TIME())";
	char	sql[256] = { 0 };
	//��װsql�������
	sprintf(sql, defstr, username, password);
	//ִ��sql���
	Execute(sql);
}
//�����ݿ��в�ѯָ���û�����Ϣ,���ú����ݴ洢��SQL_RESULT��
void	DB_QUERY(char *username)
{
	static	char  defstr[] = "select * from info where username='%s'";
	char	sql[256] = { 0 };
	//��װsql�������
	sprintf(sql, defstr, username);
	//ִ��sql���
	Execute(sql);
}
//�����ݿ���ɾ��ָ���û�
void	DB_DELETE(char *username)
{
	static	char  defstr[] = "delete from info where username='%s'";
	char	sql[256] = { 0 };
	//��װsql�������
	sprintf(sql, defstr, username);
	//ִ��sql���
	Execute(sql);
}

//���Ͷ����е���Ϣ
DWORD	SendMsg(SOCKET hSocket, PACKET& packet,CLIENT& client)
{
	Msg		msg;
	BOOL	OK;	//���ݿ��Է���
	int		n;	//��ǰ����������

	while(Status == RUNING)
	{
		msg	= MsgQue.get_msg(client.dwMessageID+1);
		if(msg.dwMessageID == 0)
			return TRUE;
		client.dwMessageID	= msg.dwMessageID;

		//��ʼ��װ�·����ݰ�,���ڲ�ͬ���ݰ��в�ͬ����װ���̣������ж���
		switch(msg.CMD_ID)
		{
			case CMD_MSG_DOWN:
			case CMD_SYS_INFO:
			case CMD_MSG_PRIVATE:
				if( msg.CMD_ID == CMD_MSG_PRIVATE )
					if( hSocket != OlInfo[string(msg.szAccepter)].hSocket )	
					{
						//��˽����Ϣ���ǲ��ǽ�����
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
				//�����Ƿ������ݰ��Ľ����ߣ����򲻷���
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
				//�����Ƿ������ݰ��Ľ����ߣ����򲻷���
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
				//�����Ƿ������ݰ��Ľ����ߣ����򲻷���
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
			//////////����������װ����/////////////////
		}
		if(OK)
		{
			if ( need_encrypt )
			{
				//������Ϣ����
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

//�����̣߳�ÿ���ͻ������ӽ�����һ�������߳�,����Ϊ�����ӵ��׽���
unsigned __stdcall ServerThread(void* Socket)
{
	CLIENT			client	= { 0 };
	PACKET			packet;
	sockaddr_in		stClientAddr;
	SOCKET			hSocket = (SOCKET)Socket;
	int				len = sizeof(stClientAddr);

	if(getpeername(hSocket,  (sockaddr*)&stClientAddr, &len))
		MessageBox( hwnd, "��ȡ��ǰ�������ӵ��û���Ϣʧ��","��ʾ", 0);
	/*freopen("c:\\1.txt","a+",stdout);
	printf( "�Է�IP��%s ", inet_ntoa(stClientAddr.sin_addr));
	printf( "�Է�PORT��%d ", ntohs(stClientAddr.sin_port));*/
	client.dwMessageID	= MsgQue.ID;	//��Ϣ���
	
/////////////��ʼ���������ӵĵ�һ�����ݰ�������Ӧ����//////////////

	if (RecvPacket( hSocket, &packet, sizeof(packet) ) == FALSE)
	{
		closesocket(hSocket);
		return 0;
	}
	////ע��͵�½���0����ʾ�ɹ�
	if (packet.Head.CMD_ID == CMD_REGISTER)		
	{
		//ע�ᴦ��
		//û�����ݿ���ϵĴ�����
		//if( UserData.count( string(packet.stRegister.szNewUser) ) != 0)
		DB_QUERY(packet.stRegister.szNewUser );
		if( !strcmp(SQL_RESULT[0].user_name, packet.stRegister.szNewUser ) )	//����û�������
		{
			closesocket(hSocket);
			return 0;
		}

/////////////////////////���û���Ϣ��ӽ����ݿ�/////////////////////////////////////
		::DB_INSERT(packet.stRegister.szNewUser, packet.stRegister.szPassword);
///////////////////////////�ύ������Ż���Ч///////////////////////////////////////
		Commit();

		//δʹ�����ݿ�ʱ���û���Ϣ���淽ʽ
		UserData.insert( make_pair(string(packet.stRegister.szNewUser), string(packet.stRegister.szPassword) ) );	//�û���Ϣ��ӽ����ݿ�
		
		packet.Head.CMD_ID				= CMD_REGISTER_RESP;
		packet.Head.Timestamp			= time(0);
		packet.Head.dwLength			= sizeof(packet.Head) + sizeof(packet.stRegisterResp);
		packet.stRegisterResp.bResult	= 0;	
		
		//ע����������������ֱ�ӽ������Ӳ�����
		send(hSocket, (char*)&packet, packet.Head.dwLength, 0);
		closesocket(hSocket);
		return 0;
	}
	else if(packet.Head.CMD_ID == CMD_LOGIN) //��½����
	{
		//����½�û����˺�����
		BOOL	bResult = CheckUser(&packet);

		//��¼�û���Ϣ
		lstrcpy( client.szUserName, packet.stLogin.szUserName);

		packet.Head.CMD_ID			= CMD_LOGIN_RESP;
		packet.Head.Timestamp		= time(0);
		packet.Head.dwLength		= sizeof(packet.Head) + sizeof(packet.stLoginResp);
		packet.stLoginResp.bResult	= bResult;
		
		//����½�ɹ�������Ҫ����
		if ( (bResult==0) && need_encrypt )
		{
			packet.stLoginResp.bResult	= LOGIN_SUCCESS_ENCRYPT;
			memcpy(packet.stLoginResp.salt, salt, 8);
		}

		//�����������ʧ�ܻ��¼ʧ������
		if(  (send(hSocket, (char*)&packet, packet.Head.dwLength, 0) == SOCKET_ERROR ) || bResult)	
		{
			closesocket(hSocket);
			return 0;
		}
	}
	else
	{
		//���ݰ��������⣬û���������
		closesocket(hSocket);
		return 0;
	}

///////////////////////////////////////////////////////////////////

	//���˵�½�ɹ������������û���Ϣ�����
	UserInfo	ui;
	ui.hSocket	= hSocket;
	ui.IP		= stClientAddr.sin_addr.S_un.S_addr;
	ui.Timestamp= packet.Head.Timestamp;
	OlInfo.insert( make_pair( string(client.szUserName), ui) );

	dwUserCount++;
	SetDlgItemInt(hwnd, IDC_COUNT, dwUserCount, 0);
/////////////////��ӷ����������û��б�//////////////////////////
	char IP[20], PORT[20], TIME[20];
	struct tm* timeinfo;
	wsprintfA(IP,   "%s", inet_ntoa(stClientAddr.sin_addr));
	wsprintfA(PORT, "%d", ntohs(stClientAddr.sin_port));
	//��ȡʱ����Ϣ
	timeinfo	= GetTimeInfo( ui.Timestamp );
	wsprintfA(TIME, "[%02d:%02d:%02d]  ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	AddUserInfo( client.szUserName, IP, TIME);

	//���¿ͻ��������û��б�
	update(hSocket, &packet, &client);
	
	//ϵͳ�㲥�û��ĵ�½
	char szBuffer[MAX_LENGTH];
	wsprintfA(szBuffer, "��ϵͳ��Ϣ����ӭ%s���������ң�", client.szUserName);
	MsgQue.push_back(  Msg(ui.Timestamp, client.szUserName, szBuffer, CMD_SYS_INFO)  );
	client.dwLastTime	= GetTickCount();

	int n;	//�����������������Խ������ݵ�����
	//������Ϣ����ѭ��
	while( Status == RUNING )
	{
		//������Ϣ�����е���Ϣ�������·�����Ƿ�����
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
			
////////////////////////�Բ�ͬ���ݰ����д���///////////////////////////////
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
	wsprintfA(szBuffer, "��ϵͳ��Ϣ��%s�˳��������ҡ�", client.szUserName);
	MsgQue.push_back(  Msg( time(0), client.szUserName, szBuffer, CMD_SYS_INFO)  );
	//SendMsg(hSocket, packet, client);
	closesocket(hSocket);

	//ɾ�������û���Ϣ
	OlInfo.erase(OlInfo.find( string(client.szUserName) ) ); 

	//�б���ɾ�����û���Ϣ
	DelUserInfo(client.szUserName );

	dwUserCount--;
	SetDlgItemInt(hwnd, IDC_COUNT, dwUserCount, 0);

	return 0;
}

//�����̣߳���������XXXX�˿ڵ���������
unsigned __stdcall ListenThread(void* param)
{
	SOCKET		hSocket;
	sockaddr_in	stSocket;
	DWORD		dwThreadID;

//////////////////////////////���ܳ�ʼ������/////////////////////////////////		
	//��ȡ8�ֽ����������Ϊ"������"	
	::rng_get_bytes(salt,8,NULL);
	if ( encrypt.init(salt) )
			need_encrypt = TRUE;
/////////////////////////////////////////////////////////////////////////////

///////////////////////////���ݿ����Ӳ���/////////////////////////////////////
	if(!Connect())
	{
		MessageBox(NULL, TEXT("���ݿ�����ʧ��"), TEXT("����"), MB_ICONWARNING);
		return 0;
	}
	
	
	memset(&stSocket, 0, sizeof(stSocket));
	hListenSocket		= socket(AF_INET,SOCK_STREAM,0);
	stSocket.sin_port	= htons(NET_PORT);
	stSocket.sin_family	= AF_INET;
	stSocket.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind( hListenSocket, (sockaddr*)&stSocket, sizeof(stSocket)) )
	{
		MessageBox(hwnd, TEXT("�޷���TCP��%d�Ŷ˿ڣ�����˿��Ƿ���������ռ��"), TEXT("��ʾ"), MB_ICONWARNING);
		ExitProcess(0);		//ֱ���˳�
	}

	//��������������ӵļ���
	listen(hListenSocket,4);
	while(Status == RUNING)
	{
		hSocket = accept(hListenSocket, NULL, 0);
		if (hSocket==INVALID_SOCKET)
			break;

		//����ͨѶ�����߳�
		chBEGINTHREADEX(NULL, 0, ServerThread, hSocket, 0, &dwThreadID);
	}
	closesocket(hListenSocket);

//////////////////////////�Ͽ����ݿ�����/////////////////////////////
	DisConnect();
	return 0;
}


//�б�������û���Ϣ
void	AddUserInfo(char* szUserName, char* szIP, char* szLoginTime)
{
	int i;
	i = ListView_AddLine(hListView);//���һ��
	ListView_SetItemText(hListView, i, 0, szUserName);
	ListView_SetItemText(hListView, i, 1, szIP);
	ListView_SetItemText(hListView, i, 2, szLoginTime);
}

//ɾ���б���ѡ���е���Ϣ
void	DelUserInfo()
{
	int i;
	i = ListView_GetSelectionMark(hListView);  //���ѡ��������
	ListView_DeleteItem(hListView, i);
}

//ɾ���б���ָ���û���Ϣ
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


//ͨѶ��·�ļ�⣬ͨ���������ݰ����������Ƿ����
//����ֵΪ0���޻���ͷ�����
int		LinkCheck(SOCKET hSocket,PACKET* packet,CLIENT*	lpClient)
{
	//�������30���޻������̽�����ݼ������
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

//��¼�û����,����0��ʾ�ɹ�
BOOL	CheckUser(PACKET* lpPacket)
{
	string name(lpPacket->stLogin.szUserName);
	//���ݿ��м���û���Ϣ
	DB_QUERY(lpPacket->stLogin.szUserName);
	//������֤���ظ���½��֤
	if( (OlInfo.count( name ) == 0) && !strcmp(SQL_RESULT[0].passwd, lpPacket->stLogin.szPassword) )
		return 0;
	return 1;
////////////δʹ�����ݿ�ʱ�ļ�ⷽ��//////////////////
	//����Ƿ���ڱ��û�
	if( UserData.count( name ) == 0 )
		return 1;
	//�������
	if( UserData[name] != string(lpPacket->stLogin.szPassword) )
		return 1;
	//����Ƿ�����ظ���¼
	if( OlInfo.count( name ) != 0)
		return 1;
	return 0;
}

//����ѡ���û�
int		BanUser()
{
	char	szBuffer[64];
	int	idx = ListView_GetSelectionMark( hListView);
	if(idx==-1)	return idx;
	ListView_GetItemText(hListView, idx, 0, szBuffer, sizeof(szBuffer) );
	MsgQue.push_back(  Msg( time(0), NULL, NULL,CMD_USER_BAN,szBuffer )  );
	return TRUE;
}

//�߳�ѡ���û�
int		KickUser()
{
	char	szBuffer[64];
	int	idx = ListView_GetSelectionMark( hListView);
	if(idx==-1)	return idx;
	ListView_GetItemText(hListView, idx, 0, szBuffer, sizeof(szBuffer) );
	MsgQue.push_back(  Msg( time(0), NULL, NULL,CMD_KICK_OUT,szBuffer )  );
	return TRUE;
}

//���ͶԿͻ��˽����û��б���µ����ݰ�
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
