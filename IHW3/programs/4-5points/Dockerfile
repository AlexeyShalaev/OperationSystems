# Сначала этап сборки
FROM alpine

# Устанавливаем необходимые инструменты для компиляции
RUN apk --no-cache add g++
RUN apk add libstdc++
RUN apk add libgcc

# Устанавливаем рабочую директорию в контейнере
WORKDIR /app

# Копируем исходные файлы программы в контейнер
COPY *.cpp .
COPY *.h .

# Компилируем программы
RUN g++ -o server server.cpp
RUN g++ -o client client.cpp

CMD ["sh"]

