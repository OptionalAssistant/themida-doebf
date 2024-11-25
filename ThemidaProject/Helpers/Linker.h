#pragma once
#include <zasm/zasm.hpp>

class Instruction;
class MemoryOperand;
class RegisterOperand;
class FlagsOperand;

Instruction* zasmToInstruction(const zasm::InstructionDetail& instruction);

void createMemoryByteUnits(MemoryOperand* memoryOp);

void createRegisterByteUnits(RegisterOperand* registerOp);

void createFlagsBitUnits(FlagsOperand* flagsOp);

void createOperandUnits(Instruction* instruction);