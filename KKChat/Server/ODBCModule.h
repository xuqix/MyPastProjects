#ifndef ODBCMODULE_H
#define ODBCMODULE_H

#include <windows.h>
#include <windowsx.h>
#include <sqlext.h>
#include "resource.h"
#include "ApiMacro.h"

#define MAX_BUF	1024

struct ODBC_RS
{
	SQLHSTMT	hStmt;			//ִ������õ� StateMent ���
	SQLSMALLINT dwCols;			//��ǰ���������
	char*	lpField[128];	//Ԥ����������ָ��
	SQLINTEGER  dwTemp;
};

//���ڴ洢������
struct SQL_OUTPUT
{
	char	user_name[18];
	char	passwd[18];
	char	time[18];
};



static SQLCHAR szDefConnStr[]= "Driver={Microsoft Access Driver (*.mdb)};dbq=database.mdb";	//���ݿ������ַ���

//�������ݿ⣬�ɹ�����TRUE�����򷵻�FALSE
BOOL	Connect();

//�ύ����
void Commit();

//�ع�����
void Rollback();

//�Ͽ����ݿ�����
void DisConnect();

//ִ��sql���,display���������0������ʾsql���ִ�н��
void Execute(char *szSQL,int display=0);

#endif