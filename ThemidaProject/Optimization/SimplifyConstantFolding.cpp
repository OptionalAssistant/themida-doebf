#include <cmath>

#include "SimplifyConstantFolding.h"
#include "../Instruction/Instruction.h"
#include "./Helpers/HelpersIterations.h"
#include "../utils/Utils.h"
#include "../emulator/emu.h"

bool SimplifyConstantFolding::run(std::list<Instruction>& instructions)
{
	printf("Simplify constant folding works\n");

    bool isExtraPass = false;

    for (auto it = instructions.begin(); it != instructions.end();) {

        auto& instruction1 = *it;

        auto& zydis_instr = instruction1.getZasmInstruction();
        
        if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
            !zydis_instr.getOperand(1).holds<zasm::Imm>()) {
            it++;
            continue;
        }
        
        printf("Found mov reg,const at count: %d\n", instruction1.getCount());

        auto nextWrite = getNextRegisterReadWriteOrWrite(std::next(it), instructions.end(),
            zydis_instr.getOperand(0).get<zasm::Reg>());

        if (nextWrite == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction2 = *nextWrite;

        printf("Found next readWrite or Write at count: %d\n", instruction2.getCount());


        for (auto itStart = it; it != nextWrite;) {
            auto itRead = getNextRegisterRead(itStart, nextWrite, zydis_instr.getOperand(0).get<zasm::Reg>());

            if (itRead == nextWrite)
                break;

            auto& readInstruction = *itRead;

            printf("Found read (Constant actually) at count: %d\n", readInstruction.getCount());

            for (int i = 0; i < readInstruction.getZasmInstruction().getOperandCount(); i++) {
                auto op = readInstruction.getZasmInstruction().getOperand(i);

                if (op.holds<zasm::Reg>() && 
                    isSameRegister(op.get<zasm::Reg>(), zydis_instr.getOperand(0).get<zasm::Reg>())) {

                    if (readInstruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::Read)
                    {
                        zasm::Imm immValue = zydis_instr.getOperand(1).get<zasm::Imm>().value<uintptr_t>();

                        readInstruction.setOperand(i, Operand(immValue));
                        readInstruction.getZasmInstruction().setOperand(i, immValue);
                        break;
                    }
                }
            }
            itStart = std::next(itRead);
        }

        printf("\n");

        it++;
    }

    
    //Sub reg,8 Add reg,16
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto instruction1 = *it;

        if ((instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub )||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>() == zasm::x86::rsp ||
            !instruction1.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
            it++;
            continue;
        }
        
        auto& op1 = instruction1.getZasmInstruction().getOperand(0);
        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto instruction2 = *it2;

        if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub )||
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(0) != op1) {
            it++;
            continue;
        }

        printf("Found unfold sub-add at count: %d\n", instruction1.getCount());
        

        int64_t result = calculateSubAdd({ instruction1.getZasmInstruction(),instruction2.getZasmInstruction()});

        printf("Result folding : %lld\n", result);
        
        auto mnemonic = result < 0 ? zasm::x86::Mnemonic::Sub : zasm::x86::Mnemonic::Add;
        int64_t immediateValue = std::abs(result); // Ensure immediate is positive

        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::ReadWrite);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction1.getZasmInstruction().getOperand(0);
        ops[1] = zasm::Imm(immediateValue);

         auto newZasmInstruction = zasm::InstructionDetail({},
             mnemonic, 2,
            ops, opAccess, opVisibility, {}, {});


        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction1.getOperand(0));
        newInstruction.addOperand(Operand(zasm::Imm(immediateValue)));

        newInstruction.setCount(countGlobal++);

        instructions.insert(it2, newInstruction);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.erase(it2);
        it = instructions.erase(it);

        isExtraPass = true;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction1 = *it;

        auto& zydis_instr = instruction1.getZasmInstruction();

        if (instruction1.getCount() == 252)
            printf("");

        if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
            !zydis_instr.getOperand(1).holds<zasm::Imm>()) {
            it++;
            continue;
        }
  
        auto nextAccess = getNextRegisterAccess(std::next(it), instructions.end(),
            zydis_instr.getOperand(0).get<zasm::Reg>());

        if (nextAccess == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction2 = *nextAccess;

        bool isReadWrite = false;
        for (int i = 0; i < instruction2.getZasmInstruction().getOperandCount(); i++) {
            auto op = instruction2.getZasmInstruction().getOperand(i);

            if (op.holds<zasm::Reg>() &&
                isSameRegister(op.get<zasm::Reg>(), zydis_instr.getOperand(0).get<zasm::Reg>())) {

                if (instruction2.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::ReadWrite)
                {
                    isReadWrite = true;
                    break;
                }
            }
        }

        if (!isReadWrite) {
            it++;
            continue;
        }
        
        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub && 
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::And &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Shr &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Shl &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Neg &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Not &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Dec &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Inc &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Or) {
            it++;
            continue;
        }

        if ((instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Sub ||
            instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Add ||
            instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::And||
            instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Xor ||
            instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Shl||
            instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Shr ||
            instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Or) &&
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
            it++;
            continue;
        }

        auto it3 = nextIter(nextAccess, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        printf("Found constant folding optimization at count: %d\n", instruction1.getCount());

        auto& instruction3 = *it3;

        auto& registerArray = instruction3.getRegistersArray();

        uintptr_t value = reg_read_(registerArray,instruction1.getZasmInstruction()
            .getOperand(0).get<zasm::Reg>(),
            instruction1.getRflags());

       

        auto& rFlags = instruction3.getRflags();

        auto nextIterator = std::next(it);
        auto& nextInstruction = *nextIterator;

        auto& registerArrayInitial = nextInstruction.getRegistersArray();

        reg_write_(registerArrayInitial, instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>(),
            value, rFlags);

        instruction1.setOperand(1, Operand(zasm::Imm(value)));
        instruction1.getZasmInstruction().setOperand(1,zasm::Imm(value));


        printf("Updated mov reg,const  instruction: %s count: %d\n",
            formatInstruction(instruction1.getZasmInstruction()).c_str(),
            instruction1.getCount());

        instructions.erase(nextAccess);

        isExtraPass = true;
    }

    return isExtraPass;
}
