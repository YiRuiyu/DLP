//
//Written on 11th, Aug, 2022
//Modified on 20th, Aug, 2022
//Written by Yi, Ruiyu
//This file contain the function of getting the binary code from section .text
//

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <inttypes.h>
#include "../include/BinaryCode.h"


#define CODEMAX 70

uint64_t Getaddress(std::string line_) //As the name
{
    std::string::size_type pos1, pos2;
    pos1 = line_.find("0");//first address is started with '0'
    pos2 = pos1 + 10;
    uint64_t address = strtoul(line_.substr(pos1, pos2-pos1).c_str(), 0, 16);// return the address as unsigned long
    //std::cout << address;
    return address;
}

//the output of readelf is like: 0x000100e8 41118145 22e006e4 2a84ef00 e05103b5 A..E"...*....Q..
//GetCode() can access to the hex form binary code.
std::string GetCode(const std::string line)
{   
    int iter = -1;
    std::string Code;
    std::string::size_type pos1, pos2;
    pos2 = line.find(" ");
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        iter++;
        if(iter>2&&iter<7)
            Code += (line.substr(pos1, pos2-pos1));        
            //std::cout << line.substr(pos1, pos2-pos1) << std::endl;
        pos1 = pos2 + 1;
        pos2 = line.find(" ", pos1);
    }
    return Code;
}

bool GetCodeAndAddress(std::string path_, std::string &Code_, uint64_t &address_)//Geting the binary code for Capstone
{
    int iter=-1;
    std::string line;
    std::string cmd = "readelf -x.text ";
    cmd = cmd + path_;//readelf -x.text software

    FILE *fp=nullptr;

    if((fp=popen(cmd.c_str(),"r"))==nullptr)
    {
        return false;
    }

    char read_str[CODEMAX]="";
    while(fgets(read_str,sizeof(read_str),fp))
    {
        iter++;
        line = read_str;
        if(iter == 0 || iter == 1) 
            continue;//skip the blank line and text
        if(iter == 2)
            address_ = Getaddress(line);
        //std::cout << "Iteration inside output: " << address_ << std::endl;
        Code_ += GetCode(line);
    }
    pclose(fp);
    return true;
    
}







//Testing
int testcode (void)
{
    std::string Code;
    uint64_t address;
    if(GetCodeAndAddress("/home/rory/Desktop/32ia.riscv", Code, address))
    {
        std::cout << address << std::endl;
        std::cout << Code << std::endl;
    }
    return 0;
}
