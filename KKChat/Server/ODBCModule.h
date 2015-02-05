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
	SQLHSTMT	hStmt;			//执行语句用的 StateMent 句柄
	SQLSMALLINT dwCols;			//当前结果集列数
	char*	lpField[128];	//预留缓冲区的指针
	SQLINTEGER  dwTemp;
};

//用于存储输出结果
struct SQL_OUTPUT
{
	char	user_name[18];
	char	passwd[18];
	char	time[18];
};



static SQLCHAR szDefConnStr[]= "Driver={Microsoft Access Driver (*.mdb)};dbq=database.mdb";	//数据库连接字符串

//连接数据库，成功返回TRUE，否则返回FALSE
BOOL	Connect();

//提交操作
void Commit();

//回滚操作
void Rollback();

//断开数据库连接
void DisConnect();

//执行sql语句,display参数如果非0，则将显示sql语句执行结果
void Execute(char *szSQL,int display=0);

#endif