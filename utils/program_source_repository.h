#pragma once

#include <string>

class ProgramSourceRepository {
public:
    static std::string GetOpenCLMathSource();
    static std::string GetKochCurveSource();
    static std::string GetGlobalMemoryPoolSource();
};
