#include "SimplifyMbaOperation.h"
#include "./Helpers/HelpersIterations.h"
#include "../utils/Utils.h"

bool SimplifyMbaOperation::run(std::list<Instruction>& instructions)
{
    bool isExtraPass = false;

    /*
    push rax --
    mov rax, r11 --
    mov r11, rax --
    pop rax --

    ------

    mov r11,r11
    */
    for (auto it = instructions.begin(); it != instructions.end();) {
       
        auto& instruction1 = *it;

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }
        auto it2 = nextIter(it, instructions.end());
        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
        auto& instruction2 = *it2;

        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(0) != op1) {
            it++;
            continue;
        }

        auto& op2 = instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>();

        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction3 = *it3;
        if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction3.getZasmInstruction().getOperand(1) != op1) {
            it++;
            continue;
        }
        auto it4 = nextIter(it3, instructions.end());
        if (it4 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction4 = *it4;

        if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ||
            !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(0) != op1) {
            it++;
            continue;
        }
        printf("Found instructions that does notging at count(mov actually): %d\n",
            instruction1.getCount());


        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction3.getZasmInstruction().getOperand(0);
        ops[1] = instruction2.getZasmInstruction().getOperand(1);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});


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

        isExtraPass = true;
        continue;
    }
    /*
    push r8 --
    xor qword ptr ss:[rsp], 0x33f70ba4 --
    pop rax --
    xor rax, 0x33f70ba4 --
    -------->
    mov rax,r8
    */
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction1 = *it;

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction2 = *it2;

        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
            it++;
            continue;
        }
        
        auto& immValue = instruction2.getZasmInstruction().getOperand(1).get<zasm::Imm>();

        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction3 = *it3;

        if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop) {
            it++;
            continue;
        }
        auto& op3 = instruction3.getZasmInstruction().getOperand(0);

        auto it4 = nextIter(it3, instructions.end());

        if (it4 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction4 = *it4;

        if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor ||
            instruction4.getZasmInstruction().getOperand(0) != op3 ||
            !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Imm>() != 
            immValue) {
            it++;
            continue;
        }

        printf("Found push xor pop xor to mov simplification at count: %d\n",
            instruction4.getCount());


        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction4.getZasmInstruction().getOperand(0);
        ops[1] = instruction1.getZasmInstruction().getOperand(0);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});

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
        
        isExtraPass = true;
        
    }
    /*
     push rbp --
     add qword ptr ss:[rsp], 0x7c7c02df --
     pop rbp
     sub rbp, 0x7c7c02df --
     ------------->
     mov rbp,rbp
    */
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction1 = *it;


        if (instruction1.getCount() == 334)
            printf("");

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();

        auto it2 = nextIter(it,instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction2 = *it2;

        if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0) {
            it++;
            continue;
        }

        auto& immValue = instruction2.getZasmInstruction().getOperand(1).get<zasm::Imm>();

        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction3 = *it3;

        if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ) {
            it++;
            continue;
        }

        auto& op2 = instruction3.getZasmInstruction().getOperand(0);

        auto it4 = nextIter(it3, instructions.end());

        auto& instruction4 = *it4;

        if ((instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub &&
            instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add) ||
            !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            instruction4.getZasmInstruction().getOperand(1) != immValue) {
            it++;
            continue;
        }

        auto& gpRegister = instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();

        auto& gpRegisterCompared = op2.get<zasm::Reg>().as<zasm::x86::Gp>();

        if (gpRegisterCompared != gpRegister.r32() &&
            gpRegisterCompared != gpRegister.r64()) {
            it++;
            continue;
        }
        if (instruction4.getZasmInstruction().getMnemonic() == instruction2.getZasmInstruction().getMnemonic()) {
            it++;
            continue;
        }

        printf("Found non sense instructions(sub ,add(nothing))(mov actually) at :%d\n",
            instruction1.getCount());


        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = op2;
        ops[1] = op1;

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction1.getOperand(0));
        newInstruction.addOperand(instruction3.getOperand(0));

        newInstruction.setCount(countGlobal++);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.insert(it4, newInstruction);

        instructions.erase(it4);
        instructions.erase(it3);
        instructions.erase(it2);
        it = instructions.erase(it);

        isExtraPass = true;

    }
    /*
    push rsi --
    mov rsi,rsp
    xchg qword ptr ss:[rsp], rsi --
    pop rsp ||  mov rsp,[rsp] --
    ------------------->
    sub rsp,8
    */
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction1 = *it;

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
        auto& instruction2 = *it2;

        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>() != zasm::x86::rsp ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }

        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction3 = *it3;

        if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xchg ||
            !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
            !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction3.getZasmInstruction().getOperand(1).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }

        auto it4 = nextIter(it3, instructions.end());

        if (it4 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction4 = *it4;

        if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop &&
            instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov) {
            it++;
            continue;
        }

        if (instruction4.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop &&
            (!instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
             instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp)) {
            it++;
            continue;
        }

        if (instruction4.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
            (!instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
            !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0)) {
            it++;
            continue;
        }

        printf("Found unoptimized stack pointer arithmetic(xchg) : %d\n", instruction1.getCount());

        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::ReadWrite);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = zasm::x86::rsp;
        ops[1] = zasm::Imm(8);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Sub, 2,
            ops, opAccess, opVisibility, {}, {});


        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(Operand(zasm::x86::rsp));
        newInstruction.addOperand(Operand(zasm::Imm(8)));

        newInstruction.setCount(countGlobal++);

        instructions.insert(it4, newInstruction);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());



        instructions.erase(it4);
        instructions.erase(it3);
        instructions.erase(it2);
        it = instructions.erase(it);

        isExtraPass = true;
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
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction1 = *it;

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
        auto& instruction2 = *it2;

        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>() != zasm::x86::rsp ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }

        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }


        auto& instruction3 = *it3;


        if ((instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
            instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
            !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
            !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction3.getZasmInstruction().getOperand(0) != op1) {
            it++;
            continue;
        }

        auto it4 = nextIter(it3, instructions.end());

        if (it4 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction4 = *it4;

        if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xchg ||
            !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
            !instruction4.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(1).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }

        auto it5 = nextIter(it4, instructions.end());

        if (it5 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction5 = *it5;

        if (instruction5.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop &&
            instruction5.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov) {
            it++;
            continue;
        }

        if (instruction5.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop &&
            (!instruction5.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
                instruction5.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp)) {
            it++;
            continue;
        }

        if (instruction5.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov &&
            (!instruction5.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
                instruction5.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
                !instruction5.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
                instruction5.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
                instruction5.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
                instruction5.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0)) {
            it++;
            continue;
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

        newInstruction.addOperand(Operand(zasm::x86::rsp));
        newInstruction.addOperand(Operand(zasm::Imm(8)));

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

        isExtraPass = true;
    }

    /*
     push r13 --
     mov r13, rdx --
     mov qword ptr ss:[rsp+0x08], r13 --
     pop r13 -- BAD PATTERN
    */

    
    for (auto it = instructions.begin(); it != instructions.end();) {

        auto& instruction1 = *it;

        if (instruction1.getCount() == 1065)
            printf("");

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
        auto& instruction2 = *it2;

        if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }

        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction3 = *it3;

        if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
            !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() < 8 ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
            !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
            instruction3.getZasmInstruction().getOperand(1).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }


        auto it4 = nextIter(it3, instructions.end());

        if (it4 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction4 = *it4;

        if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ||
            !instruction4.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction4.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }


        printf("Found push mov mov pop to mov mem: %s count: %d\n",
            formatInstruction(instruction1.getZasmInstruction()).c_str(),
            instruction1.getCount());

        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        auto disp = instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement();

        instruction3.getZasmInstruction().getOperand(0).get<zasm::Mem>().setDisplacement(disp - 8);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction3.getZasmInstruction().getOperand(0);
        ops[1] = instruction2.getZasmInstruction().getOperand(1);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction3.getOperand(0));
        newInstruction.addOperand(instruction2.getOperand(1));

        newInstruction.setCount(countGlobal++);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());
        
        instructions.insert(it4, newInstruction);

        instructions.erase(it4);
        instructions.erase(it3);
        instructions.erase(it2);
        it = instructions.erase(it);

        isExtraPass = true;
    }


    //xor r9,r9
    for (auto it = instructions.begin(); it != instructions.end();) {
        auto& instruction = *it;

        if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor ||
            instruction.getZasmInstruction().getOperand(0) != instruction.getZasmInstruction().getOperand(1)) {
            it++;
            continue;
        }

        printf("Found xor the same register at count : %d\n", instruction.getCount());


        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction.getZasmInstruction().getOperand(0);
        ops[1] = zasm::Imm(0);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction.getOperand(0));
        newInstruction.addOperand(Operand(zasm::Imm(0)));

        newInstruction.setCount(countGlobal++);

        instructions.insert(it, newInstruction);

        it = instructions.erase(it);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());
    }

    /*
    push r13 --
    mov qword ptr ss:[rsp+0x08], 0x00000038 --
    pop r13 -- PATTERN CHANGE WHEN OTHER PASSES GO
    */

    for (auto it = instructions.begin(); it != instructions.end();) {

        auto& instruction1 = *it;

        if (instruction1.getCount() == 1129) {
            printf("");
        }

        if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
            !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
            it++;
            continue;
        }

        auto it2 = nextIter(it, instructions.end());

        if (it2 == instructions.end()) {
            it++;
            continue;
        }

        auto& op1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>();
        auto& instruction2 = *it2;

        if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov &&
            instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor) ||
            !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() < 8 ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
            instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ) {
            it++;
            continue;
        }


        auto it3 = nextIter(it2, instructions.end());

        if (it3 == instructions.end()) {
            it++;
            continue;
        }

        auto& instruction3 = *it3;

        if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ||
            !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
            instruction3.getZasmInstruction().getOperand(0).get<zasm::Reg>() != op1) {
            it++;
            continue;
        }


        printf("Found push mov mov pop to mov mem: %s count: %d\n",
            formatInstruction(instruction1.getZasmInstruction()).c_str(),
            instruction1.getCount());

        zasm::InstructionDetail::OperandsAccess opAccess;
        zasm::InstructionDetail::OperandsVisibility opVisibility;

        opAccess.set(0, zasm::Operand::Access::Write);
        opAccess.set(1, zasm::Operand::Access::Read);

        opVisibility.set(0, zasm::Operand::Visibility::Explicit);
        opVisibility.set(1, zasm::Operand::Visibility::Explicit);

        auto disp = instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement();

        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().setDisplacement(disp - 8);

        std::array<zasm::Operand, 10>ops;
        ops[0] = instruction2.getZasmInstruction().getOperand(0);
        ops[1] = instruction2.getZasmInstruction().getOperand(1);

        auto newZasmInstruction = zasm::InstructionDetail({},
            zasm::x86::Mnemonic::Mov, 2,
            ops, opAccess, opVisibility, {}, {});

        Instruction newInstruction;
        newInstruction.setZasmInstruction(newZasmInstruction);

        newInstruction.addOperand(instruction2.getOperand(0));
        newInstruction.addOperand(instruction2.getOperand(1));

        newInstruction.setCount(countGlobal++);

        printf("Generated new instruction: %s count: %d\n",
            formatInstruction(newInstruction.getZasmInstruction()).c_str(),
            newInstruction.getCount());

        instructions.insert(it3, newInstruction);

        instructions.erase(it3);
        instructions.erase(it2);
        it = instructions.erase(it);

        isExtraPass = true;
    }


    return isExtraPass;
}
