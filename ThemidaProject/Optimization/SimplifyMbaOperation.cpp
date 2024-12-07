#include "SimplifyMbaOperation.h"
#include "./Helpers/HelpersIterations.h"
#include "../utils/Utils.h"


/* xor rdx, qword ptr ss : [rsp] --
xor qword ptr ss : [rsp] , rdx --
xor rdx, qword ptr ss : [rsp] --
-------------->
xchg [rsp],rdx
 */
bool Optimize3XorToXchg(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor) {
        return false;
    }
    auto& op1 = instruction1.getZasmInstruction().getOperand(0);
    auto& op2 = instruction1.getZasmInstruction().getOperand(1);

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& instruction2 = *it2;


    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor ||
        instruction2.getZasmInstruction().getOperand(1) != op1 ||
        instruction2.getZasmInstruction().getOperand(0) != op2) {
        return false;
    }

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor ||
        instruction3.getZasmInstruction().getOperand(0) != op1 ||
        instruction3.getZasmInstruction().getOperand(1) != op2) {
        return false;
    }

    printf("Found 3 xor to xchg optimization at count: %d\n", instruction1.getCount());


    auto newZasmInstruction = createXchg(op1, op2);

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(instruction1.getOperand(0));
    newInstruction.addOperand(instruction2.getOperand(1));

    newInstruction.setCount(countGlobal++);

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());

    instructions.insert(it3, newInstruction);

    instructions.erase(it3);
    instructions.erase(it2);
    it = instructions.erase(it);

    return true;
}
//xor r9,r9
bool OptimizeXorSameRegister(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor ||
        instruction.getZasmInstruction().getOperand(0) != instruction.getZasmInstruction().getOperand(1)) {
        return false;
    }

    printf("Found xor the same register at count : %d\n", instruction.getCount());


    auto newZasmInstruction = createMov(instruction.getZasmInstruction().getOperand(0), zasm::Imm(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(instruction.getOperand(0));
    newInstruction.addOperand(new BaseOperand(zasm::Imm(0)));

    newInstruction.setCount(countGlobal++);

    instructions.insert(it, newInstruction);

    it = instructions.erase(it);

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());

    return true;
}
/*
push rsi --
mov rsi,rsp
xchg qword ptr ss:[rsp], rsi --
add rsp,10
pop rsp ||  mov rsp,[rsp] --
------------------->
add rsp,8
*/
bool OptimizeXchgToRspChange(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>() != zasm::x86::rsp ||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
        return false;
    }

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }


    auto& instruction3 = *it3;


    if ((instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
        !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction3.getZasmInstruction().getOperand(0) != op1) {
         return false;
    }

    auto it4 = nextIter(it3, instructions.end());

    if (it4 == instructions.end()) {
        return false;
    }

    auto& instruction4 = *it4;

    if ((instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xchg ||
        !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
        !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction4.getZasmInstruction().getOperand(1).get<zasm::Reg>() != op1) &&
        (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xchg ||
            !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0 ||
            !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1)) {
        return false;
    }

    auto it5 = nextIter(it4, instructions.end());

    if (it5 == instructions.end()) {
        return false;
    }

    auto& instruction5 = *it5;

    if (instruction5.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop &&
        instruction5.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov) {
        return false;
    }

    if (instruction5.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop &&
        (!instruction5.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction5.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp)) {
        return false;
    }

    if (instruction5.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
        (!instruction5.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction5.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
            !instruction5.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
            instruction5.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction5.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction5.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0)) {
        return false;
    }

    int64_t result = calculateSubAdd({ instruction3.getZasmInstruction() });
    result -= 8;

    printf("Found unoptimized stack pointer arithmetic(xchg) (second case): %d\n",
        instruction1.getCount());

    auto mnemonic = result < 0 ? zasm::x86::Mnemonic::Sub : zasm::x86::Mnemonic::Add;
    int64_t immediateValue = std::abs(result);

    zasm::InstructionDetail::OperandsAccess opAccess;
    zasm::InstructionDetail::OperandsVisibility opVisibility;

    opAccess.set(0, zasm::Operand::Access::ReadWrite);
    opAccess.set(1, zasm::Operand::Access::Read);

    opVisibility.set(0, zasm::Operand::Visibility::Explicit);
    opVisibility.set(1, zasm::Operand::Visibility::Explicit);

    std::array<zasm::Operand, 10>ops;
    ops[0] = zasm::x86::rsp;
    ops[1] = zasm::Imm(immediateValue);

    auto newZasmInstruction = zasm::InstructionDetail({},
        mnemonic, 2,
        ops, opAccess, opVisibility, {}, {});


    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(new BaseOperand(zasm::x86::rsp));
    newInstruction.addOperand(new BaseOperand(zasm::Imm(8)));

    newInstruction.setCount(countGlobal++);

    instructions.insert(it5, newInstruction);

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());


    instructions.erase(it5);
    instructions.erase(it4);
    instructions.erase(it3);
    instructions.erase(it2);
    it = instructions.erase(it);

    return true;
}
/*
push rax --
mov rax, r11 --
mov r11, rax --
pop rax --
------
mov r11,r11
*/
bool OptimizePass3(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }
    auto it2 = nextIter(it, instructions.end());
    if (it2 == instructions.end()) {
        return false;
    }

    auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction2.getZasmInstruction().getOperand(0) != op1) {
        return false;
    }

    auto& op2 = instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>();

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;
    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction3.getZasmInstruction().getOperand(1) != op1) {
        return false;
    }
    auto it4 = nextIter(it3, instructions.end());
    if (it4 == instructions.end()) {
        return false;
    }

    auto& instruction4 = *it4;

    if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ||
        !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction4.getZasmInstruction().getOperand(0) != op1) {
        return false;
    }
    printf("Found instructions that does notging at count(mov actually): %d\n",
        instruction1.getCount());


    auto newZasmInstruction = createMov(instruction3.getZasmInstruction().getOperand(0),
        instruction2.getZasmInstruction().getOperand(1));
   
    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(instruction3.getOperand(0));
    newInstruction.addOperand(instruction2.getOperand(1));

    newInstruction.setCount(countGlobal++);

    instructions.insert(it4, newInstruction);


    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());

    instructions.erase(it);
    instructions.erase(it2);
    instructions.erase(it3);
    it = instructions.erase(it4);

    return true;
}
/*
push r13 --
mov r13, rdx --
mov qword ptr ss:[rsp+0x08], r13 --
pop r13 --
*/
bool OptimizePass4(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    if (instruction1.getCount() == 1065)
        printf("");

    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
    auto& instruction2 = *it2;

    if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor) ||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
        return false;
    }

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;

    if ((instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov &&
        instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor) ||
        !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() < 8 ||
        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction3.getZasmInstruction().getOperand(1).get<zasm::Reg>() != op1) {
        return false;
    }


    auto it4 = nextIter(it3, instructions.end());

    if (it4 == instructions.end()) {
        return false;
    }

    auto& instruction4 = *it4;

    if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ||
        !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
        return false;
    }


    printf("Found push mov mov pop to mov mem: %s count: %d\n",
        formatInstruction(instruction1.getZasmInstruction()).c_str(),
        instruction1.getCount());

    auto disp = instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement();

    instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().setDisplacement(disp - 8);
    instruction3.setOperand(1, instruction2.getOperand(1));
    instruction3.getZasmInstruction().setOperand(1, instruction2.getZasmInstruction().getOperand(1));

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(instruction3.getZasmInstruction()).c_str(),
        instruction3.getCount());

    instructions.erase(it4);
    instructions.erase(it2);
    it = instructions.erase(it);

    return true;
}
/*
push r8 --
xor qword ptr ss:[rsp], 0x33f70ba4 --
pop rax --
xor rax, 0x33f70ba4 --
-------->
mov rax,r8
*/
/*
push rbp --
add qword ptr ss:[rsp], 0x7c7c02df --
pop rbp
sub rbp, 0x7c7c02df --
------------->
mov rbp,rbp
*/
bool OptimizePass5(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& instruction2 = *it2;

    if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor  &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub)||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
        return false;
    }

    auto& immValue = instruction2.getZasmInstruction().getOperand(1).get<zasm::Imm>();

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop) {
        return false;
    }
    auto& op3 = instruction3.getZasmInstruction().getOperand(0);

    auto it4 = nextIter(it3, instructions.end());

    if (it4 == instructions.end()) {
        return false;
    }

    auto& instruction4 = *it4;

    if ((instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor  && 
        instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add && 
        instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub)||
        !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        instruction4.getZasmInstruction().getOperand(1).get<zasm::Imm>() !=
        immValue) {
        return false;
    }

    auto& gpRegister = instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();

    auto& gpRegisterCompared = op3.get<zasm::Reg>().as<zasm::x86::Gp>();

    if (gpRegisterCompared != gpRegister.r32() &&
        gpRegisterCompared != gpRegister.r64()) {
        return false;
    }
    if (instruction4.getZasmInstruction().getMnemonic() == instruction2.getZasmInstruction().getMnemonic() &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor) {
        return false;
    }

    if ((instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor &&
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Add &&
        instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
        (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor &&
         instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Sub &&
         instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add)) {
        return false;

    }
    printf("Found push xor pop xor to mov simplification at count: %d\n",
        instruction4.getCount());


    auto newZasmInstruction = createMov(instruction4.getZasmInstruction().getOperand(0),
        instruction1.getZasmInstruction().getOperand(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(instruction4.getOperand(0));
    newInstruction.addOperand(instruction1.getOperand(0));

    newInstruction.setCount(countGlobal++);

    instructions.insert(it4, newInstruction);

    instructions.erase(it4);
    instructions.erase(it3);
    instructions.erase(it2);
    it = instructions.erase(it);

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());

    return true;
}
///*
//push rsi --
//mov rsi,rsp
//xchg qword ptr ss:[rsp], rsi --
//pop rsp ||  mov rsp,[rsp] --Not Needed porbably(Needed probably)
//------------------->
//sub rsp,8
//*/
bool OptimizePass6(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;


    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>() != zasm::x86::rsp ||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
        return false;
    }

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;

    if (instruction1.getCount() == 1091) {
        printf("%s\n", formatInstruction(instruction3.getZasmInstruction()).c_str());

    }
    if ((instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xchg ||
        !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
        !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        instruction3.getZasmInstruction().getOperand(1).get<zasm::Reg>() != op1) &&
        (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xchg ||
            !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
            instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0 ||
            !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1)) {
        return false;
    }

    auto it4 = nextIter(it3, instructions.end());

    if (it4 == instructions.end()) {
        return false;
    }

    auto& instruction4 = *it4;

    if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop &&
        instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov) {
        return false;
    }

    if (instruction4.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop &&
        (!instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp)) {
        return false;
    }

    if (instruction4.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
        (!instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
            !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0)) {
        return false;
    }

    printf("Found unoptimized stack pointer arithmetic(xchg) : %d\n", instruction1.getCount());

    
    auto newZasmInstruction = createSub(zasm::x86::rsp, zasm::Imm(8));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);

    newInstruction.addOperand(new BaseOperand(zasm::x86::rsp));
    newInstruction.addOperand(new BaseOperand(zasm::Imm(8)));

    newInstruction.setCount(countGlobal++);

    instructions.insert(it4, newInstruction);

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());



    instructions.erase(it4);
    instructions.erase(it3);
    instructions.erase(it2);
    it = instructions.erase(it);

    return true;

}
/*
push r13 --
mov qword ptr ss:[rsp+0x08], 0x00000038 --
pop r13 -- Fuck this pass.....Needed.....
*/
bool OptimizePass7(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
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
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() < 8 ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None) {
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


    printf("Found push mov mov pop to mov mem: %s count: %d\n",
        formatInstruction(instruction1.getZasmInstruction()).c_str(),
        instruction1.getCount());

    auto disp = instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement();

    instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().setDisplacement(disp - 8);


    auto newZasmInstruction2 = createMov(instruction3.getZasmInstruction().getOperand(0),
        instruction1.getZasmInstruction().getOperand(0));

    Instruction newInstruction2;
    newInstruction2.setZasmInstruction(newZasmInstruction2);

    newInstruction2.addOperand(instruction3.getOperand(0));
    newInstruction2.addOperand(instruction1.getOperand(0));

    newInstruction2.setCount(countGlobal++);

    if (instruction1.getCount() == 45)
        printf("");

    printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction2.getZasmInstruction()).c_str(),
        newInstruction2.getCount());

    printf("Generated new instruction(2): %s count: %d\n",
        formatInstruction(instruction2.getZasmInstruction()).c_str(),
        instruction2.getCount());

    instructions.insert(it3, newInstruction2);

    instructions.erase(it3);
    it = instructions.erase(it);

    return true;
}
bool SimplifyMbaOperation::run(std::list<Instruction>::iterator it, std::list<Instruction>& instructions)
{

    if (OptimizeXchgToRspChange(it, instructions))
        return true;
    if (OptimizeXorSameRegister(it, instructions))
        return true;
    if (OptimizePass3(it, instructions))
        return true;
    if (Optimize3XorToXchg(it, instructions))
        return true;
    if (OptimizePass4(it, instructions))
        return true;
    if (OptimizePass5(it, instructions))
        return true;
    if (OptimizePass6(it, instructions))
        return true;
    if (OptimizePass7(it, instructions))
        return true;

    return false;  

}
