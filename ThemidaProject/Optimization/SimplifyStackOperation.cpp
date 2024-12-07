#include "SimplifyStackOperation.h"
#include "../utils/Utils.h"
#include "./Helpers/HelpersIterations.h"

//Simplify push reg,pop reg -> mov
bool PushPopToMove(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    auto& zydis_instr = instruction.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Push) {
        return false;
    }

    auto itNext = std::find_if(it, instructions.end(), [](Instruction& instruction) {
        return instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop;
        });

    if (itNext == instructions.end()) {
        return false;
    }

    auto& nextInstruction = *itNext;

    auto regRsp = zasm::x86::rsp;
    auto isRspAccessPresent = getNextRegisterAccess(std::next(it), itNext, regRsp);

    
    if (isRspAccessPresent != itNext) {
        return false;
    }

    printf("Simplify push pop to move : %d \n", instruction.getCount());


    auto newZasmInstruction = createMov(nextInstruction.getZasmInstruction().getOperand(0),
        zydis_instr.getOperand(0));

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

    return true;
}
//Simplify mov rax,[rsp] add rsp,8  -> pop rax
bool MovMemAddRspToPop(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
        instruction.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp) {
        return false;
    }

    auto itNext = nextIter(it, instructions.end());

    if (itNext == instructions.end()) {
        return false;
    }

    auto& nextInstruction = *itNext;

    if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add ||
        !nextInstruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
        !nextInstruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() < 8) {
        return false;
    }
    uintptr_t immValue = nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();

    printf("Simplify add rsp,8 to pop count: %d\n", instruction.getCount());


    auto newZasmInstruction = createPop(instruction.getZasmInstruction().getOperand(0),
        nextInstruction.getZasmInstruction().getOperand(0), instruction.getZasmInstruction().getOperand(1));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(instruction.getOperand(0));
    newInstruction.addOperand(nextInstruction.getOperand(0));
    newInstruction.addOperand(instruction.getOperand(1));

    newInstruction.setCount(countGlobal++);

    immValue -= 8;
    nextInstruction.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
    nextInstruction.setOperand(1, new BaseOperand(zasm::Imm(immValue)));

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());

    instructions.insert(itNext, newInstruction);

    instructions.erase(itNext);
    it = instructions.erase(it);

    return true;
}
//Simplify sub rsp,8 mov [rsp],reg -> push reg
bool SubRspMovMemToPush(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
        !instruction1.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() < 8) {
        return false;
    }

    uintptr_t immValue = instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId()
        != zasm::Reg::Id::None) {
        return false;
    }

    printf("Found sub rsp,immValue mov mem.Optimize to push at count: %d\n", instruction2.getCount());

    auto newZasmInstruction = createPush(instruction2.getZasmInstruction().getOperand(1),
        instruction1.getZasmInstruction().getOperand(0), instruction2.getZasmInstruction().getOperand(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(instruction2.getOperand(1));
    newInstruction.addOperand(instruction1.getOperand(0));
    newInstruction.addOperand(instruction2.getOperand(0));

    immValue -= 8;
    instruction1.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
    instruction1.setOperand(1, new BaseOperand(zasm::Imm(immValue)));


    newInstruction.setCount(countGlobal++);

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());

    instructions.insert(it2, newInstruction);

    instructions.erase(it2);
    it = instructions.erase(it);

    return true;
}

//push rax xor[rsp],12345 pop rbx
bool OptimizePass1(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;


    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push) {
        return false;
    }

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& instruction2 = *it2;

    if (!instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        instruction2.getZasmInstruction().getOperandAccess(0) != zasm::detail::OperandAccess::ReadWrite) {
        return false;
    }


    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop) {
        return false;
    }


    printf("Found unopyimize operation throgh push pop and op between: %s count: %d\n",
        formatInstruction(instruction1.getZasmInstruction()).c_str(),
        instruction1.getCount());



    auto newZasmInstruction2 = createMov(instruction3.getZasmInstruction().getOperand(0),
        instruction1.getZasmInstruction().getOperand(0));

    Instruction newInstruction2;
    newInstruction2.setZasmInstruction(newZasmInstruction2);

    newInstruction2.addOperand(instruction3.getOperand(0));
    newInstruction2.addOperand(instruction1.getOperand(0));

    newInstruction2.setCount(countGlobal++);

    instruction2.setOperand(0,instruction3.getOperand(0));
    instruction2.getZasmInstruction().setOperand(0, instruction3.getZasmInstruction().getOperand(0));

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction2.getZasmInstruction()).c_str(),
        newInstruction2.getCount());

    printf("Generated new instruction(2): %s count: %d\n",
        formatInstruction(instruction2.getZasmInstruction()).c_str(),
        instruction2.getCount());

    instructions.insert(it, newInstruction2);

    instructions.erase(it3);
    it = instructions.erase(it);

    return true;
}
bool SimplifyStackOperation::run(std::list<Instruction>::iterator it,std::list<Instruction>& instructions)
{

    if (PushPopToMove(it, instructions))
        return true;
    if (MovMemAddRspToPop(it, instructions))
        return true;
    if (SubRspMovMemToPush(it, instructions))
        return true;
    if (OptimizePass1(it, instructions))
        return true;

    return false;
}
