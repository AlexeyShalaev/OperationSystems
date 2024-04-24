#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "onlyfast.h"

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
    int programmer_id = std::stoi(resp.body);

    resp = client.SendRequest("TAKE_TASK:" + std::to_string(programmer_id));
    if (resp.status != onlyfast::network::ResponseStatus::OK)
    {
        std::cout << "Error in taking task: " << resp.body << std::endl;
        return 0;
    }
    if (resp.body == "No tasks")
    {
        std::cout << "No tasks" << std::endl;
        return 0;
    }
    data = parse_params(resp.body);
    std::cout << "Taken task: " << data[0] << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(std::stoi(data[1])));
    return 0;
}
