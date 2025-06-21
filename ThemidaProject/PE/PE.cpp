#include "pch.h"

#include "PE.h"
#include "../emulator/emu.h"

IMAGE_DOS_HEADER* PE::GetDosHeader()
{
    return reinterpret_cast<IMAGE_DOS_HEADER*>(this);
}

IMAGE_NT_HEADERS* PE::GetNtHeader()
{
    return reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uintptr_t>(this) + GetDosHeader()->e_lfanew);
}

IMAGE_OPTIONAL_HEADER* PE::GetOptionalHeader()
{
    return reinterpret_cast<IMAGE_OPTIONAL_HEADER*>(&GetNtHeader()->OptionalHeader);
}

IMAGE_FILE_HEADER* PE::GetFileHeader()
{
    return reinterpret_cast<IMAGE_FILE_HEADER*>(&GetNtHeader()->FileHeader);
}

bool PE::MapFile(std::vector<BYTE>& data)
{
    printf("PE MapFile working...\n");
    


  
    
    std::vector<BYTE>temp = data;
       
    PE* pe = (PE*)temp.data();
    printf("Resizing buffer..\n");

    printf("Initial size = 0x%llx\n", data.size());

    data.resize(pe->GetNtHeader()->OptionalHeader.SizeOfImage);

    printf("After resize size =  0x%llx\n", data.size());


    printf("Original buffer addr to copy to 0x%llx\n", data.data());

    printf("Temp buffer addr to copy from 0x%llx\n", temp.data());

    
    auto* pSectionHeader = IMAGE_FIRST_SECTION(pe->GetNtHeader());

    memcpy((void*)data.data(), temp.data(), 0x1000);
    for (UINT i = 0; i != pe->GetFileHeader()->NumberOfSections; ++i, ++pSectionHeader)
    {
        if (pSectionHeader->SizeOfRawData)
        {
            printf("Writing to memory address:%llx  from %llx\n",
                data.data() + pSectionHeader->VirtualAddress,
                temp.data() + pSectionHeader->PointerToRawData);

            memcpy((void*)(data.data() + pSectionHeader->VirtualAddress),
                (void*)(temp.data() + pSectionHeader->PointerToRawData),
                pSectionHeader->SizeOfRawData);
        }
    }
    return true;
}
#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)

#ifdef _WIN64
#define RELOC_FLAG RELOC_FLAG64
#else
#define RELOC_FLAG RELOC_FLAG32
#endif 

bool PE::MapRelocations()
{
    const uintptr_t LocationDelta = EmulatorCPU::baseImage - GetNtHeader()->OptionalHeader.ImageBase;
    if (LocationDelta)
    {
        if (!GetNtHeader()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
            return true;

        auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>((uintptr_t)this +
            GetNtHeader()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        while (pRelocData->VirtualAddress)
        {
            UINT AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);
            for (UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo)
            {
                if (RELOC_FLAG(*pRelativeInfo))
                {
                    UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>((uintptr_t)this + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
                    *pPatch += LocationDelta;
                }
            }
            pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);


        }
    }
}
