#include <iostream>
#include <string>
#include <map>
#include <deque>
#include <random>
#include <functional>

#include "onlyfast.h"

/*
Условимся, что запрос имеет формат:
CMD:PARAM_1;PARAM_2;PARAM_3
*/

int requests_counter = 0;

class Solution
{
public:
    using Id = int;

    Solution(int tasks_number = 10)
    {
        for (int i = 0; i < tasks_number; ++i)
        {
            tasks.push_back(new Task(generate_random_string()));
        }
    }

    struct Task
    {
    public:
        static Id auto_inc;
        Id id;
        std::string name;
        Id mentor_id = -1;
        Id programmer_id = -1;

        Task(std::string name) : id(++auto_inc),
                                 name(name) {}
    };

    using TaskPtr = Task *;

    struct Programmer
    {
    public:
        static Id auto_inc;
        Id id;
        std::string name;
        std::deque<TaskPtr> checking_tasks; // задачи на проверку
        std::deque<TaskPtr> working_tasks;  // задачи на (пере-)выполнение

        Programmer(std::string name) : id(++auto_inc),
                                       name(name)
        {
        }

        Programmer() = default;
    };

    Id add_programmer(std::string name)
    {
        Programmer programmer(name);
        programmers.insert({programmer.id, programmer});
        std::cout << "Programmer registered with id: " << programmer.id << std::endl;
        return programmer.id;
    }

    std::string take_job(std::string id_str)
    {
        Id id = std::stoi(id_str);
        if (programmers.find(id) == programmers.end())
        {
            return "programmer not found";
        }
        if (!programmers[id].checking_tasks.empty())
        {
            std::string task_id = std::to_string(programmers[id].checking_tasks.front()->id);
            std::cout << "Programmer " << id << " is checking task " << task_id << std::endl;
            return "CHECK;" + task_id; // Даем задачу на проверку
        }

        for (auto &[programmer_id, programmer] : programmers)
        {
            for (auto task : programmer.checking_tasks)
            {
                if (task->programmer_id == id)
                {
                    std::cout << "Programmer " << id << " is waiting for checking his task" << std::endl;
                    return "wait";
                }
            }
        }

        if (!programmers[id].working_tasks.empty())
        {
            TaskPtr task = programmers[id].working_tasks.front();
            std::cout << "Programmer " << id << " is working on task " << task->id << std::endl;
            return task->name + ";" + std::to_string(task->id); // Даем задачу
        }
        if (tasks.empty())
        {
            for (auto &programmer : programmers)
            {
                if (!programmer.second.working_tasks.empty())
                {
                    return "wait";
                }
            }
            return "no tasks";
        }
        TaskPtr task = tasks.front();
        tasks.pop_front();
        std::cout << "!!!!! " << task->id << std::endl;
        task->programmer_id = id;
        programmers[id].working_tasks.push_back(task);
        std::cout << "Programmer " << id << " is working on task " << task->id << std::endl;
        return task->name + ";" + std::to_string(task->id); // Даем задачу
    }

    void send_on_check(std::string programmer_id_str, std::string task_id_str)
    {
        Id programmer_id = std::stoi(programmer_id_str);
        Id task_id = std::stoi(task_id_str);

        TaskPtr task_ptr = get_task_by_id(task_id, programmers[programmer_id].working_tasks);

        if (task_ptr->mentor_id == -1)
        {
            task_ptr->mentor_id = get_minimum_loaded_programmer(programmer_id);
        }

        std::cout << "Task " << task_ptr->id << " sent to check to programmer " << task_ptr->mentor_id << std::endl;

        programmers[task_ptr->mentor_id].checking_tasks.push_back(task_ptr);
        remove_task_by_id(task_id, programmers[programmer_id].working_tasks);
    }

    void send_check_result(std::string programmer_id_str, std::string task_id_str, std::string result_str)
    {

        Id programmer_id = std::stoi(programmer_id_str);
        Id task_id = std::stoi(task_id_str);

        TaskPtr task_ptr = get_task_by_id(task_id, programmers[programmer_id].checking_tasks);
        remove_task_by_id(task_id, programmers[programmer_id].checking_tasks);

        if (result_str == "CORRECT")
        {
            // Задача выполнена успешно
            std::cout << "Task " << task_ptr->id << " is correct" << std::endl;
            delete task_ptr;
        }
        else
        {
            // Задача выполнена неуспешно
            std::cout << "Task " << task_ptr->id << " is incorrect" << std::endl;
            programmers[task_ptr->programmer_id].working_tasks.push_back(task_ptr); // программист который делал задачу
        }
    }

    Id get_minimum_loaded_programmer(Id exclude_id = -1)
    {
        Programmer min_loaded_programmer = programmers.begin()->second;
        for (const auto &[id, programmer] : programmers)
        {
            if (programmer.id == exclude_id)
            {
                continue;
            }
            if (programmer.checking_tasks.size() < min_loaded_programmer.checking_tasks.size() || min_loaded_programmer.id == exclude_id)
            {
                min_loaded_programmer = programmer;
            }
        }
        return min_loaded_programmer.id;
    }

    void middleware(int clnt_sock, const onlyfast::network::Request &request)
    {

        using namespace std;
        cout << "-------------- " << ++requests_counter << " -------------------" << endl;

        for (auto taskPtr : tasks)
        {
            cout << "Task " << taskPtr->id << " is in queue" << endl;
        }

        for (const auto &[id, programmer] : programmers)
        {
            cout << "Programmer " << id << " has " << programmer.checking_tasks.size() << " checking tasks [";
            for (auto task : programmer.checking_tasks)
            {
                cout << task->id << " ";
            }
            cout << "]" << endl;
            cout << "Programmer " << id << " has " << programmer.working_tasks.size() << " working tasks [";
            for (auto task : programmer.working_tasks)
            {
                cout << task->id << " ";
            }
            cout << "]" << endl;
            cout << endl;
        }
        cout << "---------------------------------" << endl;
    }

    template <typename T>
    TaskPtr get_task_by_id(Id id, T iterable)
    {
        for (auto task : iterable)
        {
            if (task->id == id)
            {
                return task;
            }
        }
        return nullptr;
    }

    template <typename T>
    void remove_task_by_id(Id id, T &iterable)
    {
        for (auto it = iterable.begin(); it != iterable.end(); ++it)
        {
            if ((*it)->id == id)
            {
                iterable.erase(it);
                return;
            }
        }
    }

private:
    std::map<Id, Programmer> programmers;
    std::deque<TaskPtr> tasks;

    std::string generate_random_string(size_t length = 10)
    {
        std::string str("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        std::random_device rd;
        std::mt19937 generator(rd());
        std::shuffle(str.begin(), str.end(), generator);
        return str.substr(0, length);
    }
};

// Инициализация статических переменных классов
Solution::Id Solution::Programmer::auto_inc = 0;
Solution::Id Solution::Task::auto_inc = 0;

Solution solution;

onlyfast::network::Response echo_handler(const onlyfast::Application::Params &params)
{
    if (params.empty())
    {
        return {onlyfast::network::ResponseStatus::FAILED, "No parameters"};
    }
    return {onlyfast::network::ResponseStatus::OK, params[0]};
}

onlyfast::network::Response register_programmer_handler(const onlyfast::Application::Params &params)
{
    if (params.empty())
    {
        return {onlyfast::network::ResponseStatus::FAILED, "No parameters"};
    }
    int id = solution.add_programmer(params[0]);
    return {onlyfast::network::ResponseStatus::OK, std::to_string(id)};
}

onlyfast::network::Response take_job_handler(const onlyfast::Application::Params &params)
{
    if (params.empty())
    {
        return {onlyfast::network::ResponseStatus::FAILED, "No parameters"};
    }
    return {onlyfast::network::ResponseStatus::OK, solution.take_job(params[0])};
}

onlyfast::network::Response send_on_check_handler(const onlyfast::Application::Params &params)
{
    if (params.empty())
    {
        return {onlyfast::network::ResponseStatus::FAILED, "No parameters"};
    }
    solution.send_on_check(params[0], params[1]);
    return {onlyfast::network::ResponseStatus::OK, "OK"};
}

onlyfast::network::Response send_check_result_handler(const onlyfast::Application::Params &params)
{
    if (params.empty())
    {
        return {onlyfast::network::ResponseStatus::FAILED, "No parameters"};
    }
    solution.send_check_result(params[0], params[1], params[2]);
    return {onlyfast::network::ResponseStatus::OK, "OK"};
}

int main(int argc, char **argv)
{
    onlyfast::Arguments args;
    args.AddArgument("tasks_number", onlyfast::Arguments::ArgType::INT, "Number of tasks", "10");
    args.AddArgument("host", onlyfast::Arguments::ArgType::STRING, "Host to listen on", "127.0.0.1");
    args.AddArgument("port", onlyfast::Arguments::ArgType::INT, "Port to listen on", "80");
    args.AddArgument("buffer_size", onlyfast::Arguments::ArgType::INT, "Buffer size", "1024");
    args.AddArgument("max_clients", onlyfast::Arguments::ArgType::INT, "Maximum number of connected clients", "10");
    args.AddArgument("debug", onlyfast::Arguments::ArgType::BOOL, "Debug mode", "false");
    if (!args.Parse(argc, argv))
    {
        return 0;
    }

    auto tasks_number = args.GetInt("tasks_number", 10);
    auto host = args.Get("host", "127.0.0.1");
    auto port = args.GetInt("port", 80);
    auto buffer_size = args.GetInt("buffer_size", 1024);
    auto max_clients = args.GetInt("max_clients", 10);
    auto debug = args.GetBool("debug", false);

    onlyfast::network::Server server(host, port, buffer_size, max_clients, onlyfast::network::Server::DefaultRequestHandler, debug);
    server.SetMiddleware([&](int clnt_sock, const onlyfast::network::Request &request)
                         { solution.middleware(clnt_sock, request); });

    onlyfast::Application app(server);
    app.RegisterHandler("ECHO", echo_handler);
    app.RegisterHandler("REG_PROG", register_programmer_handler);        // Регистрация программиста
    app.RegisterHandler("TAKE_JOB", take_job_handler);                   // Взять задачу (написание, исправление, проверка)
    app.RegisterHandler("SEND_ON_CHECK", send_on_check_handler);         // Отправить задачу на проверку
    app.RegisterHandler("SEND_CHECK_RESULT", send_check_result_handler); // Возвратить результат проверки
    app.Run();

    return 0;
}
