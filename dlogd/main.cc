/* Copyright (c) 2011 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <getopt.h>
#include <unistd.h>

#include <iostream>

#include "Common.h"
#include "Config.h"
#include "Debug.h"
#include "DLogStorage.h"
#include "LogManager.h"
#include "WorkDispatcher.h"

using namespace DLog;

namespace {

/**
 * Parses argv for the main function.
 */
class OptionParser {
  public:
    OptionParser(int& argc, char**& argv)
        : argc(argc)
        , argv(argv)
        , configFilename("logcabin.conf")
    {
        while (true) {
            static struct option longOptions[] = {
               {"config",  required_argument, NULL, 'c'},
               {"help",  no_argument, NULL, 'h'},
               {0, 0, 0, 0}
            };
            int c = getopt_long(argc, argv, "c:h", longOptions, NULL);

            // Detect the end of the options.
            if (c == -1)
                break;

            switch (c) {
                case 'h':
                    usage();
                    exit(0);
                case 'c':
                    configFilename = optarg;
                    break;
                case '?':
                default:
                    // getopt_long already printed an error message.
                    usage();
                    exit(1);
            }
        }

        // We don't expect any additional command line arguments (not options).
        if (optind != argc) {
            usage();
            exit(1);
        }
    }

    void usage() {
        std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
        std::cout << "Options: " << std::endl;
        std::cout << "  -h, --help     "
                  << "Print this usage information" << std::endl;
        std::cout << "  -c, --config <file>      "
                  << "Write output to <file> "
                  << "(default: logcabin.conf)" << std::endl;
    }

    int& argc;
    char**& argv;
    std::string configFilename;
};

class LogManagerReady : public LogManager::InitializeCallback {
  public:
    void initialized() {
        LOG(NOTICE, "LogManager is ready");
        PANIC("Initialize the RPC system here...");
    }
    friend class RefHelper<LogManagerReady>;
    friend class MakeHelper;
};

} // anonymous namespace

int main(int argc, char *argv[])
{
    // Parse command line args.
    OptionParser options(argc, argv);
    LOG(NOTICE, "Using config file %s", options.configFilename.c_str());

    // Parse config file.
    Config config;
    config.readFile(options.configFilename);

    // Create the work dispatcher.
    uint32_t maxThreads = config.read<uint32_t>("maxThreads", 16);
    if (maxThreads < 2)
        maxThreads = 2;
    DLog::workDispatcher = new WorkDispatcher(0, maxThreads - 1);

    // Create log manager.
    Ref<LogManager> logManager =
        make<LogManager>(config,
                         Storage::Factory::createStorageModule(config),
                         make<LogManagerReady>());

    // Fake a main loop.
    while (true) {
        Ptr<WorkDispatcher::CompletionCallback> completion =
                                    workDispatcher->popCompletion();
        if (!completion) {
            usleep(1000);
            continue;
        }
        completion->completed();
    }

    return 0;
}

