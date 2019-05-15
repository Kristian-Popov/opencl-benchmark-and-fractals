#pragma once

#include <memory>
#include <vector>

#include "devices/platform_list.h"
#include "fixtures/fixture.h"
#include "fixtures/fixture_family.h"
#include "reporters/benchmark_reporter.h"
#include "utils/duration.h"

class FixtureRunner {
public:
    struct FixturesToRun {
        bool trivial_factorial = true;
        bool damped_wave = true;
        bool koch_curve = true;
        bool multibrot_set = true;
    };

    struct RunSettings {
        FixturesToRun fixtures_to_run;
        int min_iterations = 1;
        int max_iterations = std::numeric_limits<int>::max();
        Duration target_fixture_execution_time;
        bool verify_results = true;
        bool store_results = true;
        bool debug_mode = false;
    };

    void Run(std::unique_ptr<BenchmarkReporter> reporter, RunSettings settings);

private:
    void Clear();
    void SetFloatingPointEnvironment();
    void CreateTrivialFixtures();
    template <typename T, typename D = std::normal_distribution<T>>
    void CreateDampedWave2DFixtures();
    template <typename T, typename T4>
    void CreateKochCurveFixtures();
    template <typename T, typename P>
    void FixtureRunner::CreateMultibrotSetFixtures();

    std::vector<std::shared_ptr<FixtureFamily>> fixture_families_;
    PlatformList platform_list_;
};
