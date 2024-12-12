#pragma once
#include <zasm/zasm.hpp>
#include <vector>
#include <list>

class Instruction; 

template <typename Iter>
Iter nextIter(Iter it, Iter end) {
    return (std::next(it) != end) ? std::next(it) : end;
}


int64_t calculateSubAdd(std::vector<zasm::InstructionDetail > instruction);


std::list<Instruction>::iterator getNextRegisterAccess(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
     zasm::x86::Reg& foundReg);

std::list<Instruction>::iterator getNextRegisterWrite(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
    zasm::x86::Reg& foundReg);

std::list<Instruction>::iterator getNextRegisterRead(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
    zasm::x86::Reg& foundReg);

std::list<Instruction>::iterator getNextRegisterReadWrite(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
    zasm::x86::Reg& foundReg);

std::list<Instruction>::iterator getNextRegisterReadWriteOrWrite(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
    zasm::x86::Reg& foundReg);


std::list<Instruction>::iterator getNextMemoryWrite(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd, uintptr_t address);

std::list<Instruction>::iterator getPrevMemoryWrite(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd,
 uintptr_t address);

std::list<Instruction>::iterator getNextMemoryRead(std::list<Instruction>::iterator itStart,
    std::list<Instruction>::iterator itEnd, uintptr_t address);

bool isSameRegister( zasm::x86::Reg& reg1, zasm::x86::Reg& reg2);