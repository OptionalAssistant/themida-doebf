#include "pch.h"

#include "deobf.h"
#include "../emulator/emu.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"
#include "../callbacks/callbacks.h"


BasicBlock* deobf::handleBBIntersection(uintptr_t address)
{
	BasicBlock* foundBasicBlock = FindAddressBasicBlock(globals::bb, address);

	if (!foundBasicBlock)
		return nullptr;

	auto& instructions = foundBasicBlock->instructions;

	std::list<Instruction>::iterator foundIt;
	for (auto it = instructions.begin(); it != instructions.end();it++) {
		auto& instruction = *it;
		
		if (instruction.getAddress() == address){
			foundIt = it;
			break;
		}
	}

	if (foundIt == instructions.begin())
		return foundBasicBlock;
	else
		throw std::runtime_error("It means that ");

	return foundBasicBlock;
}

void deobf::transverseBlock(uintptr_t rva,BasicBlock* bb)
{
	m_cpu->addCallback(traceCallback, bb);
	m_cpu->run(rva);
	m_cpu->removeCallback(traceCallback);

	if (reasonStop == ReasonStop::VISITED) {
	   printf("Visited address 0x%llx\n", m_cpu->getEip());
	   BasicBlock* labelBB = handleBBIntersection(m_cpu->getEip());
	   if (!labelBB)
		   throw std::runtime_error("What ??? Fatal error");

	   bb->pass1= labelBB;
	   return;
	}
	else if (reasonStop == ReasonStop::UNCOND_TRANSFER) {

		uintptr_t newEip = m_cpu->getEip();
		uintptr_t rva = newEip - EmulatorCPU::baseImage;

		auto& lastInstruction = bb->instructions.back();
		if (lastInstruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Call &&
			lastInstruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>()) {
			newEip = m_cpu->getEip() + lastInstruction.getZasmInstruction().getLength();
		}
		else if (newEip < globals::sectionBase || newEip > globals::sectionBase + globals::sectionSize) {
			if (lastInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Call) {
				printf("End at address : 0x%llx\n", lastInstruction.getAddress());
				return;
			}
		  uintptr_t rsp = m_cpu->reg_read(zasm::x86::rsp);
		  uintptr_t returnAddress;
		  m_cpu->mem_read(rsp, &returnAddress, sizeof(uintptr_t));
		  newEip = returnAddress;
		}
		BasicBlock* foundBB = handleBBIntersection(newEip);

		if (foundBB) {
			bb->pass1 = foundBB;
			return;
		}
		bb->pass1 = new BasicBlock();
		transverseBlock(newEip, bb->pass1);
		return;
	}
	else if (reasonStop == ReasonStop::JCC) {
		
		auto& instruction = bb->instructions.back();

		uintptr_t saveEip = m_cpu->getEip();

		uintptr_t newEip = saveEip + instruction.getZasmInstruction().getLength();
		
		BasicBlock* foundBB = handleBBIntersection(newEip);

		if (foundBB) {
			bb->pass1 = foundBB;
		}
		else {
			bb->pass1 = new BasicBlock();
			transverseBlock(newEip, bb->pass1);
		}
		newEip = saveEip + instruction.getZasmInstruction().getOperand(0).get<zasm::Imm>().value<uintptr_t>();

		BasicBlock* foundBB2 = handleBBIntersection(newEip);

		if (foundBB2) {
			bb->pass2 = foundBB2;
		}
		else {
			bb->pass2 = new BasicBlock();
			transverseBlock(newEip, bb->pass2);
		}
	}
}

void deobf::captureTrace(uintptr_t rva,BasicBlock* bb)
{
	transverseBlock(rva,bb);
}



void deobf::run(uintptr_t rva)
{
	globals::bb = new BasicBlock();

	captureTrace(rva,globals::bb);

    optimizer->run(globals::bb);

	logger->log("After\n");

	printOutInstructions(globals::bb);
}
