import os
import subprocess
import time

current_timestamp = str(time.time())

docker_filename = f'{current_timestamp}.Dockerfile'
docker_compose_filename = f"docker-compose-{current_timestamp}.yml"

with open(docker_filename, "w") as dockerfile:
    dockerfile.write(f"""# Use an official Python runtime as a parent image
FROM python:3.10.9-slim-buster

# Install gcc
RUN apt-get update && apt-get install -y \
    gcc \
    && rm -rf /var/lib/apt/lists/*

# Install PyYAML via pip
RUN pip install pyyaml

# Set the working directory in the container
WORKDIR /app

# Копируем исходные файлы программ в контейнер
COPY test.py test.py

COPY config.yaml config.yaml

COPY programs programs

COPY tests tests

CMD ["python", "test.py"]
""")


docker_compose_content = f"""version: '3'\nservices:
    test:
        build:
            context: .
            dockerfile: {docker_filename}
        container_name: test_os_{current_timestamp}"""

with open(docker_compose_filename, "w") as docker_compose_file:
    docker_compose_file.write(docker_compose_content)

try:
    result = subprocess.run(
        ["docker", "compose", "-f", docker_compose_filename, "up", "--build"], timeout=60).returncode
except Exception:
    result = -1
os.system(f"docker compose -f {docker_compose_filename} rm -fsv")
#os.system(f"docker system prune -a -f")
print(f"TESTING EXITED WITH CODE {result}")
os.remove(docker_filename)
os.remove(docker_compose_filename)
