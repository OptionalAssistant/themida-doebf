#include <stdexcept>

#include "HelpersIterations.h"
#include "../../Instruction/Instruction.h"


int64_t calculateSubAdd(std::vector<zasm::InstructionDetail > instructions) {
    int64_t result = 0;

    for (auto& instruction : instructions) {
        if (instruction.getMnemonic() == zasm::x86::Mnemonic::Sub)
            result -= instruction.getOperand(1).get<zasm::Imm>().value<uintptr_t>();
        else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Add)
            result += instruction.getOperand(1).get<zasm::Imm>().value<uintptr_t>();
        else
            throw std::runtime_error("Error during calculateAddSub.Mnemonic is not add or sub");
    }

    return result;
}

std::list<Instruction>::iterator getNextRegisterAccess(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
     zasm::x86::Reg& foundReg)
{
    for (; itStart != itEnd; itStart++) {
        auto& instruction = *itStart;

        for (const auto& op : instruction.getZasmInstruction().getOperands()) {
            if (op.holds<zasm::Reg>()) {
                auto reg = op.get<zasm::Reg>();
                if (isSameRegister(reg, foundReg)) {
                    return itStart;
                }
            }
            else if (op.holds<zasm::Mem>()) {
                auto baseReg = op.get<zasm::Mem>().getBase();
                if (isSameRegister(baseReg, foundReg)) {
                    return itStart;
                }
                auto indexReg = op.get<zasm::Mem>().getIndex();
                if (isSameRegister(indexReg, foundReg)) {
                    return itStart;
                }
            }
        }
    }

    return itEnd;
}

std::list<Instruction>::iterator getNextRegisterRead(std::list<Instruction>::iterator itStart, std::list<Instruction>::iterator itEnd, zasm::x86::Reg& foundReg)
{
    for (; itStart != itEnd;) {
        auto foundIt  = getNextRegisterAccess(itStart, itEnd, foundReg);
        if (foundIt == itEnd)
            return itEnd;

        auto& instruction = *foundIt;

   
        for (int i = 0; i < instruction.getZasmInstruction().getOperandCount();i++) {
            auto op = instruction.getZasmInstruction().getOperand(i);

            if (op.holds<zasm::Reg>() && isSameRegister(op.get<zasm::Reg>(), foundReg)) {
                if (instruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::Read)
                    return foundIt;
            }
            else if (op.holds<zasm::Mem>()) {
                auto baseReg = op.get<zasm::Mem>().getBase();
                if (isSameRegister(baseReg, foundReg)) {
                    return foundIt;
                }
                auto indexReg = op.get<zasm::Mem>().getIndex();
                if (isSameRegister(indexReg, foundReg)) {
                    return foundIt;
                }
            }
        }

        itStart = std::next(foundIt);
    }

    return itEnd;
}

std::list<Instruction>::iterator getNextRegisterWrite(std::list<Instruction>::iterator itStart, std::list<Instruction>::iterator itEnd, zasm::x86::Reg& foundReg)
{
    for (; itStart != itEnd;) {
        auto foundIt = getNextRegisterAccess(itStart, itEnd, foundReg);
        if (foundIt == itEnd)
            return itEnd;

        auto& instruction = *foundIt;

        for (int i = 0; i < instruction.getZasmInstruction().getOperandCount(); i++) {
            auto op = instruction.getZasmInstruction().getOperand(i);

            if (op.holds<zasm::Reg>() && isSameRegister(op.get<zasm::Reg>(), foundReg)) {
                if (instruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::Write)
                    return foundIt;
            }
        }

        itStart = std::next(foundIt);
    }

    return itEnd;
}

std::list<Instruction>::iterator getNextRegisterReadWrite(std::list<Instruction>::iterator itStart, std::list<Instruction>::iterator itEnd, zasm::x86::Reg& foundReg)
{
    for (; itStart != itEnd;) {
        auto foundIt = getNextRegisterAccess(itStart, itEnd, foundReg);
        if (foundIt == itEnd)
            return itEnd;

        auto& instruction = *foundIt;

        for (int i = 0; i < instruction.getZasmInstruction().getOperandCount(); i++) {
            auto op = instruction.getZasmInstruction().getOperand(i);

            if (op.holds<zasm::Reg>() && isSameRegister(op.get<zasm::Reg>(),foundReg)) {
                if (instruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::ReadWrite)
                    return foundIt;
            }
        }

        itStart = std::next(foundIt);
    }

    return itEnd;
}

std::list<Instruction>::iterator getNextRegisterReadWriteOrWrite(std::list<Instruction>::iterator itStart, std::list<Instruction>::iterator itEnd, zasm::x86::Reg& foundReg)
{
    for (; itStart != itEnd; itStart++) {
        auto foundIt = getNextRegisterAccess(itStart, itEnd, foundReg);
        if (foundIt == itEnd)
            return itEnd;

        auto& instruction = *foundIt;

        for (int i = 0; i < instruction.getZasmInstruction().getOperandCount(); i++) {
            auto op = instruction.getZasmInstruction().getOperand(i);

            if (op.holds<zasm::Reg>() && isSameRegister(op.get<zasm::Reg>(), foundReg)) {
                if (instruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::ReadWrite
                    || instruction.getZasmInstruction().getOperandAccess(i) == zasm::detail::OperandAccess::Write)
                    return foundIt;
            }
        }

    }

    return itEnd;
}

bool isSameRegister( zasm::x86::Reg& reg1,  zasm::x86::Reg& reg2)
{

    if (!reg1.isGp() || !reg2.isGp())
        return false;

    auto& gpRegisterCompared = reg1.as<zasm::x86::Gp>();

    auto& gpRegister = reg2.as<zasm::x86::Gp>();

    return gpRegisterCompared == gpRegister.r8() ||
        gpRegisterCompared == gpRegister.r8hi() ||
        gpRegisterCompared == gpRegister.r8lo() ||
        gpRegisterCompared == gpRegister.r16() ||
        gpRegisterCompared == gpRegister.r32() ||
        gpRegisterCompared == gpRegister.r64();
}

