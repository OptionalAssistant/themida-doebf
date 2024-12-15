#include <cmath>

#include "SimplifyConstantFolding.h"
#include "../Instruction/Instruction.h"
#include "./Helpers/HelpersIterations.h"
#include "../utils/Utils.h"
#include "../emulator/emu.h"
#include "../utils/Logger.h"

//Read constant folding
static bool foldConstantRead(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    bool isChanged = false;
    
    auto& instruction1 = *it;

    auto& zydis_instr = instruction1.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
        !zydis_instr.getOperand(1).holds<zasm::Imm>()) {
        return false;
    }

   // printf("Found mov reg,const at count: %d\n", instruction1.getCount());

    auto nextWrite = getNextRegisterReadWriteOrWrite(std::next(it), instructions.end(),
        zydis_instr.getOperand(0).get<zasm::Reg>());

    if (nextWrite == instructions.end()) {
        return false;
    }

    auto& instruction2 = *nextWrite;

    printf("Found next readWrite or Write at count: %d\n", instruction2.getCount());


    for (auto itStart = it; it != nextWrite;) {
        auto itRead = getNextRegisterRead(itStart, nextWrite, zydis_instr.getOperand(0).get<zasm::Reg>());

        if (itRead == nextWrite)
            break;

        auto& readInstruction = *itRead;

        printf("Found read (Constant actually) at count: %d\n", readInstruction.getCount());;

        if (readInstruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Push) {
            itStart = std::next(itRead);
            continue;
        }
        for (int i = 0; i < readInstruction.getZasmInstruction().getOperandCount(); i++) {
            auto& op = readInstruction.getZasmInstruction().getOperand(i);
            zasm::Imm immValue = zydis_instr.getOperand(1).get<zasm::Imm>().value<uintptr_t>();

            if (op.holds<zasm::Reg>() &&
                isSameRegister(op.get<zasm::Reg>(), zydis_instr.getOperand(0).get<zasm::Reg>())) {
                if (readInstruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::ReadCondWrite) {
                    continue;
                }
           
                if (readInstruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::Read)
                {
                    readInstruction.setOperand(i, new BaseOperand(immValue));
                    readInstruction.getZasmInstruction().setOperand(i, immValue);
                    isChanged = true;
                    break;
                }
            }
            else if (op.holds<zasm::Mem>()) {
                auto baseReg = op.get<zasm::Mem>().getBase();
                if (isSameRegister(baseReg, zydis_instr.getOperand(0).get<zasm::Reg>())) {
                    op.get<zasm::Mem>().setBase(zasm::Reg(zasm::Reg::Id::None));
                    isChanged = true;
                }
           /*     auto indexReg = op.get<zasm::Mem>().getIndex();
                if (isSameRegister(indexReg, zydis_instr.getOperand(0).get<zasm::Reg>())) {
                    op.get<zasm::Mem>().setIndex(zasm::Reg(zasm::Reg::Id::None));
                    isChanged = true;
                }*/
            }
        }
        itStart = std::next(itRead);
    }

    return isChanged;
}
//Sub reg,8 Add reg,16
static bool foldSubAddConstant(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto instruction1 = *it;

    if ((instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
        (instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>() &&
        instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>() == zasm::x86::rsp )||
        !instruction1.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
        return false;
    }

    auto& op1 = instruction1.getZasmInstruction().getOperand(0);
    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto instruction2 = *it2;

    if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub) ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
        return false;
    }

    if (op1.holds<zasm::Reg>() && instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        auto& gpRegister = instruction2.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();

        auto& gpRegisterCompared = op1.get<zasm::Reg>().as<zasm::x86::Gp>();

        if (gpRegisterCompared != gpRegister.r32() &&
            gpRegisterCompared != gpRegister.r64()) {
            return false;
        }
    }

    if ((!op1.holds<zasm::Reg>()  || !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>())&&
        op1 != instruction2.getZasmInstruction().getOperand(0)) {
        return false;
    }

   // printf("Found unfold sub-add at count: %d\n", instruction1.getCount());


    int64_t result = calculateSubAdd({ instruction1.getZasmInstruction(),instruction2.getZasmInstruction() });

    //printf("Result folding : %lld\n", result);

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
    newInstruction.addOperand(new BaseOperand(zasm::Imm(immediateValue)));

    newInstruction.setCount(countGlobal++);

    instructions.insert(it2, newInstruction);

   /* printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());*/

    instructions.erase(it2);
    it = instructions.erase(it);

    return true;
}

//ReadWrite constant folding
static bool ReadWriteConstantFolding(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

    auto& zydis_instr = instruction1.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !zydis_instr.getOperand(0).holds<zasm::Reg>() ||
        !zydis_instr.getOperand(1).holds<zasm::Imm>()) {
        return false;
    }

    auto nextAccess = getNextRegisterAccess(std::next(it), instructions.end(),
        zydis_instr.getOperand(0).get<zasm::Reg>());

    if (nextAccess == instructions.end()) {
        return false;
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
        return false;
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
        return false;
    }

    if ((instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Sub ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Add ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::And ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Xor ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Shl ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Shr ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Or) &&
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>()) {
        return false;
    }

    auto it3 = nextIter(nextAccess, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    //printf("Found constant folding optimization at count: %d\n", instruction1.getCount());

    auto& instruction3 = *it3;

    auto& registerArray = instruction3.getRegistersArray();

    uintptr_t value = reg_read_(registerArray, instruction1.getZasmInstruction()
        .getOperand(0).get<zasm::Reg>(),
        instruction1.getRflags());



    auto& rFlags = instruction3.getRflags();

    auto nextIterator = std::next(it);
    auto& nextInstruction = *nextIterator;

    auto& registerArrayInitial = nextInstruction.getRegistersArray();

    reg_write_(registerArrayInitial, instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>(),
        value, rFlags);

    instruction1.setOperand(1, new BaseOperand(zasm::Imm(value)));
    instruction1.getZasmInstruction().setOperand(1, zasm::Imm(value));


    /*printf("Updated mov reg,const  instruction: %s count: %d\n",
        formatInstruction(instruction1.getZasmInstruction()).c_str(),
        instruction1.getCount());*/

    instructions.erase(nextAccess);

    return true;
}

//Constant folding  push const
static bool OptimizePass1(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    
    bool isChanged = false;

    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction.getZasmInstruction().getOperand(0).holds<zasm::Imm>())
        return false;

    MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(instruction.getOperand(2));

    auto itNextMemoryWrite = getNextMemoryWrite(std::next(it), 
        instructions.end(), memoryOperand->getMemoryAddress());

    if (itNextMemoryWrite == instructions.end())
        return false;


    for (auto itStart = it; itStart != itNextMemoryWrite;) {

        auto itRead = getNextMemoryRead(itStart, itNextMemoryWrite, memoryOperand->getMemoryAddress());

        if (itRead == itNextMemoryWrite)
            break;
        auto& readInstruction = *itRead;

        if (readInstruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Mov) {
            
            readInstruction.getZasmInstruction().setOperand(1,
                instruction.getZasmInstruction().getOperand(0));
            readInstruction.setOperand(1,instruction.getOperand(0));
            
            //printf("Found push constant at count : %d\n", instruction.getCount());
           // printf("Found read this constant at count : %d\n", readInstruction.getCount());
            


            isChanged = true;
        }

        itStart = std::next(itRead);
    }

    return isChanged;
}
static bool OptimizePass2(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;


    if ((instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub &&
        instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add) ||
        !instruction1.getZasmInstruction().getOperand(1).holds<zasm::Imm>())
        return false;

    auto it2 = nextIter(it, instructions.begin());

    if (it2 == instructions.end())
        return false;

    auto& instruction2 = *it2;

    if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add) ||
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>())
        return false;

    auto it3 = nextIter(it2, instructions.begin());

    if (it3 == instructions.end())
        return false;

    auto& instruction3 = *it3;

    if ((instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub &&
        instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add) ||
        !instruction3.getZasmInstruction().getOperand(1).holds<zasm::Imm>())
        return false;

    if (instruction1.getZasmInstruction().getMnemonic() == instruction3.getZasmInstruction().getMnemonic() ||
        instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>() != 
        instruction3.getZasmInstruction().getOperand(1).get<zasm::Imm>())
         return false;

    if (instruction1.getZasmInstruction().getOperand(0) != instruction2.getZasmInstruction().getOperand(0) ||
        instruction1.getZasmInstruction().getOperand(0) != instruction3.getZasmInstruction().getOperand(0) ||
        instruction2.getZasmInstruction().getOperand(0) != instruction1.getZasmInstruction().getOperand(0) ||
        instruction2.getZasmInstruction().getOperand(0) != instruction3.getZasmInstruction().getOperand(0) ||
        instruction3.getZasmInstruction().getOperand(0) != instruction1.getZasmInstruction().getOperand(0) ||
        instruction3.getZasmInstruction().getOperand(0) != instruction2.getZasmInstruction().getOperand(0))
        return false;

   // printf("Found useless add and sub with register between %d\n", instruction1.getCount());

    instructions.erase(it3);
    instructions.erase(it);

    return true;
}

static bool OptimizePass3(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        !instruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>())
        return false;

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end())
        return false;

    auto& instruction2 = *it2;


    if ((instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Xor &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Or &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Not &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Shr &&
        instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Dec) ||
        instruction.getZasmInstruction().getOperand(0) !=
        instruction2.getZasmInstruction().getOperand(0))
        return false;

    if ((instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Add ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Sub ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Xor ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Or || 
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Shr) &&
        !instruction2.getZasmInstruction().getOperand(1).holds<zasm::Imm>())
        return false;

    printf("Found memory constant folding at count: %d\n", instruction.getCount());

    uintptr_t immValue = instruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    
    if (instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Sub ||
        instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Add) {
        immValue = immValue + calculateSubAdd({ instruction2.getZasmInstruction() });
    }
    else if (instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Xor) {
        immValue = immValue ^ instruction2.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    }
    else if (instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Or) {
        immValue = immValue | instruction2.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    }
    else if (instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Not) {
        immValue = ~immValue;
    }
    else if (instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Shr) {
        immValue = immValue >> instruction2.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    }
    else if (instruction2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Dec) {
        immValue = --immValue;
    }
    instruction.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
    instruction.setOperand(1, new BaseOperand(zasm::Imm(immValue)));


    printf("Generating new instruction: %d %s \n", instruction.getCount(),
        formatInstruction(instruction.getZasmInstruction()).c_str());

    instructions.erase(it2);
    return false;
}

bool SimplifyConstantFolding::run(std::list<Instruction>::iterator it, std::list<Instruction>& instructions)
{

    if (foldConstantRead(it, instructions))
        return true;
    if (foldSubAddConstant(it, instructions))
        return true;
    if (ReadWriteConstantFolding(it, instructions))
        return true;
    if (OptimizePass1(it, instructions))
        return true;
    if (OptimizePass2(it, instructions))
        return true;
    if (OptimizePass3(it, instructions))
        return true;

    return false;
}
