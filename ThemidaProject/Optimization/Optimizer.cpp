#include "Optimizer.h"
#include "BaseOptimization.h"

#include "SimplifyConstantFolding.h"
#include "SimplifyDeadcodeElimination.h"
#include "SimplifyStackOperation.h"
#include "SimplifyMbaOperation.h"
#include "../Instruction/Instruction.h"

#include "../utils/Utils.h"

void Optimizer::runMbaPasses(BasicBlock* bb)
{
	bool isChanged = false;

	auto& instructions = bb->instructions;

	do
	{
		for (auto it = instructions.begin(); it != instructions.end();) {
			isChanged = false;
			for (auto& pass : mbaPasses) {
				isChanged = pass->run(it,instructions);
				if (isChanged) {
					it = instructions.begin();
					break;
				}
			}
			if (!isChanged && it != instructions.end())
				it++;
		}
	} while (isChanged);

}

bool Optimizer::runOtherPasses(BasicBlock* bb)
{
	auto& instructions = bb->instructions;

	for (auto it = instructions.begin(); it != instructions.end();it++) {
		for (auto& pass : passes) {
		bool isChanged = pass->run(it, instructions);
			if (isChanged) {
				return true;
			}
		};
	}
	return false;
}

bool removeRedudantJumpsHelper(BasicBlock* bb,
	std::unordered_set<BasicBlock*>& visited) {
	if (!bb || visited.count(bb)) return false;  // Skip null or already visited blocks

	visited.insert(bb);  // Mark the current block as visited

	auto& lastInstruction = bb->instructions.back();

	if (lastInstruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Jmp) {
		BasicBlock* nextBlock = bb->pass1;

		uintptr_t count = getReferenceCount(globals::bb, nextBlock);

		if (count == 1)
		{
			auto& instructions = bb->instructions;

			instructions.pop_back();
			
			// Copy instructions from nextBlock to instructions
			instructions.insert(instructions.end(), nextBlock->instructions.begin(), nextBlock->instructions.end());


			bb->pass1 = nextBlock->pass1;
			bb->pass2 = nextBlock->pass2;

			/*for (auto& instruction : bb->instructions) {
				printf("Instruction: %s\n", formatInstruction_(instruction).c_str());
			}*/

			delete nextBlock;
			return true;

		}
		else {
			printf("Wtf?\n 0x%llx\n",lastInstruction.getAddress());
		}
		
	}

	if (removeRedudantJumpsHelper(bb->pass1, visited))
		return true;
	if (removeRedudantJumpsHelper(bb->pass2, visited))
		return true;

	return false;
}
void Optimizer::removeRedudantJumps(BasicBlock* bb)
{
	bool isChanged = true;
	while (isChanged)
	{
		std::unordered_set<BasicBlock*> visited;  // To track visited blocks and handle cycles
		 isChanged = removeRedudantJumpsHelper(bb, visited);
	}
	
}

Optimizer::Optimizer()
{
	passes.push_back(new SimplifyConstantFolding());
	passes.push_back(new SimplifyDeadcodeElimination());
	mbaPasses.push_back(new SimplifyMbaOperation());
	mbaPasses.push_back(new SimplifyStackOperation());
}

void Optimizer::run(BasicBlock* bb)
{
	removeRedudantJumps(bb);
	std::unordered_set<BasicBlock*> visited;  // To track visited blocks and handle cycles
	return runHelper(bb, visited);
}

void Optimizer::runHelper(BasicBlock* bb, std::unordered_set<BasicBlock*>& visited)
{
	if (!bb || visited.count(bb)) return;  // Skip null or already visited blocks

	visited.insert(bb);  // Mark the current block as visited

	bool isChanged = false;

	// Apply optimization passes to the current block
	do {
		runMbaPasses(bb);  // Example pass: Modify as per your needs
		isChanged = runOtherPasses(bb);  // Example pass: Modify as per your needs
	} while (isChanged);

	// Recursively run optimization on child blocks
	 runHelper(bb->pass1, visited);
	 runHelper(bb->pass2, visited);

}
