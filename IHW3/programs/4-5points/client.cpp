#include <iostream>
#include <string>

#include "network.h"

int main()
{
    // Пример использования:
    Network::Client client;
    while (true)
    {
        std::string resp = client.SendRequest("Hello, server!");
        std::cout << "Server response: " << resp << std::endl;
        sleep(3);
    }

    return 0;
}
