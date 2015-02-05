#include "ODBCModule.h"

#define MAX_RES	 128  //定义最大可处理结果集数

//存储查询结果
SQL_OUTPUT	SQL_RESULT[MAX_BUF];

SQLHANDLE	hEnv;				//ODBC环境句柄
SQLHANDLE	hConn;				//ODBC连接句柄
static SQLCHAR	szConnString[1024]	= { 0 };	//ODBC连接字符串
static SQLCHAR	szFullString[1024]	= { 0 };	//连接后返回的全字符串
static SQLCHAR	szSQL[1024]			= { 0 };	//输入的准备执行的SQL语句

static char szErrConn[]	= "无法连接到数据库!";
static char szOkCaption[]="成功连接到数据库，完整的连接字符串如下：";
static char szErrDDL[]	= "DDL/DCL 语句已成功执行。";
static char szErrDML[]	= "DML 语句已成功执行，Insert/Update/Delete的行数：%d。";
static char szErrDQL[]	= "查询语句已经成功执行，得到的结果集如下：";

extern HWND	ChildhWnd2;	//显示信息需要的对话框句柄


//释放结果集为各字段申请的缓冲区内存
void RsClose(ODBC_RS	*lpRs)
{
	int i;
	for (i = 0; i < lpRs->dwCols; i++)
		if(lpRs->lpField[i])
			free(lpRs->lpField[i]);
	memset(lpRs, 0, sizeof(ODBC_RS));
}

//创建“结果集”——为每个字段预先申请缓冲区，并Bind到语句句柄上
//返回：失败,FALSE,成功TRUE
BOOL RsOpen(ODBC_RS	*lpRs, SQLHSTMT	hStmt)
{
	SQLCHAR szName[128];
	SQLSMALLINT	dwNameSize, dwType;
	SQLUINTEGER	dwSize; 
	SQLSMALLINT	dwSize2, dwNullable;
	short	i;

	memset(lpRs, 0, sizeof(ODBC_RS) );
	::SQLNumResultCols( hStmt, &lpRs->dwCols);
	lpRs->hStmt	= hStmt;
	if ( (lpRs->dwCols & 0xffff) == 0)
		return FALSE;
	if ( (lpRs->dwCols & 0xffff) > MAX_RES)
		lpRs->dwCols	= MAX_RES;
	
	//为每列申请内存并绑定到statment句柄
	for (i = 1; i<=lpRs->dwCols; i++)
	{
		::SQLDescribeCol( hStmt, i, szName, sizeof(szName), &dwNameSize,\
			&dwType, &dwSize, &dwSize2, &dwNullable);
		if ( !(lpRs->lpField[i-1] = (char*)malloc( dwSize*2+1 ) ) )
		{
			RsClose(lpRs);
			return FALSE;
		}
		::SQLBindCol(hStmt, i, SQL_C_CHAR, lpRs->lpField[i-1], dwSize*2+1,  &lpRs->dwTemp);
	}
	return TRUE;
}

//获取结果集缓冲区中指定编号字段的内容
//返回：失败返回NULL，否则为指向字段内容字符串的指针
char *RsGetField(ODBC_RS *lpRs, DWORD id)
{
	if( (short)id < lpRs->dwCols)
		return lpRs->lpField[id];
	return NULL;
}

//成功返回TRUE，结果集到末尾，失败FALSE
BOOL RsMoveNext(ODBC_RS	*lpRs)
{
	//预先清除的缓冲区
	for(int i=0; i<lpRs->dwCols; i++)
		if(lpRs->lpField[i])
			lpRs->lpField[i][0] = 0 ;
	//将游标移动到下一条记录，并将内容获取到字段缓冲区中
	short	res = SQLFetchScroll(lpRs->hStmt, SQL_FETCH_NEXT, 0) ;
	if ( res == SQL_SUCCESS || res == SQL_SUCCESS_WITH_INFO )
		return TRUE;
	else
		return FALSE;
}

BOOL	Connect()
{
	SQLSMALLINT	dwTemp;
	short		res;
	//申请环境句柄和连接句柄
	res	= SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE, &hEnv);
	if ( res!=SQL_SUCCESS && res!= SQL_SUCCESS_WITH_INFO )
	{
		DisConnect();
		return FALSE;
	}

	res	= SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0);
	if ( res!=SQL_SUCCESS && res!= SQL_SUCCESS_WITH_INFO )
	{
		DisConnect();
		return FALSE;
	}

	res = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hConn);
	if ( res!=SQL_SUCCESS && res!= SQL_SUCCESS_WITH_INFO )
	{
		DisConnect();
		return FALSE;
	}
	SQLSetConnectAttr( hConn,SQL_ATTR_AUTOCOMMIT,SQL_AUTOCOMMIT_OFF,0);
	
	//连接数据库
	res = SQLDriverConnect(hConn, NULL, szDefConnStr, lstrlen((LPCSTR)szDefConnStr),\
			  szFullString,sizeof(szFullString), &dwTemp,SQL_DRIVER_COMPLETE);
	if ( res==SQL_SUCCESS || res== SQL_SUCCESS_WITH_INFO )
		return TRUE;
	else
	{
		DisConnect();
		return FALSE;
	}
}

//断开数据库连接
void DisConnect()
{
	if (hConn)
	{
		SQLEndTran(SQL_HANDLE_DBC,hConn,SQL_COMMIT);
		SQLDisconnect(hConn);
		SQLFreeHandle(SQL_HANDLE_DBC,hConn);
	}
	if (hEnv)
		SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
	hConn	= 0;
	hEnv	= 0;
}

//执行sql语句
void Execute(char *szSQL,int display)
{
	SQLSMALLINT	dwTemp;
	SQLINTEGER	dwErrCode;
	SQLCHAR		szSQLState[8], szMsg[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT	dwRecordCols;
	SQLINTEGER	dwResultRows;
	SQLCHAR		szName[128];
	SQLSMALLINT	dwNameSize, dwType, dwSize2, dwNullable;
	SQLUINTEGER	dwSize;
	ODBC_RS		stRs;
	SQLHANDLE	hStmt;
	short	res;

	res = SQLAllocHandle(SQL_HANDLE_STMT,hConn, &hStmt);
	if(res != SQL_SUCCESS && res!=SQL_SUCCESS_WITH_INFO)
		return ;
	SQLSetStmtAttr(&hStmt,SQL_ATTR_CURSOR_TYPE,(SQLPOINTER)SQL_CURSOR_STATIC,0);
	//执行sql语句
	res = SQLExecDirect(hStmt, (SQLCHAR*)szSQL, lstrlen((LPCSTR)szSQL) );
	if(res != SQL_SUCCESS && res!=SQL_SUCCESS_WITH_INFO && res != SQL_NO_DATA)
	{
		memset(szMsg, 0, sizeof(szMsg) );
		SQLGetDiagRec(SQL_HANDLE_STMT,hStmt,1,\
				   szSQLState, &dwErrCode, szMsg,\
				sizeof(szMsg), &dwTemp);
		SetDlgItemText(ChildhWnd2,IDC_STATIC1, (LPCSTR)szMsg);
		SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
	}
	//执行成功,如果是DML语句，则获得语句影响的行数
	::SQLNumResultCols( hStmt, &dwRecordCols);
	dwRecordCols &= 0xffff;
	if (!dwRecordCols)
	{
		::SQLRowCount(hStmt, &dwResultRows);
		if(display)
		{
			char str[256] = { 0 };
			if(dwResultRows==-1)
				SetDlgItemText(ChildhWnd2, IDC_STATIC1,"DDL/DCL 语句已成功执行。");
			else
			{
				wsprintf(str, "DML 语句已成功执行，Insert/Update/Delete的行数：%d。", dwResultRows);
				SetDlgItemText(ChildhWnd2, IDC_STATIC1, str);
			}
		}
		SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
	}
	//如果是select语句，则获得相关信息
	for(int i=1; i<=dwRecordCols; i++)
	{
		SQLDescribeCol(hStmt, i,\
				 szName,sizeof(szName),&dwNameSize,\
				 &dwType, &dwSize, &dwSize2, &dwNullable);
		//处理存储的信息的名字
		if(display)
		{
			if(dwSize*8>300)	dwSize=300;
			if(dwSize*8<40)		dwSize=40;
			::ListView_InsertCaption(GetDlgItem(ChildhWnd2, IDC_LIST1), i, dwSize*8, (LPTSTR)szName);
		}
	}
	//处理结果集
	char	*info;
	int		count=0, line;
	RsOpen(&stRs, hStmt);
	memset(SQL_RESULT, 0, sizeof(SQL_RESULT));
	while (1)
	{
		if (!RsMoveNext(&stRs) )	break;
		if(display)	line=ListView_AddLine(GetDlgItem(ChildhWnd2, IDC_LIST1));
		//循环获得一行中所有列的信息
		for(int i=0; i<dwRecordCols; i++)
		{
			info	= RsGetField(&stRs, i);
			//处理获得的数据库信息
			if(i==0)	strcpy( SQL_RESULT[count].user_name, info);
			if(i==1)	strcpy( SQL_RESULT[count].passwd, info);
			if(i==2)	strcpy( SQL_RESULT[count].time, info);
			if(display) ListView_SetItemText(GetDlgItem(ChildhWnd2, IDC_LIST1), line, i, info);
		}
		count++;
	}

	RsClose(&stRs);
	::SQLCloseCursor(hStmt);
	::SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

//提交操作
void Commit()
{
	SQLEndTran(SQL_HANDLE_DBC,hConn,SQL_COMMIT);
}

//回滚操作
void Rollback()
{
	SQLEndTran(SQL_HANDLE_DBC,hConn,SQL_ROLLBACK);
}