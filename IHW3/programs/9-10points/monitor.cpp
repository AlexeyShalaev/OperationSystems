#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <thread>
#include <chrono>
#include <pthread.h>
#include <signal.h>

#include "onlyfast.h"

/*
Условимся, что запрос имеет формат:
CMD:PARAM_1;PARAM_2;PARAM_3
*/

onlyfast::Monitor *monitor;

void monitoring(const std::string &payload)
{
    std::cout << payload << std::endl;
}

void sigint_handler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    if (monitor)
    {
        monitor->stop();
        delete monitor;
    }
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);

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

    monitor = new onlyfast::Monitor(host, port, buffer_size, debug);
    monitor->setHandler(monitoring);

    try
    {
        monitor->run();
    }
    catch (const onlyfast::network::Server::ServerException &e)
    {
        std::cout << e.what() << std::endl;
    }

    delete monitor;

    return 0;
}
