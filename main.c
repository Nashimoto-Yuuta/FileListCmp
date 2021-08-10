#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

LPCTSTR szVERSION = TEXT("0.1.0");
LPTSTR szDirectory1 = NULL;
LPTSTR szDirectory2 = NULL;

// Goes through the directory and stores all files into pData
void GetFileListCount(LPCTSTR szDirectory, WIN32_FIND_DATA* pData, LPDWORD dwFileNum);
BOOL HasStringInFileList(LPCTSTR szString, WIN32_FIND_DATA* pData, DWORD dwMaxNum);
BOOL IsStdOutRedirected();

void PrintInfo()
{
  _tprintf(TEXT("FileListCmp %tsv\nThis program compares the files from the first directory against the files in the second directory for duplicate filenames by displaying the file list of the first directory color coded."), szVERSION);
}

void PrintHelp()
{
  LPCTSTR      szHelp = TEXT("Usage : FileListCmp [option] [parameter]\n")
                        TEXT("Options:\n")
                        TEXT("\t-h, -help\tShow this message\n")
                        TEXT("\t-info\t\tDisplay the description of this program\n")
                        TEXT("\t-d1\t\tDirectory1 location\n")
                        TEXT("\t-d2\t\tDirectory2 location\n");

  _tprintf(TEXT("%ts\n"), szHelp);
}

BOOL Init()
{
#ifdef UNICODE
  // Sets stdout to UTF16 if this compilation happens in unicode mode
  return _setmode(_fileno(stdout), _O_WTEXT) != -1;
#else
  return true;
#endif
}

BOOL DirectoryExists(LPCTSTR szPath)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
  if (!Init())
  {
    _tprintf(TEXT("Failed to set locale %lu"), errno);
    return 1;
  }

  if (argc == 1 || (argc == 2 && (_tcscmp(argv[1], TEXT("-help")) == 0 ||
                                  _tcscmp(argv[1], TEXT("-h"))    == 0) ))
  {
    PrintHelp();
    return 0;
  }

  for(int count = 1; count < argc; count++)
  {
    if (_tcscmp(argv[count], TEXT("-d1")) == 0)
    {
      if (count+1 < argc)
      {
        szDirectory1 = argv[count+1];
        count++;
      }
      else
      {
        _tprintf(TEXT("Error : d1 not fully defined!"));
        return 1;
      }
    }
    else
    if (_tcscmp(argv[count], TEXT("-d2")) == 0)
    {
      if (count+1 < argc)
      {
        szDirectory2 = argv[count+1];
        count++;
      }
      else
      {
        _tprintf(TEXT("Error : d2 not fully defined!"));
        return 1;
      }
    }
    else
    if (_tcscmp(argv[count], TEXT("-help")) == 0 ||
        _tcscmp(argv[count], TEXT("-h"))    == 0  )
    {
      PrintHelp();
      return 0;
    }
    else
    if (_tcscmp(argv[count], TEXT("-info")) == 0)
    {
      PrintInfo();
      return 0;
    }
    else
    {
      _tprintf(TEXT("Unknown option <%ls>"), argv[count]);
      return 1;
    }
  }

  if (szDirectory1 == NULL)
  {
    _tprintf(TEXT("Error : First folder not specified!"));
    return 1;
  }

  if (szDirectory2 == NULL)
  {
    _tprintf(TEXT("Error : Second folder not specified!"));
    return 1;
  }

  if (DirectoryExists(szDirectory1) == 0)
  {
    _tprintf(TEXT("Error : First folder doesn't exist!"));
    return 1;
  }

  if (DirectoryExists(szDirectory2) == 0)
  {
    _tprintf(TEXT("Error : Second folder doesn't exist!"));
    return 1;
  }

  // Count all files
  DWORD dwFileCount1 = 0;
  DWORD dwFileCount2 = 0;
  GetFileListCount(szDirectory1, NULL, &dwFileCount1);
  GetFileListCount(szDirectory2, NULL, &dwFileCount2);

  if (dwFileCount1 == 0)
  {
    _tprintf(TEXT("Error : First folder is empty!"));
    return 1;
  }

  if (dwFileCount2 == 0)
  {
    _tprintf(TEXT("Error : Second folder is empty!"));
    return 1;
  }

  // Now store all files
  WIN32_FIND_DATA pDirectoryFiles1[dwFileCount1];
  WIN32_FIND_DATA pDirectoryFiles2[dwFileCount2];
  dwFileCount1 = dwFileCount2 = 0;
  GetFileListCount(szDirectory1, pDirectoryFiles1, &dwFileCount1);
  GetFileListCount(szDirectory2, pDirectoryFiles2, &dwFileCount2);

  // Print results
  BOOL bIsOutputRedirected = IsStdOutRedirected();
  HANDLE  hConsole;
  CONSOLE_SCREEN_BUFFER_INFO Info;
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(hConsole, &Info);
  for(int i = 0;i < dwFileCount1;i++)
  {
    if (HasStringInFileList(pDirectoryFiles1[i].cFileName, pDirectoryFiles2, dwFileCount2))
    {
      if (bIsOutputRedirected)
        _tprintf(TEXT("%ts (Both)\n"), pDirectoryFiles1[i].cFileName);
      else
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    }
    else
    {
      if (bIsOutputRedirected)
        _tprintf(TEXT("%ts\n"), pDirectoryFiles1[i].cFileName);
      else
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
    }

    if (!bIsOutputRedirected)
      _tprintf(TEXT("%ts\n"), pDirectoryFiles1[i].cFileName);
  }

  // Restore console text attributes
  SetConsoleTextAttribute(hConsole, Info.wAttributes);

  return 0;
}

void GetFileListCount(LPCTSTR szDirectory, WIN32_FIND_DATA* pData, LPDWORD dwFileNum)
{
  if (szDirectory == NULL) return;

  WIN32_FIND_DATA data;
  TCHAR tBuffer[_tcslen(szDirectory) + 4 + 1];
  _tcscpy(tBuffer, szDirectory);
  _tcscpy(tBuffer + _tcslen(szDirectory), TEXT("\\*.*"));
  HANDLE hFind = FindFirstFile(tBuffer, &data);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        if (_tcscmp(data.cFileName, TEXT(".")) != 0 &&
            _tcscmp(data.cFileName, TEXT("..")) != 0)
        {
          // Recursively go into the directory
          TCHAR tBuffer[_tcslen(szDirectory) + _tcslen(data.cFileName) + 2 + 1];
          _tcscpy(tBuffer, szDirectory);
          _tcscpy(tBuffer + _tcslen(szDirectory), TEXT("\\"));
          _tcscpy(tBuffer + _tcslen(szDirectory) + _tcslen(TEXT("\\")), data.cFileName);
          GetFileListCount(tBuffer, pData, dwFileNum);
        }
      }
      else
      {
        // It's a file
        if (pData != NULL)
          pData[*dwFileNum] = data;
        (*dwFileNum)++;
      }


    } while (FindNextFile(hFind, &data));

    if (GetLastError() == ERROR_NO_MORE_FILES) {
      FindClose(hFind);
    }
  }
}

BOOL HasStringInFileList(LPCTSTR szString, WIN32_FIND_DATA* pData, DWORD dwMaxNum)
{
  for(int i = 0;i < dwMaxNum;i++)
  {
    if (_tcscmp(szString, pData[i].cFileName) == 0)
      return TRUE;
  }

  return FALSE;
}

BOOL IsStdOutRedirected()
{
  DWORD temp;
  return GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &temp) == FALSE;
}
