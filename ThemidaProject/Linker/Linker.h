#pragma once
#include <zasm/zasm.hpp>

#include <Windows.h>


class EmulatorCPU;
class Instruction;


struct UserData {
    Instruction* head;
    Instruction* tail;
    Instruction* instructions;

    UserData() : head(nullptr), tail(nullptr), instructions(nullptr) {}
};
bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction,
    void* user_data);

Instruction* zasmToInstruction(const zasm::InstructionDetail& instruction);