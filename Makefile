CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I libs/
LDFLAGS = -lGL -lX11 -lpng -lpthread -ldl -lstdc++fs

SRC = src/main.cpp \
      src/binary_reader.cpp \
      src/digraph.cpp \
      src/dotplot.cpp \
      src/entropy.cpp \
      src/histogram.cpp \
      src/trigraph.cpp \
      src/renderer3d.cpp \
      src/file_navigator.cpp \
      src/screenshot.cpp \
      src/input_handler.cpp \
      src/ui_renderer.cpp \
      src/visualizer.cpp \
      src/visualizer_2d.cpp \
      src/visualizer_3d.cpp \
      src/visualizer_nav.cpp \
      src/visualizer_sections.cpp \
      src/binary_format.cpp

OUT = cantordust

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

run: all
	./$(OUT)
