#include "namespaces.h"
#include "config.h"
#include "config_utils.h"
#include "errors.h"
#include "serial_device.h"

#include <boost/program_options.hpp>

#include <iostream>

int main(int argc, const char* argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("device,d", po::value<std::string>(), "serial device to work with");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    auto pcr = fromProgramOptions(vm);
    if (isFail(pcr)) {
        fatal(GETFAIL(pcr));
    }

    auto sdr = SerialDevice::open(GETOK(pcr));
    if (isFail(sdr)) {
        fatal(GETFAIL(sdr));
    }
    return 0;
}
