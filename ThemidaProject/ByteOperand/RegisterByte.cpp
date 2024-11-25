#include "RegisterByte.h"

BYTE RegisterByte::getIndex()
{
    return registerIndex;
}

void RegisterByte::setIndex(BYTE index)
{
    this->registerIndex = index;
}
