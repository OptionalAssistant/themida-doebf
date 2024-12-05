#include "SimplifyStackOperation.h"
#include "../utils/Utils.h"
#include "./Helpers/HelpersIterations.h"

bool SimplifyStackOperation::run(std::list<Instruction>& instructions)
{
    bool isExtraPass = false;


    //Simplify push rax pop rbx -> mov rbx,rax
    for (auto it = instructions.begin(); it != instructions.end();) {

        auto& instruction = *it;

        auto& zydis_instr = instruction.getZasmInstruction();

        if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Push) {
            it++;
            continue;
        }
        auto itNext = nextIter(it, instructions.end());
            
        if (itNext == instructions.end()) {
            it++;
            continue;
        }

        auto& nextInstruction = *itNext;

        if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop) {
            it++;
            continue;
        }
        
        printf("Simplify push pop to move : %d \n", instruction.getCount());


        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = nextInstruction.getZasmInstruction().getOperand(0);
        ops[1] = zydis_instr.getOperand(0);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(nextInstruction.getOperand(0));
        newInstruction.addOperand(instruction.getOperand(0));

        newInstruction.setCount(countGlobal++);

        instructions.insert(itNext, newInstruction);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.erase(itNext);
        it = instructions.erase(it);
        isExtraPass = true;
    }

    //Simplify mov rax,[rsp] add rsp,8  -> pop rax
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction = *it;

        if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
            instruction.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp) {
            it++;
            continue;
        }

        auto itNext = nextIter(it, instructions.end());

        if (itNext == instructions.end()) {
            it++;
            continue;
        }

        auto& nextInstruction = *itNext;

        if(nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add ||
           !nextInstruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
           nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
           !nextInstruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
           nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() < 8) { 
            it++;
            continue;
        }
        uintptr_t immValue = nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
        
        printf("Simplify add rsp,8 to pop count: %d\n", instruction.getCount());

        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::ReadWrite);
        opAccess.set(2, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Hidden);
        opVisibility.set(2, zasm::Operand::Visibility::Hidden);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction.getZasmInstruction().getOperand(0);
        ops[1] = nextInstruction.getZasmInstruction().getOperand(0);
        ops[2] = instruction.getZasmInstruction().getOperand(1);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Pop, 3,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction.getOperand(0));
        newInstruction.addOperand(nextInstruction.getOperand(0));
        newInstruction.addOperand(instruction.getOperand(1));

        newInstruction.setCount(countGlobal++);

        immValue -= 8;
        nextInstruction.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
        nextInstruction.setOperand(1, Operand(zasm::Imm(immValue)));

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.insert(itNext,newInstruction);

        instructions.erase(itNext);
        it = instructions.erase(it);
        isExtraPass = true;
    }

    //Simplify sub rsp,8 mov [rsp],reg -> push reg
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction1 = *it;

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
            !instruction1.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() < 8) {
            it++;
            continue;
        }

        uintptr_t immValue = instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction2 = *it2;

        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId()
            != zasm::Reg::Id::None) {
            it++;
            continue;
        }

        printf("Found sub rsp,immValue mov mem.Optimize to push at count: %d\n", instruction2.getCount());

        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Read);
        opAccess.set(1, zasm::Operand::Access::ReadWrite);
        opAccess.set(2, zasm::Operand::Access::Write);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Hidden);
        opVisibility.set(2, zasm::Operand::Visibility::Hidden);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction2.getZasmInstruction().getOperand(1);
        ops[1] = instruction1.getZasmInstruction().getOperand(0);
        ops[2] = instruction2.getZasmInstruction().getOperand(0);

         auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Push, 3,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction2.getOperand(1));
        newInstruction.addOperand(instruction1.getOperand(0));
        newInstruction.addOperand(instruction2.getOperand(0));

        immValue -= 8;
        instruction1.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
        instruction1.setOperand(1, Operand(zasm::Imm(immValue)));


        newInstruction.setCount(countGlobal++);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.insert(it2,newInstruction);

        instructions.erase(it2);
        it = instructions.erase(it);

        isExtraPass = true;
    }

    return isExtraPass;
}
