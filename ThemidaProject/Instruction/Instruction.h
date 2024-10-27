#pragma once
#include <vector>

class BaseOperand;

class Instruction
{
private:
	std::vector<BaseOperand*>operand_list;
	Instruction* prev;
	Instruction* next;
public:
	std::vector<BaseOperand*> getOperands()const;

	void destroy();
	Instruction* getNext()const;
	Instruction* getPrev()const;
	
	Instruction() : prev(nullptr), next(nullptr) {}

	 void LinkInstruction();

	 void setPrev(Instruction* instruction);
	 void setNext( Instruction* instruction);


	 Instruction* insertAfter( Instruction* instruction);
	 Instruction* insertBefore( Instruction* instruction);

	 void addOperand(BaseOperand* baseOperand);
};

