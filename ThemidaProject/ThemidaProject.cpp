#include <iostream>

#include  "Utils/Utils.h"
#include "emulator/emu.h"
#include "PE/PE.h"

#include "Utils/Logger.h"
#include "./Devirt/deobf.h"

int main()
{
    logger = new Logger("D:\\log.txt");
    std::string path = "C:\\Users\\U1\\source\\repos\\ConsoleApplication6\\x64\\Release\\ConsoleApplication6_protected1221.exe";

    std::vector<BYTE>data;
    if (!ReadFile(path, data))
    {
        return 0;
    }

    std::vector<BYTE>save_data = data;

    PE::MapFile(data);

    PE* binary = (PE*)data.data();

    binary->MapRelocations();


    EmulatorCPU cpu;

    cpu.mem_map(EmulatorCPU::baseImage, data.size());

    cpu.mem_map(0x10000, 0x100000);

    cpu.mem_write(EmulatorCPU::baseImage, data.data(), data.size());

    cpu.reg_write(zasm::x86::rsp, 0x90000);

    cpu.mem_map(0x1000, 0x1000);
    cpu.reg_write(zasm::x86::gs,0x1000);
    
 
  
    
    deobf* devirt = new deobf(&cpu, binary);

    devirt->run(EmulatorCPU::baseImage + 0x2A1A2);
    //2A1A2 0x2A19D
   
}
