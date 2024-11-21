#pragma once

#include<common.h>

#define LOG_FILENAME L"AMCLog.txt"

// Colors
#define LOG_COLOR_WHITE 7
#define LOG_COLOR_GREEN 10
#define LOG_COLOR_YELLOW 14
#define LOG_COLOR_MAGENTA 13
#define LOG_COLOR_CIAN 11
#define LOG_COLOR_RED 12

#define FRIENDLY_DATEFORMAT L"%d-%m-%y, %H:%H:%S"

#define LOG_ERROR(msg, ...) AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, AMC::LOG_ERROR, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, AMC::LOG_WARNING, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, AMC::LOG_INFO, msg, ##__VA_ARGS__)

#define LOG(level) AMC::Log::GetInstance()->SetLogLevel(level);

namespace AMC {

	typedef enum{
		LOG_ERROR = 1,
		LOG_WARNING = 2,
		LOG_INFO = 3
	}LogLevel;

	class Log
	{
		private:
			FILE* fp;
			Log();
			static Log* instance;
			CTime GetFileDateTime(LPWSTR FileName) const;
			CTime GetSelfDataTime(void) const;

		public:

			Log(const Log&) = delete;
			Log& operator=(const Log&) = delete;
			Log(Log&&) = delete;
			Log& operator = (Log&&) = delete;

			static Log* GetInstance();
			void WriteLogFile(LPCSTR CallingFunc, LogLevel level, LPCTSTR lpText, ...) const;
			void SetLogLevel(LogLevel level);
			LPCTSTR GetErrorMessage(HRESULT hr);
	};
};


