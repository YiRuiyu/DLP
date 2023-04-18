///////////////////////////////////////////////////////////////////////////////
// 
// Generate PCG
// by @YRY 21/2/2023
// 
///////////////////////////////////////////////////////////////////////////////
#include "../include/cfg.h"
#include "../include/Disasm.h"
#include "../include/loop.h"

#ifndef PCG_H
#define PCG_H

typedef enum SRC_TYPE{ LoopConst = 1, CurrIter, PrevIter, PrevSelf, AddrInduct, NonConsume, NOTFIND, LW} srctype;

typedef struct PCG_LINK_T{
    int             block;                      //priducer instuctions block index
    int             id;                         //producer instruction index or non-consumed instruction index
    cs_riscv_op    *op;                         //procuder                   or non-consumed producer
    srctype         type;                       //link type                  or non-consumed hint
    PCG_LINK_T     *next;                       //next possible producer
}PCG_LINK;


typedef struct PCG_BLOCK_T{
    CFG_Node        *node;                       //pointer to block
    PCG_LINK       **producer;                   //producer list which is a 2D array storing pointer to PCG_LINK
    int              num_prod;                   //not used
    PCG_LINK        *non_consume;                //none consumed list not used yet
    int              num_non;
}PCG_BLOCK;


typedef struct PCG_FUNC_T{
    Func_T          *func;                      //pointer to func
    PCG_BLOCK_T     *blocks;                    //PCG block list
}PCG_FUNC;



void gen_pcg(cs_insn *insn);
static void gen_func_pcg(PCG_FUNC *pcg_func, cs_insn *insn);
static void in_block_link(PCG_BLOCK *block, cs_insn *insn);
static void pre_block_link(PCG_FUNC *pcg_func, PCG_BLOCK *block, cs_insn *insns);
static void aft_block_link(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns);
static void get_looped_func(PCG_FUNC **pcg_list, const LOOP **loops);
static void find_nested_loop(const LOOP **nested_loops, int *num_nested, const LOOP *loop);
static void sort_loop(const LOOP **list, int len);
static bool check_duplicate(const LOOP** nested_loops, int *num_nested, const LOOP* temp_loop);
static void find_in_produce(PCG_BLOCK *block, cs_insn *insns, int base, int offset, int pos);
static void find_out_produce(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, int *pre_block_list, int num_list, cs_insn *this_ins, int base, int offset, int pos);
static bool is_compute(int opcount);
static bool is_auipc(int opcount,int id);
static bool is_mem(int opcount,int id);
static int isMissing(PCG_LINK **producers, int index, cs_insn *this_ins);
static int find_pre_block(int **pre_block_list, PCG_BLOCK *block);
static int find_post_block(int **post_block_list,PCG_FUNC *pcg_func, PCG_BLOCK *block);
static bool cmp_op(PCG_LINK *link, cs_insn *ins,int position);
static void init(PCG_BLOCK* pcg_block);
static void dump_nested();
static void dump_pcg_func_list();
static void dump_produce();
static void dump_pre_block(CFG_Node *node, int *list, int num);
void testpcg();
//void aft_block_link(...);

#endif