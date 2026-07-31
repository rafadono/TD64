"""
gen_maps.py — Generador determinístico de mapas y baldosas de terreno para N64

Genera las imágenes de mapa en resolución nativa N64 (320x240 px, grilla de 20x15 celdas de 16x16 px)
y los sprites individuales de terreno (tile_grass, tile_desert, tile_snow, tile_mountain, tile_lava, tile_water, tile_path).

Mapas generados:
  1. Greenfield (Tutorial Grassland — Ruta en Curva S)
  2. Desert Crossing (Oasis & Dunas — Ruta en ZigZag)
  3. Frozen Highlands (Nieve & Montañas — Ruta en Espiral)
  4. Volcanic Pass (Basalto & Lava — Ruta Recta Central)
"""

from PIL import Image, ImageDraw
import os, math

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(BASE_DIR)
OUT_DIR = os.path.join(PROJECT_DIR, "assets", "sprites")
PREVIEW_DIR = os.path.join(OUT_DIR, "previews")
os.makedirs(OUT_DIR,     exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

GRID_W, GRID_H = 20, 15
TILE_SIZE = 16
SCREEN_W, SCREEN_H = 320, 240

T   = (0,   0,   0,   0)
BLK = (0,   0,   0,   255)
WHT = (255, 255, 255, 255)

# ===========================================================================
# INDIVIDUAL TILE GENERATORS (16x16 px)
# ===========================================================================

def px(img, x, y, c):
    if 0 <= x < img.width and 0 <= y < img.height:
        img.putpixel((int(x), int(y)), c)

def rect(img, x, y, w, h, c):
    for dy in range(h):
        for dx in range(w):
            px(img, x+dx, y+dy, c)

def create_tile_grass():
    img = Image.new("RGBA", (16, 16), (45, 135, 55, 255)) # Base verde césped
    # Textura y briznas de hierba
    for y in range(16):
        for x in range(16):
            if (x * 3 + y * 7) % 5 == 0:
                px(img, x, y, (55, 155, 65, 255)) # Verde claro
            elif (x * 7 + y * 3) % 7 == 0:
                px(img, x, y, (35, 115, 45, 255)) # Verde sombra
    # Detalle de flor ocasional
    px(img, 4, 4, (240, 220, 60, 255))
    px(img, 12, 10, (230, 230, 230, 255))
    return img

def create_tile_desert():
    img = Image.new("RGBA", (16, 16), (220, 185, 110, 255)) # Sand base
    for y in range(16):
        for x in range(16):
            if (x + y // 2) % 4 == 0:
                px(img, x, y, (235, 200, 130, 255)) # Onda de duna dorada
            elif (x + y // 2) % 4 == 2:
                px(img, x, y, (195, 160, 90, 255))  # Sombra de duna
    return img

def create_tile_snow():
    img = Image.new("RGBA", (16, 16), (235, 242, 250, 255)) # Nieve base
    for y in range(16):
        for x in range(16):
            if (x * 5 + y * 3) % 6 == 0:
                px(img, x, y, (255, 255, 255, 255)) # Brillo de hielo
            elif (x * 2 + y * 4) % 7 == 0:
                px(img, x, y, (200, 215, 235, 255)) # Sombra azul helado
    return img

def create_tile_mountain():
    img = Image.new("RGBA", (16, 16), (90, 95, 105, 255)) # Roca gris base
    for y in range(16):
        for x in range(16):
            if (x + y) % 3 == 0:
                px(img, x, y, (115, 120, 130, 255)) # Grieta clara
            elif (x * 4 + y * 2) % 5 == 0:
                px(img, x, y, (65, 70, 80, 255))   # Grieta oscura
    return img

def create_tile_water():
    img = Image.new("RGBA", (16, 16), (40, 100, 200, 255)) # Azul agua
    for y in range(16):
        for x in range(16):
            if (x + (y // 2)) % 3 == 0:
                px(img, x, y, (90, 160, 240, 255)) # Espuma/onda
            elif (x * 3 + y) % 5 == 0:
                px(img, x, y, (25, 70, 150, 255))  # Agua profunda
    return img

def create_tile_lava():
    img = Image.new("RGBA", (16, 16), (40, 30, 35, 255)) # Basalto oscuro
    for y in range(16):
        for x in range(16):
            if (x * 3 + y * 2) % 4 == 0:
                px(img, x, y, (230, 50, 20, 255))  # Vena de lava roja
            elif (x * 3 + y * 2) % 4 == 1:
                px(img, x, y, (255, 150, 30, 255)) # Magma naranja
    return img

def create_tile_path():
    img = Image.new("RGBA", (16, 16), (150, 115, 80, 255)) # Camino de tierra
    for y in range(16):
        for x in range(16):
            if (x * 2 + y * 5) % 4 == 0:
                px(img, x, y, (175, 140, 100, 255)) # Grava clara
            elif (x * 3 + y * 2) % 5 == 0:
                px(img, x, y, (120, 90, 60, 255))   # Grava oscura
    return img

# ===========================================================================
# MAP DRAWING FUNCTIONS (320x240 px)
# ===========================================================================

def draw_path_line(img, points, color=(160, 125, 85, 255), width=18):
    """Dibuja el camino continuado entre waypoints con borde pixelado"""
    draw = ImageDraw.Draw(img)
    for i in range(len(points) - 1):
        p1 = points[i]
        p2 = points[i+1]
        draw.line([p1, p2], fill=color, width=width)
    # Bordes del camino para realzar contraste N64
    for i in range(len(points) - 1):
        p1 = points[i]
        p2 = points[i+1]
        draw.line([p1, p2], fill=(90, 65, 40, 255), width=width+4)
        draw.line([p1, p2], fill=color, width=width)

def draw_waypoints_indicators(img, points):
    """Dibuja indicadores de inicio (verde), waypoints y final (rojo)"""
    draw = ImageDraw.Draw(img)
    for idx, (px_x, px_y) in enumerate(points):
        if idx == 0:
            color = (50, 220, 50, 255) # Start Green
            r = 6
        elif idx == len(points) - 1:
            color = (230, 40, 40, 255) # End Red
            r = 6
        else:
            color = (255, 215, 0, 220) # Waypoint Gold
            r = 3
        draw.ellipse([px_x - r, px_y - r, px_x + r, px_y + r], fill=color, outline=(0, 0, 0, 255))

def generate_map_greenfield(tiles):
    img = Image.new("RGBA", (SCREEN_W, SCREEN_H))
    # Fill base with grass tiles
    for gy in range(GRID_H):
        for gx in range(GRID_W):
            img.paste(tiles["grass"], (gx * TILE_SIZE, gy * TILE_SIZE))
    
    # Greenfield S-Curve path waypoints
    pts = [(-20, 60), (80, 60), (120, 120), (200, 120), (240, 180), (340, 180)]
    draw_path_line(img, pts, color=(160, 125, 85, 255), width=20)
    draw_waypoints_indicators(img, pts[1:-1])
    return img

def generate_map_desert(tiles):
    img = Image.new("RGBA", (SCREEN_W, SCREEN_H))
    # Base sand
    for gy in range(GRID_H):
        for gx in range(GRID_W):
            img.paste(tiles["desert"], (gx * TILE_SIZE, gy * TILE_SIZE))
    
    # Oasis pools
    for gy in range(4, 7):
        for gx in range(8, 12):
            img.paste(tiles["water"], (gx * TILE_SIZE, gy * TILE_SIZE))
    for gy in range(9, 12):
        for gx in range(14, 18):
            img.paste(tiles["water"], (gx * TILE_SIZE, gy * TILE_SIZE))

    # Desert Zigzag path
    pts = [(-20, 50), (100, 50), (100, 190), (200, 190), (200, 80), (280, 80), (280, 160), (340, 160)]
    draw_path_line(img, pts, color=(200, 160, 100, 255), width=18)
    draw_waypoints_indicators(img, pts[1:-1])
    return img

def generate_map_frozen(tiles):
    img = Image.new("RGBA", (SCREEN_W, SCREEN_H))
    # Snow & mountain checkerboard
    for gy in range(GRID_H):
        for gx in range(GRID_W):
            t = tiles["snow"] if (gx + gy) % 2 == 0 else tiles["mountain"]
            img.paste(t, (gx * TILE_SIZE, gy * TILE_SIZE))
    
    # Spiral path points
    spiral_pts = 9
    cx, cy, r = 160, 120, 90
    pts = [(-20, cy)]
    for i in range(spiral_pts):
        t = i / (spiral_pts - 1)
        angle = t * math.pi * 3.0
        radius = r * (1.0 - t * 0.7)
        pts.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius))
    pts.append((340, cy))

    draw_path_line(img, pts, color=(140, 180, 210, 255), width=16)
    draw_waypoints_indicators(img, pts[1:-1])
    return img

def generate_map_volcanic(tiles):
    img = Image.new("RGBA", (SCREEN_W, SCREEN_H))
    # Basalt base
    for gy in range(GRID_H):
        for gx in range(GRID_W):
            img.paste(tiles["mountain"], (gx * TILE_SIZE, gy * TILE_SIZE))
    
    # Lava rivers
    for gx in range(5, 15): img.paste(tiles["lava"], (gx * TILE_SIZE, 4 * TILE_SIZE))
    for gx in range(8, 18): img.paste(tiles["lava"], (gx * TILE_SIZE, 10 * TILE_SIZE))
    for gx in range(3, 12): img.paste(tiles["lava"], (gx * TILE_SIZE, 13 * TILE_SIZE))

    # Straight pass
    pts = [(-20, 120), (100, 120), (220, 120), (340, 120)]
    draw_path_line(img, pts, color=(80, 70, 75, 255), width=22)
    draw_waypoints_indicators(img, pts[1:-1])
    return img

# ===========================================================================
# EXECUTION
# ===========================================================================

def save_image(img, name):
    p = os.path.join(OUT_DIR, f"{name}.png")
    prev_p = os.path.join(PREVIEW_DIR, f"{name}_preview.png")
    img.save(p)
    scale = 2 if img.width == SCREEN_W else 4
    prev = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
    prev.save(prev_p)
    print(f"  [OK] {name}.png ({img.width}x{img.height})")

if __name__ == "__main__":
    print("Generando baldosas y mapas determinísticos de juego para N64...\n")

    # 1. Tiles individuales
    tiles = {
        "grass": create_tile_grass(),
        "desert": create_tile_desert(),
        "snow": create_tile_snow(),
        "mountain": create_tile_mountain(),
        "water": create_tile_water(),
        "lava": create_tile_lava(),
        "path": create_tile_path()
    }

    for name, t_img in tiles.items():
        save_image(t_img, f"tile_{name}")

    print("\nGenerando mapas de juego (320x240 px grilla N64)...")
    save_image(generate_map_greenfield(tiles), "map_greenfield")
    save_image(generate_map_desert(tiles), "map_desert")
    save_image(generate_map_frozen(tiles), "map_frozen")
    save_image(generate_map_volcanic(tiles), "map_volcanic")

    print("\n¡Completado! Mapas y baldosas listos para el compilador N64 mksprite.")
