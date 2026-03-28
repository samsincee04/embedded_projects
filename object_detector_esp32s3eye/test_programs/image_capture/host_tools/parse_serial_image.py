from pathlib import Path
import re
import sys

if len(sys.argv) != 3:
    print("Usage: python3 parse_serial_image.py input_log.txt output.jpg")
    sys.exit(1)

input_file = Path(sys.argv[1])
output_file = Path(sys.argv[2])

text = input_file.read_text(errors="ignore")

match = re.search(r"IMG_START\s+\d+\n(.*?)\nIMG_END", text, re.DOTALL)
if not match:
    raise ValueError("Could not find IMG_START ... IMG_END block in log file")

data_block = match.group(1).strip()
values = [int(x) for x in data_block.split(",") if x.strip()]

jpeg_bytes = bytes(values)

with open(output_file, "wb") as f:
    f.write(jpeg_bytes)

print(f"Wrote {output_file} ({len(jpeg_bytes)} bytes)")
