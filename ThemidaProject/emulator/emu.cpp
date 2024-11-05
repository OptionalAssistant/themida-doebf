#include <limits>
#include <bit>


#include <zasm/formatter/formatter.hpp>

#include "emu.h"



#include "../Utils/Utils.h"
#include "../PE/PE.h"
#include <format>
#include "../Utils/Logger.h"

void EmulatorCPU::PushValue(uintptr_t value)
{
	uintptr_t rsp = reg_read(zasm::x86::rsp);

	rsp -= 8;

	reg_write(zasm::x86::rsp,rsp);

	mem_write(rsp, &value, zasmBitsToNumericSize(zasm::BitSize::_64));

}

uintptr_t EmulatorCPU::PopValue()
{
	uintptr_t rsp = reg_read(zasm::x86::rsp);

	uintptr_t value;
	 mem_read(rsp, &value,zasmBitsToNumericSize(zasm::BitSize::_64));

	rsp += 8;

	reg_write(zasm::x86::rsp, rsp);

	return value;
}

void EmulatorCPU::HandlePush()
{
	if(instruction.getMnemonic() == zasm::x86::Mnemonic::Push)
	{ 
		uintptr_t value = GetValue(instruction.getOperand(0));

		PushValue(value);
	}
	else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Pushfq)
	{
		PushValue(rFlags);
	}
}

void EmulatorCPU::HandleCall()
{
	uintptr_t ret_addr = eip + 5;

	eip += instruction.getOperand(0).get<zasm::Imm>().value<int>();
	
	PushValue(ret_addr);
}

void EmulatorCPU::HandleMov()
{

	uintptr_t dst, value;

	value = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), value);
	
}

void EmulatorCPU::HandlePop()
{
	uintptr_t value = PopValue();

	if (instruction.getMnemonic() == zasm::x86::Mnemonic::Popfq)
	{
		WriteResult(zasm::x86::flags, value);
	}
	else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Pop)
	{
		if (instruction.getOperand(0).holds<zasm::Mem>())
		{
			uintptr_t address = CalcMemAddress(instruction.getOperand(0).get<zasm::Mem>());
			
			mem_write(address, &value, zasmBitsToNumericSize(zasm::BitSize::_64));
		}
			WriteResult(instruction.getOperand(0), value);
	}
}

void EmulatorCPU::HandleLea()
{
	const uintptr_t value = CalcMemAddress(instruction.getOperand(1).get<zasm::Mem>());

	reg_write(instruction.getOperand(0).get<zasm::Reg>(),value);
}

void EmulatorCPU::HandleNeg()
{
	INT64 value = GetValue(instruction.getOperand(0));

	uintptr_t result =   -(value);

	WriteResult(instruction.getOperand(0), result);

	if (value)
		SetCarryFlag();
	else
		ClearCarryFlag();

	CF = true;

	update_eflags(result,value,0,value,instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),
		SUB);
}

void EmulatorCPU::HandleRol()
{
	uintptr_t op1, count;

	op1 = GetValue(instruction.getOperand(0));
	
	count = GetValue(instruction.getOperand(1));
	uintptr_t tempcount;


	if(instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_64)
	 tempcount = count & 0x3F;
	else
	tempcount = count & 0x1F;
	
	switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
	{
	case  zasm::BitSize::_8:
		op1 = std::rotl<UINT8>(op1, tempcount);
		break;
	case  zasm::BitSize::_16:
		op1 = std::rotl<UINT16>(op1, tempcount);
		break;
	case  zasm::BitSize::_32:
		op1 = std::rotl<UINT32>(op1, tempcount);
		break;
	case  zasm::BitSize::_64:
		op1 = std::rotl<UINT64>(op1, tempcount);
		break;
	default:
		break;
	}

	if ((count & tempcount) != 0)
	{
		if (LSB(op1))
			SetCarryFlag();
		else
			ClearCarryFlag();
	}
	if (tempcount == 1)
	{
		BYTE bit = GetSignBit(op1, instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));

		BYTE res = bit ^ GetCarryFlag();

		if (res)
			SetOverflowFlag();
		else
			ClearOverflowFlag();


	}

	WriteResult(instruction.getOperand(0), op1);

	PrintFlags();
}

void EmulatorCPU::HandleJccJump()
{
	switch (instruction.getMnemonic())
	{
	case zasm::x86::Mnemonic::Jno:
	{
		if (!GetOverflowFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jle:
	{
		BYTE ZF = GetZeroFlag();

		BYTE SF = GetSignFlag();

		BYTE OF = GetOverflowFlag();

		if(ZF || OF != SF)
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jnp:
	{
		BYTE PF = GetParityFlag();
		if(!PF)
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jnz:
	{
		BYTE ZF = GetZeroFlag();
		if(!ZF)
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jz:
	{
		BYTE ZF = GetZeroFlag();
		if (ZF)
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jl:
	{
		if(GetSignFlag() != GetOverflowFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jp:
	{
		if(GetParityFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jnle:
	{
		if(!GetZeroFlag() && GetSignFlag() == GetOverflowFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jnb:
	{
		if (!GetCarryFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jns:
	{
		if(!GetSignFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jb:
	{
		if(GetCarryFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jbe:
	{
		if(GetZeroFlag() || GetCarryFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Js:
	{
		if (GetSignFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jnbe:
	{
		if(!GetCarryFlag() && !GetZeroFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jnl:
	{
		if(GetSignFlag() == GetOverflowFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	case zasm::x86::Mnemonic::Jo:
	{
		if (GetOverflowFlag())
			eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>() - instruction.getLength();
	}
	break;
	default:
		break;
	}
}

void EmulatorCPU::HandleAdd()
{
	uintptr_t dst, src, result = 0;

	dst = GetValue(instruction.getOperand(0));

	src = GetValue(instruction.getOperand(1));

	result = src + dst;
	

	WriteResult(instruction.getOperand(0), result);

	update_eflags(result, dst, dst, src,
		instruction.getOperand(1).getBitSize(zasm::MachineMode::AMD64),ADD);
}

void EmulatorCPU::HandleShl()
{
	
	const auto bit = instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64);
	
	uintptr_t count,mask,op1 = 0;
	
	op1 = GetValue(instruction.getOperand(0));

	count = GetValue(instruction.getOperand(1));

	
	if (bit == zasm::BitSize::_64)
		mask = 0x3F;
	else
		mask = 0x1F;

	uintptr_t tempcount = count & mask;

	uintptr_t tempdest = op1;

	while (tempcount)
	{
		if (instruction.getMnemonic() == zasm::x86::Mnemonic::Shl) {

			if (GetSignBit(op1, instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)))
			{
				SetCarryFlag();
			}
			else
				ClearCarryFlag();
			
			
		}
		else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Sar ||
			instruction.getMnemonic() == zasm::x86::Mnemonic::Shr)
		{
			if (LSB(op1))
			{
				SetCarryFlag();
			}
			else
				ClearCarryFlag();

			
		}
		if (instruction.getMnemonic() == zasm::x86::Mnemonic::Shl)
		{
			op1 = op1 << 1;
		}
		else
		{
			if (instruction.getMnemonic() == zasm::x86::Mnemonic::Sar)
			{
				switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
				{
				case zasm::BitSize::_8:
					op1 = static_cast<INT8>(op1) >> 1;
					break;
				case zasm::BitSize::_16:
					op1 = static_cast<INT16>(op1) >> 1;
					break;
					case zasm::BitSize::_32:
					op1 = static_cast<INT32>(op1)  >> 1;
					break;
				case zasm::BitSize::_64:
					 op1 = static_cast<INT64>(op1) >> 1;
					break;
				default:
					break;
				}
				
			}
			else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Shr)
			{
				op1 = op1 >> 1;
			}

		}
		tempcount--;
	}

	if (!(count & mask))
		return;

	if ((count & mask) == 1 )
	{
		if (instruction.getMnemonic() == zasm::x86::Mnemonic::Shl)
		{
			BYTE res = GetSignBit(op1, 
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)) ^ GetCarryFlag();

			if (res)
				SetOverflowFlag();
			else
				ClearOverflowFlag();

	
		}
		else {
			if (instruction.getMnemonic() == zasm::x86::Mnemonic::Sar)
				ClearOverflowFlag();
			else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Shr)
			{
				if (GetSignBit(tempdest,
					instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)))
					SetOverflowFlag();
				else
					ClearOverflowFlag();
			}
		}
	}
	CF = true;
	OF = true;
	AF = true;
	
	WriteResult(instruction.getOperand(0),
		op1);
	update_eflags(op1,tempdest, tempdest,count,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),NONE);
	
}

void EmulatorCPU::HandleAdc()
{
	uintptr_t result, op1, op2 = 0;

	op1 = GetValue(instruction.getOperand(0));
	
	op2 = GetValue(instruction.getOperand(1));
	op2 = op2 + GetCarryFlag();
	result = op1 + op2;

	WriteResult(instruction.getOperand(0), result);

	update_eflags(result,op1,op1,op2,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),ADD);
}

void EmulatorCPU::HandleSetge()
{
	uintptr_t val = 0;
	if (GetSignFlag() == GetOverflowFlag())
		val = 1;
	else
		val = 0;

	WriteResult(instruction.getOperand(0), val);
}

void EmulatorCPU::HandleMovzx()
{
	uintptr_t op1, op2;


	op1 = GetValue(instruction.getOperand(0));

	op2 = GetValue(instruction.getOperand(1));

	int32_t op11 = static_cast<uint64_t>((static_cast<int16_t>(op2)));

	if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == instruction.getOperand(1).getBitSize(zasm::MachineMode::AMD64))
	{
		WriteResult(instruction.getOperand(0), op1);
		return;
	}
	zasm::BitSize bt_op2 = instruction.getOperand(1).getBitSize(zasm::MachineMode::AMD64);
	zasm::BitSize bt_op1 = instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64);

	if (bt_op2 == zasm::BitSize::_8 && bt_op1 == zasm::BitSize::_16)
	{
		op1 &= 0xFFFFFFFFFFFF0000;
		op1 |= (op2 & 0xFF);
		
	}
	else if (bt_op2 == zasm::BitSize::_8 && bt_op1 == zasm::BitSize::_32)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFF);
	}

	else if (bt_op2 == zasm::BitSize::_8 && bt_op1 == zasm::BitSize::_64)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFF);
	}


	else if (bt_op2 == zasm::BitSize::_16 && bt_op1 == zasm::BitSize::_32)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFFFF);
	}
	else if (bt_op2 == zasm::BitSize::_16 && bt_op1 == zasm::BitSize::_64)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFFFF);
	}
	else if (bt_op2 == zasm::BitSize::_32 && bt_op1 == zasm::BitSize::_64)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFFFFFFFF);
	}
	WriteResult(instruction.getOperand(0), op1);
}

void EmulatorCPU::HandleSub()
{
	uintptr_t op1, op2,result;

	op1 = GetValue(instruction.getOperand(0));

	op2 = GetValue(instruction.getOperand(1));

	result = op1 - op2;

	WriteResult(instruction.getOperand(0), result);
	update_eflags(result, op1, op1, op2,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),SUB);

}

void EmulatorCPU::HandleRor()
{
	uintptr_t op1, op2, result;

	op2 = GetValue(instruction.getOperand(1));

	op1 = GetValue(instruction.getOperand(0));
	
	uintptr_t tempcount = 0;
	uintptr_t count_mask = 0;

	uintptr_t size = 0;
	if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_64)
		count_mask = 0x3F;
	else
		count_mask = 0x1F;

	switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
	{
	case zasm::BitSize::_8:
		op1 = std::rotr<UINT8>(op1,op2);
		break;
	case zasm::BitSize::_16:
		op1 = std::rotr<UINT16>(op1, op2);
		break;
	case zasm::BitSize::_32:
		op1 = std::rotr<UINT32>(op1, op2);
		break;
	case zasm::BitSize::_64:
		op1 = std::rotr<UINT64>(op1, op2);
		break;
	default:
		break;
	}
	if ((op2 & count_mask) != 0) {

			if (GetSignBit(op1, instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)))
				SetCarryFlag();
			else
				ClearCarryFlag();
	}

	if ((op2 & count_mask) == 1)
	{
		BYTE bit = GetSignBit(op1,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));

		BYTE result = bit ^ GetSecondMSB(op1,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));

		if (result)
			SetOverflowFlag();
		else
			ClearOverflowFlag();

	}

	WriteResult(instruction.getOperand(0), op1);

	PrintFlags();
}

void EmulatorCPU::HandleXadd()
{
	uintptr_t dst = GetValue(instruction.getOperand(0));

	uintptr_t src = GetValue(instruction.getOperand(1));
	
	uintptr_t dst_old = dst;

	uintptr_t temp = dst + src;

	uintptr_t src_res = dst;

	dst = temp;

	WriteResult(instruction.getOperand(0), dst);

	WriteResult(instruction.getOperand(1), src_res);

	update_eflags(dst, dst_old, dst_old, src, instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),
		ADD);
}

void EmulatorCPU::HandleXor()
{
	uintptr_t dst = GetValue(instruction.getOperand(0));

	uintptr_t src = GetValue(instruction.getOperand(1));

	uintptr_t dst_old = dst;

	dst ^= src;
		
	WriteResult(instruction.getOperand(0), dst);
	
	ClearOverflowFlag();

	ClearCarryFlag();

	CF = true;
	OF = true;
	AF = true;//UD

	update_eflags(dst, dst_old, dst_old, src,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),NONE);

}

void EmulatorCPU::HandleSbb()
{
	uintptr_t dst = GetValue(instruction.getOperand(0));

	uintptr_t src = GetValue(instruction.getOperand(1));

	uintptr_t dst_old = dst;
	src = src + GetCarryFlag();
	dst = dst - src;

	WriteResult(instruction.getOperand(0), dst);

	update_eflags(dst, dst_old, dst_old, src,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),SUB);
}

void EmulatorCPU::HandleMovsx()
{
	uintptr_t op1, op2;


	op1 = GetValue(instruction.getOperand(0));

	op2 = GetValue(instruction.getOperand(1));
	
	
	if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == instruction.getOperand(1).getBitSize(zasm::MachineMode::AMD64))
	{
		WriteResult(instruction.getOperand(0), op1);
		return;
	}
	zasm::BitSize bt_op2 = instruction.getOperand(1).getBitSize(zasm::MachineMode::AMD64);
	zasm::BitSize bt_op1 = instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64);
	
	if (bt_op2 == zasm::BitSize::_8 && bt_op1 == zasm::BitSize::_16)
	{
		op1 &= 0xFFFFFFFFFFFF0000;
		op1 |= (op2 & 0xFF);
		if (GetSignBit(op2, bt_op2))
		{
			op1 |= 0xFF00;
		}
	}
	else if (bt_op2 == zasm::BitSize::_8 && bt_op1 == zasm::BitSize::_32)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFF);
		if (GetSignBit(op2, bt_op2))
		{
			op1 |= 0x00000000FFFFFF00;
		}
	}
	
	else if (bt_op2 == zasm::BitSize::_8 && bt_op1 == zasm::BitSize::_64)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFF);
		if (GetSignBit(op2, bt_op2))
		{
			op1 |= 0xFFFFFFFFFFFFFF00;
		}
	}


	else if (bt_op2 == zasm::BitSize::_16 && bt_op1 == zasm::BitSize::_32)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFFFF);
		if (GetSignBit(op2, bt_op2))
		{
			op1 |= 0x00000000FFFF0000;
		}
	}
	else if (bt_op2 == zasm::BitSize::_16 && bt_op1 == zasm::BitSize::_64)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFFFF);
		if (GetSignBit(op2, bt_op2))
		{
			op1 |= 0xFFFFFFFFFFFF0000;
		}
	}
	else if (bt_op2 == zasm::BitSize::_32 && bt_op1 == zasm::BitSize::_64)
	{
		op1 &= 0x0;
		op1 |= (op2 & 0xFFFFFFFF);
		if (GetSignBit(op2, bt_op2))
		{
			op1 |= 0xFFFFFFFF00000000;
		}
	}
	WriteResult(instruction.getOperand(0), op1);
}

void EmulatorCPU::HandleAnd()
{
	uintptr_t op1 = GetValue(instruction.getOperand(0));

	uintptr_t op2 = GetValue(instruction.getOperand(1));

	uintptr_t result = op1 & op2;

	ClearOverflowFlag();
	ClearCarryFlag();

	CF = true;
	OF = true;
	AF = true;//UD

	WriteResult(instruction.getOperand(0), result);

	update_eflags(result, op1, op1, op2,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64), NONE);

}

void EmulatorCPU::HandleNot()
{
	uintptr_t value = GetValue(instruction.getOperand(0));

	value = ~value;

	WriteResult(instruction.getOperand(0), value);
}

void EmulatorCPU::HandleBswap()
{
	uintptr_t value = reg_read(instruction.getOperand(0).get<zasm::Reg>());
	switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
	{
	case zasm::BitSize::_8:
		value = std::byteswap((BYTE)value);
		break;
	case zasm::BitSize::_16:
		value = std::byteswap((WORD)value);
		break;
	case zasm::BitSize::_32:
		value = std::byteswap((DWORD)value);
		break;
	case zasm::BitSize::_64:
		value = std::byteswap((uintptr_t)value);
		break;
	default:
	{
		printf("bswap error\n");
		exit(0);
	}
	break;
	}
	
	reg_write(instruction.getOperand(0).get<zasm::Reg>(), value);
}



void EmulatorCPU::HandleDec()
{
	uintptr_t value = GetValue(instruction.getOperand(0));

	uintptr_t result = value - 1;

	CF = true;

	WriteResult(instruction.getOperand(0), result);

	update_eflags(result, value, value, 1,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),
		SUB);
}
void EmulatorCPU::HandleInc()
{
	uintptr_t value = GetValue(instruction.getOperand(0));

	uintptr_t result = value + 1;

	CF = true;

	WriteResult(instruction.getOperand(0), result);

	update_eflags(result, value, value, 1,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),
		ADD);
}

void EmulatorCPU::HandleRet()
{
	uintptr_t value = PopValue();

	eip = value;

	uintptr_t rsp = reg_read(zasm::x86::rsp);

	if(instruction.getOperand(0).holds<zasm::Imm>())
	rsp += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>();

	reg_write(zasm::x86::rsp, rsp);

	eip -= instruction.getLength();
}

void EmulatorCPU::HandlerOr()
{
	uintptr_t op1 = GetValue(instruction.getOperand(0));

	uintptr_t op2 = GetValue(instruction.getOperand(1));

	uintptr_t result = op1 | op2;

	ClearOverflowFlag();
	ClearCarryFlag();

	AF = true;//UD
	CF = true;
	OF = true;

	WriteResult(instruction.getOperand(0), result);

	update_eflags(result, op1, op1,
		op2, instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64), NONE);

	
}

void EmulatorCPU::HandleCmovb()
{
	if (GetCarryFlag())
	{
		uintptr_t op2 = GetValue(instruction.getOperand(1));

		WriteResult(instruction.getOperand(0),op2);
	}
	else
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
			
		}
		return;
	}
}

void EmulatorCPU::HandleSetz()
{
	if (GetZeroFlag())
	{
		WriteResult(instruction.getOperand(0), 1);
	}
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleExchange()
{

	uintptr_t dst = GetValue(instruction.getOperand(0));

	uintptr_t src = GetValue(instruction.getOperand(1));

	uintptr_t temorary = dst;

	dst = src;

	src = temorary;

	WriteResult(instruction.getOperand(0), dst);

	WriteResult(instruction.getOperand(1), src);
}

void EmulatorCPU::HandleSetnz()
{
	if (!GetZeroFlag())
	{
		WriteResult(instruction.getOperand(0), 1);
	}
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleSetp()
{
	if (GetParityFlag())
	{
		WriteResult(instruction.getOperand(0), 1);
	}
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleSetnle()
{
	if (!GetZeroFlag() && GetSignFlag() == GetOverflowFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleSetnb()
{
	if(!GetCarryFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleSetbe()
{
	if(GetCarryFlag() || GetZeroFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCmp()
{
	uintptr_t src1 = GetValue(instruction.getOperand(0));

	uintptr_t src2 = GetValue(instruction.getOperand(1));

	if (instruction.getOperand(1).holds<zasm::Imm>())
	{
		switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
		{
		case zasm::BitSize::_8:
			src2 = static_cast<INT8>(src2);
			break;
		case zasm::BitSize::_16:
			src2 = static_cast<INT16>(src2);
			break;
		case zasm::BitSize::_32:
			src2 = static_cast<INT32>(src2);
			break;
		case zasm::BitSize::_64:
			src2 = static_cast<INT64>(src2);
			break;
		default:
			break;
		}
	}

	uintptr_t result = src1 - src2;

	update_eflags(result, src1, src1, src2,
		instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),
		SUB);
}

void EmulatorCPU::HandleSetle()
{
	if (GetZeroFlag() || GetSignFlag() != GetOverflowFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);

}

void EmulatorCPU::HandleSetnbe()
{
	if (!GetZeroFlag() && !GetCarryFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCmovnz()
{
	if (GetZeroFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}

	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleSets()
{
	if(GetSignFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleSetb()
{
	if (GetCarryFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCmovl()
{
	if (GetSignFlag() == GetOverflowFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		
		}
		return;
	}

	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleSetno()
{
	if(!GetOverflowFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCld()
{
	ClearDirectionFlag();
}

void EmulatorCPU::HandleMovsb()
{
	uintptr_t dst = reg_read(zasm::x86::rdi);

	uintptr_t src = reg_read(zasm::x86::rsi);

	uintptr_t count_ = reg_read(zasm::x86::ecx);

	if (zasm::x86::Attribs::Rep)
	{
		for (int i = 0; i < count_; i++) {
			uintptr_t value = mem_read(src + i, &value,
				zasmBitsToNumericSize(zasm::BitSize::_8));

			mem_write(dst + i, &value, 
				zasmBitsToNumericSize(zasm::BitSize::_8));
		}
		reg_write(zasm::x86::rsi, src + count_);
		reg_write(zasm::x86::rdi, dst + count_);

		count += count_ - 1;
	}
	else
		exit(0);


	WriteResult(zasm::x86::ecx, 0);
}

void EmulatorCPU::HandleSetns()
{
	if(!GetSignFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCmovns()
{
	if (GetSignFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}

	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);

}

void EmulatorCPU::HandleCmovnb()
{
	if (GetCarryFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleCmovp()
{
	if (!GetParityFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}

	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleSetl()
{
	if (GetOverflowFlag() != GetSignFlag())
		WriteResult(instruction.getOperand(0), 1);
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCmovbe()
{
	if (GetCarryFlag() || GetZeroFlag())
	{
		uintptr_t src = GetValue(instruction.getOperand(1));

		WriteResult(instruction.getOperand(0), src);
	}
	else
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
			
		}
		return;
	}
}

void EmulatorCPU::HandleCmpxchg()
{
	uintptr_t dest = GetValue(instruction.getOperand(0));
	uintptr_t accumulator = 0;
	switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
	{
	case zasm::BitSize::_8:
		accumulator = reg_read(zasm::x86::al);
		break;
	case zasm::BitSize::_16:
		accumulator = reg_read(zasm::x86::ax);
		break;
	case zasm::BitSize::_32:
		accumulator = reg_read(zasm::x86::eax);
		break;
	case zasm::BitSize::_64:
		accumulator = reg_read(zasm::x86::rax);
		break;
	default:
		break;
	}

	if (accumulator == dest) {
		SetZeroFlag();
		WriteResult(instruction.getOperand(0), GetValue(instruction.getOperand(1)));
	}
	else {
		ClearZeroFlag();
		switch (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64))
		{
		case zasm::BitSize::_8:
			WriteResult(zasm::x86::al, GetValue(instruction.getOperand(0)));
			break;
		case zasm::BitSize::_16:
			WriteResult(zasm::x86::ax, GetValue(instruction.getOperand(0)));
			break;
		case zasm::BitSize::_32:
			WriteResult(zasm::x86::eax, GetValue(instruction.getOperand(0)));
			break;
		case zasm::BitSize::_64:
			WriteResult(zasm::x86::rax, GetValue(instruction.getOperand(0)));
			break;
		default:
			break;
		}
	}
}

void EmulatorCPU::HandleSetnp()
{
	if (!GetParityFlag())
	{
		WriteResult(instruction.getOperand(0), 1);
	}
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleSeto()
{
	if (GetOverflowFlag())
	{
		WriteResult(instruction.getOperand(0), 1);
	}
	else
		WriteResult(instruction.getOperand(0), 0);
}

void EmulatorCPU::HandleCmovnp()
{
	if (GetParityFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleCmovno()
{
	if (GetOverflowFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleTest()
{
	uintptr_t op1 = GetValue(instruction.getOperand(0));

	uintptr_t op2 = GetValue(instruction.getOperand(1));

	uintptr_t result = op1 & op2;

	ClearOverflowFlag();
	ClearCarryFlag();
	
	OF = true;
	CF = true;
	AF = true;

	update_eflags(result, op1, op1, op2, instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64),
		NONE);
}

void EmulatorCPU::HandleCmovz()
{
	if (!GetZeroFlag()) 
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}

	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleCmovs()
{
	if (!GetSignFlag())
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}

	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::HandleCmovnl()
{
	if (GetSignFlag() == GetOverflowFlag())
	{
		uintptr_t src = GetValue(instruction.getOperand(1));

		WriteResult(instruction.getOperand(0), src);
		return;
	}
	else
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
			
		}
		return;
	}

}

void EmulatorCPU::HandleCmovnbe()
{
	if (!GetZeroFlag() && !GetCarryFlag())
	{
		uintptr_t src = GetValue(instruction.getOperand(1));

		WriteResult(instruction.getOperand(0), src);
	}
	else
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
}

void EmulatorCPU::HandleCmovle()
{
	if (GetZeroFlag() ||(GetSignFlag() != GetOverflowFlag()))
	{
		uintptr_t src = GetValue(instruction.getOperand(1));

		WriteResult(instruction.getOperand(0), src);
	}
	else
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
}

void EmulatorCPU::HandleCmovnle()
{
	if (!GetZeroFlag() && GetSignFlag() == GetOverflowFlag())
	{
		uintptr_t src = GetValue(instruction.getOperand(1));

		WriteResult(instruction.getOperand(0), src);
	}
	else
	{
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
}

void EmulatorCPU::HandleCmovo()
{
	if (!GetOverflowFlag()) {
		if (instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64) == zasm::BitSize::_32)
		{
			uintptr_t dst = GetValue(instruction.getOperand(0));
			WriteResult(instruction.getOperand(0), dst);
		}
		return;
	}
	uintptr_t src = GetValue(instruction.getOperand(1));

	WriteResult(instruction.getOperand(0), src);
}

void EmulatorCPU::SetCarryFlag()
{
	rFlags |= 1;
}

void EmulatorCPU::SetParityFlag()
{
	rFlags |= 4;
}

void EmulatorCPU::SetAuxiliaryCarryFlag()
{
	rFlags |= 0x10;
}

void EmulatorCPU::SetZeroFlag()
{
	rFlags |= 0x40;
}

void EmulatorCPU::SetSignFlag()
{
	rFlags |= 0x80;
}

void EmulatorCPU::SetOverflowFlag()
{
	rFlags |=  0x800;
}

void EmulatorCPU::ClearCarryFlag()
{
	rFlags &= 0xFFFE;
}

void EmulatorCPU::ClearParityFlag()
{
	rFlags &= 0xFFFB;
}

void EmulatorCPU::ClearAuxiliaryCarryFlag()
{
	rFlags &= 0xFFEF;
}

void EmulatorCPU::ClearZeroFlag()
{
	rFlags &= 0xFFBF;
}

void EmulatorCPU::ClearSignFlag()
{
	rFlags &= 0xFF7F;
}

void EmulatorCPU::ClearOverflowFlag()
{
	rFlags &= 0xF7FF;
}

void EmulatorCPU::ClearDirectionFlag()
{
	rFlags &= 0xFBFF;
}

BYTE EmulatorCPU::GetCarryFlag()
{
	return rFlags & 1;
}

BYTE EmulatorCPU::GetParityFlag()
{
	return (rFlags >> 2) & 1;
}

BYTE EmulatorCPU::GetAuxiliaryFlag()
{
	return (rFlags >> 4) & 1;
}

BYTE EmulatorCPU::GetZeroFlag()
{
	return  (rFlags >> 6) & 1;
}

BYTE EmulatorCPU::GetSignFlag()
{
	return (rFlags >> 7) & 1;
}

BYTE EmulatorCPU::GetOverflowFlag()
{
	return  (rFlags >> 11) & 1;
}

uintptr_t EmulatorCPU::from_virtual_to_real(uintptr_t address)
{

	MemoryRegion* mr = nullptr;

	for (auto& memRegion : memory_regions) {
		if (address >= memRegion.address && address <= memRegion.address + memRegion.size) {
			mr = &memRegion;
			break;
		}
	}

	if (!mr)
		throw std::runtime_error("Invalid eip");

	return address - mr->address + (uintptr_t)mr->memory.data();
}


uintptr_t EmulatorCPU::GetValue(const zasm::Operand& op)
{

	if (op.holds<zasm::Reg>())
		return reg_read(op.get<zasm::Reg>());
	else if (op.holds<zasm::Imm>())
		return op.get<zasm::Imm>().value<uintptr_t>();
	else if (op.holds<zasm::Mem>()) {
		uintptr_t value = 0;
		 mem_read(CalcMemAddress(op.get<zasm::Mem>()), &value,
			zasmBitsToNumericSize(op.getBitSize(zasm::MachineMode::AMD64)));
		 return value;
	}
	else
	{
		printf("Fatal exit during GetValue\n");
		exit(0);
	}

}

void EmulatorCPU::WriteResult(const zasm::Operand& op,uintptr_t value)
{
	if (op.holds<zasm::Reg>())
		reg_write(op.get<zasm::Reg>(), value);
	else if (op.holds<zasm::Mem>())
		mem_write(CalcMemAddress(op.get<zasm::Mem>()), &value,
			zasmBitsToNumericSize(op.getBitSize(zasm::MachineMode::AMD64)));
	else {
		printf("Failed to writeresult");
		exit(0);
	}
}

void EmulatorCPU::PrintFlags()
{

/*	printf("ZF: 0x%X  ", GetZeroFlag());
	printf("PF: 0x%X  ", GetParityFlag());
	printf("AF: 0x%X  ", GetAuxiliaryFlag());
	printf("SF: 0x%X  ", GetSignFlag());
	printf("CF: 0x%X  ", GetCarryFlag());
	printf("OF: 0x%X\n", GetOverflowFlag());*/
}

uintptr_t EmulatorCPU::mem_read(uintptr_t address,void* bytes ,uintptr_t size)
{

	MemoryRegion* mr = nullptr;
	for (auto& memRegion : memory_regions) {
		if (address >= memRegion.address && address <= memRegion.address + memRegion.size) {
			mr = &memRegion;
			break;
		}
	}

	if (!mr) {
		PrintRegisters();
		throw std::runtime_error("Invalid memory write(Memory regions does not exist)");
	}
	
	if (address + size > mr->address + mr->size) {
		throw std::runtime_error("Memory read out of region");
	}

	void* address_ = (void*)(mr->memory.data() + address - mr->address);
	memcpy(bytes, (void*)(mr->memory.data() + address - mr->address), size);
}

void EmulatorCPU::mem_write(uintptr_t address,const void* bytes,uintptr_t size)
{

	MemoryRegion* mr = nullptr;
	
	for (auto& memRegion : memory_regions) {
		if (address >= memRegion.address && address  <= memRegion.address +  memRegion.size) {
			mr = &memRegion;
			break;
		}
	}

	if(!mr)
		throw std::runtime_error("Invalid memory write(Memory regions does not exist)");
	
	if (address + size > mr->address + mr->size) {
		throw std::runtime_error("Memory write out of region");
	}

	void* address_ = (void*)(mr->memory.data() + address - mr->address);
	memcpy(address_, bytes,size);
}

uintptr_t EmulatorCPU::CalcMemAddress(const zasm::Mem& mem)
{
	uintptr_t value = 0;
	
	if (mem.getBase().getId() == (zasm::Reg::Id)ZYDIS_REGISTER_RIP)
	{
		value = mem.getDisplacement() + eip;

		return value ;
		printf("\n");
	}
	else if (mem.getBase().getId() != zasm::Reg::Id::None)
	{
		value = reg_read(mem.getBase());
	}
	else if (mem.getSegment() == zasm::x86::gs) {
		value = reg_read(zasm::x86::gs);
	}

	if (mem.getIndex().getId() != zasm::Reg::Id::None)
	{
		value += reg_read(mem.getIndex()) * mem.getScale();
	}

	if (mem.getDisplacement())
	{
		value += mem.getDisplacement();
	}

		return value;
}

uintptr_t EmulatorCPU::CalcEffectiveMemAddress(const zasm::Operand& op,uintptr_t i)
{
	if (instruction.getMnemonic() != zasm::x86::Mnemonic::Call ||
		instruction.getMnemonic() != zasm::x86::Mnemonic::Movsb)
	{
		uintptr_t address;
		if (instruction.getMnemonic() == zasm::x86::Mnemonic::Push)
		{
			address = CalcMemAddress(op.get<zasm::Mem>());

			if (instruction.getOperandAccess(i) == zasm::detail::OperandAccess::Write)
			address = address - 8;
		}
		else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Pushfq) {
			address = reg_read(zasm::x86::rsp);
			address -= 8;
		}
		else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Pop)
		{
			address = CalcMemAddress(op.get<zasm::Mem>());

			if (instruction.getOperandAccess(i) == zasm::detail::OperandAccess::Write)
				address += 8;

		}
		else if (instruction.getMnemonic() == zasm::x86::Mnemonic::Popfq) {
			address = reg_read(zasm::x86::rsp);
		}
		else {
			address = CalcMemAddress(
				op.get<zasm::Mem>());
		}

		return address;

	}
}

void EmulatorCPU::update_eflags(uintptr_t dst,uintptr_t dst_old,uintptr_t src1,
	uintptr_t src2,zasm::BitSize bs, BinaryOp op)
{
	uintptr_t tempd = dst;

	uintptr_t temp1 = src1;
	uintptr_t temp2 = src2;
	switch (bs)
	{
	case zasm::BitSize::_8:
	{
		temp1 &= 0xFF;
		temp2 &= 0xFF;
		tempd &= 0xFF;
	}
		break;
	case zasm::BitSize::_16:
	{
		temp1 &= 0xFFFF;
		temp2 &= 0xFFFF;
		tempd &= 0xFFFF;
	}
		break;
	case zasm::BitSize::_32:
	{
		temp1 &= 0xFFFFFFFF;
		temp2 &= 0xFFFFFFFF;
		tempd &= 0xFFFFFFFF;
	}
	case zasm::BitSize::_64:
		break;
	default:
		break;
	}
	
	if (!OF)
	{
		BYTE SF1 = GetSignBit(src1,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));
		BYTE SF2 = GetSignBit(src2,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));
		BYTE SFD = GetSignBit(dst,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));
		if(op == SUB){
			if (GetSignBit(src1,
				instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)) &&
				!GetSignBit(src2,
					instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)) &&
				!GetSignBit(dst,
					instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)))
			{
				SetOverflowFlag();
			}
			else if (!GetSignBit(src1,
				instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)) &&
				GetSignBit(src2,
					instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)) &&
				GetSignBit(dst,
					instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)))
			{
				SetOverflowFlag();
			}
			else
				ClearOverflowFlag();
		}
		else if(op == ADD)
		{
			printf("");
			if (!SF1 && !SF2 && SFD)
				SetOverflowFlag();
			else if (SF1 && SF2 && !SFD)
			{
				SetOverflowFlag();
			}
			else
				ClearOverflowFlag();
		}
		else
		{
			printf("Can not set OF flag...Fatal\n");
			exit(0);
		}
	
		
	}
	
	if(!ZF){
	if (!tempd)
		SetZeroFlag();
	else
		ClearZeroFlag();
	}
	
	if(!SF){
	if (GetSignBit(dst, bs))
		SetSignFlag();
	else
		ClearSignFlag();

	}
	
	if(!PF){
	WORD numberBits = 0;
	BYTE last_byte = dst & 0xFF;
	for (uintptr_t i = 0; i < 8; i++)
	{
		if (last_byte & (1 << i))
			numberBits += 1;
	}

	if (numberBits % 2)
		ClearParityFlag();
	else
		SetParityFlag();
	}
	
	if(!CF){
	if(op == SUB)
	{
			if (temp1 < temp2)
				SetCarryFlag();
			else
				ClearCarryFlag();
		
	}
	else if(op == ADD)
	{
		
		BYTE SF1 = GetSignBit(src1,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));
		BYTE SF2 = GetSignBit(src2,
			instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));
	    BYTE SFD = GetSignBit(dst,
				instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64));

		bool Carry = (SF1 | SF2) == 1 && SFD == 0;

		if ((SF1 | SF2) == 1 && SFD == 0)
			SetCarryFlag();
		else if (SF1 && SF2)
			SetCarryFlag();
		else
			ClearCarryFlag();

	}
	else
	{
		printf("Can not set CF flag...Fatal\n");
		exit(0);
	}
	}
	
	if(!AF){
		if (op == SUB && (src1 & 0xF) < (src2 & 0xF)) {
			SetAuxiliaryCarryFlag();
		}
		else if (op == ADD && (((src1 & 0xF) + (src2 & 0xF)) > 0xF)) {
			SetAuxiliaryCarryFlag();
		}
		else
			ClearAuxiliaryCarryFlag();

	}

	PrintFlags();

	ZF = 0, PF= 0, AF= 0, OF = 0, CF = 0, SF = 0;
}
#define REG #REG
void EmulatorCPU::PrintRegisters()
{
	std::string res = std::format("RAX :0x{:x}  ", regs[RAX]);
	res += std::format("RBX :0x{:x}  ", regs[RBX]);
	res += std::format("RCX :0x{:x}  ", regs[RCX]);
	res += std::format("RDX :0x{:x}  ", regs[RDX]);
	res += std::format("RSP :0x{:x}  ", regs[RSP]);
	res += std::format("RBP :0x{:x}  ", regs[RBP]);
	res += std::format("RSI :0x{:x}  ", regs[RSI]);
	res += std::format("RDI :0x{:x}  \n", regs[RDI]);
	res += std::format("R8 :0x{:x}  ", regs[R8]);
	res += std::format("R9 :0x{:x}  ", regs[R9]);
	res += std::format("R10 :0x{:x}  ", regs[R10]);
	res += std::format("R11 :0x{:x}  ", regs[R11]);
	res += std::format("R12 :0x{:x}  ", regs[R12]);
	res += std::format("R13 :0x{:x}  ", regs[R13]);
	res += std::format("R14 :0x{:x}  ", regs[R14]);
	res += std::format("R15 :0x{:x}  ", regs[R15]);
	res+= std::format("RFLAGS :0x{:x}  \n\n", rFlags);

	logger->log(res);
	//printf("%s", res.c_str());

}

void EmulatorCPU::run(uintptr_t rva)
{
	
	isStop = false;

	eip = baseImage + rva;
	count = 0;
	while (true) {

		uintptr_t realone = from_virtual_to_real(eip);

		auto res = decoder->decode((void*)realone, ZYDIS_MAX_INSTRUCTION_LENGTH, 0x0);

		if (!res.hasValue())
		{
			printf("Failed to disassemble instruction at address:0x%llx\n", rva);
			exit(0);
		}
		instruction  = res.value();
		
		for (auto& callback : callbacks) {
			callback.callback_fn(this,eip,instruction,callback.callback_data);
		}

		switch (instruction.getMnemonic())
		{
		case zasm::x86::Mnemonic::Push:
		case zasm::x86::Mnemonic::Pushfq:
			HandlePush();
			break;
		case zasm::x86::Mnemonic::Mov:
			HandleMov();
			break;
		case zasm::x86::Mnemonic::Call:
			HandleCall();
			eip -= 5;
			break;
		case zasm::x86::Mnemonic::Pop:
		case zasm::x86::Mnemonic::Popfq:
			HandlePop();
			break;
		case zasm::x86::Mnemonic::Lea:
			HandleLea();
			break;
		case zasm::x86::Mnemonic::Jmp:
		{
				if(instruction.getOperand(0).holds<zasm::Imm>()){
					eip += instruction.getOperand(0).get<zasm::Imm>().value<uintptr_t>();
					eip -= instruction.getLength();
				}
				else if (instruction.getOperand(0).holds<zasm::Reg>() || 
					instruction.getOperand(0).holds<zasm::Mem>()) {
					eip = GetValue(instruction.getOperand(0));
					eip -= instruction.getLength();
				}
		}
			break;
		case zasm::x86::Mnemonic::Jno:
		case zasm::x86::Mnemonic::Jle:
		case zasm::x86::Mnemonic::Jnp:
		case zasm::x86::Mnemonic::Jnz:
		case zasm::x86::Mnemonic::Jl:
		case zasm::x86::Mnemonic::Jp:
		case zasm::x86::Mnemonic::Jnle:
		case zasm::x86::Mnemonic::Jnb:
		case zasm::x86::Mnemonic::Jns:
		case zasm::x86::Mnemonic::Jb:
		case zasm::x86::Mnemonic::Jbe:
		case zasm::x86::Mnemonic::Js:
		case zasm::x86::Mnemonic::Jnbe:
		case zasm::x86::Mnemonic::Jz:
		case zasm::x86::Mnemonic::Jnl:
		case zasm::x86::Mnemonic::Jo:
			HandleJccJump();
			break;
		case zasm::x86::Mnemonic::Neg:
			HandleNeg();
			break;
		case zasm::x86::Mnemonic::Rol:
			HandleRol();
			break;
		case zasm::x86::Mnemonic::Add:
			HandleAdd();
			break;
		case zasm::x86::Mnemonic::Shl:
		case zasm::x86::Mnemonic::Sar:
		case zasm::x86::Mnemonic::Shr:
			HandleShl();
			break;
		case zasm::x86::Mnemonic::Adc:
			HandleAdc();
			break;
		case zasm::x86::Mnemonic::Setnl:
			HandleSetge();
			break;
		case zasm::x86::Mnemonic::Movzx:
			HandleMovzx();
			break;
		case zasm::x86::Mnemonic::Sub:
			HandleSub();
			break;
		case zasm::x86::Mnemonic::Ror:
			HandleRor();
			break;
		case zasm::x86::Mnemonic::Xadd:
			HandleXadd();
			break;
		case zasm::x86::Mnemonic::Xor:
			HandleXor();
			break;
		case zasm::x86::Mnemonic::Sbb:
			HandleSbb();
			break;
		case zasm::x86::Mnemonic::Movsx:
		case zasm::x86::Mnemonic::Movsxd:
			HandleMovsx();
			break;
		case zasm::x86::Mnemonic::Cmpxchg:
			HandleCmpxchg();
			break;
		case zasm::x86::Mnemonic::And:
			HandleAnd();
			break;
		case zasm::x86::Mnemonic::Not:
			HandleNot();
			break;
		case zasm::x86::Mnemonic::Bswap:
			HandleBswap();
			break;
		case zasm::x86::Mnemonic::Dec:
			HandleDec();
			break;
		case zasm::x86::Mnemonic::Inc:
			HandleInc();
			break;
		case zasm::x86::Mnemonic::Ret:
			HandleRet();
			break;
		case zasm::x86::Mnemonic::Or:
			HandlerOr();
			break;
		case zasm::x86::Mnemonic::Cmovb:
			HandleCmovb();
			break;
		case zasm::x86::Mnemonic::Setz:
			HandleSetz();
			break;
		case zasm::x86::Mnemonic::Xchg:
			HandleExchange();
			break;
		case zasm::x86::Mnemonic::Setnz:
			HandleSetnz();
			break;
		case zasm::x86::Mnemonic::Setp:
			HandleSetp();
			break;
		case zasm::x86::Mnemonic::Setnle:
			HandleSetnle();
			break;
		case zasm::x86::Mnemonic::Setnb:
			HandleSetnb();
			break;
		case zasm::x86::Mnemonic::Setbe:
			HandleSetbe();
			break;
		case zasm::x86::Mnemonic::Cmp:
			HandleCmp();
			break;
		case zasm::x86::Mnemonic::Setle:
			HandleSetle();
			break;
		case zasm::x86::Mnemonic::Setnbe:
			HandleSetnbe();
			break;
		case zasm::x86::Mnemonic::Cmovnz:
			HandleCmovnz();
			break;
		case zasm::x86::Mnemonic::Sets:
			HandleSets();
			break;
		case zasm::x86::Mnemonic::Setb:
			HandleSetb();
			break;
		case zasm::x86::Mnemonic::Cmovl:
			HandleCmovl();
			break;
		case zasm::x86::Mnemonic::Setno:
			HandleSetno();
			break;
		case zasm::x86::Mnemonic::Cld:
			HandleCld();
			break;
		case zasm::x86::Mnemonic::Movsb:
			HandleMovsb();
			break;
		case zasm::x86::Mnemonic::Setns:
			HandleSetns();
			break;
		case zasm::x86::Mnemonic::Cmovns:
			HandleCmovns();
			break;
		case zasm::x86::Mnemonic::Cmovnb:
			HandleCmovnb();
			break;
		case zasm::x86::Mnemonic::Cmovp:
			 HandleCmovp();
			break;
		case zasm::x86::Mnemonic::Setl:
			HandleSetl();
			break;
		case zasm::x86::Mnemonic::Cmovbe:
			HandleCmovbe();
			break;
		case zasm::x86::Mnemonic::Setnp:
			HandleSetnp();
			break;
		case zasm::x86::Mnemonic::Seto:
			HandleSeto();
			break;
		case zasm::x86::Mnemonic::Cmovnp:
			HandleCmovnp();
			break;
		case zasm::x86::Mnemonic::Cmovno:
			HandleCmovno();
			break;
		case zasm::x86::Mnemonic::Test:
			HandleTest();
			break;
		case zasm::x86::Mnemonic::Cmovz:
			HandleCmovz();
			break;
		case zasm::x86::Mnemonic::Cmovs:
			HandleCmovs();
			break;
		case zasm::x86::Mnemonic::Cmovnl:
			HandleCmovnl();
			break;
		case zasm::x86::Mnemonic::Cmovnbe:
			HandleCmovnbe();
			break;
		case zasm::x86::Mnemonic::Cmovle:
			HandleCmovle();
			break;
		case zasm::x86::Mnemonic::Cmovnle:
			HandleCmovnle();
			break;
		case zasm::x86::Mnemonic::Cmovo:
			HandleCmovo();
			break;
		default:
			printf("FAILED");
			exit(0);
			break;
		}
		count++;
		eip += instruction.getLength();
		rva = eip - (uintptr_t)data.data();

		if (isStop)
			return;
	}
	
}

EmulatorCPU::EmulatorCPU() 
{
	decoder = new zasm::Decoder(zasm::MachineMode::AMD64);

	isStop = false;

	ZF = 0, PF = 0, AF = 0, OF = 0, CF = 0, SF = 0;
}


void EmulatorCPU::reg_write(zasm::Reg reg,uintptr_t value)
{

		if (reg.getId() == (zasm::Reg::Id)ZYDIS_REGISTER_FLAGS)
		{
			rFlags = value;
			return;
		}
		if (reg.getId() == (zasm::Reg::Id)ZYDIS_REGISTER_GS) {
			uintptr_t*  reg = &regs[Registers::GS];
			*reg = value;
			return;
		}
	auto gpRegister = reg.as<zasm::x86::Gp>().r64();

	const auto zy_reg = (ZydisRegister_)gpRegister.getId();

	uintptr_t* wreg = nullptr;
	switch (zy_reg)
	{
	case ZYDIS_REGISTER_RAX:
		wreg = &regs[Registers::RAX];
		break;
	case ZYDIS_REGISTER_RBX:
		wreg = &regs[Registers::RBX];
		break;
	case ZYDIS_REGISTER_RCX:
		wreg = &regs[Registers::RCX];
		break;
	case ZYDIS_REGISTER_RDX:
		wreg = &regs[Registers::RDX];
		break;
	case ZYDIS_REGISTER_RBP:
		wreg = &regs[Registers::RBP];
		break;
	case ZYDIS_REGISTER_RSP:
		wreg = &regs[Registers::RSP];
		break;
	case ZYDIS_REGISTER_RSI:
		wreg = &regs[Registers::RSI];
		break;
	case ZYDIS_REGISTER_RDI:
		wreg = &regs[Registers::RDI];
		break;
	case ZYDIS_REGISTER_R8:
		wreg = &regs[Registers::R8];
		break;
	case ZYDIS_REGISTER_R9:
		wreg = &regs[Registers::R9];
		break;
	case ZYDIS_REGISTER_R10:
		wreg = &regs[Registers::R10];
		break;
	case ZYDIS_REGISTER_R11:
		wreg = &regs[Registers::R11];
		break;
	case ZYDIS_REGISTER_R12:
		wreg = &regs[Registers::R12];
		break;
	case ZYDIS_REGISTER_R13:
		wreg = &regs[Registers::R13];
		break;
	case ZYDIS_REGISTER_R14:
		wreg = &regs[Registers::R14];
		break;
	case ZYDIS_REGISTER_R15:
		wreg = &regs[Registers::R15];
		break;
	default:
		printf("unknown register reg_write\n");
		exit(0);
		break;
	}

	if (reg.isGp8Lo())
	{

		*wreg = (*wreg & 0xFFFFFFFFFFFFFF00) | (value & 0xFF);
	}
	else if (reg.isGp8Hi())
	{

		*wreg = (*wreg & 0xFFFFFFFFFFFF00FF) | ((value << 8) & 0xFF00);
	}
	else if (reg.isGp16())
	{

		*wreg = (*wreg & 0xFFFFFFFFFFFF0000) | (value & 0x000000000000FFFF);
	}
	else if (reg.isGp32())
	{

		*wreg = value & 0x00000000FFFFFFFF;
	}
	else {

		*wreg = value;
	}
}

uintptr_t EmulatorCPU::reg_read( zasm::Reg reg)
{
		if (reg.getId() == (zasm::Reg::Id)ZYDIS_REGISTER_FLAGS)
		{
			return rFlags;
		}
		if (reg.getId() == (zasm::Reg::Id)ZYDIS_REGISTER_GS) {
			uintptr_t reg = regs[Registers::GS];
			return reg;
		}

	const auto gpRegister = reg.as<zasm::x86::Gp>().r64();

	const auto zy_reg = (ZydisRegister_)gpRegister.getId();

	uintptr_t wreg;
	switch (zy_reg)
	{
	case ZYDIS_REGISTER_RAX:
		wreg = regs[Registers::RAX];
		break;
	case ZYDIS_REGISTER_RBX:
		wreg = regs[Registers::RBX];
		break;
	case ZYDIS_REGISTER_RCX:
		wreg = regs[Registers::RCX];
		break;
	case ZYDIS_REGISTER_RDX:
		wreg = regs[Registers::RDX];
		break;
	case ZYDIS_REGISTER_RBP:
		wreg = regs[Registers::RBP];
		break;
	case ZYDIS_REGISTER_RSP:
		wreg = regs[Registers::RSP];
		break;
	case ZYDIS_REGISTER_RSI:
		wreg = regs[Registers::RSI];
		break;
	case ZYDIS_REGISTER_RDI:
		wreg = regs[Registers::RDI];
		break;
	case ZYDIS_REGISTER_R8:
		wreg = regs[Registers::R8];
		break;
	case ZYDIS_REGISTER_R9:
		wreg = regs[Registers::R9];
		break;
	case ZYDIS_REGISTER_R10:
		wreg = regs[Registers::R10];
		break;
	case ZYDIS_REGISTER_R11:
		wreg = regs[Registers::R11];
		break;
	case ZYDIS_REGISTER_R12:
		wreg = regs[Registers::R12];
		break;
	case ZYDIS_REGISTER_R13:
		wreg = regs[Registers::R13];
		break;
	case ZYDIS_REGISTER_R14:
		wreg = regs[Registers::R14];
		break;
	case ZYDIS_REGISTER_R15:
		wreg = regs[Registers::R15];
		break;
	default:
		printf("unknown register reg_read\n");
		exit(0);
		break;
	}

	if (reg.isGp8Lo())
	{
		return wreg & 0xFF;
	}
	else if (reg.isGp8Hi())
	{
		return (wreg & 0xFF00) >> 8;
	}
	else if (reg.isGp16())
	{
		return wreg & 0xFFFF;
	}
	else if (reg.isGp32())
	{

		return wreg & 0x00000000FFFFFFFF;
	}
	else {

		return wreg;
	}
}

void EmulatorCPU::addCallback(callbackFunction callback,void* userData)
{
	callbacks.push_back({userData,callback});
}

void EmulatorCPU::removeCallback(callbackFunction callbackDelete)
{
	for (auto it = callbacks.begin(); it != callbacks.end(); it++) {
		if (it->callback_fn == callbackDelete) {
			callbacks.erase(it);
			break;
		}
	}
}

void EmulatorCPU::stop_emu()
{
	isStop = true;
}

bool EmulatorCPU::mem_map(uintptr_t address, uintptr_t size)
{
	for (auto& memRegion : memory_regions) {
		if (address >= memRegion.address && address <= memRegion.address + memRegion.size) {
			throw std::runtime_error("Error memory region is already mapped in this range");
		}
	}	

	MemoryRegion mr;
	mr.address = address;
	mr.size = size;
	mr.memory.resize(size);

	memory_regions.push_back(mr);
	return true;
}


bool EmulatorCPU::mem_unmap(uintptr_t address)
{
	for (auto it = memory_regions.begin(); it != memory_regions.end();it++) {
		if (address >= it->address && address <= it->address + it->size) {
			memory_regions.erase(it);
			return true;
		}
	}

	throw std::runtime_error("Error!There is no memory region mapped in this range");
}







