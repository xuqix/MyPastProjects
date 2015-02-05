#include "NetModule.h"


//等待网络数据到达(dwTime指定等待时间/微秒)
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


//接受指定字节数大小的数据，直到接收到所有数据才会返回
BOOL RecvData (SOCKET hSocket, char* lpPacket, DWORD dwSize)
{
	DWORD dwStartTime;
	dwStartTime	= GetTickCount ();
	DWORD	res;
	char*	lpData = lpPacket;

	while (1)
	{
		if ( (GetTickCount() - dwStartTime) >= 10 * 1000)	//检测是否超时(10s)
			return FALSE;

		//等待数据(100ms), 如果返回值为0则超时，继续下一次等待
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


//接收一个完整的数据包，如果失败则返回FALSE，否则TRUE
BOOL RecvPacket (SOCKET hSocket, PACKET* lpBuffer, DWORD dwSize)
{
	DWORD	size;

	//接收数据包头并检测数据包是否正常
	if (RecvData (hSocket, (char*)lpBuffer, sizeof (PACKET_HEAD) ) == FALSE)
		return FALSE;
	if (lpBuffer->Head.dwLength < sizeof (PACKET_HEAD) || lpBuffer->Head.dwLength > dwSize )
		return TRUE;

	//接收余下的包数据
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

//帮助函数,能够将time(0)返回的时间转换为可读的本地时间
struct tm* GetTimeInfo(time_t Time)
{
    struct tm* info;
	info	= localtime ( &Time );
	return  info;
}