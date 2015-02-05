#ifndef NETMODULE_H
#define NETMODULE_H

#include <windows.h>
#include <time.h>
#include "MsgProcotol.h"
#include "encrypt.h"

//定义一些网络数据包操作相关的函数

//等待网络数据到达(dwTime指定等待时间/微秒)
//返回值:
//失败:返回SOCKET_ERROR
//>0：就绪描述字的正数目
//-1：出错
// 0：超时
int WaitData (SOCKET hSocket, DWORD dwTime);

//接受指定字节数大小的数据，直到接收到所有数据才会返回
//接收成功返回TRUE，否则返回FALSE
BOOL RecvData (SOCKET hSocket, char* lpData, DWORD dwSize);

//接收一个完整的数据包，如果失败则返回FALSE，否则TRUE
BOOL RecvPacket (SOCKET hSocket, PACKET* lpBuffer, DWORD dwSize);

//帮助函数,能够将time(0)返回的时间转换为可读的本地时间结构
//可以用asctime(info)直接转换为可读的时间字符串
struct tm* GetTimeInfo(time_t Time);

#endif