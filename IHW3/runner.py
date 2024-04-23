import docker

# Создание клиента Docker
client = docker.from_env()

# Путь к Dockerfile
dockerfile_path = 'C:/Users/Alex Shalaev/YandexDisk/HSE/БПИ/2 курс/Операционные системы/OperationSystems/IHW3/programs/4-5points'

# Сборка образа
image, logs = client.images.build(path=dockerfile_path)

# Вывод логов сборки
for line in logs:
    print(line)

# Запуск контейнера из собранного образа
container = client.containers.run(image, detach=True, remove=True, name='app')

# Ожидание выполнения контейнера
s = input('Press any key to stop container')

# Удаление контейнера
container.remove()

# Удаление образа
client.images.remove(image.id)