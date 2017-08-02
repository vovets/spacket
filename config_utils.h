#pragma once

#include "namespaces.h"
#include "config.h"
#include "result.h"

#include <boost/program_options.hpp>

Result<PortConfig> fromProgramOptions(const po::variables_map& vm);
