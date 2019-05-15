#pragma once

#include "coordinate_formatter.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "utils/utils.h"

CoordinateFormatter::CoordinateFormatter(size_t total_width, size_t total_height) {
    digits_ = std::max(CalcDigits(total_width), CalcDigits(total_height));
}

int CoordinateFormatter::CalcDigits(size_t i) {
    // This method will be inexact when i is more than 2^53 (numbers larger than that
    // cannot be represented by double exactly)
    // TODO add a check for this?
    return i > 0 ? (static_cast<int>(log10(static_cast<double>(i))) + 1) : 1;
}

std::string CoordinateFormatter::Format(size_t coord) {
    std::stringstream stream;
    stream << std::setw(digits_) << std::setfill('0') << coord;
    return stream.str();
}
