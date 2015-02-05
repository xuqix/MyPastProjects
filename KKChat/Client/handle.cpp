#include "handle.h"

extern HWND	hChat;

extern char	szErrInfo[64];
extern char	szUserName[20];
extern char	szPassword[20];
extern char	szServerIP[20];

extern Encrypt			encrypt;		//�����࣬�������ݰ�����
extern BOOL				need_encrypt;//�Ƿ��ܼ���
extern unsigned char	salt[8];//������
extern volatile DWORD	Status;				//��¼��½״̬,����Ϊ״̬����

extern DWORD	dwThreadID;
extern DWORD	dwLastTime;			//���һ�η������ݱ���ʱ��
extern DWORD	dwTimeCount;		//��ʱ�����������ʱ��

extern DWORD	NET_PORT;			//�洢�˿ں�	
extern SOCKET	hSocket;
extern BOOL		CantSpeak;			//����λ

//////////////////ע�����û��������ɹ�����TRUE�����򷵻�FALSE//////////////////////////////
///////////////////Ϊ���������ע���û�����һ�ε���TCP������///////////////////////////////
BOOL RegisterNewUser(HWND hDlg)
{
	DWORD	count;
	BOOL	flag;

	lstrcpyA( szErrInfo, "��Ǹ��ע��ʧ�ܡ�������һЩ����");
	count = GetDlgItemTextA(hDlg, IDC_USER, szUserName, sizeof(szUserName));
	if(count==0)	{ lstrcatA( szErrInfo, "\r\n������Ϣ���û��������벻��Ϊ��"); return FALSE; }
	count = GetDlgItemTextA(hDlg, IDC_PASS, szPassword, sizeof(szPassword));
	if(count==0)	{ lstrcatA( szErrInfo, "\r\n������Ϣ���û��������벻��Ϊ��"); return FALSE; }
	count = GetDlgItemTextA(hDlg, IDC_SERVER, szServerIP, sizeof(szServerIP));
	if(count==0)	{ lstrcatA( szErrInfo, "\r\n������Ϣ����������ȷ�ķ�����IP��ַ"); return FALSE; }
	NET_PORT	= GetDlgItemInt(hDlg, IDC_PORT, &flag, TRUE); 

	Status	= LOGIN_ING;	//���õ�½��״̬


	PACKET		packet;		//�Զ�������ݰ�
	sockaddr_in	stSocket;	//����������Ҫ�������Ϣ
	DWORD		ip_v4;		//32λip��ַ

	memset( &packet, 0, sizeof(packet) );
	memset( &stSocket, 0, sizeof(stSocket) );
	if ( (ip_v4=inet_addr( szServerIP)) == INADDR_NONE )
	{
		//IP��ַ���Ϸ�����ô�����Ϣ���õ�½״̬Ϊ��½ʧ�ܣ�LOGIN_FAIL��
		lstrcatA(szErrInfo, "\r\n������Ϣ����Ч�ķ�����IP��ַ������IP����");
		Status	= LOGIN_FAIL;
		return FALSE;
	}

	//��д��ַ�ṹ������socket�׽���
	stSocket.sin_addr.S_un.S_addr	= ip_v4;
	stSocket.sin_family = AF_INET;
	stSocket.sin_port	= htons( (u_short)NET_PORT);
	
	hSocket				= socket(AF_INET,SOCK_STREAM,0 );
	
	//���������������TCP����
	if ( connect( hSocket, (SOCKADDR*)&stSocket, sizeof(stSocket)) == SOCKET_ERROR )
	{
		lstrcatA(szErrInfo, "\r\n������Ϣ���޷������������������ӣ�������");
		Status	= LOGIN_FAIL;
		if(hSocket)		//������Ҫ���� 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return 0;
	}

	//�ɹ����ӵ�����������ʼ��װע�����ݰ�
	packet.Head.CMD_ID		= CMD_REGISTER;//��ʱ���õ�½���ݰ����ȴ�������������������
	packet.Head.dwLength	= sizeof(packet.Head) + sizeof(packet.stRegister);
	packet.Head.Timestamp	= time(NULL);
	lstrcpyA(packet.stRegister.szNewUser, szUserName);
	lstrcpyA(packet.stRegister.szPassword, szPassword);

	//����ע�����ݰ�,��һ����������ݰ�Ӧ��Ľ��պ�ע�����ļ��
	if( (send(hSocket, (char*)&packet, packet.Head.dwLength, 0) == SOCKET_ERROR) || (RecvPacket(hSocket, &packet,sizeof(packet) ) == FALSE)\
		|| (packet.stRegisterResp.bResult == LOGIN_FAIL) )
	{
		Status	= LOGIN_FAIL;
		if(hSocket)		//������Ҫ���� 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return FALSE;
	}

	//�ɹ��õ����ݰ�Ӧ��,����������ݽӷ�ʱ��
	dwLastTime	= GetTickCount();
	//�ر�����
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
#define LOGIN_CANCEL	-1	//�˳� 
*/
//���������Ǹ�while(Status != LOGIN_CHAT)	;release��ᱻ�Ż�������������ʹ��#pragmaָʾ��ȡ���Ż�
//#pragma optimize("",off)
unsigned __stdcall WorkThread(void* param)
{
	PACKET		packet;		//�Զ�������ݰ�
	sockaddr_in	stSocket;	//����������Ҫ�������Ϣ
	DWORD		ip_v4;		//32λip��ַ

	memset( &packet, 0, sizeof(packet) );
	memset( &stSocket, 0, sizeof(stSocket) );
	if ( (ip_v4=inet_addr( szServerIP)) == INADDR_NONE )
	{
		//IP��ַ���Ϸ�����ô�����Ϣ���õ�½״̬Ϊ��½ʧ�ܣ�LOGIN_FAIL��
		lstrcpyA(szErrInfo, "��Ч�ķ�����IP��ַ������IP����");
		Status	= LOGIN_FAIL;
		return 0;
	}

	//��д��ַ�ṹ������socket�׽���
	stSocket.sin_addr.S_un.S_addr	= ip_v4;
	stSocket.sin_family = AF_INET;
	stSocket.sin_port	= htons( (u_short)NET_PORT);
	
	hSocket				= socket(AF_INET,SOCK_STREAM,0 );
	
	//���������������TCP����
	if ( connect( hSocket, (SOCKADDR*)&stSocket, sizeof(stSocket)) == SOCKET_ERROR )
	{
		lstrcpyA(szErrInfo, "�޷������������������ӣ�������");
		Status	= LOGIN_FAIL;
		if(hSocket)		//������Ҫ���� 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return 0;
	}

	//�ɹ����ӵ�����������ʼ��װ��¼���ݰ�
	packet.Head.CMD_ID		= CMD_LOGIN;
	packet.Head.dwLength	= sizeof(packet.Head) + sizeof(packet.stLogin);
	packet.Head.Timestamp	= time(NULL);
	lstrcpyA(packet.stLogin.szUserName, szUserName);
	lstrcpyA(packet.stLogin.szPassword, szPassword);

	//���͵�½���ݰ�,��һ����������ݰ�Ӧ��Ľ��պ͵�½����ļ��
	if( (send(hSocket, (char*)&packet, packet.Head.dwLength, 0) == SOCKET_ERROR) || (RecvPacket(hSocket, &packet,sizeof(packet) ) == FALSE)\
																							|| (packet.stLoginResp.bResult == LOGIN_FAIL) )
	{
		lstrcpyA(szErrInfo, "�޷���½�����������������û������������");
		Status	= LOGIN_FAIL;
		if(hSocket)		//������Ҫ���� 
		{ 
			closesocket(hSocket);
			hSocket	= 0;
		}
		return 0;
	}
	
	//�ɹ��õ����ݰ�Ӧ��,����������ݽӷ�ʱ��
	dwLastTime	= GetTickCount();

	//����Ƿ���Ҫ��Ϣ���ܲ���ü�����
	if( packet.stLoginResp.bResult == LOGIN_SUCCESS_ENCRYPT)
	{
		need_encrypt	= TRUE;
		memcpy( salt, packet.stLoginResp.salt, 8 );
		encrypt.init(salt);
	}

	//���˵�½�ɹ�������״̬����������ʼѭ��������Ϣ
	Status		= LOGIN_SUCCESS;
	int n;
	while(hSocket)
	{
		//��ⳬʱ(1min)
		if( (GetTickCount() - dwLastTime) > 60 * 1000)
			break;

		if( (n=WaitData(hSocket, 255 * 1000)) == SOCKET_ERROR)	//�ȴ�����255ms
			break;

		if(n==0) continue;
		//�������ݰ�
		if(RecvPacket( hSocket, &packet, sizeof(packet) )==FALSE )
			break;

		//���ݰ�����
		if( need_encrypt )
		{
			unsigned char* lpData = (unsigned char*)&packet + sizeof(PACKET_HEAD);
			encrypt.decrypt( lpData, lpData, packet.Head.dwLength-sizeof(PACKET_HEAD) ) ;			
		}

		while(Status != LOGIN_CHAT)	;	//����Ի���δ��ɳ�ʼ����һֱ�ȴ�

/////////////////���ڲ�ͬ�����ݰ����зֱ���////////////////
		switch(packet.Head.CMD_ID)
		{
			case CMD_MSG_DOWN:	//һ����Ϣ
				::CMD_MSG_DOWN_HANDLE(&packet);
				break;

			case CMD_SYS_INFO:	//ϵͳ��Ϣ
				::CMD_SYS_INFO_HANDLE(&packet);
				break;

			case CMD_USER_LIST:	//�б���Ϣ����
				::CMD_USER_LIST_HANDLE(&packet);
				break;

			case CMD_MSG_PRIVATE://˽����Ϣ
				::CMD_MSG_PRIVATE_HANDLE(&packet);
				break;

			case CMD_USER_BAN:	//�û�����
				::CMD_USER_BAN_HANDLE(&packet);
				break;

			case CMD_KICK_OUT:	//�߳��û�
				::CMD_KICK_OUT_HANDLE(&packet);
				break;
			
			default:
				break;
///////////////////////�Զ�����Ϣ���////////////////////////
		}
		dwLastTime	= GetTickCount();
	}
	return 0;
}
//�����������ָ��Ż�
//#pragma optimize("",on)


////////////////////////////////�����·���Ϣ///////////////////////////////////
int CMD_MSG_DOWN_HANDLE(PACKET* lpPacket)
{
	char szBuffer[MAX_LENGTH+16] = { 0 } ;
	struct tm* timeinfo;
	lpPacket->Head.Timestamp	= time(NULL);
	timeinfo	= GetTimeInfo( lpPacket->Head.Timestamp);
	wsprintfA(szBuffer, "[%02d:%02d:%02d]  ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	lstrcatA( szBuffer, lpPacket->stMsgDown.szSender);
	lstrcatA( szBuffer, " ��������˵��");
	lstrcatA( szBuffer, lpPacket->stMsgDown.szContent);
	lstrcatA( szBuffer, "\r\n");
	int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
	Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
	SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szBuffer);
	return 0;
}

////////////////////////////////����ϵͳ��Ϣ//////////////////////////////////////
int CMD_SYS_INFO_HANDLE(PACKET* packet)
{
	char szBuffer[MAX_LENGTH+16] = { 0 } ;
	lstrcpyA( szBuffer, "��");
	lstrcatA( szBuffer, packet->stMsgDown.szContent);
	lstrcatA( szBuffer, "��");
	lstrcatA( szBuffer, "\r\n");
	int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
	Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
	SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szBuffer);

	//����Ƿ������û���������˳������ҵ���Ϣ�������б���Ϣ����
	if( strstr(packet->stMsgDown.szContent, "����������") != NULL )
	{
		AddUserInfo( packet->stMsgDown.szSender );

		if(strcmp(szUserName, packet->stMsgDown.szSender)!=0)
			ComboBox_AddString(hCombobox, packet->stMsgDown.szSender);//�����б�
	}
	else if( strstr(packet->stMsgDown.szContent, "�˳���������") != NULL )
	{
		//���������û��б�
		DelUserInfo( packet->stMsgDown.szSender );
		//���������б�
		ComboBox_GetCurString(szBuffer);
		if(strcmp(packet->stMsgDown.szSender, szBuffer)==0)
			ComboBox_SetCurSel(hCombobox, 0);
		if(strcmp(szUserName, packet->stMsgDown.szSender)!=0)
			ComboBox_DelString(packet->stMsgDown.szSender);
	}

	return 0;
}


///////////////////////////�����б������Ϣ////////////////////////////////////////
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
		//�����û��б�ĸ���
		strcpy(szUserName, szList);
		AddUserInfo(szUserName);

		//�����б�ĸ���
		ComboBox_AddString(hCombobox, szUserName);
		j = j + strlen(szList) + 1;
		szList = packet->stUserList.szUserList + j ;
	}
	return 0;
}

//////////////////////////�������������û���˽����Ϣ////////////////////////////
int CMD_MSG_PRIVATE_HANDLE(PACKET* lpPacket)
{
	char szBuffer[MAX_LENGTH+16] = { 0 } ;
	char temp[32]={ 0 };

	struct tm* timeinfo;
	lpPacket->Head.Timestamp	= time(NULL);
	timeinfo	= GetTimeInfo( lpPacket->Head.Timestamp);
	//wsprintfA(szBuffer, "[%02d:%02d:%02d]  ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	wsprintf(temp, "�%s�����ĵض���˵��", lpPacket->stMsgDown.szSender);
	lstrcatA( szBuffer, temp);
	lstrcatA( szBuffer, lpPacket->stMsgDown.szContent);
	lstrcatA( szBuffer, "��");
	lstrcatA( szBuffer, "\r\n");
	int len = GetWindowTextLengthA ( GetDlgItem(hChat, IDC_RECORD));
	Edit_SetSel(GetDlgItem(hChat, IDC_RECORD), len, len);
	SendDlgItemMessageA(hChat,IDC_RECORD,EM_REPLACESEL, 0, (LPARAM)szBuffer);
	//˽����Ϣ��ʾ��
	PlaySound (TEXT ("msg.wav"), NULL, SND_FILENAME | SND_ASYNC) ;	//���ֲ���API
	return 0;
}

//�û���������
int CMD_USER_BAN_HANDLE (PACKET* packet)
{
	CantSpeak	= TRUE;
	dwTimeCount	= GetTickCount();
	return 0;
}

//�߳��û�����
int CMD_KICK_OUT_HANDLE (PACKET* packet)
{
	MessageBox(hChat, "�Բ������ѱ�����Ա���������", "��ʾ", MB_ICONWARNING);
	SendMessage(hChat, WM_CLOSE, 0, 0);
	return 0;
}

