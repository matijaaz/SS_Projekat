#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <regex>
#include <fstream>
#include <iomanip>
#include "../inc/instructions.hpp"

using namespace std;


enum {
  ABS32 = 0
};  
struct Relocation {
  long location;
  long symTableRef;
  long addend;
  int type;
  Relocation() {
    location = 0; symTableRef = 0; addend= 0;
    type = ABS32;
  }

};

struct Section {
  string name;
  long addrStart;
  long size;
  long number;
  vector<char> code;
  vector<Relocation> relocations;
  map<long,Literal> literalTable;
  map<string,Literal> poolSymbols;
  Section() {
    name = ""; addrStart = 0; size = 0; number = 0; 
  }
};


struct SymbolUse {
  long offset;
  string sectionName;
  string instruction; // instrukcija koja je koristila simbol
  long index; // index instrukcije koja treba da se prepravi
  long pc; // adresa na koju ukazuje pc posle "instruction"
  SymbolUse() {
    offset = 0; sectionName = "";
    instruction = ""; index = 0;
  }

};

struct Symbol {
  static long ID;
  string name;
  long sectionNumber;
  long value;
  long size;
  bool global;
  bool isGlobal; //if global directive is used
  bool isExtern;//if extern directive is used
  long number;
  vector<SymbolUse> symbolUseList;
  Symbol() {
    name = ""; sectionNumber = 0; value = 0; size = -1;
    isGlobal = false; number = 0; isExtern = false;
  }
   Symbol(const std::string& name, long sectionNumber, long value, long size, bool isGlobal, bool isExtern, bool global)
        : name(name), sectionNumber(sectionNumber), value(value), size(size), isGlobal(isGlobal), isExtern(isExtern), global(global) {

        number = ID;
        ID = ID + 1;
    }
};


class Assembler {

friend class InstructionCoder;

private :

  static Assembler* assembler;
  InstructionCoder* coder;

  static map<string,Symbol> symbolTable;
  static map<string,Section> sectionTable;
  static long locationCounter;
  static long lineCounter;
  static string currSectionName;

  bool errorDetected;
  string errorMessage;
  bool endDetected;
  bool semanticError;
  ofstream outputTxt;
  ofstream outputObject;

  Assembler() {
    errorDetected = false;
    errorMessage = "";
    endDetected = false;
    semanticError = false;
    coder = InstructionCoder::getInstance();
  }

  bool startParsing(char* fileInputName);
  void parseLine(string line);

  bool detectWhiteSpace(string& line);
  bool detectComment(string& line);
  bool detectDirective(string& line);
  bool detectLabel(string& line);
  bool detectInstruction(string& line);

  bool insertSymbol(string symbolName,string directiveName);
  void allocateSpace(string directiveName,string number);
  void processLabel(string labelName);
  void insertUND();
  long convertStringToNumber(string& s);

  void printSymbolTable();
  void printRelocationTable(const vector<Relocation>& relocations);
  void printCodeTable(const vector<char>& code);
  void printSectionTable(const Section& section);
  void printAllSections();

  void createTxtFile(string nameOfFile);
  void createObjectFile(string nameOfFile);
  void generateRellocations();
  bool checkSymbolTable();

  void writeData(vector<char> vec, string instruction);

public :

  Assembler(const Assembler& obj)
    = delete; 


  static Assembler* getInstance()
  {
    if (assembler == nullptr) 
    {
      assembler = new Assembler(); 
      return assembler; 
    }
    else
    {
    return assembler;
    }
  }

  bool parse(int argc, char** argv);

};

#endif