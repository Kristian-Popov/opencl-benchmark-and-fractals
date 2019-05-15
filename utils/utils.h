#pragma once

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/compute.hpp"
#include "duration.h"
#include "half_precision_fp.h"

namespace Utils {
std::string ReadFile(const std::string& fileName);

template <typename T>
std::string VectorToString(const std::vector<T>& v, const std::string& delimiter = ", ") {
    std::stringstream result;
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(result, delimiter.c_str()));
    return result.str();
}

template <typename T>
void AppendVectorToVector(T& v1, const T& v2) {
    v1.insert(v1.end(), v2.begin(), v2.end());
}

template <typename D, typename T>
D MeasureDuration(boost::compute::future<T>& future) {
    future.wait();
    return future.get_event().duration<D>();
}

template <typename D>
D MeasureDuration(boost::compute::event& event) {
    event.wait();
    return event.duration<D>();
}

/*
Formats text representation of large values:
- for 1-1000, returned as is
- for 1000-999000 when last three digits are zeroes - 1K, 999K
- for 1000000-999000000 when last six digits are zeroes - 1M, 999M
*/
std::string FormatQuantityString(int value);

template <typename T>
bool AreFloatValuesClose(
    T A, T B, T maxAbsDiff,
    T maxRelDiff);  // TODO not sure about of these differences - T or e.g. long double?

template bool AreFloatValuesClose<float>(float A, float B, float maxAbsDiff, float maxRelDiff);
template bool AreFloatValuesClose<double>(double A, double B, double maxAbsDiff, double maxRelDiff);
template bool AreFloatValuesClose<half_float::half>(
    half_float::half A, half_float::half B, half_float::half maxAbsDiff,
    half_float::half maxRelDiff);

/*
Choose a unit for value from a supplied list convenient for output.
Preferred range of values suitable for output starts at 1, as small as possible.
"units" range must be sorted and must not be empty
*/
long double ChooseConvenientUnit(long double value, const std::vector<long double>& units);

/*
Choose a unit for a list of values from a supplied list convenient for output.
Preferred range of values suitable for output starts at 1, as small as possible.
"units" range must be sorted and must not be empty.
Values may have different suitable units, function selects a unit
that is convenient for as many values as possible.
*/
long double ChooseConvenientUnit(
    const std::vector<long double>& values, const std::vector<long double>& units);

/*
Compare functor that performs comparison of two pairs by first element
*/
template <typename T, typename U>
bool CompareFirst(const std::pair<T, U>& lhs, const std::pair<T, U>& rhs) {
    return lhs.first < rhs.first;
}

/*
Compare functor that performs comparison of two pairs by second element
*/
template <typename T, typename U>
bool CompareSecond(const std::pair<T, U>& lhs, const std::pair<T, U>& rhs) {
    return lhs.second < rhs.second;
}

/*
A functor that returns first element of a pair
*/
template <typename T, typename U>
T SelectFirst(const std::pair<T, U>& p) {
    return p.first;
}

/*
A functor that returns second element of a pair
*/
template <typename T, typename U>
U SelectSecond(const std::pair<T, U>& p) {
    return p.second;
}

boost::compute::kernel BuildKernel(
    const std::string& name, boost::compute::context& context, const std::string& source,
    const std::string& buildOptions = std::string(),
    const std::vector<std::string>& extensions = std::vector<std::string>());

boost::compute::program BuildProgram(
    boost::compute::context& context, const std::string& source,
    const std::string& buildOptions = std::string(),
    const std::vector<std::string>& extensions = std::vector<std::string>());

std::unordered_map<std::string, Duration> GetOpenCLEventDurations(
    const std::unordered_map<std::string, boost::compute::event>& events);

std::string CombineStrings(
    const std::vector<std::string>& strings, const std::string& delimiter = "\n");

/*
Tries to get a value from a map by key. If it is not found, use a default one supplied as a
parameter
*/
template <typename C, typename K, typename V>
V FindInMapWithDefault(const C& container, K key, V defaultValue) {
    V result;
    auto iter = container.find(key);
    if (iter != container.end()) {
        result = iter->second;
    } else {
        result = defaultValue;
    }
    return result;
}

// TODO use T as a template argument
template <typename T4, typename U4>
T4 StaticCastVector4(const U4& v) {
    return T4({// TODO convert to T instead when mapping between T and T4 is ready.
               static_cast<float>(v.s[0]), static_cast<float>(v.s[1]), static_cast<float>(v.s[2]),
               static_cast<float>(v.s[3])});
}

cl_double4 CombineTwoDouble2Vectors(const cl_double2& a, const cl_double2& b);

template <typename T>
std::string SerializeNumber(T val) {
    std::ostringstream stream;
    std::streamsize precision =
        std::max(std::numeric_limits<T>::digits10, std::numeric_limits<T>::max_digits10);
    stream << std::setprecision(precision) << std::fixed << val;
    return stream.str();
}
}  // namespace Utils

#define EXCEPTION_ASSERT(expr)                                     \
    {                                                              \
        if (!(expr)) {                                             \
            throw std::logic_error("Assert \"" #expr "\" failed"); \
        }                                                          \
    }
