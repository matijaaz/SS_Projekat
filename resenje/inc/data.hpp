#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

enum {
HALT,INTERRUPT,CALL = 2,JMP = 3,BEQ = 3,
BNE = 3, BGT = 3, XCHG = 4, 
ADD = 5,SUB = 5,MUL = 5,DIV = 5,
NOT = 6,AND = 6,OR = 6,XOR = 6,
SHL = 7, SHR = 7 , ST = 8, LD = 9,
ARITHMETIC_OP = 5 , LOGICAL_OP = 6,
SHIFT_OP = 7
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

  void copy(Symbol s) {
    this->name = s.name; this->sectionNumber = s.sectionNumber;
    this->value = s.value; this->size = s.size; this->global = s.global;
    this->isExtern = s.isExtern; this->isGlobal = s.isGlobal;
    this->number = ID;
    ID++;
  }

};

struct fileInfo{
  map<string,Symbol> symbolTable;
  map<string,Section> sectionTable;
  fileInfo(){}
};


void createObjectFile(string nameOfFile,map<string,Symbol> symbolTable,  map<string,Section> sectionTable) {
   ofstream outputObject;
   outputObject.open(nameOfFile, std::ios::binary);
   if (!outputObject) {
        std::cout << "Error: Could not open file for writing: " << nameOfFile << std::endl;
        return;
    }
        // Serijalizacija tabele simbola
    uint64_t number_of_symbol_entries = 0;
    for (const auto& entry : symbolTable) {
        const Symbol& symbol = entry.second;
        if (!symbol.global && symbol.size == -1) {
            continue;
        }
        number_of_symbol_entries++;
    }
    outputObject.write(reinterpret_cast<const char*>(&number_of_symbol_entries), sizeof(number_of_symbol_entries));

    for (const auto& entry : symbolTable) {
        const Symbol& symbol = entry.second;
        if (!symbol.global && symbol.size == -1) {
            continue;
        }
        size_t nameLength = symbol.name.size();
        outputObject.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        outputObject.write(symbol.name.c_str(), nameLength);
        outputObject.write(reinterpret_cast<const char*>(&symbol.sectionNumber), sizeof(symbol.sectionNumber));
        outputObject.write(reinterpret_cast<const char*>(&symbol.value), sizeof(symbol.value));
        outputObject.write(reinterpret_cast<const char*>(&symbol.size), sizeof(symbol.size));
        outputObject.write(reinterpret_cast<const char*>(&symbol.global), sizeof(symbol.global));
        outputObject.write(reinterpret_cast<const char*>(&symbol.isGlobal), sizeof(symbol.isGlobal));
        outputObject.write(reinterpret_cast<const char*>(&symbol.isExtern), sizeof(symbol.isExtern));
        outputObject.write(reinterpret_cast<const char*>(&symbol.number), sizeof(symbol.number));
    }

    // Serijalizacija tabele sekcija
    uint64_t number_of_section_entries = sectionTable.size();
    outputObject.write(reinterpret_cast<const char*>(&number_of_section_entries), sizeof(number_of_section_entries));

    for (const auto& entry : sectionTable) {
        const Section& section = entry.second;
        size_t nameLength = section.name.size();
        outputObject.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        outputObject.write(section.name.c_str(), nameLength);
        outputObject.write(reinterpret_cast<const char*>(&section.addrStart), sizeof(section.addrStart));
        outputObject.write(reinterpret_cast<const char*>(&section.size), sizeof(section.size));
        outputObject.write(reinterpret_cast<const char*>(&section.number), sizeof(section.number));

        uint64_t codeSize = section.code.size();
        outputObject.write(reinterpret_cast<const char*>(&codeSize), sizeof(codeSize));
        outputObject.write(section.code.data(), codeSize);

        uint64_t relocationCount = section.relocations.size();
        outputObject.write(reinterpret_cast<const char*>(&relocationCount), sizeof(relocationCount));
        for (const auto& relocation : section.relocations) {
            outputObject.write(reinterpret_cast<const char*>(&relocation.location), sizeof(relocation.location));
            outputObject.write(reinterpret_cast<const char*>(&relocation.symTableRef), sizeof(relocation.symTableRef));
            outputObject.write(reinterpret_cast<const char*>(&relocation.addend), sizeof(relocation.addend));
            outputObject.write(reinterpret_cast<const char*>(&relocation.type), sizeof(relocation.type));
        }
    }

    outputObject.close();

}

void printAllDetails(string fileName,map<string,Symbol> symbolTable, map<string,Section> sectionTable) {
    // Print Symbol Table
    ofstream outputTxt(fileName);
    outputTxt << "-------------------------------------------------------------------\n";
    outputTxt << "                            SYMBOLS TABLE                          \n";
    outputTxt << "-------------------------------------------------------------------\n";
    outputTxt << "| Name       | Section | Value | Size  | isGlobal | Global | Extern | Number |\n";
    outputTxt << "-------------------------------------------------------------------\n";
    for (const auto& entry : symbolTable) {
        const Symbol& symbol = entry.second;
        outputTxt << "| " << std::setw(10) << std::left << symbol.name
                  << " | " << std::setw(7) << std::left << symbol.sectionNumber
                  << " | " << std::setw(5) << std::left << symbol.value
                  << " | " << std::setw(5) << std::left << symbol.size
                  << " | " << std::setw(8) << std::left << (symbol.global ? "Yes" : "No")
                  << " | " << std::setw(6) << std::left << (symbol.isGlobal ? "Yes" : "No")
                  << " | " << std::setw(6) << std::left << (symbol.isExtern ? "Yes" : "No")
                  << " | " << std::setw(6) << std::left << symbol.number
                  << " |\n";
    }
    outputTxt << "-------------------------------------------------------------------\n";

    // Print all sections with their code and relocation tables
    for (const auto& entry : sectionTable) {
        const Section& section = entry.second;
        outputTxt << "\n------------------------------------\n";
        outputTxt << "Section name :" <<  section.name << std::endl;
        
        // Print Code Table
        if (!section.code.empty()) {
            outputTxt << "Code:" << std::endl;
            outputTxt << "------------------------------------\n";
            size_t i = 0;
            for (; i + 16 <= section.code.size(); i += 16) {
                for (size_t j = 0; j < 16; ++j) {
                    outputTxt << std::hex << std::right << std::setfill('0') << std::setw(2) << (static_cast<int>(section.code[i + j]) & 0xFF) << " ";
                }
                outputTxt << std::endl;
            }
            for (; i < section.code.size(); ++i) {
                outputTxt << std::hex << std::right << std::setfill('0') << std::setw(2) << (static_cast<int>(section.code[i]) & 0xFF) << " ";
            }
            outputTxt << std::endl;
        }

        // Print Relocation Table
        if (!section.relocations.empty()) {
            outputTxt << "Relocations:" << std::endl;
            outputTxt << "------------------------------------------\n";
            outputTxt << "| Location   | SymTableRef   | Addend    |\n";
            outputTxt << "------------------------------------------\n";
            for (const auto& relocation : section.relocations) {
                outputTxt << "| " << std::setw(10) << std::right << std::dec << std::uppercase << relocation.location
                          << " | " << std::setw(13) << std::left << std::dec << std::setfill(' ') << relocation.symTableRef 
                          << " | " << std::setw(9) << std::right << std::dec << std::setfill('0') << relocation.addend
                          << " |\n";
            }
        }
        
        outputTxt << "------------------------------------\n";
        outputTxt << std::endl; // Razmak između sekcija
    }
}


#endif