#include "DeadCodeElimination.h"
#include "../Instruction/Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/MemoryOperand.h"
#include "../Linker/Linker.h"

bool DeadCodeElimination::run(Instruction* instruction)
{
    bool isExtraPass = false;
    for (Instruction* currentInstruction = instruction; currentInstruction != nullptr;
        currentInstruction = currentInstruction->getNext()) {
        auto& operands = currentInstruction->getOperands();

        for (int i = 0; i < operands.size(); i++) {
            if (!operands[i]->hasUses() && operands[i]->getOperandAccess() == OperandAction::WRITE) {
               
                const auto& zasmMnemonic = currentInstruction->getZasmInstruction().getMnemonic();
                if (zasmMnemonic == zasm::x86::Mnemonic::Push) {
                    printf("Current instruction count number: %d", currentInstruction->getCount());

                    const auto& newZasmInstruction = zasm::InstructionDetail()
                        .setMnemonic(zasm::x86::Mnemonic::Sub)
                        .addOperand(zasm::x86::rsp)
                        .addOperand(zasm::Imm(8));

                    Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

                    RegisterOperand* operand = dynamic_cast<RegisterOperand*>(currentInstruction->getOperand(1));

                    newInstruction->getOperand(0)->replaceAllUses(operand);
                    //newInstruction->getOperand(0)->replaceNextPrev(operand);

                    currentInstruction->insertAfter(newInstruction);

                    currentInstruction->Delete();
                    
                }
            }
        }

    }

    return isExtraPass;
}
