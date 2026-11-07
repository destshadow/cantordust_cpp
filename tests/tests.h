#pragma once
#include "file_navigator.h"
#include "ngram_model.h"
#include "classifier_model.h"
#include "binary_format.h"
#include "binary_reader.h"
#include "metric_map.h"
#include "autocorr.h"
#include "raw_pixels.h"
#include "byte_info.h"
#include "density.h"
#include "range_sel.h"
#include "occurrences.h"
#include "trigraph.h"
#include "cloud_frame.h"
#include "cloud_color.h"
#include "camera3d.h"
#include "dotplot.h"
#include "entropy.h"
#include "view_mode.h"
#include "line_clip.h"
#include "view_tabs.h"
#include "byte_cloud.h"
#include "string_scan.h"
#include "list_state.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <random>

inline void put(std::vector<uint8_t>& b, size_t off, uint64_t value, int width) {
    for (int i = 0; i < width; ++i) b.at(off + i) = uint8_t(value >> (8*i));
}

// Ogni file controlla un argomento; main.cpp li chiama tutti
void testCore();
void testParser();
void testRaw();
void testAutocorr();
void testBytes();
void testStrings();
void testMetricMap();
void testNavigation();
void test3D();
