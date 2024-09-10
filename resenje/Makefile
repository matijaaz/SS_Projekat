CXX=g++

SRCS=src/instructions.cpp src/assembler.cpp
TARGET=assembler
LINKER_TARGET=linker
EMULATOR_TARGET =emulator  

.PHONY: all clean linker emulator clean_exe

all: $(TARGET) # kreiranje asembler.exe

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

linker: src/linker.cpp  # kreiranje linker.exe
	$(CXX) $(CXXFLAGS) -o $(LINKER_TARGET) $^

emulator: src/emulator.cpp src/terminal.cpp # kreiranje emulator.exe
	$(CXX) $(CXXFLAGS) -o $(EMULATOR_TARGET) $^

clean:
	rm -f *.txt
	rm -f *.o
	rm -f *.hex
clean_exe:
	rm -f $(TARGET)
	rm -f $(LINKER_TARGET)
	rm -f $(EMULATOR_TARGET)