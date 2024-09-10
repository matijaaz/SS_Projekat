#include "../inc/assembler.hpp"


long Symbol :: ID = 0;
map<string,Symbol> Assembler :: symbolTable;
map<string,Section> Assembler :: sectionTable;
long Assembler :: locationCounter = 0;
long Assembler :: lineCounter = 1;
Assembler* Assembler :: assembler = nullptr;
string Assembler::currSectionName = "";
//---------------------------------------------ISPISI----------------------------------------------------------
void Assembler::createTxtFile(string nameOfFile) {
  string name = nameOfFile;
  regex reg("\\.o$");
  name = regex_replace(name,reg,".txt");
  outputTxt.open(name);
  if (!outputTxt) {
            cout << "Error opening file txt file: " << name << std::endl;
  }

  printSymbolTable();
  printAllSections();
  //outputTxt << "All relocations are : ABS32.\n";

  if (outputTxt.is_open()) {
      outputTxt.close();
  }

}
void Assembler::createObjectFile(string nameOfFile) {
   outputObject.open(nameOfFile, std::ios::binary);
   if (!outputObject) {
        std::cerr << "Error: Could not open file for writing: " << nameOfFile << std::endl;
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
void Assembler::printSymbolTable() {
        outputTxt << "------------------------------------------------------------------------------------\n";
        outputTxt << "                                    SYMBOLS TABLE                                   \n";
        outputTxt << "------------------------------------------------------------------------------------\n";
        outputTxt << "| Name             | Section | Value | Size  | isGlobal | Global | Extern | Number |\n";
        outputTxt << "------------------------------------------------------------------------------------\n";

        for (const auto& entry : symbolTable) {
            const Symbol& symbol = entry.second;
            outputTxt << "| " << std::setw(16) << std::left << symbol.name
                      << " | " << std::setw(7) << std::left << symbol.sectionNumber
                      << " | " << std::setw(5) << std::left << symbol.value
                      << " | " << std::setw(5) << std::left << symbol.size
                      << " | " << std::setw(8) << std::left << (symbol.global ? "Yes" : "No")
                      << " | " << std::setw(6) << std::left << (symbol.isGlobal ? "Yes" : "No")
                      << " | " << std::setw(6) << std::left << (symbol.isExtern ? "Yes" : "No")
                      << " | " << std::setw(6) << std::left << symbol.number
                      << " |\n";
        }

        outputTxt <<  "------------------------------------------------------------------------------------\n";
}

void Assembler::printRelocationTable(const vector<Relocation>& relocations) {
     if (relocations.empty()) {
        return;
    }
    
    outputTxt << "Relocations:" << std::endl;
    outputTxt << "---------------------------------------------------------\n";
    outputTxt << "| Location   | SymTableRef   | Addend    | Type         |\n";
    outputTxt << "---------------------------------------------------------\n";

    for (const auto& relocation : relocations) {
        //cout << relocation.location << endl;
        string type;
        if(relocation.type == ABS32) type = "ABS32";

        outputTxt << "| " << setw(10) << std::right << dec << setfill('0') << uppercase << relocation.location
                  << " | " << setw(13) << std::left << dec << setfill(' ') << relocation.symTableRef 
                  << " | " << setw(9) << std::right << dec << setfill('0') << relocation.addend
                  << " | " << setw(12) << std::left << setfill(' ') << type
                  << " |\n";
    }

}

void Assembler::printCodeTable(const vector<char>& code) {
    if (code.empty()) return;
    outputTxt << "Code:" << std::endl;
    outputTxt << "-----------------------------------------------\n";
    size_t i = 0;
    for (; i + 16 <= code.size(); i += 16) {
        for (size_t j = 0; j < 16; ++j) {
            outputTxt << std::hex << std::right << std::setfill('0') << std::setw(2) << (static_cast<int>(code[i + j]) & 0xFF) << " ";
        }
        outputTxt << std::endl;
    }
    // Ispis preostalih bajtova
    for (; i < code.size(); ++i) {
        outputTxt << std::hex << std::right << std::setfill('0') << std::setw(2) << (static_cast<int>(code[i]) & 0xFF) << " ";
    }
    outputTxt << std::endl;
}

void Assembler::printSectionTable(const Section& section) {
    outputTxt << "\n------------------------------------\n";
    outputTxt << "Section name :" <<  section.name << endl;
   
    printCodeTable(section.code);
    printRelocationTable(section.relocations);

    outputTxt << "------------------------------------\n";
}

void Assembler::printAllSections() {
    for (const auto& entry : Assembler::sectionTable) {
        //cout << sectionTable["math"].relocations.size() << endl;
        printSectionTable(entry.second);
        outputTxt << std::endl; // Razmak između sekcija
    }
}
//---------------------------------------------OBRADA SIMBOLA--------------------------------------------------
void Assembler::insertUND() {
  symbolTable["UND"] = Symbol{"UND",0,0,0,false,false,false};
}

bool Assembler::checkSymbolTable() {
  for (auto it = symbolTable.begin(); it != symbolTable.end(); ++it) { 
    Symbol symbol = it->second;
    if(symbol.sectionNumber == 0 && !symbol.global && symbol.name != "UND") {
      errorMessage = "Error : Local symbol " + symbol.name + " needs to be defined.";
      return false;
    }
  }

  return true;
}

void Assembler::processLabel(string labelName) {

  if(currSectionName == "") {
      semanticError = true;
      errorMessage = "Label " + labelName + " needs to be inside section.";
      return;
  }
  
  auto it = symbolTable.find(labelName);
            
  if (it != symbolTable.end()) {
      //simbol postoji
      if(symbolTable[labelName].sectionNumber != 0) {
        errorMessage = "Label " + labelName + " is defined more than once.";
        semanticError = true;
        return;
      }
      symbolTable[labelName].value = locationCounter;
      symbolTable[labelName].sectionNumber = symbolTable[currSectionName].number;
  } else {
      // dodati simbol
      symbolTable[labelName] = Symbol{labelName,symbolTable[currSectionName].number,locationCounter,-1,false,false,false};

  }
}

long Assembler::convertStringToNumber(string& s) {
    if (s.find("0x") == 0 || s.find("0X") == 0) {
        // Heksadecimalni format
        return (std::stol(s, nullptr, 16) & 0xFFFFFFFF);
    } else if (s.find('0') == 0 && s.length() > 1) {
        // Oktalni format
        return (std::stol(s, nullptr, 8) & 0xFFFFFFFF);
    } else {
        // Decimalni format
        return (std::stol(s, nullptr, 10) & 0xFFFFFFFF);
    }
}

void Assembler::writeData(vector<char> vec,string instruction) {
  if(currSectionName == "") {
      semanticError = true;
      errorMessage = "Instruction " + instruction + " needs to be inside section.";
      return;
  }
  for (auto it = vec.begin(); it != vec.end(); ++it) {
      sectionTable[currSectionName].code.push_back(*it);  
  }

  locationCounter = locationCounter + vec.size();


}

void Assembler::generateRellocations() {
  for (auto it = symbolTable.begin(); it != symbolTable.end(); ++it) {
        string symbolName = it->first;
        Symbol symbol = it->second;
        for (auto it = symbol.symbolUseList.begin(); it != symbol.symbolUseList.end(); ++it) {
          Relocation r;
          if(symbol.sectionNumber == sectionTable[it->sectionName].number) {
           if(it->instruction == "jmp" || it->instruction == "beq" || it->instruction == "bne" || it->instruction == "bgt") {
              int number = symbol.value-it->pc;
              if(number>=-2048 && number <=2047) {
                sectionTable[it->sectionName].code[it->index + 3] = ((number>>4) & 0xFF);
                uint8_t CD = sectionTable[it->sectionName].code[it->index+2];
                sectionTable[it->sectionName].code[it->index+2] = ((CD&0xF0) | (number&0xF));
                uint8_t OCMOD = sectionTable[it->sectionName].code[it->index];
                uint8_t newMOD;
                if(it->instruction == "jmp") newMOD = 0;
                else if(it->instruction == "beq") newMOD = 1;
                else if(it->instruction == "bne") newMOD = 2;
                else if(it->instruction == "bgt") newMOD = 3;
                sectionTable[it->sectionName].code[it->index] = ((OCMOD&0xF0)|newMOD);
                continue;
              }
           }
          }
           r.location = it->offset;
           if(symbolTable[symbolName].global) {
            r.symTableRef = symbolTable[symbolName].number;
           }else {
            r.symTableRef = symbolTable[symbolName].sectionNumber;
            r.addend = symbolTable[symbolName].value;
           } 

           sectionTable[it->sectionName].relocations.push_back(r);
        }
        
       
    }
}

bool Assembler::insertSymbol(string symbolName,string directiveName) {
  
  auto it = symbolTable.find(symbolName);
  if (it != symbolTable.end()) {
        if(directiveName == ".global" || directiveName == ".extern"){
          if(!it->second.isExtern) it->second.isExtern = (directiveName == ".extern");
          if(!it->second.isGlobal) it->second.isGlobal = (directiveName == ".global");
          it->second.global = true;
        }
        else if(directiveName == ".section") {
          if(currSectionName != "") {
            // postaviti size za prethodnu sekciju - trenutna vrednost location countera
            symbolTable[currSectionName].size = locationCounter;
            sectionTable[currSectionName].size = locationCounter;
          }
          locationCounter = symbolTable[symbolName].size;
          currSectionName = symbolName;
        }
        
  } else {

    if (directiveName == ".global") symbolTable[symbolName] = Symbol{symbolName,0,0,-1,true,false,true};
    else if(directiveName == ".extern") symbolTable[symbolName] = Symbol{symbolName,0,0,-1,false,true,true};
    else if(directiveName == ".section") {
        if(currSectionName != "") {
          // postaviti size za prethodnu sekciju - trenutna vrednost location countera
          symbolTable[currSectionName].size = locationCounter;
          sectionTable[currSectionName].size = locationCounter;
        }
        //ubaci u tabelu simbola novootvorenu sekciju
        symbolTable[symbolName] = Symbol{symbolName,0,0,0,false,false,false};
        symbolTable[symbolName].sectionNumber = symbolTable[symbolName].number;

        sectionTable[symbolName] = Section{};
        sectionTable[symbolName].number = symbolTable[symbolName].number;
        sectionTable[symbolName].name = symbolName;

        currSectionName = symbolName;
        locationCounter = 0;
        
    }
        
  }

  return true;
    
}

void Assembler::allocateSpace(string directiveName, string number) {
    if(currSectionName == "") {
      semanticError = true;
      errorMessage = "Directive " + directiveName + " needs to be inside section.";
      return;
    }

    if(directiveName == ".skip") {
        long num = convertStringToNumber(number);
        for(long i = 0 ; i < num ; i++) {
          sectionTable[currSectionName].code.push_back(0);
        }
        locationCounter = locationCounter + num;

        
    }

    if(directiveName == ".word") {

        //proveriti da l je simbol
        if (!number.empty() && isalpha(static_cast<unsigned char>(number[0]))) { 
            string symbol = number;
            SymbolUse s;
            s.sectionName  = currSectionName;
            s.offset = locationCounter; 
            auto it = symbolTable.find(symbol);
            
            if (it != symbolTable.end()) {
              //simbol postoji
              symbolTable[symbol].symbolUseList.push_back(s);
            } else {
              // dodati simbol
              symbolTable[symbol] = Symbol{symbol,0,0,-1,false,false,false};
              symbolTable[symbol].symbolUseList.push_back(s);

            }
            
            for(int i = 0; i < 4; i++) {
              sectionTable[currSectionName].code.push_back(0);
            }

        } 
        
        else {

          long num = convertStringToNumber(number);
          int i = 0;
    
          //Literal l;
          //l.location = locationCounter;
          //l.value = num;
          //sectionTable[currSectionName].literalTable[num] = l;

          while (i < 4) {
            sectionTable[currSectionName].code.push_back(num & 0xFF);
            num = num >> 8;
            i++;
          }

        }
        
        locationCounter = locationCounter + 4;
    }


}
//---------------------------------------------GRAMATIKA-------------------------------------------------------
bool Assembler::detectWhiteSpace(string& line) {
  regex re("^\\s+");
  smatch match;

  if (regex_search(line, match, re)) {
        string whitespaces = match.str();
        // Ispisivanje pronađenih početnih belih znakova
        cout << "Whitespaces detected: \"" << whitespaces << "\"" << std::endl;
        // Uklanjanje početnih belih znakova iz stringa
        line = std::regex_replace(line, re, "");
        return true;
    }
    return false; 
}

bool Assembler::detectComment(string& line) {
  
  regex re("\\s*#.*$");
  smatch match;

  if (regex_search(line, match, re)) {
        string comment = match.str();
        cout << "Comment detected: \"" << comment << "\"" << std::endl;
        line = std::regex_replace(line, re, "");
        return true;
  } 
  return false;
}

bool Assembler::detectDirective(string& line) {

    regex re("^\\.global\\s+([a-zA-Z_]\\w*(?:\\s*,\\s*[a-zA-Z_]\\w*)*)$");
    regex rExt("^\\.extern\\s+([a-zA-Z_]\\w*(?:\\s*,\\s*[a-zA-Z_]\\w*)*)$");
    regex rSection("^\\.section\\s+([a-zA-Z_][a-zA-Z0-9_]*)$");
    regex rWord("^\\.word\\s+((?:[a-zA-Z_][a-zA-Z0-9_]*|\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)(?:,\\s*(?:[a-zA-Z_][a-zA-Z0-9_]*|\\d+|0[xX][0-9a-fA-F]+|0[0-7]+))*)$");
    regex rSkip("^\\.skip\\s+(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)$");
    regex end("^\\.end$");
    regex ascii("^\\.ascii\\s+\"([^\"]+)\"$");
    //(^\.ascii\s+"([^"]+)$)

    smatch match;
    //global
    if (regex_search(line, match, re)) {
        cout << "Directive .global detected :" << std::endl;
        int i = 0;
        for (size_t i = 1; i < match.size(); ++i) {
            cout << " - " << match[i] << std::endl;
            string symbols = match[i];
            regex symbol_regex("\\s*,\\s*");
            sregex_token_iterator iter(symbols.begin(), symbols.end(), symbol_regex, -1);
            sregex_token_iterator end;
            for (; iter != end; ++iter) {
              string symbol = *iter;
              insertSymbol(symbol, ".global");
            }
        }
        line = regex_replace(line, re, "");
        return true;
    } 
    //extern
    if (regex_search(line, match, rExt)) {
        cout << "Directive .extern detected :" << std::endl;
        for (size_t i = 1; i < match.size(); ++i) {
            cout << " - " << match[i] << std::endl;
            string symbols = match[i];
            regex symbol_regex("\\s*,\\s*");
            sregex_token_iterator iter(symbols.begin(), symbols.end(), symbol_regex, -1);
            sregex_token_iterator end;
            for (; iter != end; ++iter) {
              string symbol = *iter;
              insertSymbol(symbol, ".extern");
            }
        }
        line = regex_replace(line, rExt, "");
        return true;
    } 
    //section
    if (regex_search(line, match, rSection)) {
        cout << "Directive .section detected : " + match[1].str() << std::endl;
        insertSymbol(match.str(1),".section");
        line = regex_replace(line, rSection, "");
        return true;
    } 
    //skip
    if (regex_search(line, match, rSkip)) {
        cout << "Directive .skip detected : " + match[1].str() << std::endl;
        allocateSpace(".skip",match.str(1));
        line = regex_replace(line, rSkip, "");
        return true;
    } 
    //word
    if (regex_search(line, match, rWord)) {
        cout << "Directive .word detected :" << std::endl;
        for (size_t i = 1; i < match.size(); ++i) {
          cout << " - " << match[i] << std::endl;
          string symbols = match[i];
          regex symbol_regex("\\s*,\\s*");
          sregex_token_iterator iter(symbols.begin(), symbols.end(), symbol_regex, -1);
          sregex_token_iterator end;
          for (; iter != end; ++iter) {
              string symbol = *iter;
              allocateSpace(".word", symbol);
          }
          
        }
        line = regex_replace(line, rWord, "");
        return true;
    }
    //ascii
    if(regex_search(line,match,ascii)) {
      cout << "Directive .ascii " << match.str(1) <<" detected." << std::endl;
      if(currSectionName == "") {
        semanticError = true;
        errorMessage = "Directive .ascii needs to be inside section.";
        return true;
      }
      string info = match.str(1);
      for(char c : info) {
        sectionTable[currSectionName].code.push_back(c);
      }
      sectionTable[currSectionName].code.push_back(0); // add null character
      locationCounter = locationCounter + info.size() + 1;
      line = regex_replace(line,ascii,"");
      return true;
    } 
    //end
    if (regex_search(line, match, end)) {
        cout << "Directive .end detected." << std::endl;
        line = regex_replace(line, end, "");
        if(currSectionName != "") {
          sectionTable[currSectionName].size = locationCounter;
          symbolTable[currSectionName].size = locationCounter;
          locationCounter = 0;
        }
        endDetected = true;
        return true;
    } 

    return false;
}

bool Assembler::detectInstruction(string& line) {

  regex re("^(halt|int|iret|ret)$");
  regex arInstr("^(xchg|add|sub|mul|div|and|or|xor|shl|shr)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex arOne("^(push|pop|not)\\s+%(r([0-9]|1[0-5])|sp|pc)$");
  regex csrrd("^(csrrd)\\s+%(status|handler|cause),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex csrwr("^(csrwr)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*%(status|handler|cause)");
  regex jmpLiteral("^(call|jmp)\\s+(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)$");
  regex jmpSymbol("^(call|jmp)\\s+([a-zA-Z_][a-zA-Z0-9_]*)$");
  regex condJumpLiteral("^(beq|bne|bgt)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*%(r([0-9]|1[0-5])|sp|pc),\\s*(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)$");
  regex condJumpSymbol("^(beq|bne|bgt)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*%(r([0-9]|1[0-5])|sp|pc),\\s*([a-zA-Z_][a-zA-Z0-9_]*)");
  regex ld$Literal("^(ld)\\s+\\$(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex st$Literal("^(st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*\\$(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)$");
  regex ldLiteral("^(ld)\\s+(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex stLiteral("^(st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)$");
  regex ld$Symbol("^(ld)\\s+\\$([a-zA-Z_][a-zA-Z0-9_]*),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex st$Symbol("^(st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*\\$([a-zA-Z_][a-zA-Z0-9_]*)$");
  regex ldSymbol("^(ld)\\s+([a-zA-Z_][a-zA-Z0-9_]*),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex stSymbol("^(st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*([a-zA-Z_][a-zA-Z0-9_]*)$");

  regex ldStRegDirect("^(ld|st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex ldRegIndirect("^(ld)\\s+\\[%(r([0-9]|1[0-5])|sp|pc)\\],\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex stRegIndirect("^(st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*\\[%(r([0-9]|1[0-5])|sp|pc)\\]$");

  regex ldIndirectRelative("^(ld)\\s+\\[%(r([0-9]|1[0-5])|sp|pc)\\s*\\+\\s*(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)\\],\\s*%(r([0-9]|1[0-5])|sp|pc)$");
  regex stIndirectRelative("^(st)\\s+%(r([0-9]|1[0-5])|sp|pc),\\s*\\[%(r([0-9]|1[0-5])|sp|pc)\\s*\\+\\s*(\\d+|0[xX][0-9a-fA-F]+|0[0-7]+)\\]$");

  smatch match;
  if (regex_search(line, match, re)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        if(match.str() == "halt") writeData(coder->halt(),match.str());
        if(match.str() == "int") writeData(coder->interrupt(),match.str());
        if(match.str() == "ret") writeData(coder->ret(),match.str());
        if(match.str() == "iret") writeData(coder->iret(),match.str());
        line = regex_replace(line, re, "");
        return true;
  } 
  if (regex_search(line, match, arInstr)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string rS = match.str(2);          // Prvi registar
        string rD;
        rD = match.str(4);
        if(instruction == "add" || instruction == "sub" || instruction == "mul" || instruction == "div"){
            writeData(coder->arithmeticOp(instruction,rS,rD),instruction);
        }else if(instruction == "and" || instruction == "xor" || instruction == "or") {
            writeData(coder->logicalOp(instruction,rS,rD),instruction);
        }else if(instruction == "shl" || instruction == "shr"){
            writeData(coder->bitwiseShiftOp(instruction,rS,rD),instruction); 
        }else if(instruction == "xchg") {
          writeData(coder->swap(rS,rD),instruction);
        }     
        line = regex_replace(line, arInstr, "");
        return true;
  }
  if (regex_search(line, match, arOne)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        if(instruction == "not") writeData(coder->logicalOp(instruction,"",reg1),instruction);
        else {
          writeData(coder->stackOperations(instruction,reg1),instruction);
        }
        line = regex_replace(line, arOne, "");
        return true;
  }
  if (regex_search(line, match, csrrd)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        string reg2 = match.str(3);          // Drugi registar
        writeData(coder->csr(instruction,reg1,reg2),instruction);
        line = regex_replace(line, csrrd, "");
        return true;
  }
  if (regex_search(line, match, csrwr)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        string reg2 = match.str(4);          // Drugi registar
        writeData(coder->csr(instruction,reg1,reg2),instruction);
        line = regex_replace(line, csrwr, "");
        return true;
  }
  if (regex_search(line, match, jmpLiteral)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string p = match.str(2);          // literal
        writeData(coder->jmp(p,true,instruction,"",""),instruction);
        line = regex_replace(line, jmpLiteral, "");
        return true;
  }
   if (regex_search(line, match, jmpSymbol)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string p = match.str(2);          // simbol
        writeData(coder->jmp(p,false,instruction,"",""),instruction);
        line = regex_replace(line, jmpSymbol, "");
        return true;
  }
  if (regex_search(line, match, condJumpLiteral)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string rD = match.str(2);          // Prvi registar
        string rS = match.str(4);          // Drugi registar
        string literal = match.str(6); // literal
        writeData(coder->jmp(literal,true,instruction,rD,rS),instruction);
        line = regex_replace(line, condJumpLiteral, "");
        return true;
  }
  if (regex_search(line, match, condJumpSymbol)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string rD = match.str(2);          // Prvi registar
        string rS = match.str(4);          // Drugi registar
        string symbol = match.str(6);     // simbol
        writeData(coder->jmp(symbol,false,instruction,rD,rS),instruction);
        line = regex_replace(line, condJumpSymbol, "");
        return true;
  }
   if (regex_search(line, match, ld$Literal)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string literal = match.str(2);          // Literal
        string reg = match.str(3);          // Registar
        writeData(coder->ldLiteral(literal,reg,false),"ld");
        line = regex_replace(line, ld$Literal, "");
        return true;
    }
    if (regex_search(line, match, ldLiteral)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string literal = match.str(2);          // Literal
        string reg = match.str(3);          // Registar
        writeData(coder->ldLiteral(literal,reg,true),"ld");
        line = regex_replace(line, ldLiteral, "");
        return true;
    }
    if (regex_search(line, match, stLiteral)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg = match.str(2);          // Odredisni registar
        string literal = match.str(4);          // Operand
        writeData(coder->stLiteral(literal,reg),instruction);
        line = regex_replace(line, stLiteral, "");
        return true;
    }
    if (regex_search(line, match, ld$Symbol)) {
        
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string symbol = match.str(2);       // Simbol
        string reg = match.str(3);          // Registar
        writeData(coder->ldSymbol(symbol,reg,false),"ld");
        line = regex_replace(line, ld$Symbol, "");
        return true;
    }
    if (regex_search(line, match, ldSymbol)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string symbol = match.str(2);       // Simbol
        string reg = match.str(3);          // Registar
        writeData(coder->ldSymbol(symbol,reg,true),"ld");
        line = regex_replace(line, ldSymbol, "");
        return true;
    }
    if (regex_search(line, match, stSymbol)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg = match.str(2);          // Odredisni registar
        string symbol = match.str(4);      // Operand
        writeData(coder->stSymbol(symbol,reg),instruction);
        line = regex_replace(line, stSymbol, "");
        return true;
    }
    if (regex_search(line, match, ldStRegDirect)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        string regD = match.str(4);          // Drugi registar
        if(instruction=="ld") writeData(coder->regAddr(regD,reg1,true),instruction);
        else if(instruction == "st") {
          semanticError = true;
          errorMessage = "Illegal operands for st on line : " + to_string(lineCounter) + ".";
          return true;
        }
        line = regex_replace(line, ldStRegDirect, "");
        return true;
    }
    if (regex_search(line, match, ldRegIndirect)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        string regD = match.str(4);          // Drugi registar
        if(instruction=="ld") writeData(coder->regAddr(regD,reg1,false),instruction);
        line = regex_replace(line, ldRegIndirect, "");
        return true;
    }
    if (regex_search(line, match, stRegIndirect)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string regS = match.str(2);          // Prvi registar
        string regD = match.str(4);          // Drugi registar
        writeData(coder->regAddrSt(regD,regS),instruction);
        line = regex_replace(line, stRegIndirect, "");
        return true;
    }
    if (regex_search(line, match, ldIndirectRelative)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        string literal = match.str(4);          // Literal
        string rD = match.str(5); // Destination
        long num = convertStringToNumber(literal);
        if(!(num >= -2048 && num <=2047)) {
          semanticError = true;
          errorMessage = "Literal is too big in this instruction on line : " + to_string(lineCounter) + ".";
          return true;
        }
        writeData(coder->regLiteralIndirect(rD,reg1,literal),instruction);
        line = regex_replace(line, ldIndirectRelative, "");
        return true;
    }
    if (regex_search(line, match, stIndirectRelative)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        string instruction = match.str(1);   // Naziv instrukcije
        string reg1 = match.str(2);          // Prvi registar
        string literal = match.str(6);       // Literal
        string rD = match.str(4); // Destination
        long num = convertStringToNumber(literal);
        if(!(num >= -2048 && num <=2047)) {
          semanticError = true;
          errorMessage = "Literal is too big in this instruction on line : " + to_string(lineCounter) + ".";
          return true;
        }
        writeData(coder->regLiteralIndirectSt(rD,reg1,literal),instruction);
        line = regex_replace(line, stIndirectRelative, "");
        return true;
    }
    if (regex_search(line, match, st$Literal)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        semanticError = true;
        errorMessage = "Illegal operands for st on line : " + to_string(lineCounter) + ".";
        line = regex_replace(line, st$Literal, "");
        return true;
        
    }
    if (regex_search(line, match, st$Symbol)) {
        cout << "Instruction " << match.str() << " detected." << std::endl;
        semanticError = true;
        errorMessage = "Illegal operands for st on line : " + to_string(lineCounter) + ".";
        line = regex_replace(line, st$Symbol, "");
        return true;
    }



  return false;
    
}

bool Assembler::detectLabel(string& line){
    
    regex re("^([a-zA-Z_][a-zA-Z0-9_]*):");

    smatch match;
    if (regex_search(line, match, re)) {
        string labelName = match[1].str();
        cout << "Label detected: \"" << labelName << "\"" << std::endl;
        processLabel(labelName);
        line = regex_replace(line, re, "");
        return true;
    }
    return false; 
}

void Assembler::parseLine(string line) {

    bool directive = false;
    bool instruction = false;
    bool label = false;

    while(line != "") {
      detectWhiteSpace(line); //ako postoje beli znakovi na pocetku linije izbaci ih iz stringa
      //detektuje komentar i vraca se na obradu dalje ako postoji jos nesto u liniji
      if(detectComment(line)) {
        if(line == "") {return;}
      }
      label = detectLabel(line);
      directive = detectDirective(line);
      instruction = detectInstruction(line);
      if(!directive && !label && !instruction) {
        errorDetected = true;
        errorMessage = "Syntax error on line : " + to_string(lineCounter) + ".\n";
        return;
      }
      // provera da li postoji semanticka greska(tj. greska u toku asembliranja)
      if(semanticError) {
        return ;
      }
      
    }

    cout << "\n";
  
}

bool Assembler::startParsing(char* fileInputName) {

  string fileName = "tests/";
  fileName += fileInputName;
  ifstream inputFile(fileName);

  if (!inputFile.is_open()) {
    cout << "Error : Cannot open file " + fileName + "!\n";
    return false;  
  }

  std::string line;
  while (std::getline(inputFile, line)) {
    // Ovde parsiram liniju po liniju iz ulaznog fajla i sklanjam whitespace na kraju linije
    regex trailingSpaces("\\s+$");
    line = regex_replace(line, trailingSpaces, "");
    parseLine(line);
    //proveri prvo da li ima gresaka u liniji i ako ima samo vrati false
    if(errorDetected) {
      inputFile.close();
      return false;
    }
    if(semanticError) {
      inputFile.close();
      return false;
    }
    //proveri da li smo naisli na direktivu .end
    if(endDetected) {
        inputFile.close();
        //provera upotreba .global i .extern kad se zavrsi parsiranje celog fajla
        if(!checkSymbolTable()) {
          return false;
        }
        return true;
    }
    lineCounter++;
  }
  
  
  inputFile.close();
  return true;

}
bool Assembler::parse(int argc, char** argv) {
  assembler->insertUND();
  if (argc != 4) {
    cout << "Error : Bad arguments in command line.\n";
    return false;
  }
  
  regex option("^-o$");
  regex inputFile("^\\w+\\.s$");
  regex outputFile("^\\w+\\.o$");

  if (regex_match(argv[1],option) && regex_match(argv[2],outputFile) && regex_match(argv[3],inputFile)) {
    
    if(startParsing(argv[3])) {
      if(!endDetected) {
        cout << "Error : Directive .end is missing at the end of file."<< "\n\n";
        return false;
      }

      generateRellocations();
      createTxtFile(argv[2]);
      createObjectFile(argv[2]);
      return true;
    }
    else {
      cout << "\n" << errorMessage << "\n";
      return false;
    }
  }
  
  else {
    cout << "Error : Bad arguments in command line.\n";
    return false;
  }
  
}

// ASSEMBLER MAIN

int main(int argc, char** argv) {

  Assembler* assembler = Assembler::getInstance();

  
  if(assembler->parse(argc,argv)) {
    cout << "Object file is generated!\n";
  }else {
    cout << "Object file is not generated!\n";
  };

  return 0;

}


