from PIL import Image

def image_to_bitmap_cpp(image_path, variable_name="bitmap"):
    img = Image.open(image_path).convert("RGB")

    if img.size != (200, 64):
        raise ValueError("Bild muss genau 200x64 Pixel groß sein")

    bitmap = [[0 for _ in range(16)] for _ in range(200)]

    for x in range(200):
        for y in range(16):

            value = 0

            rows = [y, y + 16, y + 32, y + 48]

            for group, py in enumerate(rows):
                r, g, b = img.getpixel((x, py))

                red = r > 128
                green = g > 128

                if group == 0:
                    if red:
                        value |= (1 << 7)
                    if green:
                        value |= (1 << 6)

                elif group == 1:
                    if red:
                        value |= (1 << 5)
                    if green:
                        value |= (1 << 4)

                elif group == 2:
                    if red:
                        value |= (1 << 3)
                    if green:
                        value |= (1 << 2)

                elif group == 3:
                    if red:
                        value |= (1 << 1)
                    if green:
                        value |= (1 << 0)

            bitmap[x][y] = value

    cpp = f"const uint8_t {variable_name}[200][16] PROGMEM = {{\n"

    for x in range(200):
        cpp += "    {"
        cpp += ", ".join(f"0x{bitmap[x][y]:02X}" for y in range(16))
        cpp += "}"

        if x < 199:
            cpp += ","

        cpp += "\n"

    cpp += "};"

    return cpp


# Beispiel
cpp_code = image_to_bitmap_cpp("bild.png", "imageData")

with open("imageData.h", "w") as f:
    f.write("#include <avr/pgmspace.h>\n")
    f.write(cpp_code)

print("C++ Header erzeugt: imageData.h")