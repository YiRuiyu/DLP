CXX = g++
TARGET = analyzer
VPATH := src:include
LIB = -lcapstone
OUTPUT = ./software
OBJ = test.o Getbinarycode.o Disasm.o cfg.o loop.o pcg.o pallable.o

CXXFLAGS = -c

$(TARGET): $(OBJ)
	$(CXX) -o $(addprefix $(OUTPUT)/, $(TARGET)) $^ $(LIB)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ 

.PHONY: clean
clean:
	rm -f *.o $(TARGET)