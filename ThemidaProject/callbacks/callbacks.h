#pragma once
#include <zasm/zasm.hpp>

#include <Windows.h>


class EmulatorCPU;
class Instruction;

struct UserData {
    std::list<Instruction> instructions;
};
bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction,
    void* user_data);
