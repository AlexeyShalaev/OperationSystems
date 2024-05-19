#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>

#include <unistd.h>
#include <sstream>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

        class Server
        {
        public:
            Server(const std::string &ip = "127.0.0.1",
                   int port = 80,
                   int buffer_size = 1024,
                   RequestHandlerType handler = Server::DefaultRequestHandler,
                   bool debug = false) : debug(debug),
                                         logger(std::cout, debug),
                                         serv_sock(CreateSocket(ip, port)),
                                         buffer_size(buffer_size),
                                         request_handler(std::move(handler))
            {
                logger << "Server started at " << ip << ":" << port << "\n";
            }

            ~Server()
            {
                close(serv_sock);
            }

            void Start()
            {
                ReceiveRequests();
            }

            void SetRequestHandler(RequestHandlerType handler)
            {
                request_handler = std::move(handler);
            }

            void SetMiddleware(MiddlewareType middleware)
            {
                this->middleware = middleware;
            }

            static Response DefaultRequestHandler(const Request &request)
            {
                Response response;
                response.status = ResponseStatus::OK;
                response.body = request.body;
                return response;
            }

        private:
            int buffer_size;
            int serv_sock;
            RequestHandlerType request_handler;
            bool debug;
            logger::Logger logger;
            MiddlewareType middleware = [](const Request &request) {};

            // Функции для работы с сокетами
            int CreateSocket(const std::string &ip, int port)
            {
                int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

            void ReceiveRequests()
            {
                struct sockaddr_in clnt_addr;
                socklen_t clnt_addr_size = sizeof(clnt_addr);

                while (1)
                {
                    try
                    {
                        // Обработка клиентского подключения
                        Request request = ReadRequest(clnt_addr, clnt_addr_size);

                        middleware(request);

                        Response response = request_handler(request);
                        SendResponse(clnt_addr, clnt_addr_size, response);
                    }
                    catch (const std::exception &e)
                    {
                        logger << "Caught exception: " << e.what() << '\n';
                    }
                }

                logger << "Server stopped\n";
            }

            Request ReadRequest(struct sockaddr_in &clnt_addr, socklen_t &clnt_addr_size)
            {
                char buffer[buffer_size];
                ssize_t bytes_read = recvfrom(serv_sock, buffer, buffer_size, 0, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                if (bytes_read <= 0)
                {
                    perror("recvfrom() error");
                    throw std::runtime_error("Failed to read data from client");
                }

                Request request;
                request.body = std::string(buffer, bytes_read);
                logger << "Received request: " << request.body << "\n";
                return request;
            }

            void SendResponse(struct sockaddr_in &clnt_addr, socklen_t clnt_addr_size, const Response &response)
            {
                // Формирование ответа в строковом виде
                std::string buffer = ConvertResponseStatusToString(response.status) + ":" + response.body;
                // Отправка ответа клиенту
                ssize_t bytes_sent = sendto(serv_sock, buffer.c_str(), buffer.length(), 0, (struct sockaddr *)&clnt_addr, clnt_addr_size);
                if (bytes_sent <= 0)
                {
                    perror("write() error");
                }
                logger << "Sent response: " << buffer << "\n";
            }

            std::string ConvertResponseStatusToString(ResponseStatus status)
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
                sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (sock == -1)
                {
                    perror("socket() error");
                    exit(EXIT_FAILURE);
                }
            }

            ~Client()
            {
                close(sock);
            }

            Response SendRequest(const std::string &request_body)
            {
                ssize_t bytes_sent = sendto(sock, request_body.c_str(), request_body.length(), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (bytes_sent <= 0)
                {
                    perror("sendto() error");
                    exit(EXIT_FAILURE);
                }

                logger << "Sent request: " << request_body << "\n";

                char buffer[buffer_size];
                struct sockaddr_in from_addr;
                socklen_t from_addr_size = sizeof(from_addr);
                ssize_t bytes_read = recvfrom(sock, buffer, buffer_size, 0, (struct sockaddr *)&from_addr, &from_addr_size);
                if (bytes_read <= 0)
                {
                    perror("recvfrom() error");
                    exit(EXIT_FAILURE);
                }

                std::string response_body(buffer, bytes_read);
                logger << "Received response: " << response_body << "\n";

                size_t pos = response_body.find(':');
                std::string status = response_body.substr(0, pos);
                std::string body = response_body.substr(pos + 1);

                Response response{ConvertStringStatusToResponseStatus(status), body};
                return response;
            }

        private:
            int sock;
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

            ResponseStatus ConvertStringStatusToResponseStatus(std::string status)
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
        using AppHandlerType = std::function<network::Response(const Params &params)>;

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
                return handlers[rd.cmd](rd.params);
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

    private:
        network::Server &server;
        std::map<std::string, AppHandlerType> handlers;

        struct RequestData
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
                    params.push_back(request.substr(0, pos));
                    request.erase(0, pos + 1);
                }
                params.push_back(request);
            }
        };
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
