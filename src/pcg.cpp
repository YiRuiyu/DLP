//////////////////////////////////////////////////////////////////////////////////////////
// The main idea of PCG.cpp is to analyze loops to implement 
// Production and Consumption relationship.
//
// First, we do analysis on basic blocks finding producer for each instructions.
// Like, generating the producers list for instructions in the block
// 
// Then, for dealing with the instructions which has not producers in the block,
// backtracing the cfg(cfg.c & cfg.h) is a neccessay way to find out-block producers.
//
// Finally, still none-producers instructions find their producers from block following to
// it(previous loop producers).
//
// Generate PCG
// by @YRY 21/2/2023
// 
//////////////////////////////////////////////////////////////////////////////////////////
//TODO: Dumpfiles of inblock and outblock are wrong.
//      cfg did not consider ret as function end.

#include "../include/pcg.h"
#include "../include/BinaryCode.h" //only for testing

#define MAX_NESTED 30
#define MAX_PRE_BLOCK 9

static const int       *num_funcs = get_func_num();
static const int       *num_loops = get_loop_num();
static const Func      **func_list = get_func();
static const LOOP      **loop_list = get_loop();

//-----------------------------------------------------------------------------
// Part 0: Global variables 
// Those invisible to other modules should be guarded with static keyword
//-----------------------------------------------------------------------------
static int              num_nested;
static const LOOP     **nested_loops;
static int              num_pcg_func;
static PCG_FUNC        *pcg_func_list;
//-----------------------------------------------------------------------------
// Part 1: Generate producer list for each block in each function
//
// Input:cs_insn --->instructions
// 
//-----------------------------------------------------------------------------
void gen_pcg(cs_insn *insns)
{
    PCG_FUNC        *pcg_func;
    int i, j, x, y;

    get_looped_func(&pcg_func_list, loop_list);
    for (int i = 0; i < num_pcg_func + 1; i++)
    {
        pcg_func = &(pcg_func_list[i]);
        gen_func_pcg(pcg_func, insns);
    }
    dump_produce();
}


//This function get through all PCG_FUNC and complete the PCG.
static void gen_func_pcg(PCG_FUNC *pcg_func, cs_insn *insns)
{
    int flag;
    PCG_BLOCK   *pcg_block;

    flag = 0;                                   //indicate if loops in the func
    pcg_func->blocks = (PCG_BLOCK*)malloc(sizeof(PCG_BLOCK)*(pcg_func->func->cfg.num_nodes + 1));
    for(int i = 0; i < num_nested + 1; i++)                             //for each nested loop
    {
        if(nested_loops[i]->func != pcg_func->func && flag == 0)
            continue;
        else if(nested_loops[i]->func != pcg_func->func && flag == 1)
            break;

        flag = 1;
        for(int j = nested_loops[i]->src; j <=nested_loops[i]->dst; j++)//for each blocks in loop
        {
            //printf("The curr block is %d\n", j);
            pcg_func->blocks[j].node = &pcg_func->func->cfg.nodes[j];
            pcg_block = &(pcg_func->blocks[j]);
            in_block_link(pcg_block, insns);                                                        //Generate the in-block PCG completing *producer and *non_consume
            pre_block_link(pcg_func, pcg_block, insns, nested_loops[i]->src,nested_loops[i]->dst);  //Fill out-block data in PCG with the help of *non_consume and CFG
            aft_block_link(pcg_func, pcg_block, insns, nested_loops[i]->src,nested_loops[i]->dst);                                           //If the instruction is in a loop, the producer can be generated in last loop which is in the block after it
        }
    }
    
}

//Help function for this Part
//-----------------------------------------------------------------------------
//It will be devided into
//                      in_block_link(PCG_BLOCK *pcg_block, cs_insn *insns)
//                      pre_block_link
//                      art_block_link
//-----------------------------------------------------------------------------
static void in_block_link(PCG_BLOCK *pcg_block, cs_insn *insns)
{
    //Work: 1.Go through all instructions in this block
    //      2.For each instructions: find its producer and its consumer;
    //      3.Store them in PCG_LINK *producer and *non_consume;
    //Hint: instructions can be divided into two parts:
    //      Calculating instructions:       find producers
    //      Memory related instructions:    it is constructed by base and offset.                                       (To be done fisrt)
    //                                      There are three parts to find if a consumer is related to this instruction. (To be finished)
    int          base;
    cs_insn     *this_ins;

    pcg_block->producer=(PCG_LINK**)calloc(pcg_block->node->len,sizeof(PCG_LINK));      //2d array for producers
    for(int i = 0; i <= pcg_block->node->len; i++)
        pcg_block->producer[i] = (PCG_LINK*)calloc(2, sizeof(PCG_LINK));
    
    pcg_block->non_consume=(PCG_LINK*)calloc(pcg_block->node->len,sizeof(PCG_LINK));
    init(pcg_block);                                                                        //set all element as nullptr
    //we divide in block analysis into two parts,computing insn and mem insn
    //analyze computing insn part 
    
    base = (pcg_block->node->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
    for(int offset = pcg_block->node->len - 1; offset >= 0; offset--)                   //From block's bottom traverse to top
    {
        this_ins = &insns[base + offset];
        // printf("The consumer instruction:Base = %d ", base);
        // printf("Offset = %d ", offset);
        // printf("0x%" PRIx64 "\n", this_ins->address);
        if(is_auipc(this_ins->detail->riscv.op_count,this_ins->id))                   //TODO: To be considered
        {
            pcg_block->producer[offset][0].type     = LoopConst;
            pcg_block->producer[offset][1].type     = LoopConst;
            continue;
        }
        else if(is_compute(this_ins->detail->riscv.op_count))                        //Computing instructions(if its operands num is 3)
        {
            for(int k=1; k<=2; k++)                                                 //for rs1 and rs2 
                find_in_produce(pcg_block, insns, base, offset, k);
        }
        //load do not have producers
        else if(this_ins->id == RISCV_INS_LB|| this_ins->id == RISCV_INS_LBU || this_ins->id == RISCV_INS_LD || this_ins->id == RISCV_INS_LH ||this_ins->id == RISCV_INS_LHU || this_ins->id == RISCV_INS_LW|| this_ins->id == RISCV_INS_LWU ||this_ins->id == RISCV_INS_LUI) 
        {  
            pcg_block->producer[offset][0].type     = LW;
            pcg_block->producer[offset][1].type     = LW;
        } 
        //store have producers
        else if(this_ins->id == RISCV_INS_SW|| this_ins->id ==RISCV_INS_SB ||this_ins->id == RISCV_INS_SH)                                                                           
            find_in_produce(pcg_block, insns, base, offset, 0); 
        
    }
}


static void find_in_produce(PCG_BLOCK *pcg_block, cs_insn *insns, int base, int offset, int pos)
{
    cs_insn *this_ins;

    // printf("Base = %d\n", base);
    // printf("Offset = %d\n", offset);
    this_ins = &insns[base + offset];
    if(offset == 0)                                   //which means the first instruction of the block
        if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_IMM)
        {
            pcg_block->producer[offset][pos-1].type     = LoopConst;
            return;
        }
    for(int i = base + offset - 1; i >= base; i--)                                   // -1 is for get rid of this_ins itself
    {
        // printf("\tDuring the finding process:This inctruction base is %d, offset is %d, addr is0x%08lx\n", base, i, insns[i].address);
        // printf("\tThe k value is %d\n", pos);
        
        if(pos == 0)                                    //under SW situation                                                                                                                      //this ins is store instruction
        { 
            //for this_ins, this first half statement is aiming avoiding IMMs and the rest is for avoiding sw/sb/sh which cannot serve as producer, besides the operand cannot be ZERO
            if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_REG && insns[i].id != (RISCV_INS_SW||RISCV_INS_SB||RISCV_INS_SH) && this_ins->detail->riscv.operands[pos].reg == insns[i].detail->riscv.operands[0].reg && this_ins->detail->riscv.operands[pos].reg != 1)
            {
                pcg_block->producer[offset][pos].id       = i - base;
                pcg_block->producer[offset][pos].type     = CurrIter;
                pcg_block->producer[offset][pos].op       = &insns[i].detail->riscv.operands[0];
                break;
            }
        }
        if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_IMM)
        {
            pcg_block->producer[offset][pos-1].type     = LoopConst;
            break;
        }
        else if(insns[i].id == RISCV_INS_LB||insns[i].id == RISCV_INS_LBU||insns[i].id == RISCV_INS_LD||insns[i].id == RISCV_INS_LH||insns[i].id == RISCV_INS_LHU||insns[i].id == RISCV_INS_LW||insns[i].id == RISCV_INS_LWU)                //the produce insn is LW
        {
            if(this_ins->detail->riscv.operands[pos].reg == insns[i].detail->riscv.operands[1].mem.base)
            {
                pcg_block->producer[offset][pos-1].id       = i - base;
                pcg_block->producer[offset][pos-1].type     = AddrInduct;
                pcg_block->producer[offset][pos-1].op       = &insns[i].detail->riscv.operands[0];
                break;
            }
            else if(this_ins->detail->riscv.operands[pos].reg == insns[i].detail->riscv.operands[0].reg)
            {
                pcg_block->producer[offset][pos-1].id       = i - base;
                pcg_block->producer[offset][pos-1].type     = CurrIter;
                pcg_block->producer[offset][pos-1].op       = &insns[i].detail->riscv.operands[0];
                break;
            }
        }
            
        //for this_ins, this first half statement is aiming avoiding IMMs and the rest is for avoiding sw/sb/sh which cannot serve as producer
        else if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_REG && (insns[i].id != RISCV_INS_SW||insns[i].id != RISCV_INS_SB||insns[i].id != RISCV_INS_SH) && this_ins->detail->riscv.operands[pos].reg == insns[i].detail->riscv.operands[0].reg && this_ins->detail->riscv.operands[pos].reg != 1)
        {
            pcg_block->producer[offset][pos-1].id       = i - base;
            pcg_block->producer[offset][pos-1].type     = CurrIter;
            pcg_block->producer[offset][pos-1].op       = &insns[i].detail->riscv.operands[0];
            break;
        }
    }
}


static void init(PCG_BLOCK* pcg_block)
{
    // Initialize producers to nullptr
    for(int i = 0; i < pcg_block->node->len; i++) {
        for(int j = 0; j < 2; j++) {
            pcg_block->producer[i][j].op            = nullptr;
            pcg_block->producer[i][j].block         = -77;
            pcg_block->producer[i][j].id            = -77;
            pcg_block->producer[i][j].type          = NOTFIND;
            pcg_block->producer[i][j].next          = nullptr;
        }
    }

    // Initialize non_consume to nullptr
    for(int i = 0; i < pcg_block->node->len; i++) {
        pcg_block->non_consume[i].op = nullptr;
        pcg_block->non_consume[i].id = i;
        pcg_block->non_consume[i].type = NOTFIND;
    }
}



static void pre_block_link(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns, int src, int dst)
{
    //Work: For each instruction which has missing producers
    //      Search its producers in *non_consume of previous block on the CFG
    int         *pre_block_list;
    int          num_list;
    int          base;
    PCG_BLOCK   *pre_block;
    cs_insn     *this_ins;                                                                  //current instruciton
    
    // printf("The block id is %d\n", pcg_block->node->id);
    pre_block_list = (int*)malloc(MAX_PRE_BLOCK * sizeof(base));
    num_list = find_pre_block(&pre_block_list, pcg_block);
    dump_pre_block(pcg_block->node, pre_block_list, num_list);
    base = (pcg_block->node->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
    for(int offset = 0; offset < pcg_block->node->len - 1; offset++)                                               //for each instruction
    {
        this_ins = &insns[base + offset];
        // printf("\tThe consumer instruction:Base = %d ", base);
        // printf("\tOffset = %d ", offset);
        // printf("\t0x%" PRIx64 "\n", this_ins->address);
        switch(isMissing(pcg_block->producer, offset, this_ins))                                               //have not found producer in in-block checking
        {
            case 0:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, insns, base, offset, 0, src, dst);
                break;
            case 1:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, insns, base, offset, 1, src, dst);
                break;
            case 2:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, insns, base, offset, 2, src, dst);
                break;
            case 12:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, insns, base, offset, 1, src, dst);
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, insns, base, offset, 2, src, dst);
                break;
            default: continue;
        }
    }
    free(pre_block_list);
    pre_block_list = nullptr;
}


static void find_out_produce(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, int *pre_block_list, int num_list, cs_insn *insns, int base, int offset, int pos, int src, int dst)
{
    int          i,j;
    int          index, pb_base, pb_offset;
    cs_insn     *this_ins;
    CFG_Node    *pre_block;
    PCG_LINK    *new_link;

    this_ins = &insns[base + offset];
    for(i = 0; i < num_list + 1; i++)                                   //for each pre block
    {
        index = pre_block_list[i];
        // printf("\t\tThe block id is %d\n", index);
        pre_block = &pcg_func->func->cfg.nodes[index];
        pb_base = (pre_block->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
        // printf("This pre-block length is %d\n", pre_block->len);
        // printf("The pb_offset is %d\n", pb_offset);
        // printf("\t\t\tDuring the finding process:This inctruction base is %d\n", pb_base);
        for(pb_offset = (pb_base + pre_block->len - 1); pb_offset >= pb_base; pb_offset--)     // avoid the last ins                //for each insn
        {
            // printf("\t\t\tDuring the finding process:This inctruction base is %d, offset is %d, addr is0x%08lx\n", pb_base, pb_offset, insns[pb_offset].address);
            // printf("\t\t\tThe k value is %d\n", pos);
            //for this_ins, this first half statement is aiming avoiding IMMs and the rest is for avoiding sw/sb/sh which cannot serve as producer, besides the operand cannot be ZERO
            if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_REG && (insns[pb_offset].id != RISCV_INS_SW||insns[pb_offset].id != RISCV_INS_SB||insns[pb_offset].id != RISCV_INS_SH) && this_ins->detail->riscv.operands[pos].reg == insns[pb_offset].detail->riscv.operands[0].reg && this_ins->detail->riscv.operands[pos].reg != 1)
            {
                if(pos == 0)                                        //under SW situation
                {
                    if(pcg_block->producer[offset][pos].op == nullptr)
                    {
                        pcg_block->producer[offset][pos].block    = index;
                        pcg_block->producer[offset][pos].id       = pb_offset - pb_base;
                        if(index >= src && index <= dst)
                            pcg_block->producer[offset][pos].type     = CurrIter;
                        else
                            pcg_block->producer[offset][pos].type     = LoopConst;
                        pcg_block->producer[offset][pos].op       = &insns[pb_offset].detail->riscv.operands[0];
                        break;
                    }
                    else
                    {
                        new_link = (PCG_LINK*)malloc(sizeof(PCG_LINK));
                        new_link->block                         = index;
                        new_link->id                            = pb_offset - pb_base;
                        if(index >= src && index <= dst)
                            new_link->type                          = CurrIter;
                        else 
                            new_link->type                          = LoopConst;
                        new_link->op                            = &insns[pb_offset].detail->riscv.operands[0];
                        new_link->next                          = nullptr;
                        pcg_block->producer[offset][pos].next   = new_link; 
                        break;
                    }
                    
                }
                else
                {
                   if(pcg_block->producer[offset][pos-1].op == nullptr)
                    {
                        pcg_block->producer[offset][pos-1].block    = index;
                        pcg_block->producer[offset][pos-1].id       = pb_offset - pb_base;
                        if(index >= src && index <= dst)
                            pcg_block->producer[offset][pos].type     = CurrIter;
                        else
                            pcg_block->producer[offset][pos].type     = LoopConst;
                        pcg_block->producer[offset][pos-1].op       = &insns[pb_offset].detail->riscv.operands[0];
                        break;
                    }
                    else
                    {
                        new_link = (PCG_LINK*)malloc(sizeof(PCG_LINK));
                        new_link->block                             = index;
                        new_link->id                                = pb_offset - pb_base;
                        if(index >= src && index <= dst)
                            new_link->type                          = CurrIter;
                        else 
                            new_link->type                          = LoopConst;
                        new_link->op                                = &insns[pb_offset].detail->riscv.operands[0];
                        new_link->next                              = nullptr;
                        pcg_block->producer[offset][pos-1].next     = new_link;
                        break;
                    }
                }
            }
        }        
    }
}


static int find_pre_block(int **pre_block_list, PCG_BLOCK *block)
{
    CFG_Node    *node;
    int          num;

    num = -1;
    node = block->node;
    for(int i = 0; i < node->num_in + 1; i++)
        if(node->in[i]->type != functioncall)
        {   
            num++;
            (*pre_block_list)[num] = node->in[i]->src->id;
        }        
    
    return num;
}


static void aft_block_link(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns, int src, int dst)
{
    //Work: For each instruction whcih has missing producers,                                    //TODO: TO fix more
    //      if it is in a loop, find its producers in block after it of this loop.
    int         *aft_block_list;
    int          num_list;
    int          base;
    cs_insn     *this_ins;

    aft_block_list = (int*)malloc(MAX_PRE_BLOCK * 2 * sizeof(base));
    num_list = find_post_block(&aft_block_list, pcg_func, pcg_block, src, dst);
    //dump_post_block(pcg_block->node, aft_block_list, num_list);
    base = (pcg_block->node->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
    for(int offset = 0; offset < pcg_block->node->len - 1; offset++)                                               //for each instruction
    {
        this_ins = &insns[base + offset];
        // printf("\tThe consumer instruction:Base = %d ", base);
        // printf("\tOffset = %d ", offset);
        // printf("\t0x%" PRIx64 "\n", this_ins->address);
        switch(isMissing(pcg_block->producer, offset, this_ins))                                               //have not found producer in in-block checking
        {
            case 0:
                find_aft_produce(pcg_func, pcg_block, aft_block_list, num_list, insns, base, offset, 0, src, dst);
                break;
            case 1:
                find_aft_produce(pcg_func, pcg_block, aft_block_list, num_list, insns, base, offset, 1, src, dst);
                break;
            case 2:
                find_aft_produce(pcg_func, pcg_block, aft_block_list, num_list, insns, base, offset, 2, src, dst);
                break;
            case 12:
                find_aft_produce(pcg_func, pcg_block, aft_block_list, num_list, insns, base, offset, 1, src, dst);
                find_aft_produce(pcg_func, pcg_block, aft_block_list, num_list, insns, base, offset, 2, src, dst);
                break;
            default: continue;
        }
    }
    free(aft_block_list);
    aft_block_list = nullptr;
}


static int find_post_block(int **post_block_list,PCG_FUNC *pcg_func, PCG_BLOCK *block, int src, int dst)
{
    CFG_Node    *node;
    Func        *func;
    int          num, num_in, index;
    int          in[MAX_PRE_BLOCK*2];

    num = -1;
    num_in = -1;
    func = pcg_func->func;
    node = block->node;

    for(int i = node->id + 1; i <= dst; i ++)
        (*post_block_list)[++num] = i;

    num_in = 0;
    in[num_in] = node->id;
    for(; num_in > -1; num_in--)
    {
        index = in[num_in];
        for(int i = 0; i < func->cfg.nodes[index].num_in + 1; i++)
        {
            
            if(func->cfg.nodes[index].in[i]->src->id >= src && func->cfg.nodes[index].in[i]->src->id < dst && func->cfg.nodes[index].in[i]->type != functioncall)
            {
                in[++num_in] = func->cfg.nodes[index].in[i]->src->id;
                index = in[num_in];
                if(func->cfg.nodes[index].out[0]->dst->id == in[num_in-1] && func->cfg.nodes[index].out[1] != nullptr)
                    if(func->cfg.nodes[index].out[1]->dst->id >= src && func->cfg.nodes[index].out[1]->dst->id < dst && !check_int_duplicate(*post_block_list, num, func->cfg.nodes[index].out[1]->dst->id))
                        (*post_block_list)[++num] = func->cfg.nodes[index].out[1]->dst->id;
                else if(func->cfg.nodes[index].out[1]->dst->id == in[num_in-1] && func->cfg.nodes[index].out[0] != nullptr)
                    if(func->cfg.nodes[index].out[0]->dst->id >= src && func->cfg.nodes[index].out[0]->dst->id < dst && !check_int_duplicate(*post_block_list, num, func->cfg.nodes[index].out[0]->dst->id))
                        (*post_block_list)[++num] = func->cfg.nodes[index].out[0]->dst->id;
            }  
            else continue;
        }  
        for(int j = 0; j < num_in + 1; j++)
            in[j] = in[j + 1]; 
    }

    for(int i = 0; i < num + 1; i++)
    {
        index = (*post_block_list)[i];
        for(int j = 0; j < 2; j++)
            if(func->cfg.nodes[index].out[j] != nullptr && func->cfg.nodes[index].out[j]->dst->id > src && func->cfg.nodes[index].out[j]->dst->id <= dst)
                if(!check_int_duplicate(*post_block_list, num, func->cfg.nodes[index].out[j]->dst->id))
                    (*post_block_list)[++num] = func->cfg.nodes[index].out[j]->dst->id;
    }
        
    return num;
}


static void find_aft_produce(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, int *aft_block_list, int num_list, cs_insn *insns, int base, int offset, int pos, int src, int dst)
{
    int          i,j;
    int          index, pb_base, pb_offset;
    cs_insn     *this_ins;
    CFG_Node    *aft_block;
    PCG_LINK    *new_link;

    this_ins = &insns[base + offset];
    for(j = base + (pcg_block->node->len -1); j >= base + offset ; j--)//for this block 
    {
        if(j == base + offset)
        {
            if(pos == 0)
            {
                continue;                                               //SW cannot be Prevself
            }
            else
            {
                if(this_ins->detail->riscv.operands[pos].type == this_ins->detail->riscv.operands[0].type && this_ins->detail->riscv.operands[pos].reg == this_ins->detail->riscv.operands[0].reg)
                {
                    pcg_block->producer[offset][pos-1].block    = pcg_block->node->id;
                    pcg_block->producer[offset][pos-1].id       = j - base;
                    pcg_block->producer[offset][pos-1].type     = PrevSelf;
                    pcg_block->producer[offset][pos-1].op       = &this_ins->detail->riscv.operands[0];
                    return;
                }
            }
        }

        if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_REG && (insns[j].id != RISCV_INS_SW||insns[j].id != RISCV_INS_SB||insns[j].id != RISCV_INS_SH) && this_ins->detail->riscv.operands[pos].reg == insns[j].detail->riscv.operands[0].reg && this_ins->detail->riscv.operands[pos].reg != 1)
            {
                if(pos == 0)                                        //under SW situation
                {
                    if(pcg_block->producer[offset][pos].op == nullptr)
                    {
                        pcg_block->producer[offset][pos].block    = pcg_block->node->id;
                        pcg_block->producer[offset][pos].id       = j - base;
                        pcg_block->producer[offset][pos].type     = PrevIter;
                        pcg_block->producer[offset][pos].op       = &insns[j].detail->riscv.operands[0];
                        break;
                    }
                    else
                    {
                        new_link = (PCG_LINK*)malloc(sizeof(PCG_LINK));
                        new_link->block                         = pcg_block->node->id;
                        new_link->id                            = j - base;
                        new_link->type                          = PrevIter;
                        new_link->op                            = &insns[j].detail->riscv.operands[0];
                        new_link->next                          = nullptr;
                        pcg_block->producer[offset][pos].next   = new_link; 
                        break;
                    }
                    
                }
                else
                {
                   if(pcg_block->producer[offset][pos-1].op == nullptr)
                    {
                        pcg_block->producer[offset][pos-1].block    = pcg_block->node->id;
                        pcg_block->producer[offset][pos-1].id       = j - base;
                        pcg_block->producer[offset][pos-1].type     = PrevIter;
                        pcg_block->producer[offset][pos-1].op       = &insns[j].detail->riscv.operands[0];
                        break;
                    }
                    else
                    {
                        new_link = (PCG_LINK*)malloc(sizeof(PCG_LINK));
                        new_link->block                             = index;
                        new_link->id                                = j - base;
                        new_link->type                              = PrevIter;
                        new_link->op                                = &insns[j].detail->riscv.operands[0];
                        new_link->next                              = nullptr;
                        pcg_block->producer[offset][pos-1].next     = new_link;
                        break;
                    }
                }
            }
    }                                                                    

    for(i = 0; i < num_list + 1; i++)                                   //for each aft block
    {
        index = aft_block_list[i];
        // printf("\t\tThe block id is %d\n", index);
        aft_block = &pcg_func->func->cfg.nodes[index];
        pb_base = (aft_block->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
        // printf("This aft-block length is %d\n", aft_block->len);
        // printf("The pb_offset is %d\n", pb_offset);
        // printf("\t\t\tDuring the finding process:This inctruction base is %d\n", pb_base);
        for(pb_offset = (pb_base + aft_block->len - 1); pb_offset >= pb_base; pb_offset--)     // avoid the last ins                //for each insn
        {
            // printf("\t\t\tDuring the finding process:This inctruction base is %d, offset is %d, addr is0x%08lx\n", pb_base, pb_offset, insns[pb_offset].address);
            // printf("\t\t\tThe k value is %d\n", pos);

            //for this_ins, this first half statement is aiming avoiding IMMs and the rest is for avoiding sw/sb/sh which cannot serve as producer, besides the operand cannot be ZERO
            if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_REG && (insns[pb_offset].id != RISCV_INS_SW||insns[pb_offset].id != RISCV_INS_SB||insns[pb_offset].id != RISCV_INS_SH) && this_ins->detail->riscv.operands[pos].reg == insns[pb_offset].detail->riscv.operands[0].reg && this_ins->detail->riscv.operands[pos].reg != 1)
            {
                if(pos == 0)                                        //under SW situation
                {
                    if(pcg_block->producer[offset][pos].op == nullptr)
                    {
                        pcg_block->producer[offset][pos].block    = index;
                        pcg_block->producer[offset][pos].id       = pb_offset - pb_base;
                        pcg_block->producer[offset][pos].type     = PrevIter;
                        pcg_block->producer[offset][pos].op       = &insns[pb_offset].detail->riscv.operands[0];
                        break;
                    }
                    else
                    {
                        new_link = (PCG_LINK*)malloc(sizeof(PCG_LINK));
                        new_link->block                         = index;
                        new_link->id                            = pb_offset - pb_base;
                        new_link->type                          = PrevIter;
                        new_link->op                            = &insns[pb_offset].detail->riscv.operands[0];
                        new_link->next                          = nullptr;
                        pcg_block->producer[offset][pos].next   = new_link; 
                        break;
                    }
                    
                }
                else
                {
                   if(pcg_block->producer[offset][pos-1].op == nullptr)
                    {
                        pcg_block->producer[offset][pos-1].block    = index;
                        pcg_block->producer[offset][pos-1].id       = pb_offset - pb_base;
                        pcg_block->producer[offset][pos-1].type     = PrevIter;
                        pcg_block->producer[offset][pos-1].op       = &insns[pb_offset].detail->riscv.operands[0];
                        break;
                    }
                    else
                    {
                        new_link = (PCG_LINK*)malloc(sizeof(PCG_LINK));
                        new_link->block                             = index;
                        new_link->id                                = pb_offset - pb_base;
                        new_link->type                              = PrevIter;
                        new_link->op                                = &insns[pb_offset].detail->riscv.operands[0];
                        new_link->next                              = nullptr;
                        pcg_block->producer[offset][pos-1].next     = new_link;
                        break;
                    }
                }
            }
        }        
    }
}


static int isMissing(PCG_LINK **producers, int index, cs_insn *this_ins)                       
                                                                             // 1: op1 producer is missing 2: op2 producer is missing 3: mem op producer is missuing 12: all missing
{
    if((this_ins->id == RISCV_INS_SW|| this_ins->id ==RISCV_INS_SB ||this_ins->id == RISCV_INS_SH) && producers[index][0].type == NOTFIND)
        return 0;
    else if(producers[index][0].type == NOTFIND && producers[index][1].type == NOTFIND)
        return 12;
    else if(producers[index][0].type == NOTFIND)
        return 1;
    else if(producers[index][1].type == NOTFIND)
        return 2;
    else return -1;

}


static bool check_int_duplicate(const int* list, int num, int temp)
{
    for (int i = 0; i < num + 1; i++) {
        if (list[i] == temp) {
            return true;
        }
    }
    return false;
}








//-----------------------------------------------------------------------------
// Part 2: Get looped func and nested loop for Part 1
//
// Input:
// 
//-----------------------------------------------------------------------------

static void get_looped_func(PCG_FUNC **pcg_list, const LOOP **loops)
{
    nested_loops = (const LOOP**)malloc(MAX_NESTED * sizeof(LOOP*));
    num_nested = -1;
    num_pcg_func = -1;

    for(int i = 0; i < *num_loops; i++)                             
        find_nested_loop(nested_loops, &num_nested, &(*loops)[i]);

    sort_loop(nested_loops, num_nested);                                    
    dump_nested();

    *pcg_list = (PCG_FUNC*)malloc(num_nested * sizeof(PCG_FUNC));
    for(int i = 0; i < num_nested + 1; i++)                                 //get funcs where the loops belong to
    {
        if(i == 0)
        {
            num_pcg_func += 1;
            (*pcg_list)[num_pcg_func].func = nested_loops[i]->func;
        }
        else if(nested_loops[i]->func != (*pcg_list)[num_pcg_func].func)
        {
            num_pcg_func += 1;
            (*pcg_list)[num_pcg_func].func = nested_loops[i]->func;
        }
        else continue;
    }
    dump_pcg_func_list();
    return ;
}


//-----------------------------------------------------------------------------
// Help Function: Find nested loop
//
// Output: pcg_func_list
//-----------------------------------------------------------------------------
static void find_nested_loop(const LOOP **nested_loops, int *num_nested, const LOOP *loop)
{        
    const LOOP     *temp_loop;
    const LOOP     *buffer[MAX_NESTED] = {nullptr};
    int             num_buf;

    num_buf = 0;
    buffer[0] = loop;
    if(loop->num < 0)
        return ;


    while(num_buf >= 0)
    {
        temp_loop = buffer[0];
        for(int i = 0; i < num_buf + 1; i++)
                buffer[i] = buffer[i + 1];
        num_buf = num_buf -1;

        if(temp_loop->num >= 0)
        {

            for(int j = 0; j <= temp_loop->num; j++)
            {
                num_buf = num_buf + 1;
                buffer[num_buf] = temp_loop->inside[j];
            }  
        }
        else  
        {
            if(!check_duplicate(nested_loops, num_nested, temp_loop))
            {
                *num_nested = *num_nested + 1;
                nested_loops[*num_nested] = temp_loop;
            }    
        }
        
    }        
}


static bool check_duplicate(const LOOP** nested_loops, int *num_nested, const LOOP* temp_loop)
{
    for (int i = 0; i < *num_nested + 1; i++) {
        if (nested_loops[i] == temp_loop) {
            return true;
        }
    }
    return false;
}


static void sort_loop(const LOOP **list, int len)
{
    int i, j;
    const LOOP *temp;

    for (i = 0; i < len + 1; i++) {
        for (j = 0; j < len - i; j++) {
            if (list[j]->start_addr > list[j+1]->start_addr || (list[j]->start_addr == list[j+1]->start_addr && list[j]->end_addr > list[j+1]->end_addr)) {
                temp = list[j];
                list[j] = list[j+1];
                list[j+1] = temp;
            }
        }
    }
}


static bool is_compute(int opcount){
    return opcount==3;
}


static bool is_auipc(int opcount,int id){
    return (opcount==2 && id==RISCV_INS_AUIPC);
}


//-----------------------------------------------------------------------------
//Dump out
//-----------------------------------------------------------------------------


static void dump_nested()
{
    for(int i = 0; i < num_nested + 1; i++)
        printf("The %d Loop start address is 0x%08lx, end address is 0x%08lx, src-id is %d, dst-id is %d\t\nwhich belongs to the func start with0x%08lx\n", i, nested_loops[i]->start_addr, nested_loops[i]->end_addr, nested_loops[i]->src, nested_loops[i]->dst, nested_loops[i]->func->start_addr);
    printf("\n");
}


static void dump_pcg_func_list()
{
    for(int i = 0; i < num_pcg_func + 1; i++)
        printf("The %d pcg func start address is 0x%08lx\n", i , pcg_func_list[i].func->start_addr); 
    printf("\n");
}


static void dump_pre_block(CFG_Node *node, int *list, int num)
{
    printf("The %d block's pre_blocks are printed below:\n", node->id);
    for(int i = 0; i < num + 1; i++)
        printf("\t%d",list[i]);
    printf("\n");
}


static void dump_post_block(CFG_Node *node, int *list, int num)
{
    printf("The %d block's pre_blocks are printed below:\n", node->id);
    for(int i = 0; i < num + 1; i++)
        printf("\t%d",list[i]);
    printf("\n");
}


static void dump_produce()
{
    PCG_LINK *curr;
    int i, j, x, y;
    int flag = 0;
    for(i = 0; i < num_pcg_func + 1; i++)
        for(int loop = 0; loop < num_nested + 1; loop ++)
        {
            if(nested_loops[loop]->func != pcg_func_list[i].func && flag == 0)
                continue;
            else if(nested_loops[loop]->func == pcg_func_list[i].func)
            {
                flag = 1;
                for(j = nested_loops[loop]->src; j < nested_loops[loop]->dst + 1; j++)
                    for(x = 0; x < nested_loops[loop]->func->cfg.nodes[j].len; x++)
                        for(y = 0; y < 2; y++)
                        {
                            if(pcg_func_list[i].blocks[j].producer[x][y].type != NOTFIND)
                            {
                                curr = &pcg_func_list[i].blocks[j].producer[x][y];
                                printf("The %2d Func %3d Block %3d Ins %3d rs' PRODUCE is %3d block %3d ins and type is %2d\n", i, j, x, y, curr->block, curr->id, curr->type);
                                while(curr->next != nullptr)
                                {
                                    curr = curr->next;
                                    printf("\tThe another producer is:\n");
                                    printf("\tThe %2d Func %3d Block %3d Ins %3d rs' PRODUCE is %3d block %3d ins and type is %2d\n", i, j, x, y, curr->block, curr->id, curr->type);
                                }                            
                            }
                        }
            }
            else if(nested_loops[loop]->func != pcg_func_list[i].func && flag == 1)
            {
                flag = 0;
                break;
            }
                
        }
            
}


PCG_FUNC** get_pcg_func()
{
    return  &pcg_func_list;
}


int* get_pcg_func_num()
{
    return &num_pcg_func;
}


LOOP*** get_nested_loops()
{
    return (LOOP***)&nested_loops;
}


int* get_nested_loops_num()
{
    return &num_nested;
}



//------------------------------------------------------------------------------
//Test part
//------------------------------------------------------------------------------
void testpcg(void)
{
    size_t count;//used as iterator
    cs_insn *insn;//pointing at the instructions
	std::string Code;
    uint64_t address;
    if(GetCodeAndAddress("/home/rory/Desktop/32ia.riscv", Code, address))
    {
        unsigned char* Code_Hex = nullptr;

    	//Transform the Code into the Capstone form
    	Code_Hex = String_to_Hex(Code);

    	//Disassemblying the bit stream
    	insn = Disasm(Code_Hex, address, Code.length()/2, count);

		gen_func_list(insn, count);
		gen_cfg(insn);
        gen_loop();
        gen_pcg(insn);
    }
}