import os
import subprocess
import time


def write_to_file(filename, content):
    with open(filename, "w") as file:
        file.write(content)


def run(PROGRAM_FOLDER):
    current_timestamp = str(int(time.time()))

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

    input("Press Enter to continue...")

    os.system(f"docker compose -f {docker_compose_filename} rm -fsv")
    os.remove(docker_compose_filename)
    print(f"EMULATING EXITED WITH CODE {result}")


PROGRAM_FOLDER = "6-10points"
run(PROGRAM_FOLDER)
