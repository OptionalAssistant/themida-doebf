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
#include "../utils/Logger.h"

#include <zasm/zasm.hpp>
#include <zasm/formatter/formatter.hpp>
#include <format>

int count = 0;


bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction_,
    void* user_data) {

    std::string toLog = std::format("Trying to emulate instruction at rva:0x{:x} --\n", address);

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
         uintptr_t memoryAddress =  cpu->CalcEffectiveMemAddress(memoryOperand->getZasmOperand(), i);
         memoryOperand->setMemoryAddress(memoryAddress);
        }
    }

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

ConstantOperand* createConstantOperand(const zasm::Imm& immOp,uintptr_t index) {
    return new ConstantOperand(immOp,index);
}

RegisterOperand* createRegisterOperand(const zasm::Reg& regOperand,OperandAction op_action,uintptr_t index) {
    return new RegisterOperand(regOperand, index,op_action);
}

MemoryOperand* createMemoryOperand(const zasm::Mem& memoryOp,OperandAction op_action,uintptr_t index) {
    return new MemoryOperand(createRegisterOperand(memoryOp.getBase(),OperandAction::READ, index),
        createRegisterOperand(memoryOp.getIndex(),OperandAction::READ, index),
        createConstantOperand(memoryOp.getDisplacement(), index),
        createConstantOperand(memoryOp.getScale(), index),op_action,memoryOp,index);
}

Instruction* zasmToInstruction(const zasm::InstructionDetail& instruction_)
{
    Instruction* instruction = new Instruction(instruction_);

    for (int i = 0; i < instruction_.getOperandCount();i++) {
        if (instruction_.getOperand(i).holds<zasm::Mem>()) {
            MemoryOperand* memoryOperand = createMemoryOperand(instruction_.getOperand(i).get<zasm::Mem>(),
                zasmActionToOwn(instruction_.getOperandAccess(i)),i);
            instruction->addOperand(memoryOperand);
            memoryOperand->getBase()->setParent(instruction);
            memoryOperand->getIndex()->setParent(instruction);
        }
        else if (instruction_.getOperand(i).holds<zasm::Reg>()) {
            instruction->addOperand(createRegisterOperand(instruction_.getOperand(i).get<zasm::Reg>(),
                zasmActionToOwn(instruction_.getOperandAccess(i)),i));
        }
        else if (instruction_.getOperand(i).holds<zasm::Imm>()) {
            instruction->addOperand(createConstantOperand(instruction_.getOperand(i).get<zasm::Imm>(),i));
        }
    }

    return instruction;
}
