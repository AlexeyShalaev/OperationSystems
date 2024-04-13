import os
import subprocess
import time

FILENAME = "4points.c"

current_timestamp = str(time.time())

docker_filename = f'{current_timestamp}.Dockerfile'
docker_compose_filename = f"docker-compose-{current_timestamp}.yml"

with open(docker_filename, "w") as dockerfile:
    dockerfile.write(f"""# Сначала этап сборки
FROM alpine

# Устанавливаем необходимые инструменты для компиляции
RUN apk --no-cache add gcc musl-dev

# Устанавливаем рабочую директорию в контейнере
WORKDIR /app

ARG FILENAME

# Копируем исходные файлы программ в контейнер
COPY programs/$FILENAME main.c

# Компилируем программы
RUN gcc -o main main.c

# Указываем команду по умолчанию для запуска
CMD ["timeout", "3m", "sleep", "infinity"]
""")


docker_compose_content = f"""version: '3'\nservices:
    test:
        build:
            context: .
            dockerfile: {docker_filename}
            args:
                FILENAME: {FILENAME}
        container_name: test_os_{current_timestamp}"""

with open(docker_compose_filename, "w") as docker_compose_file:
    docker_compose_file.write(docker_compose_content)

try:
    result = subprocess.run(
        ["docker", "compose", "-f", docker_compose_filename, "up", "-d", "--build"], timeout=60).returncode
except Exception as ex:
    print("ERROR: ", ex)
    result = -1
input("Press Enter to continue...")
os.system(f"docker compose -f {docker_compose_filename} rm -fsv")
os.remove(docker_filename)
os.remove(docker_compose_filename)
print(f"TESTING EXITED WITH CODE {result}")
