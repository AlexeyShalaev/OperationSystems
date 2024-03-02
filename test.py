import os


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
                print(f"Test case {test_case_folder} PASSED.")
            else:
                print(f"Test case {test_case_folder} FAILED.")

                if output1 != expected_output1:
                    print(f"Output 1 mismatch")
                    print(f"Expected : {expected_output1}")
                    print(f"Actual: {output1}")

                print()

                if output2 != expected_output2:
                    print(f"Output 2 mismatch")
                    print(f"Expected : {expected_output2}")
                    print(f"Actual: {output2}")

                print()
    except Exception as e:
        print(f"Test case {test_case_folder} FAILED.")
        print(f"Error: {e}")
        print()


def main():
    tests_folder = "tests"

    for test_case_folder in os.listdir(tests_folder):
        test_case_folder_path = os.path.join(tests_folder, test_case_folder)
        if os.path.isdir(test_case_folder_path):
            process_test_case(test_case_folder_path)


if __name__ == "__main__":
    main()
