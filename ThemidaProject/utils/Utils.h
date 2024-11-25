#pragma once
#include <Windows.h>

#include <string>
#include <vector>

#include <zasm/zasm.hpp>


static inline constexpr WORD carryFlagMask = 1;
static inline constexpr WORD parityFlagMask = 4;
static inline constexpr WORD auxiliaryCarryFlagMask = 0x10;
static inline constexpr WORD zeroFlagMask = 0x40;
static inline constexpr WORD signFlagMask = 0x80;
static inline constexpr WORD overflowFlagMask = 0x800;

static inline constexpr std::array<WORD, 6> flagMasks = {
    1,    // Carry Flag Mask
    4,    // Parity Flag Mask
    0x10, // Auxiliary Carry Flag Mask
    0x40, // Zero Flag Mask
    0x80, // Sign Flag Mask
    0x800 // Overflow Flag Mask
};


class Instruction;
enum class OperandAction;

#define LOGGING 1;
bool ReadFile(const std::string& path, std::vector<BYTE>& bin);

BYTE GetSignBit(uintptr_t,zasm::BitSize);

BYTE GetSecondMSB(uintptr_t valie,zasm::BitSize mm);
void SetSignBit(uintptr_t value, zasm::BitSize mm);
BYTE LSB(uintptr_t value);

uintptr_t zasmBitsToNumericSize(zasm::BitSize bs);
std::string formatInstruction(const zasm::InstructionDetail& instruction);
void printInstruction(const zasm::InstructionDetail& instruction);
std::string formatBytes(const zasm::InstructionDetail& instruction, uintptr_t address);
OperandAction zasmActionToOwn(const zasm::detail::OperandAccess& actionType);
std::string myActionToString(const OperandAction action);

std::string formatInstruction(Instruction* instruction);
std::string formatFlagMask(WORD mask);