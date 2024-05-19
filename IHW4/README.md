# ИДЗ №4

    Шалаев Алексей Дмитриевич
    БПИ229

## 24 вариант

Задача о программистах. В отделе работают три программиста. Каждый программист пишет свою программу и отдает ее на проверку одному из двух оставшихся программистов выбирая его случайно и ожидая окончания проверки. Программист начинает проверять чужую программу, когда его собственная уже написана и передана на проверку. По завершении проверки, программист возвращает программу с результатом (формируемым случайно по любому из выбранных Вами законов): программа написана правильно или неправильно. Программист «спит», если отправил свою программу и не проверяет чужую программу. Программист «просыпается», когда получает заключение от другого программиста. Если программа признана правильной, программист пишет другую программу, если программа признана неправильной, программист исправляет ее и отправляет на проверку тому же программисту, который ее проверял. К исправлению своей программы он приступает, завершив проверку чужой программы. При наличии в очереди проверяемых программ и приходе заключения о неправильной своей программы программист может выбирать любую из возможных работ. Необходимо учесть, что кто-то из программистов может получать несколько программ на проверку.

## Запуск

Запуск возможен при запущенном docker и установленным python.

- `cd IHW4`
- `python runner.py`

Runner поднимет систему, выведет команды для запуска проргамм.
Запустите в отдельных окнах программы, после изучения, нажмите кнопку для продолжения в runner.py, программа очистит следы.

Пропишите `docker exec -it app ./$PROGRAM_NAME --help`, чтобы вывести флаги и параметры.

## Результаты

В папке results находятся выводы программ в виде логов.

### 4-5 баллов

Клиент-серверное взаимодействие.

### 6-10 баллов

Добавлены клиенты-мониторы, которые получают информацию.
Добавлен брокер сообщений.
Корректное завершение программ.

P.S. отличие от ИДЗ 3 в том, что TCP заменен на UDP, но из-за особенностей UDP версия программы на 6-7 баллов покрыла и остальные программы на более высокий балл в связи выстроенной архитектурой.

## Краткое руководство

Рассмотрим итоговый фреймворк из 6-10points.

### Onlyfast

В namespace `onlyfast` входят:
- `Logger` - класс, который логгирует действия, если `debug = True`
- `Arguments` - класс, управляющие аргументами программы
- `Server` - сервер, обрабатывает запросы клиентов
- `Client` - клиент, взаимодействует с сервером
- `Application` - обертка над сервером, которая обрабатывает запросы, добавляя бизнес-логику
- `Monitor` - обертка над клиентом, получает данные от обзервера
- `MonitorBroker` - брокер сообщений, обзервер

OnlyFast имеет соглашение, что все запросы имеют вид: `CMD:Param1;Param2;...;ParamN`

### server.cpp

Данная программа читает аргументы, создает сервер и приложение с обработчиками, которые решают задачу с помщью класса `Solution`.
Так же есть `AfterResponse`, которые срабатывает после запросов и уведомляет все мониторы.

### client.cpp

Здесь каждый программист - клиент, который взаимодействет с сервером.

### monitor.cpp

Монитор подписывается на сервер и получает от него данные.

## Framework OnlyFast

### Описание фреймворка "OnlyFast"
"OnlyFast" - это минималистичный фреймворк для разработки сетевых приложений на языке C++ с использованием сокетов. Фреймворк предоставляет базовые инструменты для создания серверов, клиентов и обработки сетевых запросов.

### Основные компоненты
- **onlyfast::logger::Logger**: Класс для логирования сообщений. Позволяет выводить сообщения в стандартный поток вывода или в указанный пользователем поток.
- **onlyfast::network::Server**: Класс для создания сервера. Предоставляет возможность установки обработчика запросов, промежуточных обработчиков и обработчика после ответа.
- **onlyfast::network::Client**: Класс для создания клиента. Позволяет отправлять запросы серверу и получать ответы.
- **onlyfast::Application**: Класс для управления сервером приложения. Позволяет регистрировать обработчики запросов и запускать сервер.
- **onlyfast::Monitor**: Класс для мониторинга сервера. Позволяет подписаться на уведомления от сервера и получать их.
- **onlyfast::Arguments**: Класс для парсинга аргументов командной строки. Предоставляет удобный интерфейс для получения значений аргументов.

### Использование

#### Создание сервера:
```cpp
onlyfast::network::Server server("127.0.0.1", 8080);
server.SetRequestHandler([](const onlyfast::network::Request &request) {
    // Обработка запроса
    return onlyfast::network::Response{onlyfast::network::ResponseStatus::OK, "Response"};
});
server.Start();
```

#### Создание клиента и отправка запроса:
```cpp
onlyfast::network::Client client("127.0.0.1", 8080);
auto response = client.SendRequest("Request");
```

#### Создание приложения:
```cpp
onlyfast::network::Server server("127.0.0.1", 8080);
onlyfast::Application app(server);
app.RegisterHandler("command", [](const onlyfast::Application::RequestData &request) {
    // Обработка запроса
    return onlyfast::network::Response{onlyfast::network::ResponseStatus::OK, "Response"};
});
app.Run();
```

#### Мониторинг сервера:
```cpp
onlyfast::Monitor monitor("127.0.0.1", 8080);
monitor.setHandler([](const std::string &message) {
    // Обработка уведомления
});
monitor.run();
```

### Документация

**onlyfast::logger::Logger**
**Конструктор:**
`Logger(std::ostream &stream = std::cout, bool logging = true)`: Создает объект логгера с указанным потоком вывода (по умолчанию - стандартный вывод) и флагом логирования.
**Методы:**
`operator<<`: Перегруженный оператор для записи сообщений в лог.

**onlyfast::network::Server**
**Конструктор:**
`Server(const std::string &ip = "127.0.0.1", int port = 80, int buffer_size = 1024, int max_clients = 10, RequestHandlerType handler = Server::DefaultRequestHandler, bool debug = false)`: Создает объект сервера с указанными параметрами.
**Методы:**
- `Start()`: Запускает сервер.
- `Stop()`: Останавливает сервер.
- `SetRequestHandler(RequestHandlerType handler)`: Устанавливает обработчик запросов.
- `SetMiddleware(MiddlewareType middleware)`: Устанавливает промежуточный обработчик.
- `SetAfterResponse(AfterResponseType after_response)`: Устанавливает обработчик после ответа.

**onlyfast::network::Client**
**Конструктор:**
`Client(const std::string &ip = "127.0.0.1", int port = 80, int buffer_size = 1024, bool debug = false)`: Создает объект клиента с указанными параметрами.
**Методы:**
`SendRequest(const std::string &request_body)`: Отправляет запрос серверу и возвращает ответ.

**onlyfast::Application**
**Конструктор:**
`Application(network::Server &server)`: Создает объект приложения с указанным сервером.
**Методы:**
- `RegisterHandler(const std::string &cmd, AppHandlerType handler)`: Регистрирует обработчик для указанной команды.
- `Run()`: Запускает сервер приложения.
- `Stop()`: Останавливает сервер приложения.

**onlyfast::Monitor**
**Конструктор:**
`Monitor(const std::string &ip = "127.0.0.1", int port = 80, int buffer_size = 1024, bool debug = false)`: Создает объект монитора с указанными параметрами.
**Методы:**
- `setHandler(MonitorHandlerType handler)`: Устанавливает обработчик уведомлений.
- `setSleepTime(int sleep_time)`: Устанавливает время ожидания между запросами.
- `run(bool daemon)`: Запускает монитор (daemon - запуск мониторинга в отдельном потоке).


**onlyfast::Arguments**
**Методы:**
- `AddArgument(const std::string &key, ArgType type, std::optional<std::string> description = std::nullopt, std::optional<std::string> default_value = std::nullopt)`: Добавляет аргумент командной строки.
- `Parse(int argc, char **argv)`: Парсит аргументы командной строки.
- `Get(const std::string &key, std::optional<std::string> default_value = std::nullopt)`: Возвращает значение аргумента по ключу.
- `GetBool(const std::string &key, std::optional<bool> default_value = std::nullopt)`: Возвращает значение аргумента типа bool.
- `GetInt(const std::string &key, std::optional<int> default_value = std::nullopt)`: Возвращает значение аргумента типа int.
- `GetDouble(const std::string &key, std::optional<double> default_value = std::nullopt)`: Возвращает значение аргумента типа double.


### Примеры использования
**Пример 1: Создание сервера и клиента**
```cpp
// Создание сервера
onlyfast::network::Server server("127.0.0.1", 8080);
server.SetRequestHandler([](const onlyfast::network::Request &request) {
    // Обработка запроса
    return onlyfast::network::Response{onlyfast::network::ResponseStatus::OK, "Response"};
});
server.Start();

// Создание клиента и отправка запроса
onlyfast::network::Client client("127.0.0.1", 8080);
auto response = client.SendRequest("Request");
```

**Пример 2: Создание приложения с обработчиками команд**
```cpp
// Создание сервера
onlyfast::network::Server server("127.0.0.1", 8080);
// Создание приложения
onlyfast::Application app(server);
// Регистрация обработчиков команд
app.RegisterHandler("command", [](const onlyfast::Application::RequestData &request) {
    // Обработка команды
    return onlyfast::network::Response{onlyfast::network::ResponseStatus::OK, "Response"};
});
// Запуск сервера приложения
app.Run();
```

### Заключение
Фреймворк "OnlyFast" предоставляет простой и эффективный способ создания сетевых приложений на языке C++. С его помощью вы можете легко разрабатывать серверы, клиенты и обрабатывать сетевые запросы с минимальными усилиями.