#pragma once
#include <Windows.h>
#include <vector>

class BaseOperand;

class OperandUnit
{
	BaseOperand* parent;
	std::vector<OperandUnit*>use_list;
	OperandUnit* next;
	OperandUnit* prev;
public:
	BaseOperand* getParent()const;
	void setParent(BaseOperand* operand);

	bool hasUses()const;

	std::vector<OperandUnit*>& getUseList();

	virtual OperandUnit* getNext()const;
	virtual OperandUnit* getPrev()const;

	void setPrev(OperandUnit* opPrev);
	void setNext(OperandUnit* opNext);
	void addUse(OperandUnit* useOperand);
	void ClearUseList();
	void replaceOneUse(OperandUnit* oldUse, OperandUnit* newUse);
	void replaceOperandWith(OperandUnit* newOperand);
	void deleteUse(OperandUnit* operand);
	void transferAllUses(OperandUnit* operand);
	void makeConstant();
	void emplaceBetween();
	bool hasOneUse();

	OperandUnit(BaseOperand* parent = nullptr,
		OperandUnit* next = nullptr,OperandUnit* prev = nullptr) : 
		parent(parent), next(next), prev(prev) {}

	void replaceAllUses(OperandUnit* operand);
};

