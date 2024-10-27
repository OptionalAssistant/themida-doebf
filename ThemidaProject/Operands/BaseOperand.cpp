#include "BaseOperand.h"

Instruction* BaseOperand::getParent() const
{
    return parent;
}

bool BaseOperand::hasUses() const
{
    return use_list.empty();
}

BaseOperand* BaseOperand::getNext()const
{
    return next;
}

BaseOperand* BaseOperand::getPrev()const
{
    return prev;
}
