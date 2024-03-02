import os


class Test:
    tests_folders: list[str]

    def __init__(self, tests_folder_path: str):
        self.tests_folders = []  # List of test folders
        for test_case_folder in os.listdir(tests_folder_path):
            test_case_folder_path = os.path.join(
                tests_folder_path, test_case_folder)
            if os.path.isdir(test_case_folder_path):
                self.tests_folders.append(
                    test_case_folder_path.replace("\\", "/"))

    @staticmethod
    def process_test_case(test_case_folder):
        try:
            input_file1_path = os.path.join(test_case_folder, "input_1.txt")
            input_file2_path = os.path.join(test_case_folder, "input_2.txt")
            output_file1_path = os.path.join(test_case_folder, "output_1.txt")
            output_file2_path = os.path.join(test_case_folder, "output_2.txt")

            with open(input_file1_path, 'r') as input_file1, \
                    open(input_file2_path, 'r') as input_file2, \
                    open(output_file1_path, 'r') as output_file1, \
                    open(output_file2_path, 'r') as output_file2:

                input_data1 = set(input_file1.read().strip())
                input_data2 = set(input_file2.read().strip())

                expected_output1 = ''.join(input_data1 - input_data2)
                expected_output2 = ''.join(input_data2 - input_data1)

                output1 = output_file1.read().strip()
                output2 = output_file2.read().strip()

                if output1 == expected_output1 and output2 == expected_output2:
                    print(f"TEST CASE {test_case_folder} PASSED.")
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

    def test(self):
        for test_case_folder_path in self.tests_folders:
            self.process_test_case(test_case_folder_path)

    @staticmethod
    def create_dockerfile(CMD: str):
        print("CREATING DOCKERFILE")
        with open("Dockerfile", 'w') as dockerfile:
            dockerfile.write(f"""# Берем образ с поддержкой языка С для сборки программ
FROM gcc:latest AS builder

# Устанавливаем рабочую директорию в контейнере
WORKDIR /app

ARG PROGRAM_PATH

# Копируем исходные файлы программ в контейнер
COPY $PROGRAM_PATH main.c

# Компилируем программы
RUN gcc -o main main.c

CMD {CMD}
""")

    @staticmethod
    def delete_dockerfile():
        print("DELETING DOCKERFILE")
        os.remove("Dockerfile")

    def run_program(self, program_path):
        print(f"RUNNING PROGRAM {program_path}")

        docker_compose_filename = f'docker-compose.yml'
        docker_compose_content = f"version: '3'\nservices:"

        for index, test_case_folder in enumerate(self.tests_folders, start=1):
            docker_compose_content += f"""
    test_{index}:
        build:
            context: .
            args:
                PROGRAM_PATH: {program_path}
        container_name: test_{index}
        tty: true
        volumes:
            - {test_case_folder}:/app/tests"""

        with open(docker_compose_filename, 'w') as docker_compose_file:
            docker_compose_file.write(docker_compose_content)

        print()
        result = os.system(f"docker-compose up --build")
        print()
        print(f"PROGRAM {program_path} EXITED WITH CODE {result}")
        os.remove(docker_compose_filename)
        return result

    def test_program(self, program_path):
        print(f"====================== TESTING PROGRAM {
              program_path} =========================")
        if self.run_program(program_path) == 0:
            self.test()
        else:
            print(f"PROGRAM {program_path} FAILED TO BUILD")
        print("===============================================")

    def test_programs(self, programs_folder):
        print(f"TESTING PROGRAMS IN {programs_folder}")
        for program_name in os.listdir(programs_folder):
            if program_name.endswith(".c"):
                program_path = os.path.join(
                    programs_folder, program_name).replace("\\", "/")
                self.test_program(program_path)


def main():
    Test.create_dockerfile(
        '["./main", "tests/input_1.txt", "tests/input_2.txt", "tests/output_1.txt", "tests/output_2.txt"]')
    #Test("./tests").test_programs("./programs")
    Test("./tests").test_program("./programs/5points.c")
    Test.delete_dockerfile()


if __name__ == "__main__":
    main()
