///////////////////////////////////////////////////////////////////////////////
// 
// Generate Loop Area for the code
// by @YRY
// 
///////////////////////////////////////////////////////////////////////////////

#include "../include/Disasm.h"
#define MAX_INSIDE 10

struct LOOP_T;
typedef struct LOOP_T{
    int         id;
    uint64_t    start_addr;
    uint64_t    end_addr;
    LOOP_T    *inside[MAX_INSIDE];
    int         num; 
} LOOP;

static void gen_loop(cs_insn *insn, int count);
static bool isBranch(cs_insn ins);
static LOOP* isLoop(uint64_t jump, uint64_t addr);
static void loop_append(LOOP *temp);
static void LoopisFull(int num, int *max, LOOP **list);
static void count_nested();
static void dump_loop();
void testloop(void);