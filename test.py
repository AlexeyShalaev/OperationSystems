from dataclasses import dataclass
from multiprocessing import Process
import os

import yaml


class Program:
    class Unit:
        def __init__(self, file_path: str, args: list[str]):
            self.file_path = file_path
            self.args = args if args else []

        def __str__(self):
            return f"Unit: {self.file_path}\nArgs: {self.args}\n"

        def run(self, test_folder):
            cmd = f"./{self.file_path.replace('.c', '')} {' '.join([os.path.join(test_folder, arg) for arg in self.args])}"
            print(f"Running {cmd}")
            os.system(cmd)

    def __init__(self, name: str, units: list[Unit]):
        self.name = name
        self.units = units

    def __str__(self):
        units = '\n'.join([str(unit) for unit in self.units])
        return f"Program: {self.name}\n{units}\n\n"

    def build(self):
        res = 0
        for unit in self.units:
            res += os.system(
                f"gcc -o {unit.file_path.replace('.c', '')} {unit.file_path}")

        return res

    def run(self, test_folder):
        # Assuming self.units is a list of units
        processes = []
        for unit in self.units:
            process = Process(target=unit.run, args=(test_folder, ))
            process.start()
            processes.append(process)

        # Wait for all processes to finish
        for process in processes:
            process.join()


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
        if program.build():
            print(f"PROGRAM {program.name} FAILED TO BUILD")
            return False
        status = False
        print(
            f"====================== TESTING PROGRAM {program.name} ========================="
        )
        for test_case_folder_path in self.config.tests_folders:
            for file in os.listdir(test_case_folder_path):
                if "output" in file:
                    with open(os.path.join(test_case_folder_path, file), 'w') as f:
                        f.write("INIT")
            if program.run(test_case_folder_path) == 0:
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
    import time
    i = 0
    while True:
        time.sleep(1)
        i += 1
        if i == 10:
            break


if __name__ == "__main__":
    main()
