#pragma once

class EmulatorCPU;
class Instruction;
struct BasicBlock;

bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail& instruction, void* user_data);
