//////////////////////////////////////////////////////////////////////////////////////////
// The main idea of pallable.cpp is designed for tell if each instruction can be set as Parallelizable
// 
//
// First, determine all computing instruction if they are parallelizable.
// 

// by @YRY 24/4/2023
// 
//////////////////////////////////////////////////////////////////////////////////////////
#include "../include/Disasm.h"

typedef enum palltype{ FullPara = 1, TreePara, AddrPara } ptype;

bool de_computing(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns);
void del(int i);
bool is_com_pallable(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, int offset, int flag);
bool de_memory(PCG_FUNC *pcg_func, LOOP *loop, cs_insn *insns);
void tag(cs_insn *insns);
void testpall(void);