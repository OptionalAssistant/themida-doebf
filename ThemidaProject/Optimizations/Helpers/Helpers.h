#pragma once
#include <zasm/zasm.hpp>

class BaseOperand;
class Instruction;
class RegisterOperand;

BaseOperand* findPrevAccessRegister(Instruction* currentInstruction,  RegisterOperand*);

BaseOperand* findWriteRegisterInRange(Instruction* start, Instruction* end, RegisterOperand* registerOp);