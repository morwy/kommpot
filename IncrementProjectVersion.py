#!/usr/bin/env python

import argparse
import os
import sys
from typing import List


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        dest="action",
        type=str,
        nargs="?",
        choices=[
            "NONE",
            "INCREMENT_VERSION",
            "GET_VERSION",
        ],
        default="NONE",
        const="NONE",
    )

    args = parser.parse_args()

    if args.action == "NONE":
        sys.exit(0)

    elif args.action == "INCREMENT_VERSION":
        current_working_directory = os.getcwd()
        version_filepath = os.path.join(current_working_directory, "ProjectVersion.cmake")

        version_file_lines: List[str] = []
        with open(file=version_filepath, mode="r", encoding="utf-8") as version_file:
            version_file_lines = version_file.readlines()

        for line_index, line in enumerate(version_file_lines):
            if line.startswith("set(PROJECT_VERSION_MICRO"):
                version_micro = int(line.replace("set(PROJECT_VERSION_MICRO", "").replace(")", "").strip())
                incremented_version_micro = version_micro + 1
                version_file_lines[line_index] = f"set(PROJECT_VERSION_MICRO {incremented_version_micro})\n"

        with open(file=version_filepath, mode="w", encoding="utf-8") as version_file:
            version_file.writelines(version_file_lines)

    elif args.action == "GET_VERSION":
        current_working_directory = os.getcwd()
        version_filepath = os.path.join(current_working_directory, "ProjectVersion.cmake")

        version_file_lines: List[str] = []
        with open(file=version_filepath, mode="r", encoding="utf-8") as version_file:
            version_file_lines = version_file.readlines()

        version_major = 0
        version_minor = 0
        version_micro = 0
        version_nano = 0

        for line_index, line in enumerate(version_file_lines):
            if line.startswith("set(PROJECT_VERSION_MAJOR"):
                version_major = int(line.replace("set(PROJECT_VERSION_MAJOR", "").replace(")", "").strip())

            elif line.startswith("set(PROJECT_VERSION_MINOR"):
                version_minor = int(line.replace("set(PROJECT_VERSION_MINOR", "").replace(")", "").strip())

            elif line.startswith("set(PROJECT_VERSION_MICRO"):
                version_micro = int(line.replace("set(PROJECT_VERSION_MICRO", "").replace(")", "").strip())

            elif line.startswith("set(PROJECT_VERSION_NANO"):
                version_nano = int(line.replace("set(PROJECT_VERSION_NANO", "").replace(")", "").strip())

        if version_nano != 0:
            print(f"{version_major}.{version_minor}.{version_micro}.{version_nano}")
        else:
            print(f"{version_major}.{version_minor}.{version_micro}")
