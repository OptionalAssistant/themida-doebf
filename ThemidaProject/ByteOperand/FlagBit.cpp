#include "FlagBit.h"

WORD FlagBit::getFlagMask()
{
	return flagMask;
}

void FlagBit::setFlagMask(WORD flagMask)
{
	this->flagMask = flagMask;
}

FlagAction FlagBit::getFlagAction()
{
	return flagAction;
}

void FlagBit::setFlagAction(FlagAction flagAction)
{
	this->flagAction = flagAction;
}
