#pragma once

class Instruction;

class BaseOptimization
{
public:
	virtual bool run(Instruction* instruction) = 0;
};

