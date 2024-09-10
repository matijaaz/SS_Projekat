#ifndef LINKER_H
#define LINKER_H
#include <iostream>
#include <regex>
#include <iomanip>
#include "data.hpp"
using namespace std;

struct SectionInfo {
  long index;
  long size;
  bool done;
  bool place;
  long offset;
  long startAddress;
  SectionInfo(){
    index = 0; size = 0; done = false;
    place = false; offset = 0;
    startAddress = 0;
  }
};
class Linker {

private :
  static Linker* linker;
  static map<string,ulong> linkerSymbolTable; // linkerska konacna tabela simbola
  static map<string,string> place; // sadrzi parove adresa - ImeSekcije
  static vector<fileInfo> infoVector; // sadrzi sve informacije o fajlovima po redu iz komandne linije
  static map<string,SectionInfo> sections; // naziv sekcije - (preostali size , index gde treba da se doda kod)
  static map<ulong,vector<char>> linkerCode; //agregirani kod
  
  Linker() {
    errorMessage="";
  }
  void deserialize(string filename);
  void merge();
  void countSectionsSize();
  void resolveRelocations();
  void relocatable();
  string errorMessage;


  long convertStringToNumber(string& s);
  void createHexFile();

  string fileName;
  map<string,Symbol> symbolsTable;
  map<string,Section> sectionTable;
  
public :

  Linker(const Linker& obj)
    = delete;

  static Linker* getInstance()
  {
    if (linker == nullptr) 
    {
      linker= new Linker(); 
      return linker; 
    }
    else
    {
    return linker;
    }
  }

  bool parse(int argc,char** argv);
};




#endif