#ifndef NETMODULE_H
#define NETMODULE_H

#include <windows.h>
#include <time.h>
#include "MsgProcotol.h"
#include "encrypt.h"

//����һЩ�������ݰ�������صĺ���

//�ȴ��������ݵ���(dwTimeָ���ȴ�ʱ��/΢��)
//����ֵ:
//ʧ��:����SOCKET_ERROR
//>0�����������ֵ�����Ŀ
//-1������
// 0����ʱ
int WaitData (SOCKET hSocket, DWORD dwTime);

//����ָ���ֽ�����С�����ݣ�ֱ�����յ��������ݲŻ᷵��
//���ճɹ�����TRUE�����򷵻�FALSE
BOOL RecvData (SOCKET hSocket, char* lpData, DWORD dwSize);

//����һ�����������ݰ������ʧ���򷵻�FALSE������TRUE
BOOL RecvPacket (SOCKET hSocket, PACKET* lpBuffer, DWORD dwSize);

//��������,�ܹ���time(0)���ص�ʱ��ת��Ϊ�ɶ��ı���ʱ��ṹ
//������asctime(info)ֱ��ת��Ϊ�ɶ���ʱ���ַ���
struct tm* GetTimeInfo(time_t Time);

#endif