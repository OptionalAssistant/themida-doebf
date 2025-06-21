#include "pch.h"

#include "SimplifyStackOperation.h"
#include "Helpers/HelpersIterations.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"


//Simplify push reg,pop reg -> mov
static bool PushPopToMove(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    auto& zydis_instr = instruction.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Push) {
        return false;
    }
   
    auto itNext = nextIter(it,instructions.end());

    if (itNext == instructions.end())
        return false;

    auto& nextInstruction = *itNext;
    if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop)
        return false;

    /*printf("Simplify push pop to move : %d \n", instruction.getCount());*/


    auto newZasmInstruction = createMov(nextInstruction.getZasmInstruction().getOperand(0),
        zydis_instr.getOperand(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);
    newInstruction.setRegisterValues(instruction.getRegistersArray());

    newInstruction.addOperand(nextInstruction.getOperand(0));
    newInstruction.addOperand(instruction.getOperand(0));

    newInstruction.setCount(countGlobal++);

    instructions.insert(itNext, newInstruction);

 /*   printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());*/

    instructions.erase(itNext);
    it = instructions.erase(it);

    return true;
}
//Simplify mov rax,[rsp] add rsp,8  -> pop rax
static bool MovMemAddRspToPop(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
        instruction.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp) {
        return false;
    }

    auto itNext = nextIter(it, instructions.end());

    if (itNext == instructions.end()) {
        return false;
    }

    auto& nextInstruction = *itNext;

    if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Add ||
        !nextInstruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
        !nextInstruction.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        (nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() != 8 &&
            nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() != 2)) {
        return false;
    }
    uintptr_t immValue = nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();

    /*printf("Simplify add rsp,8 to pop count: %d\n", instruction.getCount());*/


    auto newZasmInstruction = createPop(instruction.getZasmInstruction().getOperand(0),
        nextInstruction.getZasmInstruction().getOperand(0), instruction.getZasmInstruction().getOperand(1));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);
    newInstruction.setRegisterValues(instruction.getRegistersArray());

    newInstruction.addOperand(instruction.getOperand(0));
    newInstruction.addOperand(nextInstruction.getOperand(0));
    newInstruction.addOperand(instruction.getOperand(1));

    newInstruction.setCount(countGlobal++);

    immValue -= nextInstruction.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    nextInstruction.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
    nextInstruction.setOperand(1, new BaseOperand(zasm::Imm(immValue)));

    //printf("Generated new instruction: %s count: %d\n",
    //    formatInstruction(newInstruction.getZasmInstruction()).c_str(),
    //    newInstruction.getCount());

    instructions.insert(itNext, newInstruction);

    instructions.erase(itNext);
    it = instructions.erase(it);

    return true;
}
//Simplify sub rsp,8 mov [rsp],reg -> push reg
static bool SubRspMovMemToPush(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;

  
    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Sub ||
        !instruction1.getZasmInstruction().getOperand(0).holds<zasm::Reg>() ||
        instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>() != zasm::x86::rsp ||
        !instruction1.getZasmInstruction().getOperand(1).holds<zasm::Imm>() ||
        instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>() < 8) {
        return false;
    }

    uintptr_t immValue = instruction1.getZasmInstruction().getOperand(1).get<zasm::Imm>().value<uintptr_t>();
    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId()
        != zasm::Reg::Id::None) {
        return false;
    }

    /*printf("Found sub rsp,immValue mov mem.Optimize to push at count: %d\n", instruction2.getCount());*/

    auto newZasmInstruction = createPush(instruction2.getZasmInstruction().getOperand(1),
        instruction1.getZasmInstruction().getOperand(0), instruction2.getZasmInstruction().getOperand(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);
    newInstruction.setRegisterValues(instruction1.getRegistersArray());

    newInstruction.addOperand(instruction2.getOperand(1));
    newInstruction.addOperand(instruction1.getOperand(0));
    newInstruction.addOperand(instruction2.getOperand(0));

    immValue -= 8;
    instruction1.getZasmInstruction().setOperand(1, zasm::Imm(immValue));
    instruction1.setOperand(1, new BaseOperand(zasm::Imm(immValue)));


    newInstruction.setCount(countGlobal++);

  /*  printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());*/

    if (instruction2.getZasmInstruction().getOperand(1).holds<zasm::Reg>() &&
        instruction2.getZasmInstruction().getOperand(1).get<zasm::Reg>() == zasm::x86::rsp) {
        
        auto newZasmInstruction2 = createSub(instruction2.getZasmInstruction().getOperand(0),
            zasm::Imm(8));

        Instruction newInstruction2;
        newInstruction2.setZasmInstruction(newZasmInstruction2);
        newInstruction.setRegisterValues(instruction2.getRegistersArray());

        newInstruction2.addOperand(instruction2.getOperand(0));
        newInstruction2.addOperand(new BaseOperand(zasm::Imm(8)));
        
        newInstruction2.setCount(countGlobal++);


      /*  printf("Generated new instruction2: %s count: %d\n",
            formatInstruction(newInstruction2.getZasmInstruction()).c_str(),
            newInstruction2.getCount());*/
        
        instructions.insert(it2, newInstruction);
        instructions.insert(it2, newInstruction2);
        

        instructions.erase(it2);
        it = instructions.erase(it);

        return true;
    }

    instructions.insert(it2, newInstruction);

    instructions.erase(it2);
    it = instructions.erase(it);

    return true;
}

//push rax xor dword ptr:[rsp],12345 pop rbx --> mov ebx,eax xor ebx,12345 TODO Handle registers change
static bool OptimizePass1(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction1 = *it;


    if (instruction1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push) {
        return false;
    }

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end()) {
        return false;
    }

    auto& instruction2 = *it2;

    if (!instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0 ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        instruction2.getZasmInstruction().getOperandAccess(0) != zasm::detail::OperandAccess::ReadWrite) {
        return false;
    }


    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end()) {
        return false;
    }

    auto& instruction3 = *it3;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop) {
        return false;
    }

    if (instruction1.getZasmInstruction().getOperand(0) ==
        instruction3.getZasmInstruction().getOperand(0)) {
        if (instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBitSize() == zasm::BitSize::_64) {
            instruction2.getZasmInstruction().setOperand(0, instruction1.getZasmInstruction().getOperand(0));
        }
        else if (instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBitSize() == zasm::BitSize::_32) {

            auto& gpRegister1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();
            auto& gpRegister2 = instruction3.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();

            instruction2.getZasmInstruction().setOperand(0, gpRegister1.r32());
        }
        else
            throw std::runtime_error("Unknown error....\n");

        instructions.erase(it);
        instructions.erase(it3);
        return true;
    }

  /*  printf("Found unopyimize operation throgh push pop and op between: %s count: %d\n",
        formatInstruction(instruction1.getZasmInstruction()).c_str(),
        instruction1.getCount());*/

    zasm::InstructionDetail newZasmInstruction2;
    if (instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBitSize() == zasm::BitSize::_64) {
        newZasmInstruction2 = createMov(instruction3.getZasmInstruction().getOperand(0),
            instruction1.getZasmInstruction().getOperand(0));
    }
    else if (instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBitSize() == zasm::BitSize::_32) {
       
        auto& gpRegister1 = instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();
        auto& gpRegister2 = instruction3.getZasmInstruction().getOperand(0).get<zasm::Reg>().as<zasm::x86::Gp>();

        newZasmInstruction2 = createMov(gpRegister2.r32(),
            gpRegister1.r32());
    }
    else if (instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBitSize() == zasm::BitSize::_16) {
        newZasmInstruction2 = createMov(instruction3.getZasmInstruction().getOperand(0),
            instruction1.getZasmInstruction().getOperand(0));
    }
    else {
        throw std::runtime_error("Error during iptimization wtf consider different variations");
    }


    Instruction newInstruction2;
    newInstruction2.setZasmInstruction(newZasmInstruction2);
    newInstruction2.setRegisterValues(instruction1.getRegistersArray());

    newInstruction2.addOperand(instruction3.getOperand(0));
    newInstruction2.addOperand(instruction1.getOperand(0));

    newInstruction2.setCount(countGlobal++);

    instruction2.setOperand(0, newInstruction2.getOperand(0));
    instruction2.getZasmInstruction().setOperand(0, newInstruction2.getZasmInstruction().getOperand(0));
    
    //if (instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
    //    auto& regArray = instruction2.getRegistersArray();

    //    EmulatorCPU::Registers regNumber;

    //    //mov ebx,eax xor ebx,123456
    //    regNumber = zasmToEmulatorRegister(
    //        (ZydisRegister_)instruction3.getZasmInstruction().getOperand(0).get<zasm::Reg>().getId());

    //    regArray[regNumber] = reg_read_(instruction1.getRegistersArray(),
    //        instruction1.getZasmInstruction().getOperand(0).get<zasm::Reg>(), instruction1.getRflags());
    //}
    
    //printf("Generated new instruction: %s count: %d\n",
    //    formatInstruction(newInstruction2.getZasmInstruction()).c_str(),
    //    newInstruction2.getCount());

    //printf("Generated new instruction(2): %s count: %d\n",
    //    formatInstruction(instruction2.getZasmInstruction()).c_str(),
    //    instruction2.getCount());

    instructions.insert(it, newInstruction2);

    instructions.erase(it3);
    it = instructions.erase(it);

    return true;
}
/*
push r10
push qword ptr ss:[rsp+0x08]
pop r10
pop qword ptr ss:[rsp]
----------->
xchg [rsp],r10
*/
static bool OptimizePass2(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;


    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push)
        return false;

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end())
        return false;

    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push)
        return false;

    auto it3 = nextIter(it2, instructions.end());
    
    if (it3 == instructions.end())
        return false;
    
    auto& instruction3 = *it3;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop)
        return false;

    auto it4 = nextIter(it3, instructions.end());

    if (it4 == instructions.end())
        return false;

    auto& instruction4 = *it4;

    if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop)
        return false;

    if ((instruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>() &&
        instruction.getZasmInstruction().getOperand(0) != instruction3.getZasmInstruction().getOperand(0)) ||
        (instruction2.getZasmInstruction().getOperand(0).holds<zasm::Reg>() &&
            instruction2.getZasmInstruction().getOperand(0) != instruction4.getZasmInstruction().getOperand(0)))
        return false;

    if (instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() &&
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp &&
        instruction2.getZasmInstruction().getOperand(0) != instruction4.getZasmInstruction().getOperand(0))
        return false;

    if (instruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>() &&
        instruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp &&
        instruction.getZasmInstruction().getOperand(0) != instruction3.getZasmInstruction().getOperand(0))
        return false;

    if (instruction2.getZasmInstruction().getOperand(0).holds<zasm::Mem>() &&
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() == zasm::x86::rsp &&
        (instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() < 8 ||
        instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::Reg::Id::None ||
         instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() !=
         instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() ||
         instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex() !=
         instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex() ||
         instruction2.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() !=
         instruction4.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() + 8))
        return false;
    
    auto newZasmInstruction = createXchg(instruction3.getZasmInstruction().getOperand(0),
        instruction4.getZasmInstruction().getOperand(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);
    newInstruction.setRegisterValues(instruction.getRegistersArray());

    newInstruction.addOperand(instruction3.getOperand(0));
    newInstruction.addOperand(instruction4.getOperand(0));

    newInstruction.setCount(countGlobal++);

  /*  printf("Generated new instruction: %s count: %d\n",
        formatInstruction(newInstruction.getZasmInstruction()).c_str(),
        newInstruction.getCount());*/

    instructions.insert(it4, newInstruction);
    
    instructions.erase(it4);
    instructions.erase(it3);
    instructions.erase(it2);
    instructions.erase(it);

    return true;
}
/*
exchange pushed values 
 push rax
 push rbx
 mov rax, qword ptr ss:[rsp+0x20]
 mov rbx, qword ptr ss:[rsp+0x10]
 mov qword ptr ss:[rsp+0x10], rax
 mov qword ptr ss:[rsp+0x20], rbx
 pop rbx
 pop rax
*/
static bool OptimizePass3(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push)
        return false;

    auto it2 = nextIter(it, instructions.end());

    if (it2 == instructions.end())
        return false;

    
    auto& instruction2 = *it2;

    if (instruction2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push)
        return false;

    auto it3 = nextIter(it2, instructions.end());

    if (it3 == instructions.end())
        return false;

    auto& instruction3 = *it3;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov)
        return false;


    auto it4 = nextIter(it3, instructions.end());

    if (it4 == instructions.end())
        return false;

    auto& instruction4 = *it4;

    if (instruction4.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov)
        return false;



    auto it5 = nextIter(it4, instructions.end());

    if (it5 == instructions.end())
        return false;

    auto& instruction5 = *it5;

    if (instruction5.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov)
        return false;


    auto it6 = nextIter(it5, instructions.end());

    if (it6 == instructions.end())
        return false;

    auto& instruction6 = *it6;

    if (instruction6.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov)
        return false;

     auto it7 = nextIter(it6, instructions.end());

    if (it7 == instructions.end())
        return false;

    auto& instruction7 = *it7;

    if (instruction7.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop)
        return false;

    auto it8 = nextIter(it7, instructions.end());

    if (it8 == instructions.end())
        return false;

    auto& instruction8 = *it8;

    if (instruction8.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop)
        return false;
    
    if (instruction2.getZasmInstruction().getOperand(0) != instruction7.getZasmInstruction().getOperand(0) ||
        instruction.getZasmInstruction().getOperand(0) != instruction8.getZasmInstruction().getOperand(0))
        return false;

    if (!instruction3.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
        instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::Reg::Id::None ||
        (instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0x10 &&
         instruction3.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0x20))
        return false;

    if (!instruction4.getZasmInstruction().getOperand(1).holds<zasm::Mem>() ||
        instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getIndex().getId() != zasm::Reg::Id::None ||
        (instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0x10 &&
        instruction4.getZasmInstruction().getOperand(1).get<zasm::Mem>().getDisplacement() != 0x20))
        return false;

    if (instruction3.getZasmInstruction().getOperand(1) == instruction4.getZasmInstruction().getOperand(1))
        return false;

    if (!instruction5.getZasmInstruction().getOperand(1).holds<zasm::Reg>() ||
        !instruction6.getZasmInstruction().getOperand(1).holds<zasm::Reg>())
        return false;

    if((instruction5.getZasmInstruction().getOperand(1) != instruction3.getZasmInstruction().getOperand(0) &&
      instruction5.getZasmInstruction().getOperand(1) != instruction4.getZasmInstruction().getOperand(0)) ||
      (instruction6.getZasmInstruction().getOperand(1) != instruction3.getZasmInstruction().getOperand(0) &&
      instruction6.getZasmInstruction().getOperand(1) != instruction4.getZasmInstruction().getOperand(0)) ||
      instruction5.getZasmInstruction().getOperand(1) == instruction6.getZasmInstruction().getOperand(1))
        return false;

    if (!instruction5.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        !instruction6.getZasmInstruction().getOperand(0).holds<zasm::Mem>())
        return false;

    if ((instruction5.getZasmInstruction().getOperand(0) != instruction3.getZasmInstruction().getOperand(1) &&
        instruction5.getZasmInstruction().getOperand(0) != instruction4.getZasmInstruction().getOperand(1)) ||
        (instruction6.getZasmInstruction().getOperand(0) != instruction3.getZasmInstruction().getOperand(1) &&
        instruction6.getZasmInstruction().getOperand(0) != instruction4.getZasmInstruction().getOperand(1)) ||
        instruction5.getZasmInstruction().getOperand(0) == instruction6.getZasmInstruction().getOperand(0))
        return false;


   

    MemoryOperand* memoryOp1 = dynamic_cast<MemoryOperand*>(instruction3.getOperand(1));
    MemoryOperand* memoryOp2 = dynamic_cast<MemoryOperand*>(instruction4.getOperand(1));

    auto memoryWrite1 = getPrevMemoryWrite(it, instructions.begin(), memoryOp1->getMemoryAddress());
    auto memoryWrite2 = getPrevMemoryWrite(it, instructions.begin(), memoryOp2->getMemoryAddress());

    auto elem1 = *memoryWrite1;
    auto elem2 = *memoryWrite2;

    if ((elem1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push &&
        elem1.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pushfq) ||
        (elem2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push &&
            elem2.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pushfq))
        return false;

    if(elem1.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pushfq)
      dynamic_cast<MemoryOperand*>(elem1.getOperand(1))->setMemoryAddress(memoryOp2->getMemoryAddress());
    else
      dynamic_cast<MemoryOperand*>(elem1.getOperand(2))->setMemoryAddress(memoryOp2->getMemoryAddress());

    if (elem2.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pushfq)
        dynamic_cast<MemoryOperand*>(elem2.getOperand(1))->setMemoryAddress(memoryOp1->getMemoryAddress());
    else
        dynamic_cast<MemoryOperand*>(elem2.getOperand(2))->setMemoryAddress(memoryOp1->getMemoryAddress());

    //printf("Found this biiggg pattern at count: %d\n", instruction.getCount());

    instructions.insert(memoryWrite1, elem2);
    instructions.insert(memoryWrite2, elem1);

    instructions.erase(memoryWrite1);
    instructions.erase(memoryWrite2);

    instructions.erase(it);
    instructions.erase(it2);
    instructions.erase(it3);
    instructions.erase(it4);
    instructions.erase(it5);
    instructions.erase(it6);
    instructions.erase(it7);
    instructions.erase(it8);

    return true;
}

//Push dead memory e.g push rax mov [rsp],rbx -> push rbx
static bool OptimizeDeadMemoryPush(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    auto& zydis_instr = instruction.getZasmInstruction();

    if (zydis_instr.getMnemonic() != zasm::x86::Mnemonic::Push) {
        return false;
    }
    auto itNext = nextIter(it, instructions.end());

    if (itNext == instructions.end())
        return false;

    auto& nextInstruction = *itNext;

    if (nextInstruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Mov ||
        !nextInstruction.getZasmInstruction().getOperand(0).holds<zasm::Mem>() ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getBase() != zasm::x86::rsp ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getIndex().getId() != zasm::x86::Reg::Id::None ||
        nextInstruction.getZasmInstruction().getOperand(0).get<zasm::Mem>().getDisplacement() != 0) {
        return false;
    }


 /*   printf("Found dead memory memory push at count : %d\n", instruction.getCount());*/

    auto newZasmInstruction = createPush(nextInstruction.getZasmInstruction().getOperand(1),
        instruction.getZasmInstruction().getOperand(0), nextInstruction.getZasmInstruction().getOperand(0));

    Instruction newInstruction;
    newInstruction.setZasmInstruction(newZasmInstruction);
    newInstruction.setCount(countGlobal++);
    newInstruction.setRegisterValues(instruction.getRegistersArray());

    newInstruction.addOperand(nextInstruction.getOperand(1));
    newInstruction.addOperand(instruction.getOperand(0));
    newInstruction.addOperand(nextInstruction.getOperand(0));


    //printf("Generated new instruction: %s count: %d\n",
    //    formatInstruction(newInstruction.getZasmInstruction()).c_str(),
    //    newInstruction.getCount());

    instructions.insert(itNext, newInstruction);

    instructions.erase(itNext);
    it = instructions.erase(it);

    return true;
}

/*
push rax
xor ecx,0x12234554
pop rax
*/
static bool OptimizePass4(std::list<Instruction>::iterator it, std::list<Instruction>& instructions) {
    auto& instruction = *it;

    if (instruction.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Push ||
        !instruction.getZasmInstruction().getOperand(0).holds<zasm::Reg>()) {
        return false;
    }

    auto itFoundPop = std::find_if(it, instructions.end(), [](Instruction& instruction) {
        return instruction.getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop;
        });

    if (itFoundPop == instructions.end())
        return false;

    auto& instruction3 = *itFoundPop;

    if (instruction3.getZasmInstruction().getMnemonic() != zasm::x86::Mnemonic::Pop ||
        !instruction3.getZasmInstruction().getOperand(0).holds<zasm::Reg>())
        return false;

    if (instruction3.getZasmInstruction().getOperand(0) != instruction.getZasmInstruction().getOperand(0))
        return false;

   auto isRegisterWritePresent = getNextRegisterWrite(std::next(it), itFoundPop,
       instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());

 
   if (isRegisterWritePresent != itFoundPop) {
       return false;
   }

   auto isRegisterReadWritePresent = getNextRegisterReadWrite(std::next(it), itFoundPop,
       instruction.getZasmInstruction().getOperand(0).get<zasm::Reg>());


   if (isRegisterReadWritePresent != itFoundPop) {
       return false;
   }

   auto regRsp = zasm::x86::rsp;
   auto isRspAccessPresent = getNextRegisterAccess(std::next(it), itFoundPop, regRsp);


   if (isRspAccessPresent != itFoundPop) {
       return false;
   }
  
   instructions.erase(itFoundPop);
   instructions.erase(it);

   return true;
}
bool SimplifyStackOperation::run(std::list<Instruction>::iterator it,std::list<Instruction>& instructions)
{

    if (PushPopToMove(it, instructions))
        return true;
    if (MovMemAddRspToPop(it, instructions))
        return true;
    if (SubRspMovMemToPush(it, instructions))
        return true;
    if (OptimizePass1(it, instructions))
        return true;
    if (OptimizePass2(it, instructions))
        return true;
    if (OptimizePass3(it, instructions))
        return true;
    if (OptimizeDeadMemoryPush(it, instructions))
        return true;
    if (OptimizePass4(it, instructions))
        return  true;

    return false; 
}
