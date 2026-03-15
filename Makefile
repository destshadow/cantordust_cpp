CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I libs/
LDFLAGS = -lGL -lX11 -lpng -lpthread -ldl

SRC = src/main.cpp \
      src/binary_reader.cpp \
      src/digraph.cpp \
      src/visualizer.cpp

OUT = cantordust

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

run: all
	./$(OUT)
