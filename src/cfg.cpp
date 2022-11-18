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


//-----------------------------------------------------------------------------
// Part 0: Global variables 
// Those invisible to other modules should be guarded with static keyword
//-----------------------------------------------------------------------------
static Func 			*g_func_list;
static int 				g_num_funcs;
static uint64_t 		g_entry_func;
static int 				NUMFUNC = 50;
static int 				NUMBLOCK = 40;
static int 				NUMEDGE = 50;

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

	//Variables:
	bool re_alloc;
	int i, j, id, len, total_len;
	total_len = 0;
	g_num_funcs = 0;
	uint64_t addr;
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

		//isJal and not J
		if(isJal(id, riscv->operands[0].reg))
            func_start_list = func_list_append(addr, riscv, func_start_list);	
        else 
			continue;
	}

	// 2. Generate the function list from the start address list
	// - sort the address list in ascending order
	func_start_list = sort(func_start_list, g_num_funcs);
	

	// - generate the function list from address list
	g_func_list = (Func*)malloc(sizeof(Func)*NUMFUNC);
	for(i = 0; i < g_num_funcs + 1; i++)
	{
		len = (func_start_list[i+1] - func_start_list[i])/0x4;
		g_func_list[i].start_addr = func_start_list[i];
		//for the last instruction of this function
		if(i != g_num_funcs)
			g_func_list[i].len = len;
		else 
			g_func_list[i].len = count - total_len;
		total_len += len;
	}
	

	//delete the fun_start_list
	free(func_start_list);
	func_start_list == NULL;

	//dumpout the func_list
	////dump_func_list();
	// - set the entry function (e.g., main()) of the code
	//TODO
	return g_num_funcs;
}



// Helper functions for this part
// Note: for clear view of boundaries, leave 3 blank lines between functions.
// Hint: once your function is over 60 lines (or 3+ levels of indentation), 
//	it is highly likely that the complexity of its work is too complicatedfunc_list_append
//	(i.e., consisting of several sub-tasks that can be decoupled neatly). 
//	In this case, you should use helper functions to implement these 
//	sub-tasks, thereby making your code in good modularity and maintainence
//-----------------------------------------------------------------------------
static bool check_dup(uint64_t *list, uint64_t addr, int num)
{
	int i;
	for(i = 0; i < num + 1; i++)
		if(addr == list[i])
			return true;
	return false;
}



static bool isJal(int id, int reg)
{
	return id == 206 && reg !=1;
}



static bool isRet(int id)
{
	return id == 207;
}



static uint64_t* func_list_append(uint64_t address, cs_riscv *riscv, uint64_t *func_start_list)
{
	//check if overflow
	isFull(g_num_funcs, &NUMFUNC, &func_start_list);
					
	//lookup the target address in the func start address list
	//insert a new start address in the funct start address list
	//address + riscv->operands[1].imm is Jmp_addr
	if(!check_dup(func_start_list, address + riscv->operands[1].imm, g_num_funcs))
		func_start_list[++g_num_funcs] = address + riscv->operands[1].imm;
	
	return func_start_list;
}



static uint64_t* sort(uint64_t *list, int num)
{
	int i, j;
	uint64_t temp;
	for(i = 0; i < num + 1; i++)
	{
		for(j = 0;j < num - i; j++)
		{
            if(list[j] > list[j+1])
			{
                temp = list[j];
                list[j] = list[j+1];
                list[j+1] = temp;
            }
        }
	}
	return list;
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
	}
	for (int i = 0; i < g_num_funcs + 1; i++) {
		func = &g_func_list[i];
		//reset the max of edges because every func malloc its own array
		//if we don't do that, array will overflow
		NUMEDGE = 50;
		gen_cfg_edges(func, insn);
		
		//dump_cfg_edges(func);
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
	node_entries = sort(node_entries, num_node_entries);
	
	cfg.nodes = (CFG_Node*)malloc(sizeof(CFG_Node)*(num_node_entries+1));
	// dump_node_entries(num_node_entries&node_entries, node_entries);
	////dump_node_entries(func, node_entries, num_node_entries);
	// generate  cfg.num_nodes from node_entries
	for(i = 0; i < num_node_entries + 1; i++)
	{
		node = gen_node(func, i, node_entries, num_node_entries, &total_len);

		//assign node to cfg
		cfg.nodes[i] = *node;
		cfg.func = func;
		func->cfg = cfg;


		total_len += node->len;
	}
	func->cfg.num_nodes = num_node_entries;
	
	//initialized for edges
	func->cfg.num_edges = -1;
	func->cfg.edges = (CFG_Edge*)malloc(sizeof(CFG_Edge)*NUMEDGE);

	// dump_cfg_nodes(...);
	////dump_cfg_nodes(func);
	// release the memory allocated for node_entries
	free(node_entries);
	node_entries = NULL;
}



// Work: ...
static void gen_cfg_edges(Func *func, cs_insn *insn)
{
	// generate edges by checking the last instruction of each node, and
	// update the in/out edges for the corresponding nodes
	CFG_Node 	*node;
	
	for (int i = 0; i < func->cfg.num_nodes + 1; i++) {
		node = &func->cfg.nodes[i];
		new_edges(func, node, insn);
	}
}



// Helper functions for this part
//-----------------------------------------------------------------------------
static CFG_Node* gen_node(Func *func, int i, uint64_t *node_entries, int num_node_entries, int *total_len)
{
	CFG_Node *node;
	node = (CFG_Node*)malloc(sizeof(CFG_Node));
	node->id = i;
	node->start_addr = node_entries[i];
	node->num_in = -1;
	if(i == num_node_entries)// for the last one condition
		node->len = func->len - *total_len;
	else node->len = (node_entries[i+1] - node_entries[i])/0x4;
	return node;
}



// Work: scan through the function to get the entry addresses of basic blocks
static int get_node_entries(Func *func, const cs_insn *insn, uint64_t **node_entries)
{
	int i, index, num;
	num = 0;
	*node_entries = (uint64_t*)malloc(sizeof(uint64_t)*NUMBLOCK);
	(*node_entries)[0] = func->start_addr;
	index = find_index(insn, func->start_addr);//find start_addr in instruction array
	for(i = index; i < func->len + index; i++)
	{
		if(insn[i].detail->groups[0] == 1)//branch instruction
			*node_entries = bran_append(insn[i], *node_entries, &num);
		else if(insn[i].id == 206 && i != func->len + index -1)//jal instruction
			*node_entries = jal_append(insn[i], *node_entries, &num);
		
	}
	return num;
}



static void isFull(int num, int *max, uint64_t **list)
{
	if(num + 1 == *max)
	{
		*max *= 2;
		*list = (uint64_t*)realloc(*list, *max*sizeof(uint64_t));
	}
}



static void EdgeisFull(int num, int *max, CFG_Edge **list)//the only problem is how to enlarge the edge array ???
{
	if(num+1 == *max)
	{
		*max *= 2;
		*list = (CFG_Edge*)realloc(*list, *max*sizeof(CFG_Edge));
	}
}



static uint64_t* bran_append(cs_insn ins, uint64_t *list, int *num)
{
	uint64_t Jmpaddr, Nextaddr, offset;
	cs_riscv *riscv;

	riscv = &(ins.detail->riscv);
	offset = riscv->operands[2].imm;
	Jmpaddr = ins.address + offset;
	Nextaddr = ins.address + 0x4;

	isFull(*num, &NUMBLOCK, &list);
	if(!check_dup(list, Nextaddr, *num))
		list[++*num] = Nextaddr;
	
	isFull(*num, &NUMBLOCK, &list);
	if(!check_dup(list, Jmpaddr, *num))
		list[++*num] = Jmpaddr;
	
	return list;
}



static uint64_t* jal_append(cs_insn ins, uint64_t *list, int *num)
{
	uint64_t Nextaddr;
	Nextaddr = ins.address + 0x4;

	isFull(*num, &NUMBLOCK, &list);
	if(!check_dup(list, Nextaddr, *num))
		list[++*num] = Nextaddr;
	
	return list;
}



// Work: new edges originated from the src Node
static void new_edges(Func *func, CFG_Node *src, cs_insn *insn)
{
	// 1) new a fall-through edge, if any
	// Hint: dst of the edge can be obtained by src->id+1

	// 2) new a control-transfer edge, if any
	// dst = search_node(func, dst_addr)

	//Variables
	int index, id;
	cs_riscv *riscv;
	uint64_t addr, Jmp_addr, Next_addr, offset; 
	index = find_index(insn, src->start_addr);//find the func start instruction index
	cs_insn ins = insn[index + src->len - 1];//the last instruction
	riscv = &(ins.detail->riscv);
	id = ins.id;
	addr = ins.address;
	
	if(ins.detail->groups[0] == 1)						//branch instruction
	{
		offset = riscv->operands[2].imm;
		Next_addr = addr + 0x4;
		Jmp_addr = addr + offset;
		//sequnce block
		CFG_Edge *seq_edge = gen_edge(func, src, Next_addr, fallthrough);

		//branch jump block
		CFG_Edge *jmp_edge = gen_edge(func, src, Jmp_addr, ctrltrans);
		
		//update the func->cfg.edges
		edge_append(func, seq_edge);
		edge_append(func, jmp_edge);
	}	
	else if(isJal(id, riscv->operands[0].reg))
	{	
		Jmp_addr = addr + riscv->operands[1].imm;
		CFG_Edge *jal_edge = gen_edge(func, src, Jmp_addr, functioncall);
		edge_append(func, jal_edge);
	}
	else if(!isRet(id) && (addr != func->start_addr + 0x4*(func->len-1)))
	{	
		Next_addr = addr + 0x4;
		CFG_Edge *fall_edge = gen_edge(func, src, Next_addr, fallthrough);
		edge_append(func, fall_edge);
	}
	
}



static CFG_Edge* gen_edge(Func *func, CFG_Node *src, uint64_t addr, edgetype type)
{
	CFG_Edge *edge = (CFG_Edge*)malloc(sizeof(CFG_Edge));
	edge->src = src;
	if(type == functioncall)
		edge->dst = search_node_jal(addr);
	else 
		edge->dst = search_node(func, addr);
	edge->type = type;
	edge->src->out[0] = edge;
	edge->dst->in[++edge->dst->num_in] = edge;
	return edge;
}



static void edge_append(Func *func, CFG_Edge *edge)
{
	EdgeisFull(func->cfg.num_edges, &NUMEDGE, &func->cfg.edges);
	func->cfg.edges[++func->cfg.num_edges] = *edge;
}



// Work: search the CFG node in func, given the start address of the node
static CFG_Node* search_node(Func *func, uint64_t start_addr)
{
	CFG_Node 	*node;
	int i;
	for(i = 0; i < func->cfg.num_nodes+1; i++)
		if(func->cfg.nodes[i].start_addr == start_addr)
			return &func->cfg.nodes[i];
	printf("Could not find node in this func\n");
	return NULL;
}



static CFG_Node* search_node_jal(uint64_t start_addr)// for function call 
{													  // inter function block edge
	CFG_Node 	*node;
	int i;
	for(i = 0; i < g_num_funcs + 1; i++)
		if(g_func_list[i].start_addr == start_addr)
			return &g_func_list[i].cfg.nodes[0];
	printf("Could not find node in this func\n");
	return NULL;
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
	int i;
	printf("------------------------------------------------------------------------\n");
	printf("the func start address is 0x%08lx\n", func->start_addr);
	for(i = 0; i < func->cfg.num_edges + 1; i++)
	{
		printf("The %d edges information: \n",i);
		printf("The src id is %d\n",func->cfg.edges[i].src->id);
		printf("The dst id is %d\n",func->cfg.edges[i].dst->id);
		printf("The type is %d\n\n",func->cfg.edges[i].type);
	}
	printf("------------------------------------------------------------------------\n\n");
	
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

    	//Transform the Code into the Capstone form
    	Code_Hex = String_to_Hex(Code);

    	//Disassemblying the bit stream
    	insn = Disasm(Code_Hex, address, Code.length()/2, count);

		gen_func_list(insn, count);
		gen_cfg(insn);
    }
}



const Func** get_func()
{
	return (const Func**)&g_func_list;
}



const int* get_func_num()
{
	return &g_num_funcs;
}