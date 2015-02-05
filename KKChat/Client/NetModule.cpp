#include "NetModule.h"


//�ȴ��������ݵ���(dwTimeָ���ȴ�ʱ��/΢��)
int WaitData (SOCKET hSocket, DWORD dwTime)
{
	fd_set	stFdSet;
	timeval	stTimeval;

	stFdSet.fd_count	= 1;
	stFdSet.fd_array[0]	= hSocket;
	stTimeval.tv_sec	= 0;
	stTimeval.tv_usec	= dwTime;

	return select (0, &stFdSet, NULL, NULL, &stTimeval);
}


//����ָ���ֽ�����С�����ݣ�ֱ�����յ��������ݲŻ᷵��
BOOL RecvData (SOCKET hSocket, char* lpPacket, DWORD dwSize)
{
	DWORD dwStartTime;
	dwStartTime	= GetTickCount ();
	DWORD	res;
	char*	lpData = lpPacket;

	while (1)
	{
		if ( (GetTickCount() - dwStartTime) >= 10 * 1000)	//����Ƿ�ʱ(10s)
			return FALSE;

		//�ȴ�����(100ms), �������ֵΪ0��ʱ��������һ�εȴ�
		if ( (res = WaitData (hSocket, 100 * 1000) ) == SOCKET_ERROR )	
			return FALSE;
		if (!res)	
			continue;
		
		if ( (res = recv (hSocket, lpData, dwSize, 0)) == SOCKET_ERROR || !res)
			return FALSE;
		if ( res < dwSize)
		{
			lpData += res;
			dwSize -= res;
		}
		else
			return TRUE;
	}
}


//����һ�����������ݰ������ʧ���򷵻�FALSE������TRUE
BOOL RecvPacket (SOCKET hSocket, PACKET* lpBuffer, DWORD dwSize)
{
	DWORD	size;

	//�������ݰ�ͷ��������ݰ��Ƿ�����
	if (RecvData (hSocket, (char*)lpBuffer, sizeof (PACKET_HEAD) ) == FALSE)
		return FALSE;
	if (lpBuffer->Head.dwLength < sizeof (PACKET_HEAD) || lpBuffer->Head.dwLength > dwSize )
		return TRUE;

	//�������µİ�����
	size	= lpBuffer->Head.dwLength - sizeof (PACKET_HEAD);
	if (size==0) return TRUE;
	if (size)
	{
		RecvData (hSocket, (char*)lpBuffer + sizeof (PACKET_HEAD), size);
		return TRUE;
	}
	else
		return FALSE;
}

//��������,�ܹ���time(0)���ص�ʱ��ת��Ϊ�ɶ��ı���ʱ��
struct tm* GetTimeInfo(time_t Time)
{
    struct tm* info;
	info	= localtime ( &Time );
	return  info;
}