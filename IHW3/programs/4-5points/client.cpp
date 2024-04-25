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

onlyfast::Arguments args;

std::string generate_random_string(size_t length = 10)
{
    std::string str("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::random_device rd;
    std::mt19937 generator(rd());
    std::shuffle(str.begin(), str.end(), generator);
    return str.substr(0, length);
}

int generate_random_number(int min, int max)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

std::vector<std::string> parse_params(std::string request)
{
    std::vector<std::string> data;
    size_t pos = 0;
    while (pos < request.size())
    {
        size_t next_pos = request.find(';', pos);
        if (next_pos == std::string::npos)
        {
            next_pos = request.size();
        }
        data.push_back(request.substr(pos, next_pos - pos));
        pos = next_pos + 1;
    }
    return data;
}

void *programmer_work(void *arg)
{
    auto host = args.Get("host", "127.0.0.1");
    auto port = args.GetInt("port", 80);
    auto buffer_size = args.GetInt("buffer_size", 1024);
    auto debug = args.GetBool("debug", false);
    onlyfast::network::Client client(host, port, buffer_size, debug);

    std::vector<std::string> data;

    auto resp = client.SendRequest("REG_PROG:" + generate_random_string(4));
    if (resp.status != onlyfast::network::ResponseStatus::OK)
    {
        std::cout << "Error in registering programmer: " << resp.body << std::endl;
        return 0;
    }
    std::cout << "Programmer registered with id: " << resp.body << std::endl;
    std::string programmer_id = resp.body;

    while (true)
    {
        resp = client.SendRequest("TAKE_JOB:" + programmer_id);
        if (resp.status != onlyfast::network::ResponseStatus::OK)
        {
            std::cout << "Error in taking job: " << resp.body << std::endl;
            return 0;
        }

        if (resp.body == "no tasks")
        {
            std::cout << "[" << programmer_id << "] "
                      << "no tasks" << std::endl;
            break;
        }
        else if (resp.body == "wait")
        {
            std::cout << "[" << programmer_id << "] "
                      << "Wait for check" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        data = parse_params(resp.body);

        if (data[0] == "CHECK")
        {
            // пришла таска на проверку
            std::cout << "[" << programmer_id << "] "
                      << "Task for check: " << data[1] << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(generate_random_number(1, 3)));
            int verdict = (rand() % 2 == 0);
            std::string str_verdict = verdict ? "CORRECT" : "INCORRECT";
            std::cout << "[" << programmer_id << "] "
                      << "Task checked: " << data[1] << " - " << str_verdict << std::endl;
            client.SendRequest("SEND_CHECK_RESULT:" + programmer_id + ";" + data[1] + ";" + str_verdict); // отправляем результат проверки
            continue;
        }

        std::cout << "[" << programmer_id << "] "
                  << "Taken task: " << data[0] << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(generate_random_number(1, 3))); // типо делаем работу
        client.SendRequest("SEND_ON_CHECK:" + programmer_id + ";" + data[1]);            // отправляем программу на проверку
    }

    std::cout << "Programmer " << programmer_id << " finished work" << std::endl;

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    args.AddArgument("programmers_number", onlyfast::Arguments::ArgType::INT, "Number of programmers", "3");
    args.AddArgument("host", onlyfast::Arguments::ArgType::STRING, "Host to listen on", "127.0.0.1");
    args.AddArgument("port", onlyfast::Arguments::ArgType::INT, "Port to listen on", "80");
    args.AddArgument("buffer_size", onlyfast::Arguments::ArgType::INT, "Buffer size", "1024");
    args.AddArgument("debug", onlyfast::Arguments::ArgType::BOOL, "Debug mode", "false");
    if (!args.Parse(argc, argv))
    {
        return 0;
    }

    auto programmers_number = args.GetInt("programmers_number", 3);

    pthread_t threads[programmers_number];
    for (int i = 0; i < programmers_number; ++i)
    {
        if (pthread_create(&threads[i], NULL, programmer_work, NULL))
        {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < programmers_number; ++i)
    {
        if (pthread_join(threads[i], NULL))
        {
            perror("pthread_join");
            return 1;
        }
    }

    return 0;
}
