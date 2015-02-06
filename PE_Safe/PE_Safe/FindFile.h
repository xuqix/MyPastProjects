#ifndef FINDFILE_H
#define FINDFILE_H

long long  dwFileSize;		//��¼Ŀ¼�ļ��ܴ�С
unsigned int dwFileCount;	//��¼�ļ���
unsigned int dwFolderCount;	//��¼�ļ�����


void ProcessFile (char szFile[])//�����ļ�ģ��
{
	HANDLE hFile;
	dwFileCount++;
	
	if (INVALID_HANDLE_VALUE != (hFile = CreateFileA (szFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0) ) )
		dwFileSize += GetFileSize (hFile, NULL);
	CloseHandle (hFile);
}

void FindFile (char szPath[])//Ŀ¼����ģ��
{
	HANDLE	hFindFile;
	WIN32_FIND_DATAA stFindFile;
	char szSearch[MAX_PATH];
	char szFindFile[MAX_PATH];

	int len = lstrlen (szPath);
	if (szPath[len-1] != L'\\')
	{
		szPath[len] =	L'\\';
		szPath[len+1]	= 0;
	}	
	lstrcpyA (szSearch, szPath);
	lstrcatA (szSearch, TEXT("*.*") );

	bool res;
	if ( INVALID_HANDLE_VALUE != ( hFindFile = FindFirstFileA (szSearch, &stFindFile ) ))
		do
		{
			lstrcpyA (szFindFile, szPath);
			lstrcatA (szFindFile, stFindFile.cFileName);
			if (stFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if ( stFindFile.cFileName[0] != '.'  )
				{
					dwFolderCount++;
					FindFile(szFindFile);
				}
			}
			else
				ProcessFile( szFindFile);
			res	= FindNextFileA (hFindFile, &stFindFile);				
		}while ( (res != FALSE) );
		FindClose (hFindFile);
}

#endif