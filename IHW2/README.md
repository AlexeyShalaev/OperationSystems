# ИДЗ №1

    Шалаев Алексей Дмитриевич
    БПИ229

## 24 вариант

Задача о программистах. В отделе работают три программиста. Каждый программист пишет свою программу и отдает ее на проверку одному из двух оставшихся программистов выбирая его случайно и ожидая окончания проверки. Программист начинает проверять чужую программу, когда его собственная уже написана и передана на проверку. По завершении проверки, программист возвращает программу с результатом (формируемым случайно по любому из выбранных Вами законов): программа написана правильно или неправильно. Программист «спит», если отправил свою программу и не проверяет чужую программу. Программист «просыпается», когда получает заключение от другого программиста. Если программа признана правильной, программист пишет другую программу, если программа признана неправильной, программист исправляет ее и отправляет на проверку тому же программисту, который ее проверял. К исправлению своей программы он приступает, завершив проверку чужой программы. При наличии в очереди проверяемых программ и приходе заключения о неправильной своей программы программист может выбирать любую из возможных работ. Необходимо учесть, что кто-то из программистов может
получать несколько программ на проверку.
Создать многопроцессное приложение, моделирующее работу программистов.
Каждый программист — это отдельный поток.


## Параметры

Отсутствуют. В задаче сказано 3 программиста, кол-во программ не задано.
Однако, я решил задать конфигурация так (через define):

NUMBER_OF_PROGRAMMERS 3 // кол-во программистов
NUMBER_OF_PROGRAMS 5 // кол-во программ
SLEEP usleep((rand() % 3000000) + 1000000) // случайная задержка от 1 до 3 секунд
DEBUG_MODE 1 // режим вывода логов

### Результаты

В папке results находятся выводы программы в виде логов.

## Общая схема решаемой задачи

Есть структура Coworking, находящаяся в разделяемой памяти.
Она содержит в себе кол-во (необходимых и созданных) программ, счетчик для идентификации программ, наименовая семофоров и программистов.
Каждый программист имеет семафор, программу и список проверок.
Проверка работ приоритетнее написания. Проверяющим является первый программист, у которого наименьшее кол-во проверяемых работ.
Программа завершается либо написанием всех программ, либо прерыванием.

### 4-5 баллов

Множество процессов взаимодействуют с использованием именованных POSIX семафоров. Обмен данными ведется через разделяемую память в стандарте POSIX.

### 6-7 баллов

Множество процессов взаимодействуют с использованием неименованных POSIX семафоров расположенных в разделяемой памяти. Обмен данными также ведется через разделяемую память в стандарте POSIX.

### 8 баллов

Программа разделена на несколько файлов.
Множество независимых процессов взаимодействуют с использованием семафоров в стандарте UNIX SYSTEM V. Обмен данными ведется через разделяемую память в стандарте UNIX
SYSTEM V.