import re

opcode = {}

with open("Network/include/protocol/opcode.h") as f:
    for line in f:
        m = re.match(r"#define\s+(\w+)\s+(0x[0-9A-Fa-f]+)", line)
        if m:
            opcode[m.group(1)] = int(m.group(2), 16)

with open("Frontend/src/network/opcode.js", "w") as f:
    f.write("export const OPCODE = {\n")
    for k, v in opcode.items():
        f.write(f"  {k}: 0x{v:04X},\n")
    f.write("};\n")
