#include "SimplifyStackOperation.h"
#include "../Instruction/Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/MemoryOperand.h"
#include "../Linker/Linker.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"

bool SimplifyStackOperation::run(Instruction* instruction)
{
    bool isExtraPass = false;
    for (Instruction* currentInstruction = instruction; currentInstruction != nullptr;
        currentInstruction = currentInstruction->getNext()) {
   
        //Simplify dead memory push
      /*  if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Push) {

                if (!currentInstruction->getOperand(2)->hasUses()) {

                    const auto& zasmMnemonic = currentInstruction->getZasmInstruction().getMnemonic();
 
                        printf("Simplify dead memory push: %d", currentInstruction->getCount());

                        const auto& newZasmInstruction = zasm::InstructionDetail()
                            .setMnemonic(zasm::x86::Mnemonic::Sub)
                            .addOperand(zasm::x86::rsp)
                            .addOperand(zasm::Imm(8));


                        Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

                        RegisterOperand* operand = dynamic_cast<RegisterOperand*>(currentInstruction->getOperand(1));

                        newInstruction->getOperand(0)->replaceAllUses(operand);

                        newInstruction->getOperand(0)->setNext(operand->getNext());
                        newInstruction->getOperand(0)->setPrev(operand->getPrev());

                        currentInstruction->getOperand(1)->getPrev()->addUse(newInstruction->getOperand(0));

                        newInstruction->insertAfter(currentInstruction);


                        currentInstruction->Delete();

                        isExtraPass = true;

                        currentInstruction = newInstruction;

                    
                }

              
        }*/
       
        //Simplify sub rsp,8 to push
         if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Sub) {
            
            if (currentInstruction->getOperand(0)->getZasmOperand().holds<zasm::Reg>() &&
                currentInstruction->getOperand(0)->getZasmOperand().get<zasm::Reg>() == zasm::x86::rsp) {
                auto& useList = currentInstruction->getOperand(0)->getUseList();

                for (auto& use : useList) {
                    Instruction* nextInstruction = use->getParent();

                    if (nextInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
                        nextInstruction->getZasmInstruction().getOperand(0).holds<zasm::Mem>() &&
                        nextInstruction->getZasmInstruction().getOperand(0).
                        get<zasm::Mem>().getBase() == zasm::x86::rsp) {

                        printf("Simplify Sub rsp to  push: %d", currentInstruction->getCount());

                        zasm::InstructionDetail::OperandsAccess opAccess;
                        zasm::InstructionDetail::OperandsVisibility opVisibility;

                        opAccess.set(0, zasm::Operand::Access::Read);
                        opAccess.set(1, zasm::Operand::Access::ReadWrite);
                        opAccess.set(2, zasm::Operand::Access::Write);

                        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
                        opVisibility.set(1, zasm::Operand::Visibility::Hidden);
                        opVisibility.set(2, zasm::Operand::Visibility::Hidden);

                        zasm::Instruction::Attribs attribs;
                        zasm::InstructionDetail::Category category;

                        std::array<zasm::Operand, 10>ops;
                        ops[0] = nextInstruction->getZasmInstruction().getOperand(1);
                        ops[1] = currentInstruction->getZasmInstruction().getOperand(0);
                        ops[2] = nextInstruction->getZasmInstruction().getOperand(0);

                        const auto& newZasmInstruction = zasm::InstructionDetail({},
                            zasm::x86::Mnemonic::Push, 3,
                            ops, opAccess, opVisibility, {}, {});

                        Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

                        
                        newInstruction->getOperand(1)->replaceAllUses(currentInstruction->getOperand(0));
                        newInstruction->getOperand(2)->replaceAllUses(nextInstruction->getOperand(0));

                        newInstruction->getOperand(1)->setPrev(currentInstruction->getOperand(0)->getPrev());
                        newInstruction->getOperand(1)->setNext(currentInstruction->getOperand(0)->getNext());

                        if (currentInstruction->getOperand(0)->getPrev())
                            currentInstruction->getOperand(0)->getPrev()->setNext(newInstruction->getOperand(1));

                        if (currentInstruction->getOperand(0)->getNext())
                            currentInstruction->getOperand(0)->getNext()->setPrev(newInstruction->getOperand(1));


                        newInstruction->getOperand(2)->setPrev(nextInstruction->getOperand(0)->getPrev());
                        newInstruction->getOperand(2)->setNext(nextInstruction->getOperand(0)->getNext());
                 

                        if (nextInstruction->getOperand(0)->getPrev())
                            nextInstruction->getOperand(0)->getPrev()->setNext(newInstruction->getOperand(2));

                        if (nextInstruction->getOperand(0)->getNext())
                            nextInstruction->getOperand(0)->getNext()->setPrev(newInstruction->getOperand(2));


                        currentInstruction->getOperand(0)->getPrev()->addUse(newInstruction->getOperand(1));

                        newInstruction->getOperand(0)->setPrev(nextInstruction->getOperand(1)->getPrev());

                        newInstruction->insertAfter(currentInstruction);

                        MemoryOperand* memoryOp = dynamic_cast<MemoryOperand*>(newInstruction->getOperand(2));

                        MemoryOperand* oldMemoryOp = dynamic_cast<MemoryOperand*>(nextInstruction->getOperand(0));

                        memoryOp->setMemoryAddress(oldMemoryOp->getMemoryAddress());

                        nextInstruction->Delete();
                        currentInstruction->Delete();

                        isExtraPass = true;
                        currentInstruction = newInstruction;

                        break;
                    }
                }
            
        
            }}

    }
    logger->log("AGAIN");
    formateLinkedInstructions(instruction);
        
    return isExtraPass;
}
