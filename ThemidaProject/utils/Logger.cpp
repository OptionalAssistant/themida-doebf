#include "Logger.h"

Logger::Logger(const char* path)
{
    hFile = CreateFileA(
        path,                // имя файла
        GENERIC_WRITE,           // возможность записи
        0,                       // общий доступ отключен
        NULL,                    // атрибуты безопасности по умолчанию
        CREATE_ALWAYS,           // создание нового файла, если файла нет, или перезапись существующего
        FILE_ATTRIBUTE_NORMAL,   // обычный файл
        NULL                     // шаблонные файлы не используются
    );
    // Проверка, удалось ли создать файл
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("unable to createfile\n");
        exit(0);
    }
}

void Logger::log(const std::string& data)
{
    DWORD bytesWritten;
    bool writeSuccess = WriteFile(
        hFile,                  // дескриптор файла
        data.c_str(),           // буфер данных для записи
        data.length(),          // количество байтов для записи
        &bytesWritten,          // количество записанных байтов
        NULL                    // асинхронная запись не используется
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
