#include "program_source_repository.h"
#include "utils.h"

namespace
{
    const char* openclMathSource = R"(
int Pow2ForInt(int x)
{
	return (x<0) ? 0 : (1<<x);
}
)";

    const char* kochCurveSource = R"(
typedef struct __attribute__ ((packed)) Line
{
	REAL_T_4 coords;
	// First number (x) is an iteration number, second is line identifier in current iteration
	int2 ids;
} Line;

// TODO for invalid values report error in some way or return 0?
int CalcLinesNumberForIteration(int iterationCount)
{
    return Pow2ForInt(2*iterationCount);
}

// TODO for invalid values report error in some way or return 0?
int CalcGlobalId(int2 ids)
{
	int result = ids.y;
	// TODO this function can be optimized by caching these values
	for (int iterationNumber = 0; iterationNumber < ids.x; ++iterationNumber)
	{
		result += CalcLinesNumberForIteration(iterationNumber);
	}
	return result;
}
)";
}

std::string ProgramSourceRepository::GetOpenCLMathSource()
{
    return openclMathSource;
}

std::string ProgramSourceRepository::GetKochCurveSource()
{
    return Utils::CombineStrings( { openclMathSource, kochCurveSource } );
}