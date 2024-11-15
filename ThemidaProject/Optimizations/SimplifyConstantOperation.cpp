#include "SimplifyConstantOperation.h"
#include "../Instruction/Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../Operands/ConstantOperand.h"

bool SimplifyConstantOperation::run(Instruction* instruction)
{
    bool isExtraPass = false;
    for (Instruction* currentInstruction = instruction; currentInstruction != nullptr;
        currentInstruction = currentInstruction->getNext()) {

        if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
            currentInstruction->getOperand(1)->getZasmOperand().holds<zasm::Imm>() &&
            currentInstruction->getOperand(0)->getZasmOperand().holds<zasm::Reg>()) {

            if (!currentInstruction->getOperand(0)->hasUses())
                continue;
    
          //  printf("Found mov register const : %d \n", currentInstruction->getCount());

            ConstantOperand* constantOperand = dynamic_cast<ConstantOperand*>(currentInstruction->getOperand(1));

            auto& useList = currentInstruction->getOperand(0)->getUseList();

            for (auto& use : useList) {
                if (use->getOperandAccess() == OperandAction::READ) {
                 // printf("Found use : %d \n", use->getParent()->getCount());
                  use->makeConstant(constantOperand);
                }
            }
            printf("\n\n");

            currentInstruction->getOperand(0)->ClearUseList();

            isExtraPass = true;
        }
    }

    return isExtraPass;
}
