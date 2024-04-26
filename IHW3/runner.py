import os
import subprocess
import time


def write_to_file(filename, content):
    with open(filename, "w") as file:
        file.write(content)


def run(PROGRAM_FOLDER):
    current_timestamp = str(time.time())

    docker_compose_filename = f"docker-compose-{current_timestamp}.yml"

    docker_compose_content = f"""version: '3'\nservices:
        test_os_{current_timestamp}:
            build:
                context: ./programs/{PROGRAM_FOLDER}/
                dockerfile: Dockerfile
            container_name: test_os_{current_timestamp}"""

    with open(docker_compose_filename, "w") as docker_compose_file:
        docker_compose_file.write(docker_compose_content)

    try:
        result = subprocess.run(
            ["docker", "compose", "-f", docker_compose_filename, "up", "-d", "--build"], timeout=60).returncode
    except Exception as ex:
        print("ERROR: ", ex)
        result = -1

    CMD = f"docker exec -it test_os_{current_timestamp}"

    RUN_SERVER_CMD = f"{CMD} ./server"
    RUN_CLIENT_CMD = f"{CMD} ./client"
    RUN_MONITOR_CMD = f"{CMD} ./monitor"

    print(f"START SERVER: {RUN_SERVER_CMD}")
    print(f"START CLIENT: {RUN_CLIENT_CMD}")
    print(f"START MONITOR: {RUN_MONITOR_CMD}")

    server_process = subprocess.run(
        RUN_SERVER_CMD, stdout=subprocess.PIPE, text=True)
    client_process = subprocess.run(
        RUN_CLIENT_CMD, stdout=subprocess.PIPE, text=True)
    monitor_process = subprocess.run(
        RUN_MONITOR_CMD, stdout=subprocess.PIPE, text=True)

    input("Press Enter to continue...")

    write_to_file(f"server_output_{
                  current_timestamp}.txt", server_process.stdout)
    write_to_file(f"client_output_{
                  current_timestamp}.txt", client_process.stdout)
    write_to_file(f"monitor_output_{
                  current_timestamp}.txt", monitor_process.stdout)

    os.system(f"docker compose -f {docker_compose_filename} rm -fsv")
    os.remove(docker_compose_filename)
    print(f"EMULATING EXITED WITH CODE {result}")


PROGRAM_FOLDER = "10points"
run(PROGRAM_FOLDER)
