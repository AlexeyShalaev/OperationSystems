#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>
#include <optional>
#include <thread>
#include <chrono>
#include <sstream>
#include <stdexcept>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

namespace onlyfast
{
    namespace logger
    {
        class Logger
        {
        public:
            // Конструктор с потоком по умолчанию
            Logger(std::ostream &stream = std::cout, bool logging = true) : stream_(stream), logging(logging) {}

            // Перегрузка оператора <<
            template <typename T>
            Logger &operator<<(const T &value)
            {
                if (logging)
                {
                    stream_ << value;
                }

                return *this;
            }

        private:
            std::ostream &stream_; // Поток вывода
            bool logging = true;   // Флаг логирования
        };

    }

    namespace network
    {

        // Структура запроса
        struct Request
        {
            std::string ip;
            std::string body;
        };

        // Определение статусов ответов
        enum class ResponseStatus
        {
            OK,
            FAILED
        };

        // Структура ответа
        struct Response
        {
            ResponseStatus status;
            std::string body;
        };

        using RequestHandlerType = std::function<Response(const Request &)>;
        using MiddlewareType = std::function<void(const Request &)>;
        using AfterResponseType = std::function<void(const Request &, const Response &)>;

        class Server
        {
        public:
            class ServerException : public std::runtime_error
            {
            public:
                ServerException(const std::string &msg) : std::runtime_error(msg) {}
            };

        private:
            class MonitorBroker
            {
            public:
                MonitorBroker(int buffer_size = 1024) : buffer_size(buffer_size)
                {
                    std::thread(&MonitorBroker::accept_messages, this).detach();
                }

                void subscribe(int clnt_sock)
                {
                    clnt_sockets.insert(clnt_sock);
                }

                void unsubscribe(int clnt_sock)
                {
                    clnt_sockets.erase(clnt_sock);
                }

                void notify(std::string message)
                {
                    onlyfast::network::Response response{.status = onlyfast::network::ResponseStatus::OK, .body = message};
                    for (auto clnt_sock : clnt_sockets)
                    {
                        onlyfast::network::Server::SendResponse(clnt_sock, response);
                    }
                }

            private:
                std::set<int> clnt_sockets;
                int buffer_size;
                std::string unsubscribe_cmd = "UNSUBSCRIBE:";

                void accept_messages()
                {
                    char buffer[buffer_size];
                    ssize_t bytes_read;
                    fd_set readfds; // Множество файловых дескрипторов для использования с select()
                    int max_fd = 0;

                    // Устанавливаем таймаут на 1 секунду
                    struct timeval timeout;
                    timeout.tv_sec = 1;
                    timeout.tv_usec = 0;

                    while (true)
                    {
                        FD_ZERO(&readfds);

                        for (auto clnt_sock : clnt_sockets)
                        {
                            FD_SET(clnt_sock, &readfds);
                            if (clnt_sock > max_fd)
                            {
                                max_fd = clnt_sock;
                            }
                        }

                        // Вызываем select() с таймаутом
                        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
                        if (activity == -1)
                        {
                            // Ошибка при вызове select()
                            perror("Error in select()");
                            exit(EXIT_FAILURE);
                        }
                        else if (activity > 0)
                        {
                            for (auto clnt_sock : clnt_sockets)
                            {
                                // Проверяем, готовы ли к чтению
                                if (FD_ISSET(clnt_sock, &readfds))
                                {
                                    // stdin готов к чтению, читаем данные
                                    bytes_read = read(clnt_sock, buffer, buffer_size);
                                    if (bytes_read <= 0)
                                    {
                                        // Ошибка чтения или конец потока
                                        continue;
                                    }

                                    auto msg = std::string(buffer, bytes_read);
                                    if (msg.rfind(unsubscribe_cmd, 0) == 0)
                                    {
                                        unsubscribe(clnt_sock);
                                        CloseSocket(clnt_sock);
                                    }
                                }
                            }
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }
            };

        public:
            Server(const std::string &ip = "127.0.0.1",
                   int port = 80,
                   int buffer_size = 1024,
                   int max_clients = 10,
                   RequestHandlerType handler = Server::DefaultRequestHandler,
                   bool debug = false) : debug(debug),
                                         logger(std::cout, debug),
                                         serv_sock(CreateSocket(ip, port)),
                                         buffer_size(buffer_size),
                                         request_handler(std::move(handler)),
                                         monitorBroker(buffer_size)
            {
                if (listen(serv_sock, max_clients) == -1)
                {
                    perror("listen() error");
                    exit(EXIT_FAILURE);
                }

                logger << "Server started at " << ip << ":" << port << "\n";
            }

            ~Server()
            {
                Stop();
            }

            void Start()
            {
                AcceptClients();
            }

            void Stop()
            {
                close(serv_sock);
            }

            void SetRequestHandler(RequestHandlerType handler)
            {
                request_handler = std::move(handler);
            }

            void SetMiddleware(MiddlewareType middleware)
            {
                this->middleware = middleware;
            }

            void SetAfterResponse(AfterResponseType after_response)
            {
                this->after_response = after_response;
            }

            static Response DefaultRequestHandler(const Request &request)
            {
                Response response;
                response.status = ResponseStatus::OK;
                response.body = request.body;
                return response;
            }

            static void SendResponse(int client_sock, const Response &response)
            {
                // Формирование ответа в строковом виде
                std::string buffer = ConvertResponseStatusToString(response.status) + ":" + response.body;
                // Отправка ответа клиенту
                ssize_t bytes_written = write(client_sock, buffer.c_str(), buffer.length());
                if (bytes_written <= 0)
                {
                    perror("write() error");
                }
            }

            void notifyAll(std::string message)
            {
                monitorBroker.notify(message);
            }

        private:
            int buffer_size;
            int serv_sock;
            RequestHandlerType request_handler;
            bool debug;
            logger::Logger logger;
            MiddlewareType middleware = [](const Request &request) {};
            AfterResponseType after_response = [](const Request &request, const Response &response) {};
            MonitorBroker monitorBroker;

            // Функции для работы с сокетами
            int CreateSocket(const std::string &ip, int port)
            {
                int sock = socket(PF_INET, SOCK_STREAM, 0);
                if (sock == -1)
                {
                    perror("socket() error");
                    exit(EXIT_FAILURE);
                }

                struct sockaddr_in serv_addr;
                memset(&serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
                serv_addr.sin_port = htons(port);

                if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
                {
                    perror("bind() error");
                    exit(EXIT_FAILURE);
                }
                return sock;
            }

            void AcceptClients()
            {
                struct sockaddr_in clnt_addr;
                clnt_addr.sin_family = AF_INET;
                clnt_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Example IP address (localhost)
                socklen_t clnt_addr_size = sizeof(clnt_addr);
                char ip_addr[INET_ADDRSTRLEN]; // Buffer to store the IP address string

                while (1)
                {
                    try
                    {
                        int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                        if (clnt_sock == -1)
                        {
                            perror("accept() error");
                            continue;
                        }

                        // Convert binary IP address to string
                        const char *result = inet_ntop(AF_INET, &(clnt_addr.sin_addr), ip_addr, INET_ADDRSTRLEN);

                        // Обработка клиентского подключения
                        std::string body = ReadRequestBody(clnt_sock);
                        Request request{ip_addr, body};
                        if (result != NULL)
                        {
                            request.ip = result;
                        }

                        middleware(request);

                        Response response;

                        if (body == "SUBSCRIBE:")
                        {
                            monitorBroker.subscribe(clnt_sock);
                            response = {ResponseStatus::OK, "Subscribed"};
                            SendResponse(clnt_sock, response);
                        }
                        else
                        {
                            response = request_handler(request);
                            SendResponse(clnt_sock, response);
                            CloseSocket(clnt_sock);
                        }

                        logger << "Sent response: " << response.body << "\n";

                        after_response(request, response);
                    }
                    catch (const std::exception &e)
                    {
                        logger << "Caught exception: " << e.what() << '\n';
                    }
                }

                logger << "Server stopped\n";
            }

            std::string ReadRequestBody(int client_sock)
            {
                char buffer[buffer_size];
                ssize_t bytes_read = read(client_sock, buffer, buffer_size);
                if (bytes_read <= 0)
                {
                    perror("read() error");
                    throw std::runtime_error("Failed to read data from client");
                }

                return std::string(buffer, bytes_read);
            }

            static void CloseSocket(int sock)
            {
                close(sock);
            }

            static std::string ConvertResponseStatusToString(ResponseStatus status)
            {
                switch (status)
                {
                case ResponseStatus::OK:
                    return "OK";
                case ResponseStatus::FAILED:
                    return "FAILED";
                default:
                    return "UNKNOWN";
                }
            }
        };

        class Client
        {
        public:
            Client(const std::string &ip = "127.0.0.1",
                   int port = 80,
                   int buffer_size = 1024,
                   bool debug = false) : debug(debug),
                                         logger(std::cout, debug),
                                         serv_addr(CreateSocketAddress(ip, port)),
                                         buffer_size(buffer_size)

            {
            }

            Response SendRequest(const std::string &request_body)
            {
                int sock = socket(PF_INET, SOCK_STREAM, 0);
                if (sock == -1)
                {
                    perror("socket() error");
                    exit(EXIT_FAILURE);
                }

                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
                {
                    throw Server::ServerException("Server disconnected!");
                }

                ssize_t bytes_written = write(sock, request_body.c_str(), request_body.length());
                if (bytes_written <= 0)
                {
                    perror("write() error");
                    exit(EXIT_FAILURE);
                }

                logger << "Sent request: " << request_body << "\n";

                char buffer[buffer_size];
                ssize_t bytes_read = read(sock, buffer, buffer_size);
                if (bytes_read <= 0)
                {
                    perror("read() error");
                    exit(EXIT_FAILURE);
                }

                close(sock);

                std::string response_body(buffer, bytes_read);

                logger << "Received response: " << response_body << "\n";

                size_t pos = response_body.find(':');
                std::string status = response_body.substr(0, pos);
                std::string body = response_body.substr(pos + 1);

                Response response{ConvertStringStatusToResponseString(status), body};

                return response;
            }

        protected:
            struct sockaddr_in serv_addr;
            int buffer_size;
            bool debug;
            logger::Logger logger;

            struct sockaddr_in CreateSocketAddress(const std::string &ip, int port)
            {
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = inet_addr(ip.c_str());
                addr.sin_port = htons(port);
                return addr;
            }

            ResponseStatus ConvertStringStatusToResponseString(std::string status)
            {
                if (status == "OK")
                {
                    return ResponseStatus::OK;
                }
                else if (status == "FAILED")
                {
                    return ResponseStatus::FAILED;
                }
                else
                {
                    return ResponseStatus::FAILED;
                }
            }
        };

    }

    class Application
    {
    public:
        using Params = std::vector<std::string>;

        struct RequestData : network::Request
        {
            std::string cmd;
            Params params;

            RequestData(std::string request)
            {
                size_t pos = request.find(':');
                cmd = request.substr(0, pos);
                request = request.substr(pos + 1);

                while ((pos = request.find(';')) != std::string::npos)
                {
                    auto str = request.substr(0, pos);
                    if (!str.empty())
                    {
                        params.push_back(str);
                    }
                    request.erase(0, pos + 1);
                }
                if (!request.empty())
                {
                    params.push_back(request);
                }
            }
        };

    public:
        using AppHandlerType = std::function<network::Response(const RequestData &)>;

        Application(network::Server &server) : server(server)
        {
            server.SetRequestHandler([this](const network::Request &request)
                                     { return this->RequestHandler(request); });
        }

        network::Response RequestHandler(const network::Request &request)
        {
            try
            {
                RequestData rd(request.body);

                if (handlers.find(rd.cmd) == handlers.end())
                {
                    return {network::ResponseStatus::FAILED, "Unknown command"};
                }
                return handlers[rd.cmd](rd);
            }
            catch (const std::exception &e)
            {
                return {network::ResponseStatus::FAILED, e.what()};
            }
        }

        void RegisterHandler(const std::string &cmd, AppHandlerType handler)
        {
            handlers[cmd] = handler;
        }

        void Run()
        {
            server.Start();
        }

        void Stop()
        {
            server.Stop();
        }

    private:
        network::Server &server;
        std::map<std::string, AppHandlerType> handlers;
    };

    class Monitor : network::Client
    {
    public:
        using MonitorHandlerType = std::function<void(const std::string &)>;

        Monitor(const std::string &ip = "127.0.0.1",
                int port = 80,
                int buffer_size = 1024,
                bool debug = false) : Client(ip, port, buffer_size, debug)
        {
        }

        ~Monitor()
        {
            stop();
        }

        void stop()
        {
            logger << "Unsubscribing\n";
            unsubscribe();
            logger << "Closing socket\n";
            close(clnt_socket);
        }

        void setHandler(MonitorHandlerType handler)
        {
            this->handler = handler;
        }

        void setSleepTime(int sleep_time)
        {
            if (sleep_time < 0)
            {
                throw std::invalid_argument("Invalid sleep time");
            }
            this->sleep_time = sleep_time;
        }

        void run()
        {
            if (!subscribe())
            {
                std::cout << "Couldn't subscribe" << std::endl;
                return;
            }

            char buffer[buffer_size];

            while (true)
            {

                ssize_t bytes_read = read(clnt_socket, buffer, buffer_size);

                if (bytes_read <= 0)
                {
                    throw network::Server::ServerException("Server disconnected!");
                }

                std::string response_body(buffer, bytes_read);

                handler(response_body);
            }
        }

    private:
        MonitorHandlerType handler;
        int sleep_time = 1000; // milliseconds
        int clnt_socket;

        bool subscribe()
        {
            std::string request_body = "SUBSCRIBE:";
            clnt_socket = socket(PF_INET, SOCK_STREAM, 0);
            if (clnt_socket == -1)
            {
                perror("socket() error");
                exit(EXIT_FAILURE);
            }

            if (connect(clnt_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            {
                perror("connect() error");
                exit(EXIT_FAILURE);
            }

            ssize_t bytes_written = write(clnt_socket, request_body.c_str(), request_body.length());
            if (bytes_written <= 0)
            {
                perror("write() error");
                exit(EXIT_FAILURE);
            }

            logger << "Sent request: " << request_body << "\n";

            char buffer[buffer_size];
            ssize_t bytes_read = read(clnt_socket, buffer, buffer_size);
            if (bytes_read <= 0)
            {
                perror("read() error");
                exit(EXIT_FAILURE);
            }

            std::string response_body(buffer, bytes_read);

            logger << "Received response: " << response_body << "\n";

            size_t pos = response_body.find(':');
            std::string status = response_body.substr(0, pos);
            std::string body = response_body.substr(pos + 1);

            network::Response response{ConvertStringStatusToResponseString(status), body};

            if (response.status != network::ResponseStatus::OK)
            {
                return false;
            }
            return true;
        }

        void unsubscribe()
        {
            std::string request_body = "UNSUBSCRIBE:";

            ssize_t bytes_written = write(clnt_socket, request_body.c_str(), request_body.length());
            if (bytes_written <= 0)
            {
                perror("write() error");
                exit(EXIT_FAILURE);
            }
        }
    };

    class Arguments
    {
    public:
        enum class ArgType
        {
            STRING,
            INT,
            DOUBLE,
            BOOL
        };

        struct ArgData
        {
            ArgType type;
            std::optional<std::string> value;
            std::optional<std::string> description;
        };

        void
        AddArgument(const std::string &key, ArgType type, std::optional<std::string> description = std::nullopt, std::optional<std::string> default_value = std::nullopt)
        {
            args[key] = {.type = type, .value = default_value, .description = description};
        }

        bool Parse(int argc, char **argv)
        {
            for (int i = 0; i < argc; ++i)
            {
                if (IsKey(argv[i]))
                {
                    auto key = GetKey(argv[i]);
                    if (args.find(key) == args.end())
                    {
                        Help();
                        return false;
                    }

                    if (i + 1 < argc && !IsKey(argv[i + 1]))
                    {

                        args[key].value = argv[i + 1];
                    }
                    else
                    {
                        if (key == "help")
                        {
                            Help();
                            return false;
                        }
                        args[key].value = "true"; // flag, e.g. -v
                    }
                }
            }
            return true;
        }

        std::string Get(const std::string &key, std::optional<std::string> default_value = std::nullopt)
        {
            if (args.find(key) == args.end())
            {
                if (default_value.has_value())
                {
                    return default_value.value();
                }
                throw std::runtime_error("Unknown argument");
            }
            auto arg = args[key];
            if (arg.type != ArgType::STRING)
            {
                if (arg.value.has_value())
                {
                    return arg.value.value();
                }
                throw std::runtime_error("Invalid argument type");
            }
            return arg.value.value();
        }

        bool GetBool(const std::string &key, std::optional<bool> default_value = std::nullopt)
        {
            if (args.find(key) == args.end())
            {
                if (default_value.has_value())
                {
                    return default_value.value();
                }
                throw std::runtime_error("Unknown argument");
            }
            auto arg = args[key];
            if (arg.type != ArgType::BOOL)
            {
                if (arg.value.has_value())
                {
                    return arg.value.value() == "true";
                }
                throw std::runtime_error("Invalid argument type");
            }
            return arg.value.value() == "true";
        }

        int GetInt(const std::string &key, std::optional<int> default_value = std::nullopt)
        {
            if (args.find(key) == args.end())
            {
                if (default_value.has_value())
                {
                    return default_value.value();
                }
                throw std::runtime_error("Unknown argument");
            }
            auto arg = args[key];
            if (arg.type != ArgType::INT)
            {
                if (arg.value.has_value())
                {
                    return std::stoi(arg.value.value());
                }
                throw std::runtime_error("Invalid argument type");
            }
            try
            {
                return std::stoi(arg.value.value());
            }
            catch (std::exception &e)
            {
                if (default_value.has_value())
                {
                    return default_value.value();
                }
                throw std::runtime_error("Invalid argument value");
            }
        }

        double GetDouble(const std::string &key, std::optional<double> default_value = std::nullopt)
        {
            if (args.find(key) == args.end())
            {
                if (default_value.has_value())
                {
                    return default_value.value();
                }
                throw std::runtime_error("Unknown argument");
            }
            auto arg = args[key];
            if (arg.type != ArgType::DOUBLE)
            {
                if (arg.value.has_value())
                {
                    return std::stod(arg.value.value());
                }
                throw std::runtime_error("Invalid argument type");
            }
            try
            {
                return std::stod(arg.value.value());
            }
            catch (std::exception &e)
            {
                if (default_value.has_value())
                {
                    return default_value.value();
                }
                throw std::runtime_error("Invalid argument value");
            }
        }

    private:
        std::unordered_map<std::string, ArgData> args;

        bool IsKey(const std::string &arg)
        {
            return arg[0] == '-';
        }

        std::string GetKey(const std::string &arg)
        {
            return arg.substr(arg.find_first_not_of('-'));
        }

        void Help()
        {
            std::cout << "Usage: " << std::endl;
            for (const auto &[key, value] : args)
            {
                std::cout << key << " - " << value.description.value_or("No description") << std::endl;
            }
        }
    };
}