///////////////////////////////////////////////////////////////////////////////
// 
// Generate Loop Area for the code
// by @YRY
// 
///////////////////////////////////////////////////////////////////////////////
#include "../include/cfg.h"

#ifndef LOOP_H
#define LOOP_H

#define MAX_INSIDE 10

struct LOOP_T;
typedef struct LOOP_T{
    Func_T      *func;
    int         id;
    uint64_t    start_addr;
    uint64_t    end_addr;
    LOOP_T      *inside[MAX_INSIDE];
    int         num; 
} LOOP;

void  gen_loop();
static bool  isBranch(cs_insn ins);
static LOOP* isLoop(uint64_t jump, uint64_t addr, Func *func);
static void  loop_append(LOOP *temp);
static void  LoopisFull(int num, int *max, LOOP **list);
static void  count_nested();
static void  dump_loop();
const LOOP** get_loop(void);
const int*   get_loop_num(void);
void testloop(void);

#endif