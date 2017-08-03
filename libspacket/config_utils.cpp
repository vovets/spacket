#include <spacket/config_utils.h>
#include <spacket/errors.h>
#include "namespaces.h"

Result<PortConfig> fromProgramOptions(const po::variables_map& vm) {
    if (!vm.count("device")) {
        return fail<PortConfig>(Error::ConfigNoDevSpecified);
    }
    return PortConfig{vm["device"].as<std::string>()};
}
