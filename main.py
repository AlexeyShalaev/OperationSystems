import yaml


def read_config(file_path):
    with open(file_path, 'r') as file:
        config = yaml.safe_load(file)
    return config


def process_programs(config):
    programs = config.get('programs', {})
    for program_name, program_data in programs.items():
        print(f"Program: {program_name}")
        for file_name, args in program_data.items():
            print(f"  File: {file_name}")
            if args:
                print("    Args:")
                for arg in args:
                    print(f"      - {arg}")


if __name__ == "__main__":
    file_path = "config.yaml"  # Path to your YAML file
    config = read_config(file_path)
    process_programs(config)
