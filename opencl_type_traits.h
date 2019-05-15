#pragma once

#include "half_precision_fp.h"

template <typename T>
struct OpenClTypeTraits {
    static const char* const type_name;
    static const char* const required_extension;
    static const char* const short_description;
    // TODO add typedefs for different vector types, including half
};

const char* const OpenClTypeTraits<float>::type_name = "float";
// No extensions needed for single precision arithmetic
const char* const OpenClTypeTraits<float>::required_extension = "";
const char* const OpenClTypeTraits<float>::short_description = "single-precision";

const char* const OpenClTypeTraits<double>::type_name = "double";
const char* const OpenClTypeTraits<double>::required_extension = "cl_khr_fp64";
const char* const OpenClTypeTraits<double>::short_description = "double-precision";

const char* const OpenClTypeTraits<half_float::half>::type_name = "half";
const char* const OpenClTypeTraits<half_float::half>::required_extension = "cl_khr_fp16";
const char* const OpenClTypeTraits<half_float::half>::short_description = "half-precision";

// Collect extensions needed for a given type.
template <typename T>
std::vector<std::string> CollectExtensions() {
    std::string required_extension = OpenClTypeTraits<T>::required_extension;
    std::vector<std::string> result;
    if (!required_extension.empty()) {
        result.push_back(required_extension);
    }
    return result;
}
