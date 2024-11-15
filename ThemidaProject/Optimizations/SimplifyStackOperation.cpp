#include "SimplifyStackOperation.h"
#include "../Instruction/Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/MemoryOperand.h"
#include "../Linker/Linker.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"
#include "Helpers/Helpers.h"

bool SimplifyStackOperation::run(Instruction* instruction)
{
    bool isExtraPass = false;
    for (Instruction* currentInstruction = instruction; currentInstruction != nullptr;
        currentInstruction = currentInstruction->getNext()) {
   
        //Simplify dead memory push
        if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Push) {

                if (!currentInstruction->getOperand(2)->hasUses()) {
 
                     //   printf("Simplify dead memory push: %d \n", currentInstruction->getCount());


                        zasm::InstructionDetail::OperandsAccess opAccess;
                        zasm::InstructionDetail::OperandsVisibility opVisibility;

                        opAccess.set(0, zasm::Operand::Access::ReadWrite);
                        opAccess.set(1, zasm::Operand::Access::Read);

                        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
                        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

                        std::array<zasm::Operand, 10>ops;
                        ops[0] = zasm::x86::rsp;
                        ops[1] = zasm::Imm(8);

                        const auto& newZasmInstruction = zasm::InstructionDetail({},
                            zasm::x86::Mnemonic::Sub, 2,
                            ops, opAccess, opVisibility, {}, {});


                        Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

                        RegisterOperand* operand = dynamic_cast<RegisterOperand*>(currentInstruction->getOperand(1));

                        newInstruction->getOperand(0)->replaceOperandWith(operand);

                        if (currentInstruction->getOperand(1)->getPrev()) {

                            MemoryOperand* oldMemory = dynamic_cast<MemoryOperand*>(currentInstruction->getOperand(2));

                            currentInstruction->getOperand(1)->getPrev()->replaceOneUse(
                                operand, newInstruction->getOperand(0));

                            currentInstruction->getOperand(1)->getPrev()->deleteUse(oldMemory->getBase());

                            currentInstruction->getOperand(2)->getNext()
                                ->setPrev(currentInstruction->getOperand(2)->getPrev());

                            if (currentInstruction->getOperand(2)->getPrev()) {
                                currentInstruction->getOperand(2)->getPrev()
                                    ->setNext(currentInstruction->getOperand(2)->getNext());
                            }

                        }
                   
                        if (currentInstruction->getOperand(0)->getPrev()) {
                            currentInstruction->getOperand(0)->getPrev()->deleteUse(currentInstruction->getOperand(0));
                        }
                        newInstruction->insertAfter(currentInstruction);


                        currentInstruction->Delete();

                        isExtraPass = true;

                        currentInstruction = newInstruction;

                    
                }

              
        }
       
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

                //        printf("Simplify Sub rsp to  push: %d \n", currentInstruction->getCount());

                        zasm::InstructionDetail::OperandsAccess opAccess;
                        zasm::InstructionDetail::OperandsVisibility opVisibility;

                        opAccess.set(0, zasm::Operand::Access::Read);
                        opAccess.set(1, zasm::Operand::Access::ReadWrite);
                        opAccess.set(2, zasm::Operand::Access::Write);

                        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
                        opVisibility.set(1, zasm::Operand::Visibility::Hidden);
                        opVisibility.set(2, zasm::Operand::Visibility::Hidden);

                        std::array<zasm::Operand, 10>ops;
                        ops[0] = nextInstruction->getZasmInstruction().getOperand(1);
                        ops[1] = currentInstruction->getZasmInstruction().getOperand(0);
                        ops[2] = nextInstruction->getZasmInstruction().getOperand(0);

                        const auto& newZasmInstruction = zasm::InstructionDetail({},
                            zasm::x86::Mnemonic::Push, 3,
                            ops, opAccess, opVisibility, {}, {});

                        Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

                        
                        newInstruction->getOperand(1)->replaceOperandWith(currentInstruction->getOperand(0));
                        newInstruction->getOperand(2)->replaceOperandWith(nextInstruction->getOperand(0));
            

                        if(currentInstruction->getOperand(0)->getPrev())
                        currentInstruction->getOperand(0)->getPrev()
                            ->replaceOneUse(currentInstruction->getOperand(0), newInstruction->getOperand(1));

                        newInstruction->getOperand(0)->setPrev(nextInstruction->getOperand(1)->getPrev());

                        if (nextInstruction->getOperand(1)->getPrev()) {
                            nextInstruction->getOperand(1)->getPrev()->replaceOneUse(
                                nextInstruction->getOperand(1),
                                newInstruction->getOperand(0));
                        }
                

                        MemoryOperand* oldMemoryOp = dynamic_cast<MemoryOperand*>(nextInstruction->getOperand(0));
                        MemoryOperand* memoryOp = dynamic_cast<MemoryOperand*>(newInstruction->getOperand(2));

                        newInstruction->getOperand(1)->deleteUse(oldMemoryOp->getBase());

                        if (newInstruction->getOperand(1)->getPrev()) {
                            newInstruction->getOperand(1)->getPrev()->addUse(memoryOp->getBase());
                            memoryOp->getBase()->setPrev(newInstruction->getOperand(1)->getPrev());
                        }

                        newInstruction->insertAfter(currentInstruction);

          

                        memoryOp->setMemoryAddress(oldMemoryOp->getMemoryAddress());

                        nextInstruction->Delete();
                        currentInstruction->Delete();

                        isExtraPass = true;
                        currentInstruction = newInstruction;

                        break;
                    }
                }
            
        
            }
         }

         //Simplify add rsp,8 to pop
         if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Add &&
             currentInstruction->getZasmInstruction().getOperand(0).holds<zasm::Reg>() &&
             currentInstruction->getZasmInstruction().getOperand(0).get<zasm::Reg>() == zasm::x86::rsp &&
             currentInstruction->getZasmInstruction().getOperand(1).holds<zasm::Imm>() &&
             currentInstruction->getZasmInstruction().getOperand(1).get<zasm::Imm>() == 8) {

             RegisterOperand* op1 = dynamic_cast<RegisterOperand*>(currentInstruction->getOperand(0));

             if (!op1) {
                 printf("Wtf??\n");
                 throw std::runtime_error("Error during simplify add rsp 8 to pop");
             }

             BaseOperand* foundOperand = findPrevAccessRegister(currentInstruction->getPrev(), op1);

             if (!foundOperand) {
                 printf("Errrr?????");
                 continue;
             }
             Instruction* parentInstruction = foundOperand->getParent();

             if (parentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
                 parentInstruction->getZasmInstruction().getOperand(0).holds<zasm::Reg>() &&
                 parentInstruction->getZasmInstruction().getOperand(1).holds<zasm::Mem>() &&
                 parentInstruction->getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() == zasm::x86::rsp) {


             //    printf("Simplify add rsp,8 to pop : %d %s \n", currentInstruction->getCount(),
                   //  formatInstruction(currentInstruction->getZasmInstruction()).c_str());

               //  printf("Found mov mem before add rsp,8 pop detecting %d %s \n", parentInstruction->getCount(),
                     //formatInstruction(parentInstruction->getZasmInstruction()).c_str());



                 zasm::InstructionDetail::OperandsAccess opAccess;
                 zasm::InstructionDetail::OperandsVisibility opVisibility;

                 opAccess.set(0, zasm::Operand::Access::Write);
                 opAccess.set(1, zasm::Operand::Access::ReadWrite);
                 opAccess.set(2, zasm::Operand::Access::Read);

                 opVisibility.set(0, zasm::Operand::Visibility::Explicit);
                 opVisibility.set(1, zasm::Operand::Visibility::Hidden);
                 opVisibility.set(2, zasm::Operand::Visibility::Hidden);

                 std::array<zasm::Operand, 10>ops;
                 ops[0] = parentInstruction->getZasmInstruction().getOperand(0);
                 ops[1] = currentInstruction->getZasmInstruction().getOperand(0);
                 ops[2] = parentInstruction->getZasmInstruction().getOperand(1);

                 const auto& newZasmInstruction = zasm::InstructionDetail({},
                     zasm::x86::Mnemonic::Pop, 3,
                     ops, opAccess, opVisibility, {}, {});

                 Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

                 newInstruction->getOperand(0)->replaceOperandWith(parentInstruction->getOperand(0));
                 newInstruction->getOperand(1)->replaceOperandWith(currentInstruction->getOperand(0));
                 
                 if (currentInstruction->getOperand(0)->getPrev()) {
                    
                     currentInstruction->getOperand(0)->getPrev()->replaceOneUse(
                         currentInstruction->getOperand(0), newInstruction->getOperand(1)
                     );

                     MemoryOperand* oldMemoryBase = dynamic_cast<MemoryOperand*>(parentInstruction->getOperand(1));
                     MemoryOperand* newMemoryBase = dynamic_cast<MemoryOperand*>(newInstruction->getOperand(2));


                     currentInstruction->getOperand(0)->getPrev()->replaceOneUse(
                         oldMemoryBase->getBase(), newMemoryBase->getBase()
                     );

                     newMemoryBase->getBase()->setPrev(currentInstruction->getOperand(0)->getPrev());
                     newMemoryBase->setMemoryAddress(oldMemoryBase->getMemoryAddress());

                 }

                 if (parentInstruction->getOperand(1)->getPrev()) {
                     parentInstruction->getOperand(1)->getPrev()->replaceOneUse(
                     parentInstruction->getOperand(1),newInstruction->getOperand(2)
                     );
                 }

                 newInstruction->getOperand(2)->setPrev(parentInstruction->getOperand(1)->getPrev());



                 MemoryOperand* oldMemoryOp = dynamic_cast<MemoryOperand*>(parentInstruction->getOperand(1));
                 MemoryOperand* memoryOp = dynamic_cast<MemoryOperand*>(newInstruction->getOperand(2));


                 newInstruction->insertAfter(currentInstruction);

                 parentInstruction->Delete();
                 currentInstruction->Delete();

                 currentInstruction = newInstruction;

                 isExtraPass = true;

             }
         }

         //Simplify push pop to move
         if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Push) {
             
             BaseOperand* rspOperand = currentInstruction->getOperand(1);

             if (!rspOperand->getNext() || !rspOperand->hasOneUser() ||
                 !currentInstruction->getOperand(2)->hasOneUse())
                 continue;

             Instruction* nextInstruction = rspOperand->getNext()->getParent();

             if (nextInstruction->getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop)
                 continue;

             RegisterOperand* registerOperand = dynamic_cast<RegisterOperand*>(currentInstruction->getOperand(0));
             
             if (registerOperand) {
                 BaseOperand* writeBetween = findWriteRegisterInRange(currentInstruction,
                     nextInstruction, registerOperand);

                 
                 if (writeBetween) {
                     printf("Detecting write to pushed register %d continue...\n", currentInstruction->getCount());
                     continue;
                 }
                     
             }
             printf("Simplify push pop to move : %d \n", currentInstruction->getCount());


             zasm::InstructionDetail::OperandsAccess opAccess;
             zasm::InstructionDetail::OperandsVisibility opVisibility;

             opAccess.set(0, zasm::Operand::Access::Write);
             opAccess.set(1, zasm::Operand::Access::Read);

             opVisibility.set(0, zasm::Operand::Visibility::Explicit);
             opVisibility.set(1, zasm::Operand::Visibility::Explicit);

             std::array<zasm::Operand, 10>ops;
             ops[0] = nextInstruction->getOperand(0)->getZasmOperand();
             ops[1] = currentInstruction->getOperand(0)->getZasmOperand();
                
             const auto& newZasmInstruction = zasm::InstructionDetail({},
                 zasm::x86::Mnemonic::Mov, 2,
                 ops, opAccess, opVisibility, {}, {});


             Instruction* newInstruction = zasmToInstruction(newZasmInstruction);

             newInstruction->getOperand(0)->replaceOperandWith(nextInstruction->getOperand(0));
             
             if (currentInstruction->getOperand(0)->getPrev()) {
                 currentInstruction->getOperand(0)->getPrev()->replaceOneUse(currentInstruction->getOperand(0),
                     newInstruction->getOperand(1));
             }

             newInstruction->getOperand(1)->setPrev(currentInstruction->getOperand(0)->getPrev());

             if (currentInstruction->getOperand(1)->getPrev()) {
                 currentInstruction->getOperand(1)->getPrev()
                     ->replaceAllUses(nextInstruction->getOperand(1));

             currentInstruction->getOperand(1)->getPrev()->setNext(nextInstruction->getOperand(1)->getNext());

             }

             if (nextInstruction->getOperand(1)->getNext())
                 nextInstruction->getOperand(1)->getNext()->setPrev(currentInstruction->getOperand(1)->getPrev());


             if (currentInstruction->getOperand(2)->getNext()) {
                 currentInstruction->getOperand(2)->
                     getNext()->setPrev(currentInstruction->getOperand(2)->getPrev());
             }

             newInstruction->insertAfter(nextInstruction);

             currentInstruction->Delete();
             nextInstruction->Delete();

             currentInstruction = newInstruction;
             
             isExtraPass = true;

             logger->log("Okey\n");
             formateLinkedInstructions(instruction);
         }
    }
    
  
    return isExtraPass;
}
