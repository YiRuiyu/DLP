//
//Written on 12th, Aug, 2022
//
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <../../capstone/capstone.h>
#include <../../capstone/platform.h>

#ifndef DISASM_H
#define DISASM_H



struct platform {
	cs_arch arch;
	cs_mode mode;
	unsigned char *code;
	size_t size;
	const char *comment;
};
unsigned char* String_to_Hex(std::string instruct);
void print_string_hex(const char *comment, unsigned char *str, size_t len);
void print_insn_detail(cs_insn *ins);
void printInsts(cs_insn *insn_, size_t count_);
cs_insn* Disasm(unsigned char* Code_, uint64_t address_, size_t length_, size_t &count_);
int testdisasm (void);


#endif
