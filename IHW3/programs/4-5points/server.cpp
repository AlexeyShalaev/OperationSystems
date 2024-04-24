#include <iostream>
#include <string>
#include <map>
#include <queue>
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

    Solution(int tasks_number = 10)
    {
        for (int i = 0; i < tasks_number; ++i)
        {
            tasks.push(Task(generate_random_string(), generate_random_number(1, 10)));
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
        std::queue<Task> checking_tasks; // задачи на проверку
        std::queue<Task> working_tasks;  // задачи на (пере-)выполнение

        Programmer(std::string name) : id(++auto_inc),
                                       name(name)
        {
        }
    };

    Id add_programmer(std::string name)
    {
        Programmer programmer(name);
        programmers.insert({programmer.id, programmer});
        return programmer.id;
    }

    std::string take_job(std::string id_str)
    {
        Id id = std::stoi(id_str);
        if (programmers.find(id) == programmers.end())
        {
            return "programmer not found";
        }
        Programmer programmer = programmers[id];
        if (!programmer.checking_tasks.empty())
        {
            return "CHECK;" + std::to_string(programmer.checking_tasks.front().id); // Даем задачу на проверку
        }
        if (!programmer.working_tasks.empty())
        {
            Task task = programmer.working_tasks.front();
            return task.name + ";" + std::to_string(task.id); // Даем задачу
        }
        if (tasks.empty())
        {
            return "no tasks";
        }
        Task task = tasks.front();
        tasks.pop();
        task.programmer_id = programmer.id;
        programmer.working_tasks.push(task);
        return task.name + ";" + std::to_string(task.id); // Даем задачу
    }

    void send_on_check(std::string programmer_id_str, std::string task_id_str)
    {
        Id programmer_id = std::stoi(programmer_id_str);
        Id task_id = std::stoi(task_id_str);

        Task task = programmers[programmer_id].working_tasks.front();
        programmers[programmer_id].working_tasks.pop();

        if (task.mentor_id != -1)
        {
            programmers[task.mentor_id].checking_tasks.push(task);
            return;
        }

        Programmer programmer = get_minimum_loaded_programmer(programmer_id).checking_tasks.push(task_id);
        task.mentor_id = programmer.id;
        programmer.checking_tasks.push(task);
    }

    void send_check_result(std::string programmer_id_str, std::string task_id_str, std::string result_str)
    {
        Id programmer_id = std::stoi(programmer_id_str);
        Id task_id = std::stoi(task_id_str);

        Task task = programmers[programmer_id].checking_tasks.front();
        programmers[programmer_id].checking_tasks.pop();

        Programmer programmer = programmers[task.programmer_id]; // программист который делал задачу
        if (result_str == "OK")
        {
            // Задача выполнена успешно
        }
        else
        {
            programmer.working_tasks.push(task);
        }
    }

    Programmer get_minimum_loaded_programmer(Id exclude_id = -1)
    {
        Programmer min_loaded_programmer = programmers.begin()->second;
        for (auto &programmer : programmers)
        {
            if (programmer.first == exclude_id)
            {
                continue;
            }
            if (programmer.second.checking_tasks.size() < min_loaded_programmer.checking_tasks.size())
            {
                min_loaded_programmer = programmer.second;
            }
        }
        return min_loaded_programmer;
    }

private:
    std::map<Id, Programmer> programmers;
    std::queue<Task> tasks;

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
    onlyfast::Application app(server);
    app.RegisterHandler("ECHO", echo_handler);
    app.RegisterHandler("REG_PROG", register_programmer_handler);        // Регистрация программиста
    app.RegisterHandler("TAKE_JOB", take_job_handler);                   // Взять задачу (написание, исправление, проверка)
    app.RegisterHandler("SEND_ON_CHECK", send_on_check_handler);         // Отправить задачу на проверку
    app.RegisterHandler("SEND_CHECK_RESULT", send_check_result_handler); // Возвратить результат проверки
    app.Run();
    return 0;
}
