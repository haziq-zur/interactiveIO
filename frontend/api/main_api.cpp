#include <iostream>
#include <string>
#include <vector>

#include "cli_api.h"
#include "instrument_controller.h"

// =============================================================================
// Command-line API entry point.
//
// Drives the InstrumentController from command-line arguments and prints a JSON
// result to stdout, e.g.:
//
//   interactiveIO-api --address 192.168.1.100 --port 5025 --command "*IDN?"
//   interactiveIO-api --protocol visa --address "TCPIP::10.0.0.5::INSTR" --clear
//   interactiveIO-api --address 192.168.1.100 --image ":DISP:DATA? PNG" \
//       --output screen.png
//
// Run with --help for the full list of options. The process exit code is 0 on
// success, 2 for usage/input errors, and 1 for other operational failures.
// =============================================================================

int main(int argc, char** argv)
{
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<size_t>(argc - 1) : 0);
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    iio::InstrumentController controller;
    return iio::api::runCli(controller, args, std::cout, std::cerr);
}
