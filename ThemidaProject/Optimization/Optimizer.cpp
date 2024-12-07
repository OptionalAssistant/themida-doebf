#include "Optimizer.h"
#include "BaseOptimization.h"

#include "SimplifyConstantFolding.h"
#include "SimplifyDeadcodeElimination.h"
#include "SimplifyStackOperation.h"
#include "SimplifyMbaOperation.h"
#include "../Instruction/Instruction.h"

void Optimizer::runMbaPasses(std::list<Instruction>& instructions)
{
	bool isChanged = false;

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

bool Optimizer::runOtherPasses(std::list<Instruction>& instructions)
{

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

Optimizer::Optimizer()
{
	passes.push_back(new SimplifyConstantFolding());
	passes.push_back(new SimplifyDeadcodeElimination());
	mbaPasses.push_back(new SimplifyMbaOperation());
	mbaPasses.push_back(new SimplifyStackOperation());
}

bool Optimizer::run(std::list<Instruction>& instructions)
{

	bool isChanged;
	do
	{
		runMbaPasses(instructions);
		isChanged = runOtherPasses(instructions);
	} while (isChanged);
	
	return true;
}