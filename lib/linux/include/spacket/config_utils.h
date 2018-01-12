#pragma once

#include <spacket/port_config.h>
#include <spacket/result.h>

#include <boost/program_options.hpp>

Result<PortConfig> fromProgramOptions(
    const boost::program_options::variables_map& vm);
