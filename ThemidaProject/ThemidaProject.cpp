#include "pch.h"

#include "Utils/Utils.h"
#include "Utils/Logger.h"
#include "emulator/emu.h"
#include "PE/PE.h"
#include "Devirt/deobf.h"


std::map<std::string, std::string> parseArguments(int argc, char* argv[]) {
    std::map<std::string, std::string> arguments;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg[0] == '-') {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                arguments[arg] = argv[i + 1];
                ++i;
            }
            else {
                arguments[arg] = ""; // Флаг без значения
            }
        }
    }

    return arguments;
}

void printHelp() {
    std::cout << "Usage: program [options]\n"
        << "Options:\n"
        << "  -b <path>      Path to binary file (required)\n"
        << "  -o <path>      Path to log file (default: D:\\log.txt)\n"
        << "  -rva <rva>     RVA of obfuscated function (hex or decimal)\n"
        << "  -sectionStart  <address> Start address of section (hex or decimal)\n"
        << "  -sectionSize   <size>    Size of section (hex or decimal)\n"
        << "  -help          Show this help message\n";
}

int main(int argc, char* argv[])
{
    auto args = parseArguments(argc, argv);

    if (args.count("-help")) {
        printHelp();
        return EXIT_SUCCESS;
    }

    if(!args.count("-rva")) {
        std::cerr << "Error: RVA to start of obfuscated function  (at obf section) (-rva) is required\n";
        printHelp();
        return 1;
    }
    
    if (!args.count("-sectionStart")) {
        std::cerr << "Error: sectionStart   (RVA of obf section) (-sectionStart) is required\n";
        printHelp();
        return 1;
    }

    // Проверка обязательного аргумента -b
    if (!args.count("-sectionSize")) {
        std::cerr << "Error: sectionSize is required (-sectionSize) is required\n";
        printHelp();
        return 1;
    }

    if (!args.count("-o")) {
        std::cerr << "Error: Log output file path (-o) is required\n";
        printHelp();
        return 1;
    }

    logger = new Logger(args["-o"].c_str());

    // Путь к бинарному файлу
    std::string path = args["-b"];

    std::vector<BYTE> data;
    if (!ReadFile(path, data)) {
        std::cerr << "Error: Failed to read file " << path << "\n";
        return 1;
    }

    std::vector<BYTE> save_data = data;

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
    
 
  
    try {

        uint64_t start = std::stoull(args["-sectionStart"], nullptr, 0);
        globals::sectionBase = static_cast<uintptr_t>(start) + EmulatorCPU::baseImage;
        
       // Парсим размер секции (поддержка hex/dec)
       uint64_t size = std::stoull(args["-sectionSize"], nullptr, 0);
       globals::sectionSize = size;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: Invalid section address/size format.\n";
        return 1;
    }

    deobf* devirt = new deobf(&cpu, binary);

    try {
        uint64_t rva = std::stoull(args["-rva"], nullptr, 0);
        devirt->run(EmulatorCPU::baseImage + rva);
    }
    catch (const std::exception& e) {
         std::cerr << "Error: Invalid RVA format (" << args["-rva"] << "). Must be a number (hex, decimal, or octal).\n";
         return 1;
    }
    
    //16581CD 0x16581C8 C35EE0
   
}
