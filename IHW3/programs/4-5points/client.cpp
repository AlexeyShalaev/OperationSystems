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
    onlyfast::network::Client client;
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

        std::cout << resp.body << std::endl;

        if (resp.body == "no tasks")
        {
            std::cout << "no tasks" << std::endl;
            break;
        }
        else if (resp.body == "wait")
        {
            std::cout << "Wait for check" << std::endl;
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

    pthread_exit(NULL);
}

int main()
{
    int number_of_programmers = 10;
    pthread_t threads[number_of_programmers];
    for (int i = 0; i < number_of_programmers; ++i)
    {
        if (pthread_create(&threads[i], NULL, programmer_work, NULL))
        {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < number_of_programmers; ++i)
    {
        if (pthread_join(threads[i], NULL))
        {
            perror("pthread_join");
            return 1;
        }
    }

    return 0;
}
