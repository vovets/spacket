#include "config_utils.h"
#include "errors.h"

Result<PortConfig> fromProgramOptions(const po::variables_map& vm) {
    if (!vm.count("device")) {
        return fail<PortConfig>(Error::ConfigNoDevSpecified);
    }
    return PortConfig{vm["device"].as<std::string>()};
}
