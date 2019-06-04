#ifndef KPV_FIXTURE_REGISTER_MACROS_H_
#define KPV_FIXTURE_REGISTER_MACROS_H_

#include "fixture_autoregistrar.h"

// Based on
// https://github.com/catchorg/Catch2/blob/1c99b0ff8189f59563973193909af4b511485890/include/internal/catch_common.h
#define KPV_UNIQUE_NAME_LINE2(name, line) name##line
#define KPV_UNIQUE_NAME_LINE(name, line) KPV_UNIQUE_NAME_LINE2(name, line)
#define KPV_UNIQUE_NAME(name) KPV_UNIQUE_NAME_LINE(name, __COUNTER__)

#define REGISTER_FIXTURE(category_id, fixture_factory)                                       \
    namespace {                                                                              \
    const kpv::FixtureAutoregistrar KPV_UNIQUE_NAME(instance){category_id, fixture_factory}; \
    }

#endif  // KPV_FIXTURE_REGISTER_MACROS_H_
