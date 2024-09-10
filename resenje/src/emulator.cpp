#include "../inc/emulator.hpp"
Emulator* Emulator::emulator = nullptr;

void Emulator::illegalInstruction(){
  interrupt(1);
}

void Emulator::mem32Set(uint32_t address, uint32_t word){
  uint32_t pom = word;
  for(int i = 0 ; i < 4 ; i++) {
    machine.memory[address+i] = word&0xFF;
    word >>= 8; 
  }
  uint8_t c = pom&0xff;
  if(address == TERM_OUT)  write(STDOUT_FILENO, &c, 1);

}

uint32_t Emulator::mem32Get(uint32_t addr) {
  uint32_t first = machine.memory[addr+3];
  uint32_t second = machine.memory[addr+2];
  uint32_t third = machine.memory[addr+1];
  uint32_t fourth = machine.memory[addr+0];
  return (((((first << 8) | second) << 8) | third) << 8) | fourth;
}

void Emulator::read_keyboard(){
  uint8_t c;
  if(read(STDIN_FILENO, &c, 1) > 0) {
		mem32Set(TERM_IN,c);
		terminalInterrupt = true;
	} 
}
//-----------------------------------------------INSTRUCTIONS---------------------------------------------------------------
void Emulator::halt(){
  machine.halt = true;
}

void Emulator::interrupt(uint32_t cause) {
  push(machine.control_registers[STATUS]);
  push(machine.registers[PC]);
  machine.control_registers[CAUSE] = cause;
  machine.control_registers[STATUS] = machine.control_registers[STATUS]|(0x4); //onemogucavamo prekide
  machine.registers[PC] = machine.control_registers[HANDLER];
}

void Emulator::push(uint32_t word){
  machine.registers[SP] = machine.registers[SP] - 4;
  mem32Set(machine.registers[SP],word);
}

uint32_t Emulator::pop(){
  uint32_t word = mem32Get(machine.registers[SP]);
  machine.registers[SP] = machine.registers[SP] + 4;
  return word;
}

void Emulator::call(Instruction& i){
  switch (i.MODE)
  {
  case 0:
    push(machine.registers[PC]);
    machine.registers[PC] = machine.registers[i.A]+machine.registers[i.B]+i.DDD; 
    break;
  case 1:
    push(machine.registers[PC]);
    machine.registers[PC] = mem32Get(machine.registers[i.A]+machine.registers[i.B]+i.DDD);
    break;
  default:
    illegalInstruction();
    break;
  }
}

void Emulator::jump(Instruction& i) {
  switch (i.MODE)
  {
  case 0:
    //cout << std::hex << " A : " << static_cast<int>(i.A) << endl;
    //cout << std::hex;
    machine.registers[PC] = machine.registers[i.A] + i.DDD;
    //cout << machine.registers[PC] << endl;
    break;
  case 1:
    if(machine.registers[i.B] == machine.registers[i.C]) machine.registers[PC] = machine.registers[i.A] + i.DDD;
    break;
  case 2:
    if(machine.registers[i.B] != machine.registers[i.C]) machine.registers[PC] = machine.registers[i.A] + i.DDD;
    break;
  case 3:
     if((signed)machine.registers[i.B] > (signed)machine.registers[i.C]) machine.registers[PC] = machine.registers[i.A] + i.DDD;
    break;
  case 8:
    machine.registers[PC] = mem32Get(machine.registers[i.A] + i.DDD);
    break;
  case 9:
    if(machine.registers[i.B] == machine.registers[i.C]) machine.registers[PC] = mem32Get(machine.registers[i.A] + i.DDD);
    break;
  case 10:
    if(machine.registers[i.B] != machine.registers[i.C]) machine.registers[PC] = mem32Get(machine.registers[i.A] + i.DDD);
    break;
  case 11:
    if((signed)machine.registers[i.B] > (signed)machine.registers[i.C]) machine.registers[PC] = mem32Get(machine.registers[i.A] + i.DDD);
    break;
  default:
    illegalInstruction();
    break;
  }
}
void Emulator::load(Instruction& i){
  switch (i.MODE)
  {
  case 0:
    if(i.A == 0) return;
    machine.registers[i.A] = machine.control_registers[i.B];
    break;
  case 1:
    if(i.A == 0) return;
    machine.registers[i.A] = machine.registers[i.B] + i.DDD; 
    break;
  case 2:
    if(i.A == 0) return;
    machine.registers[i.A] = mem32Get(machine.registers[i.B]+machine.registers[i.C]+i.DDD);
    //cout << "Indeks registra : " << static_cast<int>(i.A) << " " << machine.registers[i.A] << endl;
    break;
  case 3:
    if(i.A != 0) machine.registers[i.A] = mem32Get(machine.registers[i.B]);
    if(i.B != 0) machine.registers[i.B] = machine.registers[i.B] + i.DDD;
    break;
  case 4:
    machine.control_registers[i.A] = machine.registers[i.B];
    break;
  case 5:
    machine.control_registers[i.A] = machine.control_registers[i.B]|i.DDD;
    break;
  case 6:
    machine.control_registers[i.A] = mem32Get(machine.registers[i.B]+machine.registers[i.C]+i.DDD);
    break;
  case 7:
    machine.control_registers[i.A] = mem32Get(machine.registers[i.B]);
    if(i.B != 0) machine.registers[i.B] = machine.registers[i.B] + i.DDD;
    break; 
  default:
    illegalInstruction();
    break;
  }

}

void Emulator::store(Instruction& i){
  switch (i.MODE)
  {
  case 0:
    mem32Set(machine.registers[i.A]+machine.registers[i.B]+i.DDD,machine.registers[i.C]);
    break;
  case 1:
    if(i.A != 0) machine.registers[i.A] = machine.registers[i.A] + i.DDD;
    mem32Set(machine.registers[i.A],machine.registers[i.C]);
    break;
  case 2:
    mem32Set(mem32Get(machine.registers[i.A]+machine.registers[i.B]+i.DDD),machine.registers[i.C]);
    break;
  default:
    illegalInstruction();
  }
}

void Emulator::shift(Instruction& i){
  if(i.A == 0) return;
  switch (i.MODE)
  {
  case 0:
    machine.registers[i.A] = machine.registers[i.B] << machine.registers[i.C]; 
    break;
  case 1:
    machine.registers[i.A] = machine.registers[i.B] >> machine.registers[i.C]; 
    break;
  default:
    illegalInstruction();
  }
}

void Emulator::xchg(Instruction& i){
  uint32_t temp = machine.registers[i.B];
  if(i.B != 0) machine.registers[i.B] = machine.registers[i.C];
  if(i.C != 0) machine.registers[i.C] = temp;
}

void Emulator::arithmeticOP(Instruction& instruction){
  if(instruction.A == 0) return;
  switch (instruction.MODE)
  {
  case 0:
    machine.registers[instruction.A] = machine.registers[instruction.B] + machine.registers[instruction.C];
    break;
  case 1 :
    machine.registers[instruction.A] = machine.registers[instruction.B] - machine.registers[instruction.C];
    break;
  case 2 :
    machine.registers[instruction.A] = machine.registers[instruction.B] * machine.registers[instruction.C];
    break;
  case 3 :
    if(machine.registers[instruction.C] == 0) {illegalInstruction();return;}
    machine.registers[instruction.A] = machine.registers[instruction.B] / machine.registers[instruction.C];
    break;
  default:
    illegalInstruction();
    break;
  }
}

void Emulator::logicalOP(Instruction& i){
  if(i.A == 0) return;
  switch (i.MODE)
  {
  case 0:
    machine.registers[i.A] = ~machine.registers[i.B];
    break;
  case 1:
    machine.registers[i.A] = machine.registers[i.B] & machine.registers[i.C];
    break;
  case 2:
    machine.registers[i.A] = machine.registers[i.B] | machine.registers[i.C];
    break;
  case 3:
    machine.registers[i.A] = machine.registers[i.B] ^ machine.registers[i.C];
    break;
  default:
    illegalInstruction();
  }
}

//--------------------------------------------------------------------------------------------------------------------------

uint32_t Emulator::convertStringToNumber(string& s){
  
  // Heksadecimalni format
  ulong number= std::stoul(s, nullptr, 16);
  if(number > 0xFFFFFFFF) { badAddr = true; return 0;}
  return (number & 0xFFFFFFFF);
   
}

void Emulator::reset(){
 machine.registers[R0] = 0;
 machine.control_registers[STATUS] = 0;
 machine.registers[PC] = 0x40000000;
 machine.memory.clear();
}


void Emulator::printProcessorState(){
  cout << "\n-----------------------------------------------------------------\n";
	printf("Emulated processor executed halt instruction:\n");
	printf("Emulated processor state:\n");
	for (int i = 0; i < 16; i++) {
		if (i < 10) cout << " ";
		printf("r%d=0x%08x   ", i, machine.registers[i]);
		if (i % 4 == 3) cout << endl;
	}
}
void Emulator::incPC(){ machine.registers[PC] = machine.registers[PC] + 1;}

void Emulator::executeInstruction(Instruction& instruction){
  switch (instruction.OP)
  {
  case HALT: halt();break;
  case ARITHMETIC_OP: arithmeticOP(instruction); break;
  case LOGICAL_OP: logicalOP(instruction); break;
  case XCHG: xchg(instruction);break;
  case SHIFT_OP: shift(instruction);break;
  case ST: store(instruction);break;
  case LD: load(instruction);break;
  case JMP: jump(instruction);break;
  case CALL: call(instruction);break;
  case INTERRUPT : interrupt(4); break;
  default:  // interrupt
    illegalInstruction();
    break;
  }
}

void Emulator::fetchInstruction(Instruction& instruction){

  uint8_t OCMOD = machine.memory[machine.registers[PC]];
  instruction.OP = OCMOD >> 4; 
  instruction.MODE = (OCMOD&0xF);
  incPC();
  uint8_t AB = machine.memory[machine.registers[PC]];
  instruction.A = AB >> 4; 
  instruction.B = (AB&0xF);  
  incPC();
  uint8_t CD = machine.memory[machine.registers[PC]];
  instruction.C = CD >> 4;
  incPC();
  uint32_t DDD = machine.memory[machine.registers[PC]];
  instruction.DDD = ((DDD << 4)|(CD&0xF));
  if (instruction.DDD & 0x800)
		instruction.DDD |= 0xfffff000;
  incPC();
}

void Emulator::emulateCode(){
  auto it = machine.memory.find(machine.registers[PC]);
  if(it == machine.memory.end()) {machine.halt = true;} //ako nema koda na adresi 0x40000000,zavrsi emulaciju odmah
  init_terminal();
  while(!machine.halt) {
    Instruction instruction;
    if(machine.registers[PC] >= 0xFFFFFF00) { badAddr=true;return;} // losa adresa
    fetchInstruction(instruction);
    executeInstruction(instruction);
    // obrada prekida
    read_keyboard();
    if(terminalInterrupt) {
      if((machine.control_registers[STATUS]&0x6) == 0) {
        interrupt(3);
        terminalInterrupt = false;
      }
    }
  }
  printProcessorState();
  close_terminal();
}

void Emulator::parse(int argc,char** argv) {
  // parsiranje komandne linije
  regex inputHex("^\\w+\\.hex$");
  if(argc != 2) {
    errorMessage = "Bad arguments in command line.";
    return;
  }
  smatch match;
  string fileName = argv[1];
  if(!regex_search(fileName,match,inputHex)) {errorMessage = "Bad arguments in command line."; return;}

  // prolazak kroz .hex fajl i inicijalizacija memorije
  ifstream inputFile(fileName);
  if (!inputFile.is_open()) {
    errorMessage = "Cannot open file " + fileName + "!";
    return; 
  }
  regex trailingSpaces("\\s+$");
  string line;
  regex pattern("^([0-9A-Fa-f]+):((?: [0-9A-Fa-f]{2})+)$");
  while(getline(inputFile,line)) {
    line = regex_replace(line, trailingSpaces, "");
    if(!regex_search(line,match,pattern)) {errorMessage = "Bad .hex file format!" ; return; }
    string address = match.str(1);
    string bytes = match.str(2);
    regex space(" ");
    sregex_token_iterator iter(bytes.begin(), bytes.end(), space, -1);
    sregex_token_iterator end;
    uint32_t ADDRESS = convertStringToNumber(address);
    //cout << std::hex<< ADDRESS << " "; 
    for (; iter != end; ++iter) {
        if(ADDRESS >= 0xFFFFFF00) { badAddr = true; break;} // 0xFFFFFF00 - 0xFFFFFFFF rezervisano
        string byte = *iter;
        if(byte.empty()) continue; // prazan string
        uint32_t BYTE = convertStringToNumber(byte);
        //cout << std::hex <<  BYTE << " ";
        machine.memory[ADDRESS] = BYTE; 
        ADDRESS++;     
    }
    //cout << endl;
    
    if(badAddr) {errorMessage = "Content of file " + fileName + " cannot be loaded into memory!"; return;}
  
  }
  
}


void Emulator::emulate(int argc,char** argv){
  reset();
  parse(argc,argv);
  if(errorMessage != "") return;
  emulateCode();
  if(badAddr){errorMessage == "Memory access violation!\n";}
 }

// EMULATOR 
int main(int argc,char** argv) {
  Emulator* emulator = Emulator::getInstance();
  emulator->emulate(argc,argv);
  if(emulator->error() != "") cout << emulator->error() << endl; 
  return 0;
}