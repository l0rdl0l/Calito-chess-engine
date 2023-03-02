CXX = g++
CXXFLAGS = -O3 --std=c++17 -pthread -march=native -mtune=native
DEPS = game.h engine.h mutexes.h ttable.h bitboard.h eval.h constants.h move.h Makefile
OBJ = move.o game.o engine.o uci.o ttable.o eval.o

CalitoEngine: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp $(DEPS)
	$(CXX) -c -flto $(CXXFLAGS) $< 


clean:
	rm $(OBJ)
	rm CalitoEngine
