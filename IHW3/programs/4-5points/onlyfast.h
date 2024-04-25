#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <vector>
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
        using MiddlewareType = std::function<void(int clnt_sock, const Request &)>;

        class Server
        {
        public:
            Server(const std::string &ip = "127.0.0.1",
                   int port = 80,
                   int buffer_size = 1024,
                   int max_clients = 10,
                   RequestHandlerType handler = Server::DefaultRequestHandler,
                   bool debug = true) : debug(debug),
                                         logger(std::cout, debug),
                                         serv_sock(CreateSocket(ip, port)),
                                         buffer_size(buffer_size),
                                         request_handler(std::move(handler))
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
                close(serv_sock);
            }

            void Start()
            {
                AcceptClients();
            }

            void SetRequestHandler(RequestHandlerType handler)
            {
                request_handler = std::move(handler);
            }

            void SetMiddleware(MiddlewareType middleware)
            {
                this->middleware = middleware;
            }

        private:
            int buffer_size;
            int serv_sock;
            RequestHandlerType request_handler;
            bool debug;
            logger::Logger logger;
            MiddlewareType middleware = [](int clnt_sock, const Request &request) {};

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
                socklen_t clnt_addr_size = sizeof(clnt_addr);

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

                        // Обработка клиентского подключения
                        Request request = ReadRequest(clnt_sock);

                        middleware(clnt_sock, request);

                        Response response = request_handler(request);
                        SendResponse(clnt_sock, response);

                        CloseSocket(clnt_sock);
                    }
                    catch (const std::exception &e)
                    {
                        logger << "Caught exception: " << e.what() << '\n';
                    }
                }

                logger << "Server stopped\n";
            }

            Request ReadRequest(int client_sock)
            {
                char buffer[buffer_size];
                ssize_t bytes_read = read(client_sock, buffer, buffer_size);
                if (bytes_read <= 0)
                {
                    perror("read() error");
                    throw std::runtime_error("Failed to read data from client");
                }

                Request request;
                request.body = std::string(buffer, bytes_read);
                logger << "Received request: " << request.body << "\n";
                return request;
            }

            void SendResponse(int client_sock, const Response &response)
            {
                // Формирование ответа в строковом виде
                std::string buffer = ConvertResponseStatusToString(response.status) + ":" + response.body;
                // Отправка ответа клиенту
                ssize_t bytes_written = write(client_sock, buffer.c_str(), buffer.length());
                if (bytes_written <= 0)
                {
                    perror("write() error");
                }
                logger << "Sent response: " << buffer << "\n";
            }

            static void CloseSocket(int sock)
            {
                close(sock);
            }

            static Response DefaultRequestHandler(const Request &request)
            {
                Response response;
                response.status = ResponseStatus::OK;
                response.body = request.body;
                return response;
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
                    perror("connect() error");
                    exit(EXIT_FAILURE);
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

        private:
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
}
