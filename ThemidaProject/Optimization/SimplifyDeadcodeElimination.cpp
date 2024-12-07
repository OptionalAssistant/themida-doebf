#include "SimplifyDeadcodeElimination.h"
#include "../Instruction/Instruction.h"
#include "../utils/Utils.h"
#include "./Helpers/HelpersIterations.h"
#include "../utils/Logger.h"

//mov rbx,rbx
bool OptimizeMovSameRegisters(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    auto& zydis_instr = instruction.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
        !zydis_instr.getOperand(1).holds<zasm::Reg>() ||
        zydis_instr.getOperand(0).get<zasm::Reg>() != zydis_instr.getOperand(1).get<zasm::Reg>()) {
        return false;
    }
    printf("Found the same move register at address: %lld\n", instruction.getCount());
    it = instructions.erase(it);
    return true;
}

//Push dead memory e.g push rax mov [rsp],rbx -> push rbx
bool OptimizeDeadMemoryPush(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    auto& zydis_instr = instruction.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Push) {
        return false;
    }
    auto itNext = nextIter(it, instructions.end());

    if (itNext == instructions.end())
        return false;

    auto& nextInstruction = *itNext;

    if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !nextInstruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0) {
        return false;
    }
    if (instruction.getCount() == 368) {
        logger->log("check");
        printOutInstructions(instructions);
        printf("%s\n",formatInstruction_(nextInstruction).c_str());
    }

    printf("Found dead memory memory push at count : %d\n", instruction.getCount());

    auto newZasmInstruction = createPush(nextInstruction.getZasmInstruction().getOperand(1),
        instruction.getZasmInstruction().getOperand(0), nextInstruction.getZasmInstruction().getOperand(0));

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

    return true;
}

//Elimination add sub 0 mutation
bool OptimizeAddSub0(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if ((instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
        !instruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        instruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() != 0) {
        return false;
    }

    printf("Found adding 0 to register dead code: %d\n", instruction.getCount());

    it = instructions.erase(it);

    return true;
}
//Write before read
bool OptimizeWriteBeforeRead(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getCount() == 309) {
        printf("");
    }
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
    printf("Found dead mov at count: %d \n", instruction.getCount());

    printf("Instruction that overwrites %d\n", nextWrite.getCount());

    it = instructions.erase(it);

    return true;
}
//Remove later just for test
bool RemoveJumps(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Jmp) {
        instructions.erase(it);
        return true;
    }
    return false;
}
bool SimplifyDeadcodeElimination::run(std::list<Instruction>::iterator it, std::list<Instruction>& instructions)
{
 
    if (OptimizeMovSameRegisters(it, instructions))
        return true;
    if (OptimizeDeadMemoryPush(it, instructions))
        return true;
    if (OptimizeAddSub0(it, instructions))
        return true;
    if (OptimizeWriteBeforeRead(it, instructions))
        return true;
    if (RemoveJumps(it, instructions))
        return true;
        
	return false;
}
