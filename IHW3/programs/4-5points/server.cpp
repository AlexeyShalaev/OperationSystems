#include <iostream>
#include <string>
#include <functional>

#include "onlyfast.h"

/*
Условимся, что запрос имеет формат:
CMD:PARAM_1;PARAM_2;PARAM_3
*/

onlyfast::network::Response echo_handler(const onlyfast::Application::Params &params)
{
    if (params.empty())
    {
        return {onlyfast::network::ResponseStatus::FAILED, "No parameters"};
    }
    return {onlyfast::network::ResponseStatus::OK, params[0]};
}

int main()
{
    onlyfast::network::Server server;
    onlyfast::Application app(server);
    app.RegisterHandler("ECHO", echo_handler);
    app.Run();
    return 0;
}
