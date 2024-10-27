#pragma once
#include <fstream>
#include <string>
#include <Windows.h>

class Logger
{
private:
	const char* m_path;
	HANDLE hFile;
public:
	Logger(const char* path);
	void log(const std::string& msg);
	void close();
	~Logger();
};


inline  Logger* logger;