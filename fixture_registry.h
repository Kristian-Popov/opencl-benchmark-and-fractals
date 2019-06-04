#ifndef KPV_FIXTURE_REGISTRY_H_
#define KPV_FIXTURE_REGISTRY_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "devices/platform_list.h"
#include "fixtures/fixture_family.h"

namespace kpv {

// class PlatformList;

// TODO forbid construction outside allowed context if possible
class FixtureRegistry {
public:
    typedef std::function<std::shared_ptr<FixtureFamily>(const PlatformList&)> FixtureFactory;
    typedef std::unordered_multimap<std::string, FixtureFactory> FactoryList;
    void Register(const std::string& category_id, FixtureFactory fixture_factory) {
        factories_.emplace(category_id, fixture_factory);
    }

    // We need a singleton so macros can register factories using this global instance
    static FixtureRegistry& instance() {
        static FixtureRegistry* instance = nullptr;
        if (!instance) {
            instance = new FixtureRegistry();
        }
        return *instance;
    }

    std::vector<std::string> GetAllCategories() {
        std::vector<std::string> result;
        for (const auto& p : factories_) {
            if (result.empty() || (result.back() != p.first)) {
                result.push_back(p.first);
            }
        }
        return result;
    }

    FactoryList::iterator begin() { return factories_.begin(); }

    FactoryList::iterator end() { return factories_.end(); }

    FactoryList::const_iterator cbegin() const { return factories_.cbegin(); }

    FactoryList::const_iterator cend() const { return factories_.cend(); }

    typedef std::string key_type;
    typedef FixtureFactory mapped_type;
    typedef std::pair<const std::string, FixtureFactory> value_type;

private:
    FixtureRegistry() {}

    FactoryList factories_;
};
}  // namespace kpv

#endif  // KPV_FIXTURE_REGISTRY_H_
