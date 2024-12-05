#include "SimplifyDeadcodeElimination.h"
#include "../Instruction/Instruction.h"
#include "../utils/Utils.h"
#include "./Helpers/HelpersIterations.h"

bool SimplifyDeadcodeElimination::run(std::list<Instruction>& instructions)
{
	bool isExtraPass = false;
	 
    //mov rbx,rbx
    for (auto it = instructions.begin(); it != instructions.end();) {

        auto& instruction = *it;

        auto& zydis_instr = instruction.getZasmInstruction();

        if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
            !zydis_instr.getOperand(1).holds<zasm::Reg>() ||
            zydis_instr.getOperand(0).get<zasm::Reg>() != zydis_instr.getOperand(1).get<zasm::Reg>()) {
            it++;
            continue;
        }
        printf("Found the same move register at address: %lld\n", instruction.getCount());
        it = instructions.erase(it);
        isExtraPass = true;
    }

    //Push dead memory e.g push rax mov [rsp],rbx -> push rbx
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction = *it;

        auto& zydis_instr = instruction.getZasmInstruction();

        if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Push) {
            it++;
            continue;
        }
        auto itNext = nextIter(it, instructions.end());

        if (itNext == instructions.end())
            break;

        auto& nextInstruction = *itNext;

        if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !nextInstruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0) {
            it++;
            continue;
        }
        
        printf("Found dead memory memory push at count : %d\n", instruction.getCount());


        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Read);
        opAccess.set(1, zasm::Operand::Access::ReadWrite);
        opAccess.set(2, zasm::Operand::Access::Write);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Hidden);
        opVisibility.set(2, zasm::Operand::Visibility::Hidden);

        std::array<zasm::Operand, 10>ops;
        ops[0] = nextInstruction.getZasmInstruction().getOperand(1);
        ops[1] = instruction.getZasmInstruction().getOperand(0);
        ops[2] = nextInstruction.getZasmInstruction().getOperand(0);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Push, 3,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);
        newInstruction.setCount(countGlobal++);

        newInstruction.addOperand(nextInstruction.getOperand(1));
        newInstruction.addOperand(instruction.getOperand(0));
        newInstruction.addOperand(nextInstruction.getOperand(0));


        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.insert(itNext, newInstruction);

        instructions.erase(itNext);
        it = instructions.erase(it);

        isExtraPass = true;
    }
   
    //Elimination add sub 0 mutation
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction = *it;

        if ((instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
            !instruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            instruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() != 0) {
            it++;
            continue;
        }

        printf("Found adding 0 to register dead code: %d\n", instruction.getCount());

        it = instructions.erase(it);

        isExtraPass = true;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction = *it;



        if ((instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov &&
            instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
            !instruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto itNextWrite = getNextRegisterWrite(std::next(it), instructions.end(),
            instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());

        if (itNextWrite == instructions.end()) {
            it++;
            continue;
        }

        auto foundRead = getNextRegisterRead(std::next(it), itNextWrite,
            instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());


        if (foundRead != itNextWrite) {
            it++;
            continue;
        }

        auto foundReadWrite = getNextRegisterReadWrite(std::next(it), itNextWrite,
            instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());

        if (foundReadWrite != itNextWrite) {
            it++;
            continue;
        }

        printf("Found dead mov at count: %d \n", instruction.getCount());

        it = instructions.erase(it);

    }
    
	return isExtraPass;
}
