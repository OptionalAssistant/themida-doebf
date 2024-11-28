#include <Windows.h>
#include <string>
#include <vector>

#include "Linker.h"
#include "../utils/Utils.h"

#include "../Instruction/Instruction.h"

#include "../Operands/MemoryOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/ConstantOperand.h"
#include "../Operands/FlagsOperand.h"

#include "../ByteOperand/RegisterByte.h"
#include "../ByteOperand/MemoryByte.h"
#include "../ByteOperand/FlagBit.h"

#include "../utils/Utils.h"

#include <zasm/zasm.hpp>
#include <stdexcept>


FlagsOperand* createFlagsOperand(const zasm::Reg& regOperand, OperandAction op_action, uintptr_t index) {
    return new FlagsOperand(regOperand, index, op_action);
}

ConstantOperand* createConstantOperand(const zasm::Imm& immOp, uintptr_t index) {
    return new ConstantOperand(immOp, index);
}

RegisterOperand* createRegisterOperand(const zasm::Reg& regOperand, OperandAction op_action, uintptr_t index) {
    return new RegisterOperand(regOperand, index, op_action);
}

MemoryOperand* createMemoryOperand(const zasm::Mem& memoryOp, OperandAction op_action, uintptr_t index) {
    return new MemoryOperand(createRegisterOperand(memoryOp.getBase(), OperandAction::READ, index),
        createRegisterOperand(memoryOp.getIndex(), OperandAction::READ, index),
        createConstantOperand(memoryOp.getDisplacement(), index),
        createConstantOperand(memoryOp.getScale(), index), op_action, memoryOp, index);
}
void createMemoryByteUnits(MemoryOperand* memoryOp) {

    createRegisterByteUnits(memoryOp->getBase());
    createRegisterByteUnits(memoryOp->getIndex());

    const uintptr_t memoryAddress = memoryOp->getMemoryAddress();

    const int32_t opSize = memoryOp->getZasmOperand().get<zasm::Mem>().getByteSize();

    auto& memoryBytes = memoryOp->getOperandUnits();

    for (int i = 0; i < opSize; i++) {
        memoryBytes.push_back(new MemoryByte(memoryAddress + i, memoryOp));
    }
}

void createRegisterByteUnits(RegisterOperand* registerOp) {

    if (registerOp->getZasmOperand().get<zasm::Reg>().getId() == zasm::Reg::Id::None)
        return;

   const zasm::Reg registerValue = registerOp->getZasmOperand().get<zasm::Reg>();

    auto& registerBytes = registerOp->getOperandUnits();

    if (registerValue.isGp8Lo()) {
        registerBytes.push_back(new RegisterByte(0, registerOp));
        return;
    }
    if (registerValue.isGp8Hi()) {
        registerBytes.push_back(new RegisterByte(1, registerOp));
        return;
    }

    if (registerValue.isGp16()) {
        for (int i = 0; i < 2; i++) {
            registerBytes.push_back(new RegisterByte(i, registerOp));
        }
        return;
    }

    if (registerValue.isGp64() || registerValue.isGp32()) {
        for (int i = 0; i < 8; i++) {
            registerBytes.push_back(new RegisterByte(i, registerOp));
        }
        return;
    }
}

void createFlagsBitUnits(FlagsOperand* flagsOp) {
    const auto& cpuFlags = flagsOp->getParent()->getZasmInstruction().getCPUFlags();

    auto& flagBits = flagsOp->getOperandUnits();

    if (flagsOp->getParent()->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pushfq) {
        for (auto& flag : flagMasks) {
            flagBits.push_back(new FlagBit(flag, FlagAction::READ,
                flagsOp));
        }
        return;
    }

    if (flagsOp->getParent()->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Popfq) {
        for (auto& flag : flagMasks) {
            flagBits.push_back(new FlagBit(flag, FlagAction::WRITE,
                flagsOp));
        }
        return;
    }

    for (auto& flag : flagMasks) {
        if (cpuFlags.set0.value() & flag ||
            cpuFlags.set1.value() & flag ||
            cpuFlags.modified.value() & flag ||
            cpuFlags.undefined.value() & flag) {
            flagBits.push_back(new FlagBit(flag, FlagAction::WRITE,
                flagsOp));
        }
        else if (cpuFlags.tested.value() & flag) {
            flagBits.push_back(new FlagBit(flag, FlagAction::READ, flagsOp));
        }
    }

}
void createOperandUnits(Instruction* instruction) {
    
    for (auto& op : instruction->getOperands()) {
        MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(op);

        if (memoryOperand) {
            createMemoryByteUnits(memoryOperand);
            continue;
        }

        RegisterOperand* registerOperand = dynamic_cast<RegisterOperand*>(op);

        if (registerOperand) {
            createRegisterByteUnits(registerOperand);
            continue;
        }

        FlagsOperand* flagsOperand = dynamic_cast<FlagsOperand*>(op);

        if (flagsOperand) {
            createFlagsBitUnits(flagsOperand);
            continue;
        }

    }
}

Instruction* zasmToInstruction(const zasm::InstructionDetail& instruction_)
{
    Instruction* instruction = new Instruction(instruction_);

    for (int i = 0; i < instruction_.getOperandCount(); i++) {
        if (instruction_.getOperand(i).holds<zasm::Mem>()) {
            MemoryOperand* memoryOperand = createMemoryOperand(instruction_.getOperand(i).get<zasm::Mem>(),
                zasmActionToOwn(instruction_.getOperandAccess(i)), i);
            instruction->addOperand(memoryOperand);
            memoryOperand->getBase()->setParent(instruction);
            memoryOperand->getIndex()->setParent(instruction);
        }
        else if (instruction_.getOperand(i).holds<zasm::Reg>() && 
            instruction_.getOperand(i).get<zasm::Reg>() != zasm::x86::rip) {
            if (instruction_.getOperand(i).get<zasm::Reg>().isGp()) {
                instruction->addOperand(createRegisterOperand(instruction_.getOperand(i).get<zasm::Reg>(),
                    zasmActionToOwn(instruction_.getOperandAccess(i)), i));
            }
            else if (instruction_.getOperand(i).get<zasm::Reg>() == zasm::x86::rflags) {
                instruction->addOperand(createFlagsOperand(instruction_.getOperand(i).get<zasm::Reg>(),
                    zasmActionToOwn(instruction_.getOperandAccess(i)), i));
            }
            else
                throw std::runtime_error("Error during zasmToInstruction");
        }
        else if (instruction_.getOperand(i).holds<zasm::Imm>()) {
            instruction->addOperand(createConstantOperand(instruction_.getOperand(i).get<zasm::Imm>(), i));
        }

    }

    return instruction;
}
