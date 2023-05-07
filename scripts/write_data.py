import struct

# Open the input file in text mode
with open('workload.txt', 'r') as f_in:

    # Read the number-value pairs from the input file and convert them to hex
    lines = f_in.read().splitlines()
    hex_values = []
    for line in lines:
        values = line.split()
        if not values[0].isdigit():  # discard the first value if not an integer
            values = values[1:]
        number, value = values
        int_number = int(number)
        if int_number < 0:
            hex_number = format((1 << 32) + int_number, '08x')
        else:
            hex_number = format(struct.unpack('<i', struct.pack('<i', int_number))[0], '08x')
        int_value = int(value)
        if int_value < 0:
            hex_value = format((1 << 32) + int_value, '08x')
        else:
            hex_value = format(struct.unpack('<i', struct.pack('<i', int_value))[0], '08x')
        hex_values.append(hex_number)
        hex_values.append(hex_value)

# Open the output file in binary mode and write the hex values to it
with open('output_file.bin', 'wb') as f_out:

    # Write each hex value to every 4 bytes of the output file
    for hex_value in hex_values:
        try:
            byte_value = bytes.fromhex(hex_value)
            f_out.write(byte_value[::-1])  # reverse byte order for little-endian
        except ValueError:
            if hex_value.startswith('ffffff'):  # handle negative numbers
                byte_value = struct.pack('<i', int(hex_value, 16) - (1 << 32))
            else:
                byte_value = struct.pack('<i', int(hex_value, 16))
            f_out.write(byte_value[::-1])