#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <map>
using namespace std;

//OP CODE
enum {
HALT,INTERRUPT,CALL = 2,JMP = 3,BEQ = 3,
BNE = 3, BGT = 3, XCHG = 4, 
ADD = 5,SUB = 5,MUL = 5,DIV = 5,
NOT = 6,AND = 6,OR = 6,XOR = 6,
SHL = 7, SHR = 7 , ST = 8, LD = 9
};

struct Instruction {
  uint8_t OP ;
  uint8_t MODE ;
  uint8_t A;
  uint8_t B;
  uint8_t C;
  uint32_t DDD;
  Instruction() {
    OP = 0; MODE = 0; A = 0;
    B = 0; C = 0; DDD = 0;
  }
};

struct Literal {
  long value;
  long location;
  Literal() {
    value = 0 ; location = 0;
  }
};

class InstructionCoder {

private:

  friend class Assembler;

  static InstructionCoder* coder;
  static map<long,Literal> literalTable;

  InstructionCoder() {

  }

  vector<char> data;
  vector<char> helper;

  void transform(string rS,string rD, Instruction& arOp);
  void compile(Instruction& arOp);
  void help(Instruction dir,string literal,bool symbol);

public :

  InstructionCoder(const InstructionCoder& obj)
    = delete; 

  static InstructionCoder* getInstance()
  {
    if (coder == nullptr) 
    {
      coder = new InstructionCoder(); 
      return coder; 
    }
    else
    {
    return coder;
    }
  }

  long convertStringToNumber(string& s);

  vector<char> halt();
  vector<char> interrupt();
  vector<char> ret();
  vector<char> iret();
  vector<char> arithmeticOp(string name, string rS, string rD);
  vector<char> logicalOp(string name, string rS, string rD);
  vector<char> bitwiseShiftOp(string name, string rS, string rD);
  vector<char> swap(string rS,string rD);
  vector<char> jmp(string operand, bool literal, string instruction,string rd,string rs);
  vector<char> ldLiteral(string literal, string reg, bool direct);
  vector<char> ldSymbol(string symbol, string reg, bool direct);
  vector<char> regAddr(string rD,string rS, bool direct);
  vector<char> regLiteralIndirect(string rD,string rS,string literal);
  vector<char> stackOperations(string instruction,string reg);
  vector<char> stLiteral(string literal, string reg);
  vector<char> stSymbol(string symbol, string reg);
  vector<char> csr(string instruction,string rS, string rD);
  vector<char> regAddrSt(string rD,string rS);
  vector<char> regLiteralIndirectSt(string rD,string rS,string literal);

  void addSymbol(string operand,int pomeraj,string instruction);
  bool check(string symbol,Instruction dir);

}; 

#endif