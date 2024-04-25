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

class Solution
{
public:
    using Id = int;

    Solution(int tasks_number = 5)
    {
        for (int i = 0; i < tasks_number; ++i)
        {
            tasks.push_back(Task(generate_random_string()));
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

    struct Programmer
    {
    public:
        static Id auto_inc;
        Id id;
        std::string name;
        std::deque<Task> checking_tasks; // задачи на проверку
        std::deque<Task> working_tasks;  // задачи на (пере-)выполнение

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
            std::string task_id = std::to_string(programmers[id].checking_tasks.front().id);
            std::cout << "Programmer " << id << " is checking task " << task_id << std::endl;
            return "CHECK;" + task_id; // Даем задачу на проверку
        }

        for (auto &[programmer_id, programmer] : programmers)
        {
            for (auto task : programmer.checking_tasks)
            {
                if (task.programmer_id == id)
                {
                    std::cout << "Programmer " << id << " is waiting for checking his task" << std::endl;
                    return "wait";
                }
            }
        }

        if (!programmers[id].working_tasks.empty())
        {
            Task task = programmers[id].working_tasks.front();
            std::cout << "Programmer " << id << " is working on task " << task.id << std::endl;
            return task.name + ";" + std::to_string(task.id); // Даем задачу
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
        Task task = tasks.front();
        tasks.pop_front();
        task.programmer_id = id;
        programmers[id].working_tasks.push_back(task);
        std::cout << "Programmer " << id << " is working on task " << task.id << std::endl;
        return task.name + ";" + std::to_string(task.id); // Даем задачу
    }

    void send_on_check(std::string programmer_id_str, std::string task_id_str)
    {
        Id programmer_id = std::stoi(programmer_id_str);
        Id task_id = std::stoi(task_id_str);

        Task task = programmers[programmer_id].working_tasks.front();
        programmers[programmer_id].working_tasks.pop_front();

        if (task.mentor_id != -1)
        {
            programmers[task.mentor_id].checking_tasks.push_back(task);
            return;
        }

        Id mentor_id = get_minimum_loaded_programmer(programmer_id);
        task.mentor_id = mentor_id;
        programmers[mentor_id].checking_tasks.push_back(task);
        std::cout << "Task " << task.id << " sent to check to programmer " << mentor_id << std::endl;
    }

    void send_check_result(std::string programmer_id_str, std::string task_id_str, std::string result_str)
    {
        Id programmer_id = std::stoi(programmer_id_str);
        Id task_id = std::stoi(task_id_str);

        Task task = programmers[programmer_id].checking_tasks.front();
        programmers[programmer_id].checking_tasks.pop_front();

        if (result_str == "CORRECT")
        {
            // Задача выполнена успешно
            std::cout << "Task " << task.id << " is correct" << std::endl;
        }
        else
        {
            // Задача выполнена неуспешно
            std::cout << "Task " << task.id << " is incorrect" << std::endl;
            programmers[task.programmer_id].working_tasks.push_back(task); // программист который делал задачу
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
        for (const Task &task : tasks)
        {
            std::cout << "Task: " << task.name << ", id = " << task.id << ", programmer_id = " << task.programmer_id << ", mentor_id = " << task.mentor_id << std::endl;
        }
    }

private:
    std::map<Id, Programmer> programmers;
    std::deque<Task> tasks;

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

int main()
{
    onlyfast::network::Server server;
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
