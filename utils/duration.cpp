#include "duration.h"

#include "boost/compute.hpp"
#include "nlohmann/json.hpp"

Duration::Duration(const boost::compute::event& event)
    : duration_(event.duration<InternalType>()) {}

double Duration::AsSeconds() const {
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration_).count();
}

void to_json(nlohmann::json& j, const Duration& p) {
    j = nlohmann::json::object({{"durationDoubleNs", p.duration().count()}});
}

void from_json(const nlohmann::json& j, Duration& p) {
    double val{0.0};
    j.at("durationDoubleNs").get_to(val);
    p = Duration{std::chrono::duration<double, std::nano>(val)};
}
