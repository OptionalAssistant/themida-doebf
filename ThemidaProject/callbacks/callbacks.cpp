#include <format>

#include "../Instruction/Instruction.h"

#include "../Operands/MemoryOperand.h"
#include "../Helpers/Linker.h"

#include "../emulator/emu.h"
#include "callbacks.h"
#include "../utils/Utils.h"


int count = 0;


bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction_,
    void* user_data) {

    std::string toLog = std::format("Trying to emulate instruction at rva:0x{:x}  {} --\n", 
        address,formatInstruction(instruction_));
    printf("%s\n", toLog.c_str());
    /*    printf("%s  ", toLog.c_str());
        printf("|%s  |  ", formatBytes(instruction_, cpu->from_virtual_to_real(address)).c_str());
        printf("%s  \n", formatInstruction(instruction_).c_str());*/

        //  logger->log(toLog);

          //  cpu->LogInstruction(instruction_, cpu->from_virtual_to_real(address));

    UserData* data = static_cast<UserData*>(user_data);


    Instruction* instruction = zasmToInstruction(instruction_);

    auto& operands = instruction->getOperands();

    for (int i = 0; i < operands.size(); i++) {

        MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(operands[i]);

        if (memoryOperand) {
            uintptr_t memoryAddress = cpu->CalcEffectiveMemAddress(memoryOperand->getZasmOperand(), i);
            memoryOperand->setMemoryAddress(memoryAddress);
        }
    }
       
    createOperandUnits(instruction);

    if (!data->head) {
        data->head = data->tail = data->instructions = instruction;
    }
    else {
        instruction->insertAfter(data->tail);
        data->tail = instruction;
    }


    if (count == 1000) {
        cpu->stop_emu();
    }


    count++;

    return true;

}