import re

opcode = {}

with open("Network/include/protocol/opcode.h") as f:
    for line in f:
        # Match both hex (0x1234) and decimal (200) numbers
        m = re.match(r"#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)", line)
        if m:
            name = m.group(1)
            value_str = m.group(2)
            
            # Parse hex or decimal
            if value_str.startswith('0x'):
                opcode[name] = int(value_str, 16)
            else:
                opcode[name] = int(value_str, 10)

with open("Frontend/src/network/opcode.js", "w") as f:
    f.write("export const OPCODE = {\n")
    
    # Group by prefix for better organization
    categories = {
        'CMD_': '  // Command Codes\n',
        'RES_': '\n  // Response Codes\n',
        'ERR_': '\n  // Error Codes\n',
        'NTF_': '\n  // Notification Codes\n',
    }
    
    last_prefix = None
    for k, v in sorted(opcode.items()):
        # Detect category change and add comment
        prefix = k.split('_')[0] + '_'
        if prefix in categories and prefix != last_prefix:
            f.write(categories[prefix])
            last_prefix = prefix
        
        # Write opcode (use hex for CMD, decimal for others)
        if k.startswith('CMD_'):
            f.write(f"  {k}: 0x{v:04X},\n")
        else:
            f.write(f"  {k}: {v},\n")
    
    f.write("};\n")
