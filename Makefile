CXX      = g++
# -isystem: libs/ sono librerie esterne, i loro warning non ci riguardano
CXXFLAGS = -std=c++17 -O2 -Wall -isystem libs/
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
      src/binary_format.cpp \
      src/raw_pixels.cpp \
      src/wavelength_rgb.cpp \
      src/hilbert.cpp \
      src/metric_map.cpp \
      src/ngram_model.cpp \
      src/classifier_model.cpp \
      src/byte_cloud.cpp \
      src/one_tuple.cpp \
      src/nav_bars.cpp \
      src/autocorr.cpp \
      src/visualizer_raw.cpp \
      src/visualizer_inspector.cpp

OUT = cantordust
OBJ = $(SRC:src/%.cpp=build/%.o)

# Build incrementale: ricompila solo i .cpp modificati (e quelli che
# includono un header modificato, grazie a -MMD -MP)
all: $(OUT)

$(OUT): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(OUT)

# make run              -> apre /bin/ls
# make run FILE=percorso -> apre quel file
run: all
	./$(OUT) $(FILE)

TEST_OUT ?= build/core_tests
TEST_SRC = tests/core_tests.cpp src/file_navigator.cpp src/ngram_model.cpp \
           src/classifier_model.cpp src/binary_format.cpp src/binary_reader.cpp \
           src/metric_map.cpp src/hilbert.cpp src/wavelength_rgb.cpp \
           src/autocorr.cpp src/raw_pixels.cpp

.PHONY: all clean run test
test:
	mkdir -p $(dir $(TEST_OUT))
	$(CXX) $(CXXFLAGS) -I src $(TEST_SRC) -o $(TEST_OUT)
	./$(TEST_OUT)

-include $(OBJ:.o=.d)
