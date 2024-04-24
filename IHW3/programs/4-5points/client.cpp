#include <iostream>
#include <string>

#include "onlyfast.h"

int main()
{
    onlyfast::network::Client client;
    std::string resp;
    client.SendRequest("ECHO:HelloWorld!");
    client.SendRequest("TEST:HelloWorld!");

    return 0;
}
