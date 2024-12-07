#include <format>


#include "../emulator/emu.h"
#include "callbacks.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"
#include "../Instruction/Instruction.h"

bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction_,
    void* user_data) {

    UserData* data = static_cast<UserData*>(user_data);
    Instruction instruction = zasmToInstruction(instruction_);
    instruction.setCount(countGlobal);
    instruction.setAddress(address);
    instruction.setRegisterValues(cpu->getRegistersValues());

    auto& operands = instruction.getOperands();

    for (int i = 0; i < operands.size(); i++) {
        auto& op = operands[i];

        MemoryOperand* memOperand = dynamic_cast<MemoryOperand*>(op);

        if (memOperand) {
            uintptr_t memoryAddress = cpu->CalcEffectiveMemAddress(memOperand->getZasmOperand(), i);
            memOperand->setMemoryAddress(memoryAddress);
        }

        op->setOperandAccess(instruction_.getOperandAccess(i));
    }

    std::string toLog = std::format("Trying to emulate instruction at rva:0x{:x} count : {:d} | {} \n",
        address, instruction.getCount(), formatInstruction_(instruction));
    logger->log(toLog);

    data->instructions.push_back(instruction);
    /*
    if (countGlobal == 1000) {
        cpu->stop_emu();
    }*/

    if(address == 0x000000014002FAE3)
        cpu->stop_emu();

    countGlobal++;

    return true;

}