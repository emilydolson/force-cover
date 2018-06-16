import sys

def validate_line(line, second_bar):

    if second_bar == -1: # This isn't a valid code coverage line
        return False 

    line = line[second_bar+1:].strip()
    line = line.replace("/*_FORCE_COVER_START_*/", "")
    line = line.replace("/*_FORCE_COVER_END_*/", "")
    if line == "":
        return False
    if line.startswith("//"):
        return False
    if line.startswith("/*"):
        return False
    if line.startswith("*"):
        return False
    if line == "{":
        return False
    if line == "}":
        return False
    return True

def is_first_line(line):
    return line.split("|")[0].strip() == "1"

lines = []
with open(sys.argv[1]) as infile:
    force_cover_active = 0
    
    for line in infile:
        if is_first_line(line):
            #Zero out regions at beginning of new file just in case stuff got screwed up
            force_cover_active = 0

        force_cover_active += line.count("_FORCE_COVER_START_")

        if not force_cover_active:
            lines.append(line)
            continue

        if force_cover_active:
            first_bar = line.find("|")
            second_bar = line.find("|", first_bar+1)
            
            if validate_line(line, second_bar) and line[second_bar-1].strip() == "":
                lines.append("".join([line[:second_bar-1], "0", line[second_bar:]]))
            else:
                lines.append(line)
        else:
            lines.append(line)

        force_cover_active -= line.count("_FORCE_COVER_END_")

with open(sys.argv[1], "w") as infile:
    infile.writelines(lines)