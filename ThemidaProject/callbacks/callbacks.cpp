#include <format>


#include "../emulator/emu.h"
#include "callbacks.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"
#include "../Instruction/Instruction.h"

bool traceCallback(EmulatorCPU* cpu, uintptr_t address, zasm::InstructionDetail instruction_,
    void* user_data) {

    if (address == EmulatorCPU::baseImage + 0x114c501)      
        printf("");

    BasicBlock* foundBasicBlock = FindAddressBasicBlock(globals::bb, address);

    std::string toLog = std::format("Trying to emulate instruction at rva:0x{:x} count : {:d} | {} \n",
        address - EmulatorCPU::baseImage, countGlobal, formatInstruction(instruction_));
    logger->log(toLog);


    if (foundBasicBlock) {
        printf("Intersection BB found at address : 0x%llx\n",address);
        reasonStop = ReasonStop::VISITED;
        cpu->stop_emu_before();
        return false;
    }
    BasicBlock* bb = static_cast<BasicBlock*>(user_data);
    Instruction instruction = zasmToInstruction(instruction_);
    instruction.setCount(countGlobal);
    instruction.setAddress(address);
    instruction.setRegisterValues(cpu->getRegistersValues());

    auto& operands = instruction.getOperands();



    for (int i = 0; i < operands.size(); i++) {
        auto& op = operands[i];

        MemoryOperand* memOperand = dynamic_cast<MemoryOperand*>(op);

        if (memOperand) {
            uintptr_t memoryAddress = cpu->CalcEffectiveMemAddress(memOperand->getZasmOperand(), i);
            memOperand->setMemoryAddress(memoryAddress);
        }

        op->setOperandAccess(instruction_.getOperandAccess(i));
    }


    bb->instructions.push_back(instruction);

    countGlobal++;

    if (instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Call &&
        instruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>()) {
        reasonStop = ReasonStop::UNCOND_TRANSFER;
        cpu->stop_emu_before();
    }
    else if (instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Jmp ||
        instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Call ||
        instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Ret){

        printf("Found jmp/call/ret instruction at eip : 0x%llx\n", cpu->getEip());
        reasonStop = ReasonStop::UNCOND_TRANSFER;
        cpu->stop_emu_after();
    }
    else if (cpu->getEip() < globals::sectionBase || cpu->getEip() > globals::sectionBase + globals::sectionSize) {
        printf("Found exit from obfuscation section at eip : 0x%llx\n", cpu->getEip());
        reasonStop = ReasonStop::UNCOND_TRANSFER;
        cpu->stop_emu_before();
    }
    else if (instruction.getZasmInstruction().getMnemonic() >= zasm::x86::Mnemonic::Jb &&
        instruction.getZasmInstruction().getMnemonic() <= zasm::x86::Mnemonic::Jz &&
        instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Jmp) {
        printf("Found jcc jump at eip: 0x%llx\n", cpu->getEip());
        reasonStop = ReasonStop::JCC;
        cpu->stop_emu_before();
    }

    return true;
}