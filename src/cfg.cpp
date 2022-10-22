///////////////////////////////////////////////////////////////////////////////
// 
// Generate Control Flow Graphs (CFGs) for functions in the code segement
// by @LXF, @YRY
// 
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <new>
#include <cstdlib>
#include "../include/cfg.h"
#include "../include/BinaryCode.h"//only for testing

#define NUMFUNC 200
#define NUMBLOCK 100
#define REALLOC 200


//-----------------------------------------------------------------------------
// Part 0: Global variables 
// Those invisible to other modules should be guarded with static keyword
//-----------------------------------------------------------------------------
static Func 			*g_func_list;
static int 				g_num_funcs;
static uint64_t 		g_entry_func;



//-----------------------------------------------------------------------------
// Part 1: Generate a list of functions. At current stage, we omit the building
//	of Call Graph (CG) as our analysis is intra-procedure. If 
//	inter-procedure analysis is desired, then a CG data structure is needed
// Note: each part often contains one or a few primary functions that consume
// 	a few major data structures produced by other parts, and produce a few
//	major data structures of this part. The other functions serve as helper
//	functions that either decompose the functionality of a primary function
//	into smaller pieces, or dump out intermediate results for observations
//-----------------------------------------------------------------------------

// Primary functions
//-----------------------------------------------------------------------------

// Input: 
// - *ins: program disassemblying instructions
// Output:
// - func_list: the list of functions (a global variable)
// - return the number of functions in the list
// Work: to generate a list of functions from the code, each including its entry
//	address, its length, and its cfg
// By: @LXF (Date: 2022-10-10)
// By: @YRY (Date: 2022-10-14 2022-10-16)
int gen_func_list(cs_insn *ins, int count)
{
	// 1. Generate a list of function start addresses by scanning the code in order
	// for each instr {
	// 	if (instr is jal) {
	// 		calculate the target address
	//		lookup the target address in the func start address list
	//		insert a new start address in the funct start address list 
	// 	} else if (instr is jalr) {
	//		......
	//	} else {
	//		......
	//	}
	// }
	bool re_alloc;
	int i, j, id, len, total_len;
	total_len = 0;
	g_num_funcs = 0;
	uint64_t addr, offset, Jmp_addr, Next_addr, temp;
	uint64_t *func_start_list;
	func_start_list = (uint64_t*)malloc(sizeof(uint64_t)*NUMFUNC);
	
	cs_riscv *riscv;
	cs_detail *detail;
	
	//insert the fisrt addr of instruction into func_start_list
	func_start_list[0] = ins[0].address;

	// for each instr
	for(j = 0; j < count; j++)
	{
		//fetch the instruction details
		addr = ins[j].address;
		riscv = &(ins[j].detail->riscv);
		detail = ins[j].detail;
		id = ins[j].id;
		//inst is jal and not j
		if(id == 206 && riscv->operands[0].reg != 1)
        {
            //calculate the target address
			offset = riscv->operands[1].imm;
			Jmp_addr = addr + offset;
			Next_addr = addr + 0x4;

			//check if overflow
			if(g_num_funcs == NUMFUNC)
			{
				func_start_list = (uint64_t*)realloc(func_start_list, NUMFUNC + REALLOC);
				re_alloc = true;
			}
				
			
			//lookup the target address in the func start address list
			//insert a new start address in the funct start address list
			if(!check_dup(func_start_list, Jmp_addr))
			{
				//printf("0x%08lx\n", Jmp_addr);
				func_start_list[++g_num_funcs] = Jmp_addr;
			}
				
        }
        else continue;
	}
	// 2. Generate the function list from the start address list
	// - sort the address list in ascending order
	for(i = 0; i < g_num_funcs + 1; i++)
	{
		for(j = 0;j < g_num_funcs - i; j++)
		{
            if(func_start_list[j] > func_start_list[j+1])
			{
                temp = func_start_list[j];
                func_start_list[j] = func_start_list[j+1];
                func_start_list[j+1] = temp;
            }
        }
		
	}
	
	/*----------------------------------------
	for(i = 0; i < g_num_funcs + 1; i++)    //dump out the func_start_list
	{
		printf("0x%08lx\n", func_start_list[i]);
	}
	----------------------------------------*/

	// - generate the function list from address list
	if(re_alloc)
		g_func_list = (Func*)malloc(sizeof(Func) * (NUMFUNC+REALLOC));
	else g_func_list = (Func*)malloc(sizeof(Func)*NUMFUNC);
	for(i = 0; i < g_num_funcs + 1; i++)
	{
		len = (func_start_list[i+1] - func_start_list[i])/0x4;
		g_func_list[i].start_addr = func_start_list[i];
		if(i != g_num_funcs)
			g_func_list[i].len = len;
		else g_func_list[i].len = count - total_len;
		total_len += len;
	}

	//delete the fun_start_list
	free(func_start_list);
	func_start_list == NULL;

	//dumpout the func_list
	dump_func_list();
	// - set the entry function (e.g., main()) of the code
	//TODO
	return g_num_funcs;
}



// Helper functions for this part
// Note: for clear view of boundaries, leave 3 blank lines between functions.
// Hint: once your function is over 60 lines (or 3+ levels of indentation), 
//	it is highly likely that the complexity of its work is too complicated
//	(i.e., consisting of several sub-tasks that can be decoupled neatly). 
//	In this case, you should use helper functions to implement these 
//	sub-tasks, thereby making your code in good modularity and maintainence
//-----------------------------------------------------------------------------
static bool check_dup(uint64_t *func_start_list, uint64_t addr)
{
	int i;
	for(i = 0; i < g_num_funcs + 1; i++)
		if(addr == func_start_list[i])
			return true;
	return false;
}

/*static uint64_t get_entry()//fix
{
	FILE *fp=nullptr;
	std::string line;
	char cmd[] = "readelf -x.text | grep main";
	if((fp=popen(cmd.c_str(),"r"))==nullptr)
    {
        return 0;
    }

    char read_str[100]="";
    fgets(read_str,sizeof(read_str),fp);
	line = read_str;
	return strtoul(line.substr(8, 7).c_str(), 0, 16);
}*/



// Work: dump out the list of fuctions for check of correctness
static void dump_func_list()
{
	int i;
	for(i = 0; i < g_num_funcs + 1; i++)
		printf("The %d func start address is 0x%08lx\nThe %d func length is %d\n\n",i ,g_func_list[i].start_addr,i, g_func_list[i].len);
}



//-----------------------------------------------------------------------------
// Part 2: Generate CFG for each function
//-----------------------------------------------------------------------------

// Primary functions
//-----------------------------------------------------------------------------

// Input: 
// - g_func_list: the function list
// Output:
// - the CFG for each function
// Work: generate the CFG for each function
void gen_cfg(cs_insn *insn)
{

	Func 		*func;
	for (int i = 0; i < g_num_funcs + 1; i++) {
		func = &g_func_list[i];
		gen_cfg_nodes(func, insn);
		//gen_cfg_edges(func);		
	}

}



// Work: ...
// Note: any function not visible (non-API functions) to other modules should
// 	be made static to prevent collisions with functions in other modules
//	that happen to have the same function name. Similar principle also
//	applies to global variables (global variables not used by other modules
//	should be protected with static to prevent visibility to other modules)
static void gen_cfg_nodes(Func *func, cs_insn *insn)
{
	CFG			cfg;
	CFG_Node 	*node;
	uint64_t 	*node_entries, temp;
	int 		num_node_entries, i, j, total_len;
	total_len = 0;
	
	num_node_entries = get_node_entries(func, insn, &node_entries);

	// - sort the address list in ascending order
	for(i = 0; i < num_node_entries + 1; i++)
	{
		for(j = 0;j < num_node_entries - i; j++)
		{
            if(node_entries[j] > node_entries[j+1])
			{
                temp = node_entries[j];
                node_entries[j] = node_entries[j+1];
                node_entries[j+1] = temp;
            }
        }
		
	}
	cfg.nodes = (CFG_Node*)malloc(sizeof(CFG_Node)*(num_node_entries+1));
	// dump_node_entries(num_node_entries&node_entries, node_entries);
	dump_node_entries(func, node_entries, num_node_entries);
	// generate  cfg.num_nodes from node_entries
	for(i = 0; i < num_node_entries + 1; i++)
	{
		node = (CFG_Node*)malloc(sizeof(CFG_Node));
		node->id = i;
		node->start_addr = node_entries[i];
		if(i == num_node_entries)
			node->len = func->len - total_len;
		else node->len = (node_entries[i+1] - node_entries[i])/0x4;

		cfg.nodes[i] = *node;
		cfg.func = func;
		func->cfg = cfg;
		total_len += node->len;
	}
	func->cfg.num_nodes = num_node_entries;
	// dump_cfg_nodes(...);
	dump_cfg_nodes(func);
	// release the memory allocated for node_entries
	free(node_entries);
	node_entries = NULL;
}



// Work: ...
static void gen_cfg_edges(Func *func)
{
	// generate edges by checking the last instruction of each node, and
	// update the in/out edges for the corresponding nodes
	CFG_Node 	*node;

	for (int i = 0; i < func->cfg.num_nodes; i++) {
		node = &func->cfg.nodes[i];
		new_edges(func, node);
	}
}



// Helper functions for this part
//-----------------------------------------------------------------------------

// Work: scan through the function to get the entry addresses of basic blocks
static int get_node_entries(Func *func, const cs_insn *insn, uint64_t **node_entries)
{
	int i, index, num;
	uint64_t Jmpaddr, offset;
	cs_riscv *riscv;
	num = 0;
	*node_entries = (uint64_t*)malloc(sizeof(uint64_t)*NUMBLOCK);
	(*node_entries)[0] = func->start_addr;
	index = find_index(insn, func->start_addr);//find start_addr in instruction array
	for(i = index; i < func->len + index -1; i++)
	{
		if(insn[i].detail->groups[0] == 1)//branch instruction
		{
			
			if(num == NUMBLOCK)//check if overflow
				*node_entries = (uint64_t*)realloc(*node_entries, NUMBLOCK + REALLOC);
			
			riscv = &(insn[i].detail->riscv);
			offset = riscv->operands[2].imm;
			Jmpaddr = insn[i].address + offset;
			if(!check_dup(*node_entries, insn[i].address + 0x4))
				(*node_entries)[++num] = insn[i].address + 0x4;
			if(!check_dup(*node_entries, Jmpaddr))
				(*node_entries)[++num] = Jmpaddr;
		}	
	}
	return num;
}



// Work: new edges originated from the src Node
static void new_edges(Func *func, CFG_Node *src)
{
	// 1) new a fall-through edge, if any
	// Hint: dst of the edge can be obtained by src->id+1

	// 2) new a control-transfer edge, if any
	// dst = search_node(func, dst_addr)
}



// Work: search the CFG node in func, given the start address of the node
static CFG_Node* search_node(Func *func, uint64_t start_addr)
{
	CFG_Node 	*node;

	// TODO: 
	return node;
}



// Work: find the instruction in cs_insn
static int find_index(const cs_insn *insn, uint64_t addr)
{
	int i;
	for(i = 0;; i++)
		if(insn[i].address == addr)
			return i;
}



static void dump_node_entries(Func *func, uint64_t *node_entries, int num_node_entries)
{
	int i;
	printf("--------------------------------------------------------------------------\n");
	for(i = 0; i < num_node_entries + 1; i++)
		printf("The %d entry is 0x%08lx\n", i, node_entries[i]);
}



static void dump_cfg_nodes(Func *func)
{
	int i;
	CFG cfg = func->cfg;
	for(i = 0; i < cfg.num_nodes + 1; i++)
		printf("The %d node info:\nId is %d\t\tStart_addr is 0x%08lx\t\tLength is %d\n\n", i, cfg.nodes[i].id, cfg.nodes[i].start_addr, cfg.nodes[i].len);
}



static void dump_cfg_edges(Func *func)
{

}



void testcfg(void)
{
	
	size_t count;//used as iterator
    cs_insn *insn;//pointing at the instructions
	std::string Code;
    uint64_t address;
    if(GetCodeAndAddress("/home/rory/Desktop/32ia.riscv", Code, address))
    {
        unsigned char* Code_Hex = nullptr;
    	uint64_t address = 65716;

    	//Transform the Code into the Capstone form
    	Code_Hex = String_to_Hex(Code);

    	//Disassemblying the bit stream
    	insn = Disasm(Code_Hex, address, Code.length()/2, count);

		gen_func_list(insn, count);
		gen_cfg(insn);
    }
}