#include "../inc/instructions.hpp"
#include "../inc/assembler.hpp"

map<long,Literal> InstructionCoder::literalTable;
InstructionCoder* InstructionCoder::coder = nullptr;

bool InstructionCoder::check(string symbol,Instruction dir) {
  auto it = Assembler::sectionTable[Assembler::currSectionName].poolSymbols.find(symbol);
  if (it != Assembler::sectionTable[Assembler::currSectionName].poolSymbols.end()) {

      //simbol postoji u bazenu
      long pomeraj = (Assembler::locationCounter + 4) - Assembler::sectionTable[Assembler::currSectionName].poolSymbols[symbol].location;
      if(pomeraj >= -2048 && pomeraj <= 2047) { // pomeraj staje u 12 bita kao signed broj
        dir.DDD = - pomeraj; 
        compile(dir);
        return true;
      }
  }

  return false;
}

void InstructionCoder::help(Instruction dir,string literal, bool symbol) {
      Instruction jmp;
      jmp.OP = JMP;
      jmp.MODE = 0; 
      jmp.A = 15;
      jmp.DDD = 4;
      compile(jmp);

      if(symbol){
        for(int i = 0; i < 4; i++) {
          data.push_back(0);
        }
      }else {
        long DDD = convertStringToNumber(literal);
        for(int i = 0; i < 4; i++) {
        data.push_back(DDD&0xFF);
        DDD >>= 8;
        }
      }
      

      dir.DDD = -8;
      compile(dir); 

      if(symbol) {
        addSymbol(literal,4,"");
        Literal l;
        l.location = Assembler::locationCounter + 4;
        l.value = 0;
        Assembler::sectionTable[Assembler::currSectionName].poolSymbols[literal] = l;
      }else {
        
        //dodati literal u tabelu litarala
        Literal l;
        l.location = Assembler::locationCounter + 4;
        l.value = convertStringToNumber(literal);
        Assembler::sectionTable[Assembler::currSectionName].literalTable[l.value] = l;

      }
      
}
void InstructionCoder::addSymbol(string operand,int pomeraj,string instruction){
    auto it = Assembler::symbolTable.find(operand);
    SymbolUse s;
    s.sectionName  = Assembler::currSectionName;
    s.offset = Assembler::locationCounter + pomeraj;
    s.instruction = instruction;
    if(instruction == "jmp" || instruction == "bne" || instruction == "bgt" || instruction == "beq") {
        s.index = Assembler::sectionTable[Assembler::currSectionName].code.size() + 8; 
        s.pc = Assembler::locationCounter + 12;
    }      
    if (it != Assembler::symbolTable.end()) {
        //simbol postoji
        Assembler::symbolTable[operand].symbolUseList.push_back(s);
    } else {
        // dodati simbol
        Assembler::symbolTable[operand] = Symbol{operand,0,0,-1,false,false,false};
        Assembler::symbolTable[operand].symbolUseList.push_back(s);
    }
}
long InstructionCoder::convertStringToNumber(string& s) {
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

void InstructionCoder::transform(string rS,string rD, Instruction& arOp) {
  

  if(rS == "sp") arOp.C = 14;
  else if(rS == "pc") arOp.C = 15;

  if(rD == "sp") {
    arOp.A = 14;
    arOp.B = 14;
  }
  else if(rD == "pc") {
    arOp.A = 15;
    arOp.B = 15;
  }

  if(rS[0] == 'r') {
    int numS = stoi(rS.substr(1)); 
    arOp.C = numS;
  }

  if(rD[0] == 'r') {
    int numD = stoi(rD.substr(1)); 
    arOp.A = numD;
    arOp.B = numD;
  }

  if(rS == "status") arOp.C = 0;
  else if(rS == "handler") arOp.C = 1;
  else if(rS == "cause") arOp.C = 2;

  if(rD == "status") {
    arOp.A = 0;
    arOp.B = 0;
  }
  else if(rD == "handler") {
    arOp.A = 1;
    arOp.B = 1;
  }
  else if(rD == "cause") {
    arOp.A = 2;
    arOp.B = 2;
  }
}

void InstructionCoder::compile(Instruction& arOp) {

  //OCMOD
  uint8_t OCMOD;
  OCMOD = arOp.OP;
  OCMOD = ((OCMOD << 4) | arOp.MODE);
  data.push_back(OCMOD); 

  //AB
  uint8_t rArB;
  rArB = arOp.A;
  rArB = ((rArB << 4) | arOp.B);
  data.push_back(rArB);

  //CD
  uint8_t rCD;
  rCD = arOp.C;
  rCD = ((rCD << 4) | (arOp.DDD&0xF));
  data.push_back(rCD);

  //DD
  uint8_t DD;
  DD = ((arOp.DDD>>4) & 0xFF);
  data.push_back(DD);
}
// INSTRUCTION FORMAT 

// OCMOD AB CD DD , a u memoriji - DD CD AB OCMOD

vector<char> InstructionCoder::halt() { // op = 0000, mode = 0000
  data.clear();
  uint8_t OCMOD;
  OCMOD = HALT;
  OCMOD = ((OCMOD << 4) | 0x0);
  data.push_back(OCMOD);
  for(int i = 0 ; i < 3 ; i++) {
    data.push_back(0);
  }
  return data;
}

vector<char> InstructionCoder::interrupt() {
  data.clear(); 
  uint8_t OCMOD;
  OCMOD = INTERRUPT;
  OCMOD = ((OCMOD << 4) | 0x0);  
  data.push_back(OCMOD); // op = 0001, mode = 0000
  for(int i = 0 ; i < 3 ; i++) {
    data.push_back(0);
  }
  return data;
}

vector<char> InstructionCoder::arithmeticOp(string name, string rS, string rD) {
  // add - op = 0101, mode = 0000
  // sub - op = 0101, mode = 0001
  // mul - op = 0101, mode = 0010
  // div - op = 0101, mode = 0011
  
  data.clear();
  Instruction arOp;
  if(name == "add") {
    arOp.OP = ADD;
    arOp.MODE = 0x0000;
  }
  else if(name == "sub") {
    arOp.OP = SUB;
    arOp.MODE = 0x0001;
  }
  else if(name == "mul") {
    arOp.OP = MUL;
    arOp.MODE = 2;
  }
  else if(name == "div") {
    arOp.OP = DIV;
    arOp.MODE = 3;
  }

  transform(rS,rD,arOp);
  
  compile(arOp);

  return data;

}

vector<char> InstructionCoder::logicalOp(string name, string rS, string rD){
  data.clear();
  // not - op = 0110, mode = 0000
  // and - op = 0110, mode = 0001
  // or - op = 0110, mode = 0010
  // xor - op = 0110, mode = 0011

  Instruction i;

  if(name == "not") {
    i.OP = NOT;
    i.MODE = 0;
  }
  else if(name == "and") {
    i.OP = AND;
    i.MODE = 1;
  }
  else if(name == "or") {
    i.OP = OR;
    i.MODE = 2;
  }
  else if(name == "xor") {
    i.OP = XOR;
    i.MODE = 3;
  }

  transform(rS,rD,i);

  compile(i);

  return data;

}

vector<char> InstructionCoder::bitwiseShiftOp(string name, string rS, string rD) {
  data.clear();
  // shl - op = 0111, mode = 0000
  // shr - op = 0111, mode = 0001

  Instruction i;

  if(name == "shl") {
    i.OP = SHL;
    i.MODE = 0;
  }
  else if(name == "shr") {
    i.OP = SHR;
    i.MODE = 1;
  }
  transform(rS,rD,i);

  compile(i);

  return data;
}

vector<char> InstructionCoder::swap(string rS,string rD){
  // xchg - op = 0100, mode = 0000

  data.clear();

  Instruction i;

  i.OP = XCHG;
  i.MODE = 0;

  transform(rS,rD,i);

  i.A = 0;

  compile(i);

  return data;
}

vector<char> InstructionCoder::jmp(string operand, bool literal,string instruction,string rd,string rs){
  // jmp literal - op = 0011, mode = 0000
  data.clear();
  Instruction imm; 
  Instruction dir;
  if(instruction == "jmp") {
    imm.OP = JMP;
    imm.MODE = 0;
    imm.A = 15;
    dir.OP = JMP;
    dir.MODE = 8;
    dir.A = 15;  
  }else if(instruction == "call") {
    imm.OP = CALL;
    imm.MODE = 0;
    imm.A = 15;
    dir.OP = CALL;
    dir.MODE = 1;
    dir.A = 15; 
  }else if(instruction =="beq") {
    transform(rs,rd,imm);
    imm.OP = BEQ;
    imm.MODE = 1;
    imm.A = 15;
    transform(rs,rd,dir);
    dir.OP = BEQ;
    dir.MODE = 9;
    dir.A = 15;
  }else if(instruction =="bne") {
    transform(rs,rd,imm);
    imm.OP = BNE;
    imm.MODE = 2;
    imm.A = 15;
    transform(rs,rd,dir);
    dir.OP = BNE;
    dir.MODE = 10;
    dir.A = 15;
  }else if(instruction =="bgt") {
    transform(rs,rd,imm);
    imm.OP = BGT;
    imm.MODE = 3;
    imm.A = 15;
    transform(rs,rd,dir);
    dir.OP = BGT;
    dir.MODE = 11;
    dir.A = 15;
  }
  Instruction i;
  i.OP = imm.OP;
  i.MODE = imm.MODE;
  i.B = imm.B;
  i.C = imm.C;
  if(literal) {

    transform("","",i);
    long DDD = convertStringToNumber(operand);
    if(DDD >= -2048 && DDD <= 2047) { // ne sme negativno svakako , u asembleru je provera
      i.DDD = DDD;
      compile(i);
    }
    else {
      // ako ne moze da stane u 12 bita literal , kao odrediste skoka

      auto it = Assembler::sectionTable[Assembler::currSectionName].literalTable.find(convertStringToNumber(operand));
      if (it != Assembler::sectionTable[Assembler::currSectionName].literalTable.end()) {

        //literal postoji u tabeli literala, odma ugradi jmp do njega
        long pomeraj = (Assembler::locationCounter + 4) - Assembler::sectionTable[Assembler::currSectionName].literalTable[convertStringToNumber(operand)].location;
        if(pomeraj >= -2048 && pomeraj <= 2047) { // pomeraj staje u 12 bita kao signed broj
            i.OP = dir.OP;
            i.MODE = dir.MODE;
            i.A = dir.MODE;
            i.DDD = -pomeraj;
            compile(i);
            return data;
        }
      }

      // ako literal nije u tabeli literala :
      // jmp pc(4), op - 0011, mode = 0000
      // .word literal
      // jmp (pc-8) , op - op - 0011, mode = 1000
      Instruction jmp;

      jmp.OP = JMP;
      jmp.MODE = 0; 
      jmp.A = 15;
      jmp.DDD = 4;
      compile(jmp);

      long DDD = convertStringToNumber(operand);
      for(int i = 0; i < 4; i++) {
        data.push_back(DDD&0xFF);
        DDD >>= 8;
      }

      i.OP = dir.OP;
      i.MODE = dir.MODE;
      i.A = dir.A;
      i.DDD = -8;
      compile(i); 

      //dodati literal u tabelu litarala
      Literal l;
      l.location = Assembler::locationCounter + 4;
      l.value = convertStringToNumber(operand);
      Assembler::sectionTable[Assembler::currSectionName].literalTable[l.value] = l;
    }
    
  }
  else {
    // ako je simbol

    auto it = Assembler::symbolTable.find(operand);
            
    if (it != Assembler::symbolTable.end()) {
      //simbol postoji
      if(it->second.sectionNumber == Assembler::sectionTable[Assembler::currSectionName].number) {
        int number = (Assembler::locationCounter+4)-it->second.value;
        if(number>=-2048 && number <=2047) {
          i.A = imm.A;
          i.DDD = -number;
          compile(i);
          return data;
        }
      }
    } 

    if(check(operand,dir)){
      return data;
    }
    
    Instruction jmp;

    jmp.OP = JMP;
    jmp.MODE = 0; 
    jmp.A = 15;
    jmp.DDD = 4;
    compile(jmp);

    for(int i = 0 ; i < 4 ; i++) {
      data.push_back(0);
    }

    if(instruction == "call") instruction ="jmp";
    addSymbol(operand,4,instruction);

    i.OP = dir.OP;
    i.MODE = dir.MODE;
    i.A = dir.A;
    i.DDD = -8;
    compile(i); 

    Literal l;
    l.location = Assembler::locationCounter + 4;
    l.value = 0;
    Assembler::sectionTable[Assembler::currSectionName].poolSymbols[operand] = l;

  }

  return data;
}

vector<char> InstructionCoder::ldLiteral(string literal, string reg, bool direct){
  //direct - ako je false ne treba citati iz memorie , ako je true onda treba citati iz memorije
  data.clear();
  Instruction imm; 
  Instruction dir;
  cout << literal << endl;
  transform("",reg,dir);
  dir.OP = LD;
  dir.MODE = 2;
  dir.B = 0;
  dir.C = 15;

  transform("",reg,imm);
  imm.OP = LD;
  imm.MODE = 1;
  imm.B = 0;
  imm.C = 0;

  Instruction i;
  if(!direct) {
    // example : ld $1,%r1 => r1 <= 1
    long DDD = convertStringToNumber(literal);
    if(DDD >= -2048 && DDD <= 2047) { 
      imm.DDD = DDD;
      compile(imm);
    }
    else {
      // ako ne moze da stane u 12 bita literal , kao odrediste skoka

      auto it = Assembler::sectionTable[Assembler::currSectionName].literalTable.find(convertStringToNumber(literal));
      if (it != Assembler::sectionTable[Assembler::currSectionName].literalTable.end()) {

        //literal postoji u tabeli literala
        long pomeraj = (Assembler::locationCounter + 4) - Assembler::sectionTable[Assembler::currSectionName].literalTable[convertStringToNumber(literal)].location;
        if(pomeraj >= -2048 && pomeraj <= 2047) { // pomeraj staje u 12 bita kao signed broj
            dir.DDD = - pomeraj;
            compile(dir);
            return data;
        }
      }

      // ako literal nije u tabeli literala ili jeste ali je pomeraj postao prevelik :
      // jmp pc(4), op - 0011, mode = 0000
      // .word literal
      // relativni pomeraj do literala
      help(dir,literal,false);
    }
    
  }else {

    // example ld 1, %r1 => r1 <= mem32[1]
    // preko dve instrukcije ako je literal veci od 12 bita
    long DDD = convertStringToNumber(literal);
    if(DDD >= -2048 && DDD <= 2047) { 
      dir.DDD = DDD;
      dir.C = 0;
      compile(dir);
    }
    else {

      // ako ne moze da stane u 12 bita literal , kao odrediste skoka
      auto it = Assembler::sectionTable[Assembler::currSectionName].literalTable.find(convertStringToNumber(literal));
      if (it != Assembler::sectionTable[Assembler::currSectionName].literalTable.end()) {
        //literal postoji u tabeli literala
        long pomeraj = (Assembler::locationCounter + 4) - Assembler::sectionTable[Assembler::currSectionName].literalTable[convertStringToNumber(literal)].location;
        if(pomeraj >= -2048 && pomeraj <= 2047) { // pomeraj staje u 12 bita kao signed broj
            dir.DDD = - pomeraj;
            compile(dir);
            // dodati jos registarsko indirektno adresiranje
            transform("",reg,dir);
            dir.C = 0;
            dir.DDD = 0;
            compile(dir);            
            return data;
        }
      }
      help(dir,literal,false);
      transform("",reg,dir);
      dir.C = 0;
      dir.DDD = 0;
      compile(dir);
    }
}
  return data;
}


vector<char> InstructionCoder::ldSymbol(string symbol, string reg, bool direct){
  data.clear();
  Instruction imm; 
  Instruction dir;
  transform("",reg,dir);
  dir.OP = LD;
  dir.MODE = 2;
  dir.B = 0;
  dir.C = 15;

  transform("",reg,imm);
  imm.OP = LD;
  imm.MODE = 1;
  imm.B = 0;
  imm.C = 0;

  if(!direct) {
    if(check(symbol,dir)){
      return data;
    }
    help(dir,symbol,true);
  } else {

    if(!check(symbol,dir)){
      help(dir,symbol,true);
    }
    
    // dodati jos registarsko indirektno adresiranje
    transform("",reg,dir);
    dir.C = 0;
    dir.DDD = 0;
    compile(dir);            
    return data;
  }

  return data;

}

vector<char> InstructionCoder::regAddr(string rD,string rS, bool direct){
  data.clear();
  Instruction indir; 
  Instruction dir;
  transform(rS,rD,dir);
  dir.OP = LD;
  dir.B = dir.C;
  dir.C = 0;
  dir.MODE = 1;

  transform(rS,rD,indir);
  indir.OP = LD;
  indir.MODE = 2;
  indir.B = indir.C;
  indir.C = 0;

  if(direct) {
    compile(dir);
  }else {
    compile(indir);

  }

  return data;
}

vector<char> InstructionCoder::regLiteralIndirect(string rD,string rS,string literal){
  data.clear();
  Instruction dir;
  transform(rS,rD,dir);
  dir.OP = LD;
  dir.B = dir.C;
  dir.C = 0;
  dir.MODE = 2;
  dir.DDD = convertStringToNumber(literal);
  compile(dir);
  return data;
}


vector<char> InstructionCoder::stackOperations(string instruction,string reg){
  data.clear();
  Instruction i;
  if(instruction == "pop") {
    i.OP = LD;
    i.MODE = 3;
    transform("",reg,i);
    i.B = 14;
    i.DDD = 4;
  }else if(instruction == "push"){
    i.OP = ST;
    i.MODE = 1;
    transform(reg,"",i);
    i.A = 14;
    i.DDD = -4;
  }

  compile(i);
  return data;

}

vector<char> InstructionCoder::ret(){
  data.clear();
  Instruction i;
  i.OP = LD;
  i.MODE = 3;
  i.A = 15;
  i.B = 14;
  i.DDD = 4;
  compile(i);
  return data;
}

vector<char> InstructionCoder::csr(string instruction,string rS, string rD){
  data.clear();
  Instruction i;
  i.OP = LD;
  if(instruction == "csrrd") {
    i.MODE = 0;
    transform(rS,rD,i);
    i.B = i.C;
    i.C = 0;
  }else if(instruction == "csrwr") {
    i.MODE = 4;
    transform(rS,rD,i);
    i.B = i.C;
    i.C = 0;
  }

  compile(i);
  return data;
}


vector<char> InstructionCoder::iret(){
  data.clear();

  Instruction add; // sp <= sp + 8
  add.OP = LD;
  add.MODE = 1;
  add.A = 14;
  add.B = 14;
  add.DDD = 8;
  compile(add);

  Instruction status; // ld status
  status.OP = LD;
  status.MODE = 6;
  status.A = 0;
  status.B = 14;
  status.DDD = -4;
  compile(status);

  Instruction pc; // ld pc
  pc.OP = LD;
  pc.MODE = 2;
  pc.A = 15;
  pc.B = 14;
  pc.DDD = -8;
  compile(pc);

  return data;

}


vector<char> InstructionCoder::stLiteral(string literal, string reg){
  data.clear();
  Instruction imm;
  imm.OP = ST;
  imm.MODE = 0;
  transform(reg,"",imm);

  Instruction dir;
  dir.OP = ST;
  dir.MODE = 2;
  dir.A = 15;
  transform(reg,"",dir);

  long DDD = convertStringToNumber(literal);
  if(DDD >= -2048 && DDD <= 2047) { 
    imm.DDD = DDD;
    compile(imm);
  }else {

    auto it = Assembler::sectionTable[Assembler::currSectionName].literalTable.find(convertStringToNumber(literal));
    if (it != Assembler::sectionTable[Assembler::currSectionName].literalTable.end()) {

        //literal postoji u tabeli literala
        long pomeraj = (Assembler::locationCounter + 4) - Assembler::sectionTable[Assembler::currSectionName].literalTable[convertStringToNumber(literal)].location;
        if(pomeraj >= -2048 && pomeraj <= 2047) { // pomeraj staje u 12 bita kao signed broj
            dir.DDD = - pomeraj; 
            compile(dir);
            return data;
        }
      } 

    help(dir,literal,false);

  }


  return data;
}



vector<char> InstructionCoder::stSymbol(string symbol, string reg){

  data.clear();
  Instruction imm;
  imm.OP = ST;
  imm.MODE = 0;
  transform(reg,"",imm);

  Instruction dir;
  dir.OP = ST;
  dir.MODE = 2;
  dir.A = 15;
  transform(reg,"",dir);


  //provera da li se simbol nalazi u bazenu, potencijalno uraditi i za sve ostalo
  if(check(symbol,dir)) {
    return data;
  }

  help(dir,symbol,true);


  return data;
}

vector<char> InstructionCoder::regAddrSt(string rD,string rS){
  data.clear();
  Instruction dir;
  dir.OP = ST;
  dir.MODE = 0;
  transform(rS,rD,dir);
  dir.B = 0;
  compile(dir);
  return data;
}


vector<char> InstructionCoder::regLiteralIndirectSt(string rD,string rS,string literal){
  data.clear();
  Instruction i;
  i.OP = ST;
  i.MODE = 0;
  transform(rS,rD,i);
  i.B = 0;
  i.DDD = convertStringToNumber(literal);
  compile(i);
  return data;

}



