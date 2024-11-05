#pragma once
#include "../Instruction/Instruction.h"
#include <vector>


namespace zasm {
	class Operand;
}
enum class OperandAction {
	READ = 0,
	WRITE,
	READWRITE
};
class BaseOperand
{
private:
	Instruction* parent;
	std::vector<BaseOperand*>use_list;
	BaseOperand* next;
	BaseOperand* prev;
	OperandAction op_action;
	zasm::Operand operand;
public:
	Instruction* getParent()const;
	void setParent(Instruction* instruction);

	bool hasUses()const;

	std::vector<BaseOperand*>& getUseList();

	virtual BaseOperand* getNext()const;
	virtual BaseOperand* getPrev()const;
	
	void setPrev(BaseOperand* opPrev);
	void setNext(BaseOperand* opNext);
	void addUse(BaseOperand* useOperand);

	OperandAction getOperandAccess();
	void setOperandAction(OperandAction action);

	zasm::Operand getZasmOperand();

	virtual void LinkOperand() = 0;

	BaseOperand(OperandAction op_action,const zasm::Operand& operand, Instruction* parent = nullptr, BaseOperand* next = nullptr,
		BaseOperand* prev = nullptr) : parent(parent), next(next),
		prev(prev), op_action(op_action),operand(operand) {}

	virtual void destroy() = 0;

	void replaceAllUses(BaseOperand* operand);
	void replaceNextPrev(BaseOperand* operand);
	
	void deleteAllUses();
};

