#pragma once
#include <Windows.h>
#include <vector>
#include <winternl.h>

class PE
{
public:
	IMAGE_DOS_HEADER* GetDosHeader();
	IMAGE_NT_HEADERS* GetNtHeader();
	IMAGE_OPTIONAL_HEADER* GetOptionalHeader();
	IMAGE_FILE_HEADER* GetFileHeader();
	static bool MapFile(std::vector<BYTE>& bin);
	bool MapRelocations();
};

