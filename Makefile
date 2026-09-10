CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I libs/
UNAME    := $(shell uname)

ifeq ($(UNAME), Linux)
    LDFLAGS = -lGL -lX11 -lpng -lpthread -ldl -lstdc++fs
endif
ifeq ($(UNAME), Darwin)
    LDFLAGS = -framework OpenGL -L/usr/X11/lib -lX11 -lpng -lpthread -rpath /usr/X11/lib
    CXXFLAGS += -I/usr/X11/include
endif

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
      src/binary_format.cpp \
      src/raw_pixels.cpp \
      src/wavelength_rgb.cpp \
      src/hilbert.cpp \
      src/metric_map.cpp \
      src/ngram_model.cpp \
      src/classifier_model.cpp \
      src/byte_cloud.cpp \
      src/one_tuple.cpp \
      src/overview_bar.cpp \

OUT = cantordust

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

run: all
	./$(OUT)

TEST_OUT ?= build/core_tests
TEST_SRC = tests/core_tests.cpp src/file_navigator.cpp src/ngram_model.cpp \
           src/classifier_model.cpp src/binary_format.cpp src/binary_reader.cpp \
           src/metric_map.cpp src/hilbert.cpp src/wavelength_rgb.cpp

.PHONY: all clean run test
test:
	mkdir -p $(dir $(TEST_OUT))
	$(CXX) $(CXXFLAGS) -I src $(TEST_SRC) -o $(TEST_OUT)
	./$(TEST_OUT)
