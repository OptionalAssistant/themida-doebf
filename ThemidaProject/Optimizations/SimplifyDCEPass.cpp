#include "SimplifyDCEPass.h"
#include "../Instruction/Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../utils/Utils.h"

bool SimplifyDCEPass::run(Instruction* instruction)
{
    bool isExtraPass = false;
    for (Instruction* currentInstruction = instruction; currentInstruction != nullptr;
        currentInstruction = currentInstruction->getNext()) {

        //Simplify move the same register e.g mov rbx,rbx
        if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
            currentInstruction->getOperand(0)->getZasmOperand().holds<zasm::Reg>() &&
            currentInstruction->getOperand(1)->getZasmOperand().holds<zasm::Reg>()) {
            
            if (currentInstruction->getOperand(0)->getZasmOperand().get<zasm::Reg>() ==
                currentInstruction->getOperand(1)->getZasmOperand().get<zasm::Reg>()) {
                
              //  printf("Simplify the same register move : %d \n", currentInstruction->getCount());

                BaseOperand* firstOperand = currentInstruction->getOperand(0);

                BaseOperand* secondOperand = currentInstruction->getOperand(1);

                if (secondOperand->getPrev()) {
                    secondOperand->getPrev()->deleteUse(secondOperand);
                }

                firstOperand->transferAllUses(firstOperand->getPrev());

                firstOperand->emplaceBetween();

               Instruction* previous = currentInstruction->getPrev();

                currentInstruction->Delete();

                currentInstruction = previous;

                isExtraPass = true;

            }

        }

    }

    return isExtraPass;
}
