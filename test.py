from dataclasses import dataclass
import os
import subprocess
import tempfile

import yaml


class Program:
    class Unit:
        def __init__(self, file_path: str, args: list[str]):
            self.file_path = file_path
            self.args = args if args else []

        def __str__(self):
            return f"Unit: {self.file_path}\nArgs: {self.args}\n"

    def __init__(self, name: str, units: list[Unit]):
        self.name = name
        self.units = units

    def __str__(self):
        units = '\n'.join([str(unit) for unit in self.units])
        return f"Program: {self.name}\n{units}\n\n"

    def run(self, tests_folders):
        copy_commands = ''
        run_commands = ''
        binary_commands = ''

        for unit in self.units:
            unit_filename = unit.file_path.split('/')[-1]
            bin_filename = unit_filename.split('.c')[0]
            copy_commands += f"COPY {unit.file_path} {unit_filename}\n"
            run_commands += f"RUN gcc -o {bin_filename} {unit_filename}\n"
            binary_commands += f"./{bin_filename} {' '.join(unit.args)} && "

        binary_commands = binary_commands.strip(" && ")
        dockerfile_name = f'{self.name}.Dockerfile'

        with open(dockerfile_name, "w") as dockerfile:
            dockerfile.write(f"""# Берем образ с поддержкой языка С для сборки программ
FROM gcc:latest AS builder

# Устанавливаем рабочую директорию в контейнере
WORKDIR /app

# Копируем исходные файлы программ в контейнер
{copy_commands}

{run_commands}

CMD {binary_commands}
""")
        docker_compose_filename = f"docker-compose-{self.name}.yml"
        docker_compose_content = f"version: '3'\nservices:"

        for index, test_case_folder in enumerate(tests_folders, start=1):
            docker_compose_content += f"""
    test_{index}:
        build:
            context: .
            dockerfile: {dockerfile_name}
        container_name: test_{index}
        tty: true
        volumes:
            - {test_case_folder}:/app/tests"""

        with open(docker_compose_filename, "w") as docker_compose_file:
            docker_compose_file.write(docker_compose_content)

        try:
            result = subprocess.run(
                ["docker", "compose", "-f", docker_compose_filename, "up", "--build"], timeout=10).returncode
        except Exception:
            result = -1
        os.system(f"docker compose -f {docker_compose_filename} rm -fsv")
        os.system(f"docker system prune -a -f")
        print(f"PROGRAM {self.name} EXITED WITH CODE {result}")
        os.remove(dockerfile_name)
        os.remove(docker_compose_filename)
        return result


@dataclass
class Config:
    programs: list[Program]
    tests_folders: list[str]

    @staticmethod
    def load_config(config_path: str, tests_folder_path: str = "./tests"):
        # if not os.path.isabs(tests_folder_path):
        #    tests_folder_path = os.path.abspath(tests_folder_path)

        with open(config_path, "r") as file:
            config = yaml.safe_load(file)

        if len(config.keys()) != 1:
            raise ValueError("Config file must have only 1 parameter.")

        programs: list[Program] = []

        programs_folder = list(config.keys())[0]

        for program_name, program_data in config.get(programs_folder, {}).items():
            units = []
            for file_name, args in program_data.items():
                file_path = os.path.join(
                    programs_folder, file_name).replace("\\", "/")
                units.append(Program.Unit(file_path, args))
            programs.append(Program(program_name, units))

        tests_folders = []  # List of test folders
        for test_case_folder in os.listdir(tests_folder_path):
            test_case_folder_path = os.path.join(
                tests_folder_path, test_case_folder)
            if os.path.isdir(test_case_folder_path):
                tests_folders.append(test_case_folder_path.replace("\\", "/"))

        return Config(programs, tests_folders)

    def __str__(self) -> str:
        tests_folders_str = '\n'.join(self.tests_folders)
        return f"Programs:\n\n{''.join([str(program) for program in self.programs])}Tests Folders:\n{tests_folders_str}\n\n"


class Test:
    config: Config

    def __init__(self, config):
        self.config = config

    @staticmethod
    def process_test_case(test_case_folder):
        try:
            input_file1_path = os.path.join(test_case_folder, "input_1.txt")
            input_file2_path = os.path.join(test_case_folder, "input_2.txt")
            output_file1_path = os.path.join(test_case_folder, "output_1.txt")
            output_file2_path = os.path.join(test_case_folder, "output_2.txt")

            with open(input_file1_path, "r") as input_file1, open(
                input_file2_path, "r"
            ) as input_file2, open(output_file1_path, "r") as output_file1, open(
                output_file2_path, "r"
            ) as output_file2:

                input_data1 = set(input_file1.read().strip())
                input_data2 = set(input_file2.read().strip())

                expected_output1 = "".join(input_data1 - input_data2)
                expected_output2 = "".join(input_data2 - input_data1)

                output1 = output_file1.read().strip()
                output2 = output_file2.read().strip()

                if output1 == expected_output1 and output2 == expected_output2:
                    print(f"TEST CASE {test_case_folder} PASSED.")
                    return 1
                else:
                    print(f"TEST CASE {test_case_folder} FAILED.")

                    if output1 != expected_output1:
                        print(f"OUTPUT 1 MISMATCH")
                        print(f"EXPECTED : {expected_output1}")
                        print(f"ACTUAL: {output1}")

                    print()

                    if output2 != expected_output2:
                        print(f"OUTPUT 2 MISMATCH")
                        print(f"EXPECTED : {expected_output2}")
                        print(f"ACTUAL: {output2}")

                    print()
        except Exception as e:
            print(f"TEST CASE {test_case_folder} FAILED.")
            print(f"ERROR: {e}")
            print()
        return 0

    def test(self):
        passed_tests = 0
        for test_case_folder_path in self.config.tests_folders:
            passed_tests += self.process_test_case(test_case_folder_path)
        print(f"{passed_tests} / {len(self.config.tests_folders)}")
        if passed_tests == len(self.config.tests_folders):
            return True
        return False

    def test_program(self, program: Program):
        status = False
        print(
            f"====================== TESTING PROGRAM {program.name} ========================="
        )
        for test_case_folder_path in self.config.tests_folders:
            for file in os.listdir(test_case_folder_path):
                if "output" in file:
                    with open(os.path.join(test_case_folder_path, file), 'w') as f:
                        f.write("INIT")
        if program.run(self.config.tests_folders) == 0:
            status = self.test()
        else:
            print(f"PROGRAM {program.name} FAILED TO RUN")
        print("===============================================")
        return status

    def run(self):
        result = '\n\n------------------------\n'
        for program in self.config.programs:
            res = self.test_program(program)
            result += f'{program.name}: {"OK" if res else "ERROR"}\n'
        print(result)


def main():
    config = Config.load_config("config.yaml")
    Test(config).run()


if __name__ == "__main__":
    main()
