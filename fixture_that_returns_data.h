#pragma once

#include <vector>

#include "fixture.h"

template <typename T>
class FixtureThatReturnsData: public Fixture
{
public:
    virtual std::vector<std::vector<T>> GetResults() = 0;
};