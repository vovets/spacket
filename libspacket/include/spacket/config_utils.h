#pragma once

#include <spacket/config.h>
#include <spacket/result.h>

#include <boost/program_options.hpp>

Result<PortConfig> fromProgramOptions(
    const boost::program_options::variables_map& vm);
