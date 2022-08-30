OPT=-g
# OPT=-g -O2
LDFLAGS=$(OPT) -L/opt/local/lib
LDLIBS=-lSDL2 -framework OpenGL -framework Cocoa -framework IOkit
CXXFLAGS=-Wall -I/opt/local/include -std=c++17 $(OPT) -fsigned-char

main: main.o dis6502.o 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

main.o: cpu6502.h dis6502.h

