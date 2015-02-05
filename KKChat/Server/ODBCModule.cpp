#include "ODBCModule.h"

#define MAX_RES	 128  //�������ɴ���������

//�洢��ѯ���
SQL_OUTPUT	SQL_RESULT[MAX_BUF];

SQLHANDLE	hEnv;				//ODBC�������
SQLHANDLE	hConn;				//ODBC���Ӿ��
static SQLCHAR	szConnString[1024]	= { 0 };	//ODBC�����ַ���
static SQLCHAR	szFullString[1024]	= { 0 };	//���Ӻ󷵻ص�ȫ�ַ���
static SQLCHAR	szSQL[1024]			= { 0 };	//�����׼��ִ�е�SQL���

static char szErrConn[]	= "�޷����ӵ����ݿ�!";
static char szOkCaption[]="�ɹ����ӵ����ݿ⣬�����������ַ������£�";
static char szErrDDL[]	= "DDL/DCL ����ѳɹ�ִ�С�";
static char szErrDML[]	= "DML ����ѳɹ�ִ�У�Insert/Update/Delete��������%d��";
static char szErrDQL[]	= "��ѯ����Ѿ��ɹ�ִ�У��õ��Ľ�������£�";

extern HWND	ChildhWnd2;	//��ʾ��Ϣ��Ҫ�ĶԻ�����


//�ͷŽ����Ϊ���ֶ�����Ļ������ڴ�
void RsClose(ODBC_RS	*lpRs)
{
	int i;
	for (i = 0; i < lpRs->dwCols; i++)
		if(lpRs->lpField[i])
			free(lpRs->lpField[i]);
	memset(lpRs, 0, sizeof(ODBC_RS));
}

//�����������������Ϊÿ���ֶ�Ԥ�����뻺��������Bind���������
//���أ�ʧ��,FALSE,�ɹ�TRUE
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
	
	//Ϊÿ�������ڴ沢�󶨵�statment���
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

//��ȡ�������������ָ������ֶε�����
//���أ�ʧ�ܷ���NULL������Ϊָ���ֶ������ַ�����ָ��
char *RsGetField(ODBC_RS *lpRs, DWORD id)
{
	if( (short)id < lpRs->dwCols)
		return lpRs->lpField[id];
	return NULL;
}

//�ɹ�����TRUE���������ĩβ��ʧ��FALSE
BOOL RsMoveNext(ODBC_RS	*lpRs)
{
	//Ԥ������Ļ�����
	for(int i=0; i<lpRs->dwCols; i++)
		if(lpRs->lpField[i])
			lpRs->lpField[i][0] = 0 ;
	//���α��ƶ�����һ����¼���������ݻ�ȡ���ֶλ�������
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
	//���뻷����������Ӿ��
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
	
	//�������ݿ�
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

//�Ͽ����ݿ�����
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

//ִ��sql���
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
	//ִ��sql���
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
	//ִ�гɹ�,�����DML��䣬�������Ӱ�������
	::SQLNumResultCols( hStmt, &dwRecordCols);
	dwRecordCols &= 0xffff;
	if (!dwRecordCols)
	{
		::SQLRowCount(hStmt, &dwResultRows);
		if(display)
		{
			char str[256] = { 0 };
			if(dwResultRows==-1)
				SetDlgItemText(ChildhWnd2, IDC_STATIC1,"DDL/DCL ����ѳɹ�ִ�С�");
			else
			{
				wsprintf(str, "DML ����ѳɹ�ִ�У�Insert/Update/Delete��������%d��", dwResultRows);
				SetDlgItemText(ChildhWnd2, IDC_STATIC1, str);
			}
		}
		SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
	}
	//�����select��䣬���������Ϣ
	for(int i=1; i<=dwRecordCols; i++)
	{
		SQLDescribeCol(hStmt, i,\
				 szName,sizeof(szName),&dwNameSize,\
				 &dwType, &dwSize, &dwSize2, &dwNullable);
		//����洢����Ϣ������
		if(display)
		{
			if(dwSize*8>300)	dwSize=300;
			if(dwSize*8<40)		dwSize=40;
			::ListView_InsertCaption(GetDlgItem(ChildhWnd2, IDC_LIST1), i, dwSize*8, (LPTSTR)szName);
		}
	}
	//��������
	char	*info;
	int		count=0, line;
	RsOpen(&stRs, hStmt);
	memset(SQL_RESULT, 0, sizeof(SQL_RESULT));
	while (1)
	{
		if (!RsMoveNext(&stRs) )	break;
		if(display)	line=ListView_AddLine(GetDlgItem(ChildhWnd2, IDC_LIST1));
		//ѭ�����һ���������е���Ϣ
		for(int i=0; i<dwRecordCols; i++)
		{
			info	= RsGetField(&stRs, i);
			//�����õ����ݿ���Ϣ
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

//�ύ����
void Commit()
{
	SQLEndTran(SQL_HANDLE_DBC,hConn,SQL_COMMIT);
}

//�ع�����
void Rollback()
{
	SQLEndTran(SQL_HANDLE_DBC,hConn,SQL_ROLLBACK);
}