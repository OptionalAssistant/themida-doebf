#include "pch.h"

#include "Logger.h"

Logger::Logger(const char* path)
    : m_path(path)
{
    hFile = CreateFileA(
        path,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("unable to createfile\n");
        exit(0);
    }
}

void Logger::log(const std::string& data)
{
    DWORD bytesWritten;
    bool writeSuccess = WriteFile(
        hFile,
        data.c_str(),
        (DWORD)data.length(),
        &bytesWritten,
        NULL
    );

    if (!writeSuccess)
    {
        printf("Unable to write to log file\n");
        exit(0);
    }
}

void Logger::close()
{
    CloseHandle(hFile);
}

Logger::~Logger()
{
	close();
}
