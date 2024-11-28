#pragma once
#include <Windows.h>
#include <vector>

#include <zasm/zasm.hpp>

class Instruction;
class OperandUnit;

enum class OperandAction {
	READ = 0,
	WRITE,
	READWRITE
};


class BaseOperand
{
private:
	
	Instruction* parent;
	WORD index;
	std::vector<OperandUnit*>operandUnits;
	OperandAction op_action;
	zasm::Operand operand;
public:
	virtual void Link() = 0;
	virtual void Unlink();
	virtual void destroy() = 0;

	bool hasUses();
	
	zasm::Operand getZasmOperand();
	Instruction* getParent()const;
	void setParent(Instruction* instruction);

	std::vector<OperandUnit*>& getOperandUnits();

	uintptr_t getIndex();
	void setIndex(uintptr_t index);

	OperandAction getOperandAccess();
	void setOperandAction(OperandAction action);

	BaseOperand(OperandAction op_action, const zasm::Operand& operand, 
		uintptr_t index, Instruction* parent = nullptr) : parent(parent),
		 op_action(op_action), operand(operand), index(index) {}

	BaseOperand(const BaseOperand&) = delete;
	BaseOperand& operator=(const BaseOperand&) = delete;

	virtual ~BaseOperand();


};

