CXX = g++ -std=c++11
CXXFLAGS = -Wall -O3
LDLIBS = -lSDL2

chaos: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp $(LDLIBS)
	
clean:
	rm -f chaos
