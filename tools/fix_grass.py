from PIL import Image

# Load grass_top (for green color reference) and dirt
grass_top = Image.open('assets/textures/grass_top.png').convert('RGBA')
dirt = Image.open('assets/textures/dirt.png').convert('RGBA')

print(f"grass_top: {grass_top.size}")
print(f"dirt: {dirt.size}")

# Use dirt's resolution as target
size = max(dirt.size[0], dirt.size[1])
print(f"Target size: {size}x{size}")

# Upscale grass_top to target size (nearest-neighbor for pixel-art look)
gt = grass_top.resize((size, size), Image.NEAREST)

# Ensure dirt is the right size
dt = dirt.resize((size, size), Image.NEAREST)

# Build grass_side: green top portion, dirt bottom
# Grass covers roughly top 1/4 with a transition zone
grass_rows = size // 4          # pure grass rows
transition_rows = size // 16    # transition zone

result = Image.new('RGBA', (size, size))

for y in range(size):
    for x in range(size):
        dr, dg, db, da = dt.getpixel((x, y))
        gr, gg, gb, ga = gt.getpixel((x, y))

        if y < grass_rows:
            result.putpixel((x, y), (gr, gg, gb, 255))
        elif y < grass_rows + transition_rows:
            # Smooth transition
            t = 1.0 - (y - grass_rows) / float(transition_rows)
            r = int(gr * t + dr * (1 - t))
            g = int(gg * t + dg * (1 - t))
            b = int(gb * t + db * (1 - t))
            result.putpixel((x, y), (r, g, b, 255))
        else:
            result.putpixel((x, y), (dr, dg, db, 255))

result.save('assets/textures/grass_side.png')
print(f"Created grass_side.png at {size}x{size}")

# Verify
v = Image.open('assets/textures/grass_side.png')
print(f"Verify: {v.size} {v.mode}")
for row in [0, grass_rows-1, grass_rows, grass_rows+transition_rows, size-1]:
    print(f"  row {row}: {v.getpixel((size//2, row))}")
