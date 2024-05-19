#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <thread>
#include <chrono>
#include <pthread.h>

#include "onlyfast.h"

/*
Условимся, что запрос имеет формат:
CMD:PARAM_1;PARAM_2;PARAM_3
*/

void monitoring(const std::string &payload)
{
    std::cout << payload << std::endl;
}

int main(int argc, char **argv)
{
    onlyfast::Arguments args;
    args.AddArgument("host", onlyfast::Arguments::ArgType::STRING, "Host to listen on", "127.0.0.1");
    args.AddArgument("port", onlyfast::Arguments::ArgType::INT, "Port to listen on", "80");
    args.AddArgument("buffer_size", onlyfast::Arguments::ArgType::INT, "Buffer size", "1024");
    args.AddArgument("debug", onlyfast::Arguments::ArgType::BOOL, "Debug mode", "false");
    if (!args.Parse(argc, argv))
    {
        return 0;
    }

    auto host = args.Get("host", "127.0.0.1");
    auto port = args.GetInt("port", 80);
    auto buffer_size = args.GetInt("buffer_size", 1024);
    auto debug = args.GetBool("debug", false);
    onlyfast::Monitor monitor(host, port, buffer_size, debug);
    monitor.setHandler(monitoring);
    monitor.run();
    return 0;
}
