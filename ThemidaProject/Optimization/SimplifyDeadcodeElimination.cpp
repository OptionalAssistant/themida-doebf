#include "SimplifyDeadcodeElimination.h"
#include "../Instruction/Instruction.h"
#include "../utils/Utils.h"
#include "./Helpers/HelpersIterations.h"
#include "../utils/Logger.h"

//mov rbx,rbx
static bool OptimizeMovSameRegisters(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    auto& zydis_instr = instruction.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
        !zydis_instr.getOperand(1).holds<zasm::Reg>() ||
        zydis_instr.getOperand(0).get<zasm::Reg>() != zydis_instr.getOperand(1).get<zasm::Reg>()) {
        return false;
    }
  //  printf("Found the same move register at address: %lld\n", instruction.getCount());
    it = instructions.erase(it);
    return true;
}

//Elimination add sub 0 mutation
static bool OptimizeAddSub0(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if ((instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
        !instruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        instruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() != 0) {
        return false;
    }

    //printf("Found adding 0 to register dead code: %d\n", instruction.getCount());

    it = instructions.erase(it);

    return true;
}
//Write before read(Register)
static bool OptimizeWriteBeforeRead(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if ((instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Or &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Shl &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor) ||
        !instruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }

    auto itNextWrite = getNextRegisterWrite(std::next(it), instructions.end(),
        instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());

    if (itNextWrite == instructions.end()) {
        return false;
    }

    auto foundRead = getNextRegisterRead(std::next(it), std::next(itNextWrite),
        instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());


    if (foundRead != std::next(itNextWrite)) {
        return false;
    }

    auto foundReadWrite = getNextRegisterReadWrite(std::next(it), std::next(itNextWrite),
        instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());

    if (foundReadWrite != std::next(itNextWrite)) {
        return false;
    }

    auto& nextWrite = *itNextWrite;
    //printf("Found dead mov at count: %d \n", instruction.getCount());

   // printf("Instruction that overwrites %d\n", nextWrite.getCount());

    it = instructions.erase(it);

    return true;
}
//Remove later just for test
static bool RemoveJumpsZero(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Jmp &&
        instruction.getZasmInstruction().getOperand(0).holds<zasm::Imm>() &&
        instruction.getZasmInstruction().getOperand(0).get<zasm::Imm>() == 0) {
        printf("Detected jump offset 0 at count:%d\n", instruction.getCount());

        instructions.erase(it);
        return true;
    }
    return false;
}
//Remove dead memory push (not linear) e.g push reg mov [rsp + 0x70],reg2 -> push reg2
static bool RemoveDeadMemoryPushWrite(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {

    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push)
        return false;

    MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(instruction.getOperand(2));

    auto itNextMemoryWrite = getNextMemoryWrite(std::next(it),
        instructions.end(), memoryOperand->getMemoryAddress());

    if (itNextMemoryWrite == instructions.end())
        return false;

    auto& nextMemoryWrite = *itNextMemoryWrite;

    if (nextMemoryWrite.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov)
        return false;

    auto isReadPresent = getNextMemoryRead(it, itNextMemoryWrite, memoryOperand->getMemoryAddress());

    if (isReadPresent != itNextMemoryWrite)
        return false;

    if (nextMemoryWrite.getZasmInstruction().getOperand(1).holds<zasm::Reg>()) {
        auto isWritePresent = getNextRegisterReadWriteOrWrite(it, itNextMemoryWrite,
            nextMemoryWrite.getZasmInstruction().getOperand(1).get<zasm::Reg>());

        if (isWritePresent != itNextMemoryWrite)
            return false;
    }

    /*printf("Found dead push...Memory was overwritten at count %d\n",
        instruction.getCount());*/

    //printf("Next instruction %d\n",nextMemoryWrite.getCount());

   instruction.setOperand(0, nextMemoryWrite.getOperand(1));
    instruction.getZasmInstruction().setOperand(0, nextMemoryWrite.getZasmInstruction().getOperand(1));

    instructions.erase(itNextMemoryWrite);

    return true;
}
static bool OptimizePass1(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    
    auto instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor)
        return false;

    auto it2 = nextIter(it,instructions.end());

    if (it2 == instructions.end())
        return false;

    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor)
        return false;

    if (!instruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>())
        return false;

    if (instruction.getZasmInstruction().getOperand(0) != instruction2.getZasmInstruction().getOperand(0))
        return false;

    //printf("Found xor the same constant(DEAD) at count: %d\n", instruction.getCount());

    instructions.erase(it2);
    instructions.erase(it);

    return true;
}

bool SimplifyDeadcodeElimination::run(std::list<Instruction>::iterator it, std::list<Instruction>& instructions)
{
 
    if (OptimizeMovSameRegisters(it, instructions))
        return true;
    if (OptimizeAddSub0(it, instructions))
        return true;
    if (OptimizeWriteBeforeRead(it, instructions))
        return true;
    //if (RemoveJumpsZero(it, instructions))
    //    return true;
    if (RemoveDeadMemoryPushWrite(it, instructions))
        return true;
    if (OptimizePass1(it, instructions))
        return true;

	return false;
}
