#ifndef KPV_FIXTURE_AUTOREGISTRAR_H_
#define KPV_FIXTURE_AUTOREGISTRAR_H_

#include "fixture_registry.h"

namespace kpv {

class FixtureAutoregistrar {
public:
    typedef std::function<std::shared_ptr<FixtureFamily>(const PlatformList&)> FixtureFactory;
    FixtureAutoregistrar(const std::string& category_id, FixtureFactory fixture_factory) {
        FixtureRegistry::instance().Register(category_id, fixture_factory);
    }
};
}  //  namespace kpv

#endif  // KPV_FIXTURE_AUTOREGISTRAR_H_
