#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <thread>
#include <chrono>

#include "onlyfast.h"

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

int main()
{
    onlyfast::network::Client client;
    std::vector<std::string> data;

    auto resp = client.SendRequest("REG_PROG:Alex");
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
        if (resp.body == "No tasks")
        {
            std::cout << "No tasks" << std::endl;
            break;
        }

        data = parse_params(resp.body);

        if (data[0] == "CHECK")
        {
            // пришла таска на проверку
            std::cout << "Task for check: " << data[1] << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(generate_random_number(1, 5)));
            int verdict = (rand() % 2 == 0);
            client.SendRequest("SEND_CHECK_RESULT:" + programmer_id + ";" + data[1] + ";" + verdict ? "CORRECT" : "INCORRECT"); // отправляем результат проверки
        }

        std::cout << "Taken task: " << data[0] << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(generate_random_number(1, 5))); // типо делаем работу
        client.SendRequest("SEND_ON_CHECK:" + programmer_id + ";" + data[1]);            // отправляем программу на проверку
    }

    return 0;
}
