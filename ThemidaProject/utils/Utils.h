#pragma once
#include <Windows.h>

#include <string>
#include <vector>

#include <zasm/zasm.hpp>

class Instruction;
enum class OperandAction;

#define LOGGING 1;
bool ReadFile(const std::string& path, std::vector<BYTE>& bin);
/*

void PrintInstruction(const zasm::InstructionDetail& instruction,uintptr_t address);

uintptr_t CalcMemAddress(const zasm::Mem& mem);
*/

BYTE GetSignBit(uintptr_t,zasm::BitSize);

BYTE GetSecondMSB(uintptr_t valie,zasm::BitSize mm);
void SetSignBit(uintptr_t value, zasm::BitSize mm);
BYTE LSB(uintptr_t value);

uintptr_t zasmBitsToNumericSize(zasm::BitSize bs);
std::string formatInstruction(const zasm::InstructionDetail& instruction);
void printInstruction(const zasm::InstructionDetail& instruction);
std::string actionToString(const zasm::detail::OperandAccess& actionType);


OperandAction zasmActionToOwn(const zasm::detail::OperandAccess& actionType);
std::string formatBytes(const zasm::InstructionDetail& instruction, uintptr_t address);


void formateLinkedInstructions(Instruction* instruction);


void resetCounter(Instruction* head);

std::string formateLinkOneInstruction(Instruction* instruction);