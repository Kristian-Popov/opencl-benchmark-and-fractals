#ifndef KPV_RUN_SETTINGS_H_
#define KPV_RUN_SETTINGS_H_

#include <string>
#include <vector>

#include "utils/duration.h"

namespace kpv {

struct DeviceConfiguration {
    DeviceConfiguration(bool initial_status)
        : host_device(initial_status),
          cpu_opencl_devices(initial_status),
          gpu_opencl_devices(initial_status),
          other_opencl_devices(initial_status) {}
    bool host_device = true;
    bool cpu_opencl_devices = true;
    bool gpu_opencl_devices = true;
    bool other_opencl_devices = true;
};

struct RunSettings {
    std::string output_file_name;
    std::vector<std::string> category_list;  // Unsorted list of categories
    int min_iterations = 1;
    int max_iterations = std::numeric_limits<int>::max();
    Duration target_execution_time;
    bool verify_results = true;
    bool store_results = true;
    std::string additional_params;
    enum Operation { kList, kRunAllExcept, kRunOnly } operation;
    DeviceConfiguration device_config = DeviceConfiguration(true);
};

}  // namespace kpv

#endif  // KPV_RUN_SETTINGS_H_
