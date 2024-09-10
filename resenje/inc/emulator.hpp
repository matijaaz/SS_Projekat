#ifndef EMULATOR_H
#define EMULATOR_H
#include <map>
#include <regex>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "terminal.hpp"
#include "data.hpp"
using namespace std;

struct Machine {
map<uint32_t,uint8_t> memory;
uint32_t registers[16] = {0};
uint32_t control_registers[3] = {0};
bool halt = false;
};

enum Control {
STATUS = 0, HANDLER = 1, CAUSE = 2,
SP = 14 , PC = 15 , R0 = 0
};


class Emulator {
private :
static Emulator* emulator;
string errorMessage;
bool badAddr;
bool terminalInterrupt;

Emulator() {
  errorMessage = "";
  badAddr = false;
  terminalInterrupt = false;
}

uint32_t convertStringToNumber(string& s);
Machine machine;

void emulateCode();
void reset();
void fetchInstruction(Instruction& instruction);
void executeInstruction(Instruction& instruction);
void printProcessorState();
void incPC();

//instructions
void halt();
void arithmeticOP(Instruction& instruction);
void logicalOP(Instruction& i);
void xchg(Instruction& i);
void shift(Instruction& i);
void store(Instruction& i);
void load(Instruction& i);
void jump(Instruction& i);
void call(Instruction& i);
void interrupt(uint32_t cause);


void push(uint32_t word);
uint32_t pop();

void read_keyboard();

//memory
uint32_t mem32Get(uint32_t addr);
void mem32Set(uint32_t address, uint32_t word);

//interrupts
void illegalInstruction();
public : 

Emulator(const Emulator& obj)
    = delete;

static Emulator* getInstance()
  {
    if (emulator == nullptr) 
    {
      emulator = new Emulator(); 
      return emulator; 
    }
    else
    {
    return emulator;
    }
  }

void emulate(int argc,char** argv);
void parse(int argc, char** argv);
string error(){return errorMessage;}
};





#endif