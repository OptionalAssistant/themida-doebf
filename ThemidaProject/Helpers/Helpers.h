#pragma once
#include <list>
#include <zasm/zasm.hpp>
#include "../Instruction/Instruction.h"

std::list<Instruction>::iterator findReadAccessRegister(std::list<Instruction>& list,
	std::list<Instruction>::iterator start,
	zasm::Reg& searchedRegister);