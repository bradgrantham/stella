OPT=-g
# OPT=-g -O2
LDFLAGS=$(OPT) -L/opt/local/lib
LDLIBS=-lSDL2 -framework OpenGL -framework Cocoa -framework IOkit
CXXFLAGS=-Wall -I/opt/local/include -DUSE_BG80D=$(USE_BG80D) -std=c++17 $(OPT) -fsigned-char

main: main.cpp dis6502.cpp
