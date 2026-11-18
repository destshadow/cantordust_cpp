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
      src/similarity.cpp \
      src/visualizer_inspector_similarity.cpp \
      src/entropy.cpp \
      src/histogram.cpp \
      src/trigraph.cpp \
      src/renderer3d.cpp \
      src/renderer3d_cube.cpp \
      src/renderer3d_guides.cpp \
      src/cloud_frame.cpp \
      src/cloud_color.cpp \
      src/visualizer_3d_draw.cpp \
      src/file_navigator.cpp \
      src/screenshot.cpp \
      src/input_handler.cpp \
      src/ui_renderer.cpp \
      src/visualizer.cpp \
      src/visualizer_2d.cpp \
      src/visualizer_canvas.cpp \
      src/visualizer_status.cpp \
      src/visualizer_3d.cpp \
      src/visualizer_nav.cpp \
      src/binary_format.cpp \
      src/binary_format_elf.cpp \
      src/binary_format_pe.cpp \
      src/raw_pixels.cpp \
      src/wavelength_rgb.cpp \
      src/hilbert.cpp \
      src/metric_map.cpp \
      src/metric_map_color.cpp \
      src/ngram_model.cpp \
      src/classifier_model.cpp \
      src/template_file.cpp \
      src/byte_cloud.cpp \
      src/string_scan.cpp \
      src/overview.cpp \
      src/visualizer_overview.cpp \
      src/visualizer_inspector_overview.cpp \
      src/signatures.cpp \
      src/signature_checks.cpp \
      src/visualizer_signatures.cpp \
      src/visualizer_cursor.cpp \
      src/visualizer_views.cpp \
      src/visualizer_hex.cpp \
      src/string_tag.cpp \
      src/visualizer_strings.cpp \
      src/visualizer_inspector_string.cpp \
      src/one_tuple.cpp \
      src/nav_bars.cpp \
      src/autocorr.cpp \
      src/autocorr_period.cpp \
      src/pixel_guess.cpp \
      src/visualizer_raw.cpp \
      src/visualizer_options.cpp \
      src/visualizer_inspector.cpp \
      src/visualizer_inspector_file.cpp \
      src/visualizer_inspector_value.cpp \
      src/visualizer_inspector_summary.cpp \
      src/visualizer_legend.cpp \
      src/visualizer_highlight.cpp \
      src/visualizer_links.cpp \
      src/occurrences.cpp \
      src/view_results.cpp \
      src/nav_bars_draw.cpp

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
TEST_SRC = $(wildcard tests/*.cpp) src/file_navigator.cpp src/ngram_model.cpp \
           src/classifier_model.cpp src/template_file.cpp src/binary_format.cpp src/binary_format_elf.cpp \
           src/binary_format_pe.cpp src/binary_reader.cpp \
           src/dotplot.cpp src/similarity.cpp src/entropy.cpp src/metric_map.cpp src/metric_map_color.cpp src/hilbert.cpp src/wavelength_rgb.cpp \
           src/autocorr.cpp src/autocorr_period.cpp src/raw_pixels.cpp src/occurrences.cpp \
           src/trigraph.cpp src/cloud_frame.cpp src/cloud_color.cpp src/byte_cloud.cpp \
           src/string_scan.cpp src/string_tag.cpp \
           src/signatures.cpp src/signature_checks.cpp src/pixel_guess.cpp \
           src/view_results.cpp src/overview.cpp src/digraph.cpp src/histogram.cpp src/one_tuple.cpp

.PHONY: all clean run test golden check-ui
test:
	mkdir -p $(dir $(TEST_OUT))
	$(CXX) $(CXXFLAGS) -I src $(TEST_SRC) -o $(TEST_OUT)
	./$(TEST_OUT)

# Riscrive le impronte delle immagini (tests/test_golden.cpp). Da usare solo
# quando un'immagine cambia apposta.
golden:
	mkdir -p $(dir $(TEST_OUT))
	$(CXX) $(CXXFLAGS) -I src $(TEST_SRC) -o $(TEST_OUT)
	GOLDEN_PRINT=1 ./$(TEST_OUT) > tests/golden_hashes.inc
	@echo "impronte riscritte: $$(wc -l < tests/golden_hashes.inc) immagini"

# Confronta le schermate dell'app con quelle dell'ultimo commit: una riga di
# esito. Serve uno schermo X (vedi tools/check_ui.sh).
check-ui: all
	@bash tools/check_ui.sh

-include $(OBJ:.o=.d)
