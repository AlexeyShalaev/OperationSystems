#include <iostream>
#include <string>

#include "onlyfast.h"

int main()
{
    onlyfast::network::Client client;

    std::string resp = client.SendRequest("ECHO:HelloWorld!");
    std::cout << "Server response: " << resp << std::endl;
    sleep(3);
    resp = client.SendRequest("TEST:HelloWorld!");
    std::cout << "Server response: " << resp << std::endl;

    return 0;
}
