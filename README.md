# ИДЗ №1

    Шалаев Алексей Дмитриевич
    БПИ229

## 34 вариант

    Разработать программу, которая на основе анализа двух входных ASCII–строк формирует на выходе две другие строки. В первой из выводимых строк содержатся символы, которых нет во второй исходной строке. Во второй выводимой строке содержатся символы отсутствующие в первой входной строке (разности символов). Каждый символ в соответствующей выходной строке должен встречаться только один раз. Входными и выходными параметрами являются имена четырех файлов, задающих входные и выходные строки.


## Параметры

`./main <input_file1> <input_file2> <output_file1> <output_file2>`

## Тестирование

`python test.py`

Тестирование возможно при запущенном docker.
Программа запустит каждый *.c файл и проверит результаты.

### Тесты

| Тест                  | 1-ая входная строка | 2-ая входная строка | 1-ая выходная строка | 2-ая выходная строка |
|-----------------------|----------------------|----------------------|----------------------|----------------------|
| Классический вариант  | 123                  | 234                  | 1                    | 4                    |
| Абсолютно разные      | 1                    | 2                    | 1                    | 2                    |
| Нет пересечения 1     | 1                    | 1                    |                      |                      |
| Нет пересечения 2     | 1 2 3                | 3 2 1                |                      |                      |
| Пустые данные         |                      |                      |                      |                      |

## Общая схема решаемой задачи

### 4 балла

Для решения задачи мы используем три процесса, которые взаимодействуют между собой через неименованные каналы. Ниже приведена общая схема взаимодействия этих процессов:

1. **Первый процесс**:
   - Читает данные из входных файлов.
   - Создает три неименованных канала для связи с другими процессами.
   - Передает данные второму процессу через один из каналов.

2. **Второй процесс**:
   - Получает данные от первого процесса через один из каналов.
   - Анализирует входные данные и формирует 2 строки выходных данных.
   - Передает 2 строки выходных данных третьему процессу через 2 канала.

3. **Третий процесс**:
   - Получает данные от второго процесса через 2 канала.
   - Записывает результаты в файлы.

Процессы взаимодействуют между собой путем передачи данных через неименованные каналы, а также связываются с входными и выходными файлами для чтения и записи данных. Вся эта схема позволяет эффективно обрабатывать входные данные и записывать результаты в выходные файлы.

### 5 баллов

Для решения задачи мы используем три процесса, которые взаимодействуют между собой через именованные каналы. Ниже приведена общая схема взаимодействия этих процессов:

1. **Первый процесс**:
   - Читает данные из входных файлов.
   - Создает три именованных канала (fifo1, fifo2, fifo3) для связи с другими процессами.
   - Передает данные второму процессу через fifo1.

2. **Второй процесс**:
   - Получает данные от первого процесса через fifo1.
   - Анализирует входные данные и формирует 2 строки выходных данных.
   - Передает 2 строки выходных данных третьему процессу через fifo2 и fifo3.

3. **Третий процесс**:
   - Получает данные от второго процесса через fifo2 и fifo3.
   - Записывает результаты в файлы.

Процессы взаимодействуют между собой путем передачи данных через именованные каналы, а также связываются с входными и выходными файлами для чтения и записи данных. Вся эта схема позволяет эффективно обрабатывать входные данные и записывать результаты в выходные файлы.

### 6 баллов

Для решения задачи мы используем два процесса, которые взаимодействуют между собой через неименованные каналы. Ниже приведена общая схема взаимодействия этих процессов:

1. **Первый процесс**:
   - Читает данные из входных файлов.
   - Создает три неименованных канала для связи с другими процессами.
   - Передает данные второму процессу через один из каналов.
   - Получает данные от второго процесса через 2 канала.
   - Записывает результаты в файлы.

2. **Второй процесс**:
   - Получает данные от первого процесса через один из каналов.
   - Анализирует входные данные и формирует 2 строки выходных данных.
   - Передает 2 строки выходных данных первому процессу через 2 канала.

Процессы взаимодействуют между собой путем передачи данных через неименованные каналы, а также связываются с входными и выходными файлами для чтения и записи данных. Вся эта схема позволяет эффективно обрабатывать входные данные и записывать результаты в выходные файлы.

### 7 баллов

Для решения задачи мы используем два процесса, которые взаимодействуют между собой через именованные каналы. Ниже приведена общая схема взаимодействия этих процессов:

1. **Первый процесс**:
   - Читает данные из входных файлов.
   - Создает три именованных канала (fifo1, fifo2, fifo3) для связи с другими процессами.
   - Передает данные второму процессу через fifo1.
   - Получает данные от второго процесса через fifo2 и fifo3.
   - Записывает результаты в файлы.

2. **Второй процесс**:
   - Получает данные от первого процесса через fifo1.
   - Анализирует входные данные и формирует 2 строки выходных данных.
   - Передает 2 строки выходных данных третьему процессу через fifo2 и fifo3.

Процессы взаимодействуют между собой путем передачи данных через именованные каналы, а также связываются с входными и выходными файлами для чтения и записи данных. Вся эта схема позволяет эффективно обрабатывать входные данные и записывать результаты в выходные файлы.