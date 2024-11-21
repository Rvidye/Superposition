#include <common.h>

namespace AMC {

	Log* Log::instance = nullptr;

	Log::Log()
	{
		if (!AllocConsole()) {
			OutputDebugString(L"Could Not Allocate Console \n");
		}
		
		freopen_s(&fp,"CONOUT$", "w", stdout);
		if (!fp) {
			OutputDebugString(L"Could Not Create Log File\n");
		}
	}

	Log* Log::GetInstance()
	{
		if (instance == nullptr)
		{
			instance = new Log();
		}
		return instance;
	}

	CTime Log::GetFileDateTime(LPWSTR FileName) const{

		FILETIME ftCreate, ftAccess, ftWrite;
		HANDLE hFile;
		CTime result = NULL;
		CTime FileTime;
			
		hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			return result;
		}

		if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
			return result;

		FileTime = ftWrite;

		CloseHandle(hFile);
		result = (CTime)FileTime;
		return result;
	}

	CTime Log::GetSelfDataTime(void) const {
		WCHAR szExeFileName[MAX_PATH];
		GetModuleFileName(nullptr, szExeFileName, MAX_PATH);
		return GetFileDateTime(szExeFileName);
	}

	inline void setcolor(int textcol, int backcol) {
		if ((textcol % 16) == (backcol % 16))textcol++;
		textcol %= 16; backcol %= 16;
		unsigned short wAttributes = ((unsigned)backcol << 4) | (unsigned)textcol;
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdOut, wAttributes);
	}


	void Log::WriteLogFile(LPCSTR CallingFunc, LogLevel level,  LPCTSTR lpText, ...) const{
		CTime Today = CTime::GetCurrentTime();

		CString sMsg;
		CString sLine;
		va_list ptr;
		va_start(ptr, lpText);
		sMsg.FormatV(lpText, ptr);
		va_end(ptr);
		sLine.Format(L"%S %s : %s\n", CallingFunc, (LPCTSTR)Today.FormatGmt(L"%d.%m.%Y %H:%M"), (LPCTSTR)sMsg);

		CStringA line(sLine);
		line += "\n";
		HANDLE hStdout;
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

		switch (level) {
			case LOG_ERROR:
				setcolor(LOG_COLOR_RED, 0);
			break;
			case LOG_WARNING:
				setcolor(LOG_COLOR_YELLOW, 0);
			break;
			case LOG_INFO:
				setcolor(LOG_COLOR_WHITE, 0);
			break;
			default:
				setcolor(LOG_COLOR_WHITE, 0);
			break;
		}

		WriteFile(hStdout,line,lstrlen(sLine),nullptr,nullptr);

		//std::cout << sLine.GetBuffer();

		//_wfopen_s(&fp,LOG_FILENAME, L"a");
		//if (fp) {
			//fwprintf(fp, L"%s\n", sLine.GetBuffer());
			//fclose(fp);
		//}
		setcolor(LOG_COLOR_WHITE, 0);
	}

	void Log::SetLogLevel(LogLevel level)
	{
		switch (level) {
		case LOG_ERROR:
			setcolor(LOG_COLOR_RED, 0);
			break;
		case LOG_WARNING:
			setcolor(LOG_COLOR_YELLOW, 0);
			break;
		case LOG_INFO:
			setcolor(LOG_COLOR_WHITE, 0);
			break;
		default:
			setcolor(LOG_COLOR_WHITE, 0);
			break;
		}
	}

	LPCTSTR	Log::GetErrorMessage(HRESULT hr)
	{ 
		LPVOID errorMsg;
		DWORD result = FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						nullptr,
						hr,
						0,
						(LPWSTR)&errorMsg,
						0,
						nullptr);

		if (result == 0)
		{
			return L"Unknown Error";
		}
		else
		{
			return static_cast<LPCTSTR>(errorMsg);
		}
	}
};

