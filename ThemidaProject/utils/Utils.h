#pragma once
#include <Windows.h>

#include <string>
#include <vector>
#include <variant>
#include <zasm/zasm.hpp>

#include "../emulator/emu.h"

class Instruction;
class MemoryOperand;
class Operand;
enum class OperandAction;

static inline constexpr WORD carryFlagMask = 1;
static inline constexpr WORD parityFlagMask = 4;
static inline constexpr WORD auxiliaryCarryFlagMask = 0x10;
static inline constexpr WORD zeroFlagMask = 0x40;
static inline constexpr WORD signFlagMask = 0x80;
static inline constexpr WORD overflowFlagMask = 0x800;


bool ReadFile(const std::string& path, std::vector<BYTE>& bin);

BYTE GetSignBit(uintptr_t,zasm::BitSize);

BYTE GetSecondMSB(uintptr_t valie,zasm::BitSize mm);
void SetSignBit(uintptr_t value, zasm::BitSize mm);
BYTE LSB(uintptr_t value);

uintptr_t zasmBitsToNumericSize(zasm::BitSize bs);


std::string formatInstruction(const zasm::InstructionDetail& instruction);
void printInstruction(const zasm::InstructionDetail& instruction);
std::string formatBytes(const zasm::InstructionDetail& instruction, uintptr_t address);



inline uintptr_t countGlobal = 0;

Instruction zasmToInstruction(zasm::InstructionDetail& instruction);

std::string formatInstruction_( Instruction& instruction);

EmulatorCPU::Registers zasmToEmulatorRegister(const ZydisRegister_& reg);

std::string actionToString(const zasm::detail::OperandAccess& actionType);

zasm::InstructionDetail createMov(const zasm::Operand& op1, const zasm::Operand& op2);

zasm::InstructionDetail createSub(const zasm::Operand& op1,const zasm::Operand& op2);

zasm::InstructionDetail createXchg(const zasm::Operand& op1, const zasm::Operand& op2);

zasm::InstructionDetail createPop(const zasm::Operand& op1, const zasm::Operand& op2,const zasm::Operand& op3);

zasm::InstructionDetail createPush(const zasm::Operand& op1, const zasm::Operand& op2, const zasm::Operand& op3);


void printOutInstructions(std::list<Instruction>& instructins);