#include "Optimizer.h"
#include "BaseOptimization.h"

#include "SimplifyConstantFolding.h"
#include "SimplifyDeadcodeElimination.h"
#include "SimplifyStackOperation.h"
#include "SimplifyMbaOperation.h"
#include "../Instruction/Instruction.h"

Optimizer::Optimizer()
{
	passes.push_back(new SimplifyConstantFolding());
	passes.push_back(new SimplifyDeadcodeElimination());
    passes.push_back(new SimplifyStackOperation());
	passes.push_back(new SimplifyMbaOperation());
}

bool Optimizer::run(std::list<Instruction>& instructions)
{
	bool isExtraPass = false;
	for (auto& pass : passes) {
		isExtraPass |= pass->run(instructions);
	}
	return isExtraPass;
}