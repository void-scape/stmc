import sys
import matplotlib.pyplot as plt

name = sys.argv[1] if len(sys.argv) > 1 else "viridis"
n = 128

cmap = plt.get_cmap(name)
colors = []
for i in range(n):
    r, g, b, _ = cmap(i / (n - 1))
    r = int(r * 31 + 0.5) & 0x1F
    g = int(g * 63 + 0.5) & 0x3F
    b = int(b * 31 + 0.5) & 0x1F
    colors.append((r << 11) | (g << 5) | b)

print(f"static const uint16_t palette[{n}] = {{")
for i in range(0, n, 8):
    row = ", ".join(f"0x{v:04X}" for v in colors[i : i + 8])
    print(f"    {row},")
print("};")
