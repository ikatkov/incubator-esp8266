import subprocess
from datetime import datetime
from string import Template

fileName = "include/build_version.h"

template = """
#ifndef BUILD_VERSION_H
#define BUILD_VERSION_H

#define GIT_REVISION "$git_version"
#define BUILD_TIMESTAMP "$build_timestamp"

#endif
"""


def update_template_header(git_version, build_timestamp):
    with open(fileName, "w") as file:
        src = Template(template)
        content = src.substitute(
            {"git_version": git_version, "build_timestamp": build_timestamp}
        )
        file.write(content)


revision = (
    subprocess.check_output(["git", "describe", "--tags", "--always"])
    .strip()
    .decode("utf-8")
)

timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")


# Update template header file
update_template_header(revision, timestamp)

# Print a message indicating that the update was successful
print(fileName, " has been updated successfully.")

# flags
print("GIT_REV='\"%s\"'" % revision)
print("BUILD_TIME='\"%s\"'" % timestamp)
