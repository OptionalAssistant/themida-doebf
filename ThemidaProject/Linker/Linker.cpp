#include <Windows.h>
#include <string>
#include <vector>

#include "Linker.h"
#include "../emulator/emu.h"
#include "../Instruction/Instruction.h"

#include "../Operands/MemoryOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/ConstantOperand.h"
#include "../utils/Utils.h"

#include <zasm/zasm.hpp>
#include <zasm/formatter/formatter.hpp>

int count = 0;


bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction_,
    void* user_data) {

    printf("Emulating instruction at address:0x%llx  ", address);

    printInstruction(instruction_);

    UserData* data = static_cast<UserData*>(user_data);

  
    Instruction* instruction = zasmToInstruction(instruction_);

    if (!data->head) {
        data->head = data->tail = data->instructions = instruction;
    }
    else {
        data->tail->insertAfter(instruction);
        data->tail = instruction;
    }
    
    for (auto& instruction = data->head; 
        instruction != nullptr; instruction = instruction->getNext()) {
        instruction->LinkInstruction();
    }

    if (count == 1000)
        cpu->stop_emu();

    count++;

    return true;

}

ConstantOperand* createConstantOperand(const zasm::Imm& immOp) {
    return new ConstantOperand();
}

RegisterOperand* createRegisterOperand(const zasm::Reg& regOperand) {
    return new RegisterOperand(regOperand);
}

MemoryOperand* createMemoryOperand(const zasm::Mem& memoryOp) {
    MemoryOperand* memoryOperand = new MemoryOperand();

    memoryOperand->setBase(createRegisterOperand(memoryOp.getBase()));
    memoryOperand->setIndex(createRegisterOperand(memoryOp.getIndex()));
    memoryOperand->setDisplacement(createConstantOperand(memoryOp.getDisplacement()));
    memoryOperand->setScale(createConstantOperand(memoryOp.getScale()));

    return memoryOperand;
}

Instruction* zasmToInstruction(const zasm::InstructionDetail& instruction_)
{
    Instruction* instruction = new Instruction();

    for (int i = 0; i < instruction_.getOperandCount();i++) {
        if (instruction_.getOperand(i).holds<zasm::Mem>()) {
            instruction->addOperand(createMemoryOperand(instruction_.getOperand(i).get<zasm::Mem>()));
        }
        else if (instruction_.getOperand(i).holds<zasm::Reg>()) {
            instruction->addOperand(createRegisterOperand(instruction_.getOperand(i).get<zasm::Reg>()));
        }
        else if (instruction_.getOperand(i).holds<zasm::Imm>()) {
            instruction->addOperand(createConstantOperand(instruction_.getOperand(i).get<zasm::Imm>()));
        }
    }

    return instruction;
}
