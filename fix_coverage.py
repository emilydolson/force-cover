#!/usr/bin/env python

# Author: Emily Dolson
# Copyright: 2018
# License: MIT

"""
fix_coverage.py is a script for modifying C++ coverage reports to
mark uninstantiated templates as executable regions missing coverage.

Usage: ./fix_coverage.py [coverage_report_file]

Assumes that coverage file has the format:
[source_line_number] | [execution_count]| [code]

./force_coverage must have been run on the source code before it was
compiled in order for this to work.
"""

import sys


def validate_line(line, second_bar):
    """
    Returns true if this line contains code that should be executre.
    Arguments: line - a string containing the full text of this line
               second_bar - an int indicating the index in the string
                            at which the second "|" character occurs,
                            denoting the start of the code portion.
    """

    if second_bar == -1:  # This isn't a valid code coverage line
        return False

    line = line[second_bar+1:].strip()  # Get the code part of the line
    line = line.replace("/*_FORCE_COVER_START_*/", "")  # Don't muck up line
    line = line.replace("/*_FORCE_COVER_END_*/", "")    # with our comments

    if line == "":              # Don't report empty lines as uncovered
        return False
    if line.startswith("//"):   # Don't report commented lines as uncovered
        return False
    if line.startswith("/*"):   # Don't report commented lines as uncovered
        return False            # TODO: There could still be code on this line
    if line.startswith("*"):    # * often indicats line is part of a comment
        return False
    if line == "{":             # A single brace can't be executed
        return False
    if line == "}":
        return False

    # Have another rule for skipping lines? Add it here!

    return True


def is_first_line(line):
    """
    Returns true if line is the first line of a new file
    """
    return line.split("|")[0].strip() == "1"


def main():

    if len(sys.argv) < 2:
        print("Usage: python fix_coverage.py [name_of_coverage_report_file]")
        exit(1)

    lines = []  # Accumulate list of lines to glue back together at the end
    with open(sys.argv[1]) as infile:
        force_cover_active = 0  # Counter of open nested templates

        for line in infile:
            if is_first_line(line):
                # Zero out regions at beginning of new file just
                # in case stuff got screwed up
                force_cover_active = 0

            force_cover_active += line.count("_FORCE_COVER_START_")

            if not force_cover_active:  # Don't need to change line because
                lines.append(line)      # we aren't in a template definition
                continue

            if force_cover_active:  # In template. Might need to do stuff.
                first_bar = line.find("|")
                second_bar = line.find("|", first_bar+1)

                if validate_line(line, second_bar) and \
                        line[second_bar-1].strip() == "":
                    # If this line could have been executed but wasn't (no
                    # number between first and second bars), put a zero
                    # before the second bar, indicating that it was
                    # executed zero times. Test coverage viewers will interpret
                    # this as meaning the line should have been covered
                    # but wasn't.
                    lines.append("".join([line[:second_bar-1],
                                 "0", line[second_bar:]]))
                else:
                    # There's already an execution count - this
                    # template must have been instantiated
                    lines.append(line)

            # Closing force_cover comments happens at the end of the line
            force_cover_active -= line.count("_FORCE_COVER_END_")

    # Rewrite file with modified execution counts
    with open(sys.argv[1], "w") as infile:
        infile.writelines(lines)

if __name__ == "__main__":
    main()
