#include "../inc/linker.hpp"

long Symbol::ID = 0;
Linker* Linker::linker = nullptr;
map<string,string> Linker :: place;
vector<fileInfo> Linker:: infoVector; // informacije o fajlu
map<string,ulong> Linker :: linkerSymbolTable;
map<string,SectionInfo> Linker :: sections;
map<ulong,vector<char>> Linker :: linkerCode;
long Linker::convertStringToNumber(string& s) {
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
void Linker::deserialize(string filename){
  map<string,Symbol>symbolTable;
  map<string,Section> sectionTable;
  ifstream inputObject(filename,std::ios::binary);
   if (!inputObject) {
        std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
        return;
    }

    // Deserijalizacija tabele simbola
    uint64_t number_of_symbol_entries;
    inputObject.read(reinterpret_cast<char*>(&number_of_symbol_entries), sizeof(number_of_symbol_entries));
    //std::cout << "Number of symbol entries: " << number_of_symbol_entries << std::endl;

    for (uint64_t i = 0; i < number_of_symbol_entries; ++i) {
        Symbol symbol;
        size_t nameLength;
        inputObject.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        symbol.name.resize(nameLength);
        inputObject.read(&symbol.name[0], nameLength);
        inputObject.read(reinterpret_cast<char*>(&symbol.sectionNumber), sizeof(symbol.sectionNumber));
        inputObject.read(reinterpret_cast<char*>(&symbol.value), sizeof(symbol.value));
        inputObject.read(reinterpret_cast<char*>(&symbol.size), sizeof(symbol.size));
        inputObject.read(reinterpret_cast<char*>(&symbol.global), sizeof(symbol.global));
        inputObject.read(reinterpret_cast<char*>(&symbol.isGlobal), sizeof(symbol.isGlobal));
        inputObject.read(reinterpret_cast<char*>(&symbol.isExtern), sizeof(symbol.isExtern));
        inputObject.read(reinterpret_cast<char*>(&symbol.number), sizeof(symbol.number));
        symbolTable[symbol.name] = symbol;
    }

    // Deserijalizacija tabele sekcija
    uint64_t number_of_section_entries;
    inputObject.read(reinterpret_cast<char*>(&number_of_section_entries), sizeof(number_of_section_entries));
    //std::cout << "Number of section entries: " << number_of_section_entries << std::endl;

    for (uint64_t i = 0; i < number_of_section_entries; ++i) {
        Section section;
        size_t nameLength;
        inputObject.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        section.name.resize(nameLength);
        inputObject.read(&section.name[0], nameLength);
        inputObject.read(reinterpret_cast<char*>(&section.addrStart), sizeof(section.addrStart));
        inputObject.read(reinterpret_cast<char*>(&section.size), sizeof(section.size));
        inputObject.read(reinterpret_cast<char*>(&section.number), sizeof(section.number));

        uint64_t codeSize;
        inputObject.read(reinterpret_cast<char*>(&codeSize), sizeof(codeSize));
        section.code.resize(codeSize);
        inputObject.read(section.code.data(), codeSize);

        uint64_t relocationCount;
        inputObject.read(reinterpret_cast<char*>(&relocationCount), sizeof(relocationCount));
        //std::cout << "Number of reloaction entries: " << relocationCount << std::endl;
        section.relocations.resize(relocationCount);
        for (auto& relocation : section.relocations) {
            inputObject.read(reinterpret_cast<char*>(&relocation.location), sizeof(relocation.location));
            inputObject.read(reinterpret_cast<char*>(&relocation.symTableRef), sizeof(relocation.symTableRef));
            inputObject.read(reinterpret_cast<char*>(&relocation.addend), sizeof(relocation.addend));
            inputObject.read(reinterpret_cast<char*>(&relocation.type), sizeof(relocation.type));
        }
        sectionTable[section.name] = section;
    }

    

    fileInfo f;
    f.symbolTable = symbolTable;
    f.sectionTable = sectionTable;
    
  
    infoVector.push_back(f);
    inputObject.close();
}

void Linker::countSectionsSize() {
  //izracunavanje velicina sekcija
  for (int i = 0 ; i < infoVector.size() ; i++) {
    map<string,Symbol> symTable = infoVector[i].symbolTable;
    for (auto it = symTable.begin(); it != symTable.end(); ++it) {
        if(it->second.size != -1 && it->second.name != "UND") {
            sections[it->second.name].size += it->second.size;
        }
    }
  }
  map<ulong,long> offsetSize;
  vector<char> code;
  for(auto it = place.begin(); it != place.end() ; it++) {
    sections[it->first].offset = convertStringToNumber(it->second);
    sections[it->first].place = true;
    linkerCode[convertStringToNumber(it->second)] = code;  
    offsetSize[convertStringToNumber(it->second)] = sections[it->first].size;
  }
  long lastOffset = 0;
  //preklapanje
  for(auto code = linkerCode.begin() ; code != linkerCode.end(); code++) {
    long start = code->first;
    long end = code->first + offsetSize[code->first];
    lastOffset = end;
    long startNext;
    auto n = next(code);
    if(n != linkerCode.end()) {
      startNext = n->first;
    }else {
      break;
    }
    if(startNext >= start && startNext < end) {
      string name1;
      string name2;
      for(auto name = place.begin() ; name != place.end() ; name++) {
        if(convertStringToNumber(name->second) == start) {name1 = name->first; continue;}
        if(convertStringToNumber(name->second) == startNext) {name2 = name->first; continue;}
      }
      errorMessage = "Section " + name1 + " overlaps section " + name2 + ".\n";
      return;
    }
  }
  for(auto it = sections.begin(); it != sections.end() ; it++) {
    if(it->second.place) continue;
    it->second.offset = lastOffset;
  }
  linkerCode[lastOffset] = code;

}

void Linker::relocatable() {
  symbolsTable["UND"] = Symbol{"UND",0,0,0,false,false,false};
  for(int i = 0 ; i < infoVector.size(); i++) {
    //dodajemo sve eksterne simbole iz ovog fajla i nazive sekcija
    map<string,Symbol>& symTable = infoVector[i].symbolTable;
    for(auto entry = symTable.begin();entry != symTable.end() ; entry++) {
      if(entry->first == "UND") continue;
      if(entry->second.size != -1) {
        auto it = symbolsTable.find(entry->second.name);
        if(it == symbolsTable.end()) {
          Symbol s; // section
          s.copy(entry->second);
          symbolsTable[entry->second.name] = s;
          symbolsTable[entry->second.name].sectionNumber = symbolsTable[entry->second.name].number;
          Section sec;
          sectionTable[entry->second.name] = sec;
          sectionTable[entry->second.name].name = entry->second.name;
         
        }else {symbolsTable[entry->second.name].size += entry->second.size;
        } 
      }
      if(entry->second.sectionNumber == 0) {
        auto it = symbolsTable.find(entry->second.name);
        if(it == symbolsTable.end()) {
          Symbol s; // extern
          s.copy(entry->second);
          symbolsTable[entry->second.name] = s;
        
        } 
      }
    }

    //iteriranje kroz sekcije tekuceg fajla
    for(auto p = infoVector[i].sectionTable.begin(); p!= infoVector[i].sectionTable.end(); p++) {
      Section& section = p->second;
      string sectionName = section.name;
      vector<char>& code = sectionTable[sectionName].code;
      for(auto sym = infoVector[i].symbolTable.begin(); sym != infoVector[i].symbolTable.end(); sym++) {
        if(sym->second.name == "UND") continue;
        if(sym->second.size != -1) continue;
        if(sym->second.sectionNumber == infoVector[i].symbolTable[sectionName].number && sym->second.size == -1) {
          auto it = symbolsTable.find(sym->second.name);
          Symbol s;
          if(it != symbolsTable.end()) {
            
            //provera da l je globalan
            if(it->second.sectionNumber != 0) {
              errorMessage = "Symbol " + sym->second.name + " has multiple definitions!\n";
              return;
            }else if(it->second.sectionNumber == 0) { 
              //eksteran
              
              s.name = sym->second.name;
            }
          } else {
              //dodaj simbol ako ne postoji
              s.copy(sym->second);symbolsTable[s.name] = s;
             
              

            }
          symbolsTable[s.name].sectionNumber = symbolsTable[sectionName].number;
          symbolsTable[s.name].value = sym->second.value + code.size();
          
           
        }
        
      }
      for(int j = 0; j<section.relocations.size() ; j++ ) {
        Relocation r = section.relocations[j];
        r.location += code.size();
        string name;
        for(auto sym = infoVector[i].symbolTable.begin();sym!=infoVector[i].symbolTable.end();sym++) {
          if(r.symTableRef==sym->second.number) name = sym->second.name;
        }
        auto check = symbolsTable.find(name);
        if(check == symbolsTable.end()) {
          Symbol s;
          s.name = name;
          s.copy(s);
          s.global = true;
          s.isGlobal = true;
          s.sectionNumber = 0;
          symbolsTable[s.name] = s;
        }
        r.symTableRef = symbolsTable[name].number;
        if(symbolsTable[name].size != -1) r.addend += code.size();
        sectionTable[section.name].relocations.push_back(r);
      }
      for(int i = 0; i < section.code.size(); i++) {
        code.push_back(section.code[i]);
      }
    }

    
  }
}
//hex
void Linker::merge() {
  // izracunati velicinu svih sekcija
  countSectionsSize();
  if(errorMessage != "")return;
  // spajanje
  for(int i = 0 ; i < infoVector.size(); i++) {
    map<string,Symbol> symbolTable = infoVector[i].symbolTable;
    for (auto it = infoVector[i].sectionTable.begin() ; it!= infoVector[i].sectionTable.end() ; ++it) {
      Section section = it->second;
      long offset = sections[section.name].offset;
      vector<char>& code = linkerCode[offset];
      bool pom = false;
      int index;
      if(sections[section.name].index != 0) {
        index = sections[section.name].index;
      } else {
        index = code.size();
        pom = true;
      }
      // offset je adresa od koje krece da se smestaju podaci
      long addr = index + offset;
      
      if(!sections[section.name].done) {
        // finalna pocetna adresa sekcije u memoriji
        sections[section.name].startAddress = addr;
        
      }

      for(int i = 0 ; i < section.code.size() ; i++) {
        if(pom) {
          code.push_back(section.code[i]);
        }else {
          code[index] = section.code[i];
        }
        index++;
      }
      sections[section.name].index = index;
      for(int i = 0; i < it->second.relocations.size(); i++) {
        it->second.relocations[i].location += addr; 
      }
      infoVector[i].symbolTable[section.name].value = addr;
      for(auto sym = infoVector[i].symbolTable.begin() ; sym != infoVector[i].symbolTable.end(); sym++) {
        if(sym->second.size == -1 && (sym->second.sectionNumber==symbolTable[section.name].number)) {
           auto it = linkerSymbolTable.find(sym->second.name);
           if(it != linkerSymbolTable.end()) {
              errorMessage = "Symbol " + sym->second.name + " has multiple definitions!\n";
              return;
           } 
           linkerSymbolTable[sym->second.name] = sym->second.value + addr;
        }
      }
      if(!sections[section.name].done) {
        int add = sections[section.name].size - symbolTable[section.name].size;
        for (int j = 0 ; j < add ; j++) {
            code.push_back(0);
            index++;
        }
        sections[section.name].done = true;
      }
    }
  }

  for(auto code = linkerCode.begin() ; code != linkerCode.end(); code++) {
    cout << std::hex << "Start address : " << code->first <<" size : " << std :: dec << code->second.size() << endl;

  }
  cout << "Place of sections in memory : " << endl;
  for(auto section = sections.begin() ; section != sections.end(); section++) {
    long start= section->second.startAddress;
    long end= section->second.startAddress + section->second.size;
    cout << std :: hex;
    cout << "Section : " << section->first << " ,start address: " <<  start  << " ,end address: " << end << endl;  
  }


  //createHexFile();
}

void Linker::resolveRelocations() {
  for (int i = 0; i < infoVector.size() ; i++) {
    for (auto it = infoVector[i].sectionTable.begin() ; it != infoVector[i].sectionTable.end(); it++ ) {
        Section& s = it->second;
        for(int j = 0 ; j < s.relocations.size() ; j++) {
          long patch;
          long location;
          long symRef = s.relocations[j].symTableRef; 
          for(auto sym = infoVector[i].symbolTable.begin() ; sym != infoVector[i].symbolTable.end() ; sym++) {
              if(sym->second.number == symRef) {
                if(sym->second.size != -1) { //znaci da je sekcija referisana
                  patch = sym->second.value + s.relocations[j].addend;
                }else { // onda je referisan simbol
                  auto refSymbol = linkerSymbolTable.find(sym->second.name);
                  if(refSymbol != linkerSymbolTable.end()) {
                    // simbol je pronadjen
                    patch = linkerSymbolTable[sym->second.name];
                    }else {
                      errorMessage = "Symbol " + sym->second.name + " cannot be resolved!\n";
                      return;
                    }
                }
                location = s.relocations[j].location;
                break;
              }
          }
          for(auto code = linkerCode.begin() ; code != linkerCode.end(); code++) {
                if(code->first <= location && location < code->first + code->second.size()) {
                    long index = location - code->first;
                    for(long x = index ;x < index + 4 ; x++) {
                      code->second[x] = (patch&0xFF);
                      patch >>= 8;
                    }
                    break;
                }
          }
        }
    }
  }
}

void Linker::createHexFile(){
  std::ofstream outFile(fileName);
  if (!outFile.is_open()) {
    std::cerr << "Could not open the file " << fileName << " for writing." << std::endl;
    return;
  }
  for (const auto& entry : linkerCode) {
    unsigned long address = entry.first;
    const std::vector<char>& data = entry.second;
    for (size_t i = 0; i < data.size(); ++i) {
      if (i % 8 == 0) {
        outFile << std::setw(4) << std::setfill('0') << std::hex << address + i << ": ";
      }
      outFile << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i] & 0xFF) << " ";
      if (i % 8 == 7) {
        outFile << std::endl;
      }
    }
    if (data.size() % 8 != 0) {
      outFile << std::endl;
    }
  }
  outFile.close();
}
bool Linker::parse(int argc,char** argv) {
  smatch match;
  regex option("^-o$");
  regex hex("^-hex$");
  regex relocatable("^-relocatable$");
  regex outputHex("^\\w+\\.hex$");
  regex inputFile("^\\w+\\.o$");
  regex place("^-place=([a-zA-Z_]\\w*)@(0[xX][0-9a-fA-F]+)$");
  bool outHex = false,checkOptionO = false;
  bool hexFlag = false,relocatableFlag = false;
  bool optionFlag = false,input = false;
  if(argc <= 1) {
    cout << "Command line arguments missing.\n";
    return false;
  }
  // bar jedna od opcija -hex ili -relocatable mora da se navede
  for (int i = 1 ; i < argc; i++) {
   string arg = argv[i];
   // prepoznavanje opcije -o <naziv_izlazne_datoteke>(naziv.o ili naziv.hex)
   if(regex_match(argv[i],option)) {
      optionFlag = true;
      if(input) {
        cout << "Bad arguments in command line.\n";
        return false;
      }
      if(checkOptionO) {
        cout << "Option -o has to be used only once.\n";
      }
      if((i+1)<argc) {
        if(!(regex_match(argv[i+1],outputHex) || regex_match(argv[i+1],inputFile))) {
          cout << "File of type .o or .hex missing after option -o.\n";
          return false;
        }else {
          string pom = argv[i+1];
          smatch m;
          if(regex_search(pom,m,outputHex)) {
            outHex = true;
            fileName = m.str();
          }
          else if(regex_search(pom,m,inputFile)) {
            outHex = false;
            fileName = m.str();
          }
          i++;
          checkOptionO = true;
        }
      }else {
        cout << "File of type .o or .hex missing after option -o.\n";
        return false;
      }
    }
    
    //prepznavanje -hex
    else if(regex_match(argv[i],hex)) {
      optionFlag = true;
      if(input) {
        cout << "Bad arguments in command line.\n";
        return false;
      } 
      if(hexFlag) {
        cout << "Option -hex has to be used only once.\n";
        return false;
      }
      if(relocatableFlag) {
        cout << "Option -hex cant be used together with option -relocatable.\n";
        return false;
      }
      hexFlag = true; }

    //prepznavanje -relocatable
    else if(regex_match(argv[i],relocatable)) {
      optionFlag = true; 
      if(input) {
        cout << "Bad arguments in command line.\n";
        return false;
      }
      if(relocatableFlag) {
        cout << "Option -relocatable has to be used only once.\n";
        return false;
      }
      if(hexFlag) {
        cout << "Option -relocatable cant be used together with option -hex.\n";
        return false;
      }
      relocatableFlag = true; }

    //prepoznavanje -place
    
    else if(regex_search(arg,match,place)) {
      optionFlag = true;
      if(input) {
        cout << "Bad arguments in command line.\n";
        return false;
      }
      // ovde cu da izvucem ime sekcije i adresu preko matcha
      string sectionName = match.str(1);
      string address = match.str(2);
      auto it = Linker::place.find(sectionName);
      if(it != Linker::place.end()) {
        cout << "Section " + sectionName + " start address was defined multiple times!\n";
        return false;
      }
      for(auto it = Linker::place.begin() ; it!= Linker::place.end(); it++) {
        if(convertStringToNumber(it->second) == convertStringToNumber(address)) {
          cout << "Section " + sectionName + " overlaps section " + it->first + "!\n";
          return false;
        }
      }

      Linker::place[sectionName] = address;
    }
      // prepoznaje ulazne .o fajlove za linker
    else if(regex_match(argv[i],inputFile)) {
      if(!optionFlag) {
        cout << "Bad arguments in command line.\n";
        return false;
      }
      deserialize(argv[i]);
      input = true;
    }
      // ni sa jednim regexom nema poklapanja, vrati gresku
    else {
      cout << "Bad arguments in command line.\n";
      return false;
    }
  }

  // dodatne provere oko opcija
  if(hexFlag && !outHex) {
    cout << "Output file needs to be .hex.\n";
    return false;
  }
  if(outHex && relocatableFlag) {
    cout << "Output file needs to be .o.\n";
    return false;
  }
  if(!relocatableFlag && !hexFlag) {
    cout << "At least one option of -relocatable or -hex need to be used.\n";
    return false;
  }
  if(!input) {
    cout << "Input .o file(s) missing.\n";
    return false;
  }


  // poziv funkcije za obradu ucitanih fajlova
  if(hexFlag)merge();
  if(relocatableFlag) Linker::relocatable();
  
  if(errorMessage != "") {
    cout << errorMessage << endl;
    return false;
  }
  
  // razresavanje relokacija
  if(hexFlag)resolveRelocations();

  if(errorMessage != "") {
    cout << errorMessage << endl;
    return false;
  }

  if(hexFlag)createHexFile();
  if(relocatableFlag) {
    createObjectFile(fileName,symbolsTable,sectionTable);
    regex p("\\.o$");
    fileName = regex_replace(fileName,p,".txt");
    printAllDetails(fileName,symbolsTable,sectionTable);
  }

  return true;
}

// LINKER
int main(int argc,char** argv) {
  Linker* linker = Linker::getInstance();
  if(linker->parse(argc,argv)) {
    cout << "Linker generated a file!\n";
  }else {

    cout << "Linker did not generate a file!\n";
  }
  return 0;
}