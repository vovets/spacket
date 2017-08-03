#include "namespaces.h"
#include <spacket/config.h>
#include <spacket/config_utils.h>
#include <spacket/errors.h>
#include <spacket/serial_device.h>
#include <spacket/buffer_utils.h>

#include <boost/program_options.hpp>

#include <iostream>

// never returns
void fatal(Error e) {
    std::cerr << "error " << toInt(e) << ": " << toString(e) << std::endl;
    exit(1);
}


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

    auto fatal = [](Error e) {
                     std::cerr << "fatal error [" << toInt(e) << "]: " << toString(e) << std::endl;
                     return 1;
                 };

    rcallv(pc, fatal, fromProgramOptions, vm);

    rcallv(sd, fatal, SerialDevice::open, std::move(pc));

    auto wb = Buffer{1, 2, 3, 4, 5, 6};
    std::cout << hr(wb) << " " << wb.size() << std::endl;
    rcall(fatal, sd.write, wb);
    rcallv(rb, fatal, sd.read, ch::seconds(1), 65536);
    std::cout << hr(rb) << " " << rb.size() << std::endl;

    return 0;
}
