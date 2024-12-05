#pragma once
#include <variant>

class Operand;
class MemoryOperand;

using OperandVariant = std::variant<Operand, MemoryOperand>;