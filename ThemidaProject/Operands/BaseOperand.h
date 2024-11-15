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

class ConstantOperand;

class BaseOperand
{
private:
	Instruction* parent;
	std::vector<BaseOperand*>use_list;
	BaseOperand* next;
	BaseOperand* prev;
	uintptr_t index;
	
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
	void ClearUseList();
	void replaceOneUse(BaseOperand* oldUse, BaseOperand* newUse);
	void replaceOperandWith(BaseOperand* newOperand);
	void deleteUse(BaseOperand* operand);
	void transferAllUses(BaseOperand* operand);
	void makeConstant(ConstantOperand* constantOp);
	void emplaceBetween();
	bool hasOneUse();
	bool hasOneUser();

	uintptr_t getIndex();
	void setIndex(uintptr_t index);

	OperandAction getOperandAccess();
	void setOperandAction(OperandAction action);

	zasm::Operand getZasmOperand();

	virtual void LinkOperand() = 0;

	BaseOperand(OperandAction op_action,const zasm::Operand& operand,uintptr_t index ,Instruction* parent = nullptr, BaseOperand* next = nullptr,
		BaseOperand* prev = nullptr) : parent(parent), next(next),
		prev(prev), op_action(op_action),operand(operand) ,index(index){}

	virtual void destroy() = 0;

	void replaceAllUses(BaseOperand* operand);
	
};

