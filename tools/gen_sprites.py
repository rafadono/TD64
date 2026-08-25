"""
gen_sprites.py — Generator determinístico de sprites de alta calidad para N64

Genera 28 sprites únicos en 32x32 y 16x16 px con sombreado 3D de alta fidelidad,
siluetas específicas por facción y detalles visuales distintivos:
  4 Facciones × 6 Tipos de Unidad = 24 Unidades
  4 Proyectiles (uno por facción con efectos visuales únicos)

Paletas y Facciones:
  DAWNGUARD  — Azul Cobalto / Dorado / Blanco (Caballeros Sagrados)
  IRONBONE   — Morado Oscuro / Verde Tóxico / Negro (No-muertos)
  ASHCLAW    — Rojo Sangre / Naranja Fuego / Marrón Cuero (Salvajes)
  VEILSTORM  — Cian / Violeta / Blanco Eléctrico (Magos Arcanos)
"""

from PIL import Image
import os, math

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(BASE_DIR)
OUT_DIR = os.path.join(PROJECT_DIR, "assets", "sprites")
PREVIEW_DIR = os.path.join(OUT_DIR, "previews")
os.makedirs(OUT_DIR,     exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

T   = (0,   0,   0,   0)    # Transparent
BLK = (0,   0,   0,   255)  # Outline black
WHT = (255, 255, 255, 255)

# ===========================================================================
# FACTION PALETTES & DEFINITIONS
# ===========================================================================

PALETTES = {
    "dawnguard": {
        "primary":   (50,  100, 210, 255),  # Cobalt Blue
        "secondary": (210, 175, 40,  255),  # Shiny Gold
        "light":     (140, 185, 255, 255),  # Specular Blue
        "dark":      (25,  50,  110, 255),  # Shadow Blue
        "accent":    (255, 245, 130, 255),  # Holy Gold Glow
        "skin":      (235, 195, 160, 255),  # Human skin
        "eye":       (90,  210, 255, 255),  # Blue eyes
        "proj":      (255, 235, 70,  255),  # Golden Holy Bolt
    },
    "ironbone": {
        "primary":   (100, 25,  140, 255),  # Dark Purple
        "secondary": (50,  190, 50,  255),  # Toxic Green
        "light":     (160, 75,  200, 255),  # Light Purple
        "dark":      (40,  10,  60,  255),  # Deep Shadow Purple
        "accent":    (110, 255, 110, 255),  # Bright Toxic Glow
        "skin":      (175, 195, 175, 255),  # Pale Bone / Undead skin
        "eye":       (0,   255, 90,  255),  # Green glow eyes
        "proj":      (80,  255, 80,  255),  # Toxic Necro Skull
    },
    "ashclaw": {
        "primary":   (200, 50,  25,  255),  # Blood Red
        "secondary": (130, 70,  25,  255),  # Leather Brown
        "light":     (255, 120, 70,  255),  # Bright Orange Shading
        "dark":      (100, 20,  10,  255),  # Dark Crimson Shadow
        "accent":    (255, 190, 40,  255),  # Fire Yellow Accent
        "skin":      (135, 95,  50,  255),  # Orc Bronze Skin
        "eye":       (255, 70,  0,   255),  # Burning Red Eyes
        "proj":      (255, 110, 20,  255),  # Fiery Spear/Ball
    },
    "veilstorm": {
        "primary":   (35,  185, 220, 255),  # Electric Cyan
        "secondary": (150, 70,  235, 255),  # Deep Violet
        "light":     (140, 230, 250, 255),  # Plasma Light Cyan
        "dark":      (15,  85,  125, 255),  # Shadow Cyan
        "accent":    (210, 130, 255, 255),  # Arcane Violet Spark
        "skin":      (195, 205, 230, 255),  # Pale Elven skin
        "eye":       (190, 90,  255, 255),  # Purple Glow Eyes
        "proj":      (90,  215, 255, 255),  # Arcane Spark
    },
}

FACTION_ORDER = ["dawnguard", "ironbone", "ashclaw", "veilstorm"]

# ===========================================================================
# PIXEL HELPERS & SHADING ALGORITHMS
# ===========================================================================

def img32(): return Image.new("RGBA", (32, 32), T)
def img16(): return Image.new("RGBA", (16, 16), T)

def px(img, x, y, c):
    if 0 <= x < img.width and 0 <= y < img.height:
        img.putpixel((int(x), int(y)), c)

def rect(img, x, y, w, h, c):
    for dy in range(h):
        for dx in range(w):
            px(img, x+dx, y+dy, c)

def shaded_rect(img, x, y, w, h, base, light, dark):
    """Rectángulo con sombreado 3D (Luz desde arriba-izquierda)"""
    rect(img, x, y, w, h, base)
    # Borde superior e izquierdo (luz)
    for dx in range(w): px(img, x+dx, y, light)
    for dy in range(h): px(img, x, y+dy, light)
    # Borde inferior y derecho (sombra)
    for dx in range(w): px(img, x+dx, y+h-1, dark)
    for dy in range(h): px(img, x+w-1, y+dy, dark)

def hrect(img, x, y, w, h, fill, border=BLK):
    rect(img, x, y, w, h, fill)
    for dx in range(w):
        px(img, x+dx, y,   border)
        px(img, x+dx, y+h-1, border)
    for dy in range(h):
        px(img, x, y+dy, border)
        px(img, x+w-1, y+dy, border)

def circle(img, cx, cy, r, c):
    for dy in range(-r, r+1):
        for dx in range(-r, r+1):
            if dx*dx + dy*dy <= r*r:
                px(img, cx+dx, cy+dy, c)

def outline(img, border=BLK):
    """Borde exterior limpio de 1px"""
    w, h = img.size
    data = [img.getpixel((x,y)) for y in range(h) for x in range(w)]
    def is_solid(x, y):
        if 0<=x<w and 0<=y<h:
            return data[y*w+x][3] > 0
        return False
    for y in range(h):
        for x in range(w):
            if data[y*w+x][3] > 0:
                for nx,ny in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
                    if not is_solid(nx,ny):
                        px(img, nx, ny, border)

def save(img, filename, preview_scale=4):
    p = f"{OUT_DIR}/{filename}.png"
    img.save(p)
    pw, ph = img.width * preview_scale, img.height * preview_scale
    prev = img.resize((pw, ph), Image.NEAREST)
    prev.save(f"{PREVIEW_DIR}/{filename}_preview.png")
    print(f"  [OK] {filename}.png")

# ===========================================================================
# SPRITE SHEET LAYOUT (debe coincidir con AnimDef en src/entities/entities.c)
# ===========================================================================
#
# Cada unidad se genera como un sheet animado de 8 frames en tira horizontal:
#   frame 0     -> idle   (pose estatica, identica a como era antes)
#   frames 1-4  -> walk   (ciclo de caminata, piernas alternando)
#   frames 5-7  -> attack (ciclo de ataque: golpe atras, golpe, retorno)
#
# En vez de dibujar 8 poses a mano por unidad, cada dibujante desplaza unos
# pocos pixeles las piernas (caminata) o el arma principal (ataque) sobre la
# MISMA geometria ya definida abajo — sigue siendo 100% deterministico/por
# codigo, solo que ahora parametrizado por frame.

FRAME_W       = 32
FRAME_H       = 32
FRAMES_TOTAL  = 8
IDLE_FRAME    = 0
WALK_FRAMES   = (1, 2, 3, 4)
ATTACK_FRAMES = (5, 6, 7)

def anim_offsets(frame):
    """Devuelve (leg1_dx, leg1_dy, leg2_dx, leg2_dy, weapon_dx, weapon_dy)
    para el frame dado (0..7). Frame 0 (idle) siempre es todo-cero, o sea
    identico a la pose original."""
    if frame == IDLE_FRAME:
        return (0, 0, 0, 0, 0, 0)

    if frame in WALK_FRAMES:
        walk_pattern = [
            ( 1, -1, -1,  0),
            ( 0,  0,  0, -1),
            (-1, -1,  1,  0),
            ( 0,  0,  0, -1),
        ]
        l1dx, l1dy, l2dx, l2dy = walk_pattern[WALK_FRAMES.index(frame)]
        return (l1dx, l1dy, l2dx, l2dy, 0, 0)

    atk_pattern = [(-1, 0), (1, 1), (0, 1)]  # windup, golpe, retorno — offsets chicos para no despegar el arma del cuerpo
    wdx, wdy = atk_pattern[ATTACK_FRAMES.index(frame)]
    return (0, 0, 0, 0, wdx, wdy)

# ===========================================================================
# FACTION-SPECIFIC UNIT DRAWERS
# ===========================================================================

def draw_scout(p, faction, frame=0):
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Piernas en pose de carrera
    rect(img, 10+l1dx, 22+l1dy, 4, 6, p["dark"])
    rect(img, 17+l2dx, 24+l2dy, 4, 4, p["dark"])
    px(img, 9+l1dx, 27+l1dy, p["secondary"])  # bota/garra
    px(img, 20+l2dx, 27+l2dy, p["secondary"])

    # Torso
    shaded_rect(img, 11, 13, 10, 9, p["primary"], p["light"], p["dark"])

    # Cabeza según facción
    if faction == "dawnguard":
        # Casco ligero con pluma dorada
        shaded_rect(img, 12, 6, 8, 7, p["secondary"], p["accent"], p["dark"])
        px(img, 15, 4, p["accent"]); px(img, 15, 5, p["accent"])
        hrect(img, 14, 8, 2, 2, p["eye"])
    elif faction == "ironbone":
        # Shade envuelto en capucha de sombras
        shaded_rect(img, 11, 5, 10, 8, p["dark"], p["primary"], BLK)
        circle(img, 14, 8, 1, p["eye"])
        circle(img, 18, 8, 1, p["eye"])
        px(img, 14, 8, WHT); px(img, 18, 8, WHT)
    elif faction == "ashclaw":
        # Runner feroz con piel bronce y pinturas de guerra
        shaded_rect(img, 12, 6, 8, 7, p["skin"], p["light"], p["dark"])
        px(img, 13, 9, p["primary"]); px(img, 17, 9, p["primary"]) # warpaint
        rect(img, 11, 5, 10, 2, p["secondary"]) # vincha
    else: # veilstorm
        # Wisp/Maguito flotante con cola de energía
        circle(img, 16, 9, 4, p["light"])
        circle(img, 16, 9, 2, WHT)
        px(img, 15, 8, p["accent"]); px(img, 17, 8, p["accent"])

    # Armas / Detalles frontales — se animan con el ciclo de ataque
    if faction == "dawnguard":
        # Daga dorada brillante
        rect(img, 22+wdx, 14+wdy, 5, 2, p["secondary"])
        rect(img, 26+wdx, 12+wdy, 2, 6, p["accent"])
    elif faction == "ironbone":
        # Garra espectral verde
        rect(img, 22+wdx, 15+wdy, 4, 2, p["secondary"])
        px(img, 25+wdx, 14+wdy, p["accent"]); px(img, 26+wdx, 16+wdy, p["accent"])
    elif faction == "ashclaw":
        # Cuchillo de hueso
        rect(img, 22+wdx, 15+wdy, 5, 2, p["secondary"])
        px(img, 26+wdx, 14+wdy, WHT)
    else:
        # Daga de cristal violeta
        rect(img, 22+wdx, 14+wdy, 5, 2, p["secondary"])
        px(img, 26+wdx, 13+wdy, p["accent"])

    outline(img)
    return img

def draw_warrior(p, faction, frame=0):
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Piernas armadas
    shaded_rect(img, 9+l1dx, 22+l1dy, 5, 7, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 18+l2dx, 22+l2dy, 5, 7, p["primary"], p["light"], p["dark"])

    # Torso masivo
    shaded_rect(img, 8, 11, 16, 11, p["primary"], p["light"], p["dark"])

    # Emblema / Pechera según facción
    if faction == "dawnguard":
        # Cruz dorada de Templario
        rect(img, 15, 13, 2, 7, p["secondary"])
        rect(img, 13, 15, 6, 2, p["secondary"])
        px(img, 15, 15, p["accent"])
        # Escudo con emblema (estatico)
        shaded_rect(img, 3, 12, 6, 9, p["secondary"], p["accent"], p["dark"])
        rect(img, 5, 14, 2, 5, p["primary"])
        # Espada plateada/dorada — se anima con el ataque
        shaded_rect(img, 24+wdx, 9+wdy, 3, 14, p["accent"], WHT, p["secondary"])
        rect(img, 23+wdx, 12+wdy, 5, 2, p["secondary"])
    elif faction == "ironbone":
        # Costillar expuesto y escudo de hueso
        for y in range(13, 20, 2):
            rect(img, 12, y, 8, 1, p["skin"])
        # Escudo de huesos (estatico)
        shaded_rect(img, 3, 12, 6, 9, p["skin"], WHT, p["dark"])
        px(img, 5, 15, p["secondary"]) # joya verde
        # Espada oxidada con brillo tóxico — se anima con el ataque
        shaded_rect(img, 24+wdx, 9+wdy, 3, 14, p["dark"], p["secondary"], BLK)
        rect(img, 23+wdx, 12+wdy, 5, 2, p["secondary"])
    elif faction == "ashclaw":
        # Orc Brutal con hacha doble de hierro
        rect(img, 11, 13, 10, 7, p["secondary"]) # loincloth/cinto
        px(img, 15, 16, p["accent"])
        # Hacha gigante — se anima con el ataque
        shaded_rect(img, 24+wdx, 5+wdy, 3, 18, p["secondary"], p["accent"], p["dark"])
        shaded_rect(img, 21+wdx, 6+wdy, 9, 5, p["dark"], p["light"], BLK)
    else: # veilstorm
        # Spellblade con runas arcanas y espada de cristal violeta
        rect(img, 10, 13, 12, 8, p["secondary"])
        px(img, 15, 16, p["accent"])
        # Espada de cristal brillante — se anima con el ataque
        shaded_rect(img, 24+wdx, 8+wdy, 3, 15, p["accent"], WHT, p["primary"])
        circle(img, 25+wdx, 8+wdy, 2, p["light"])

    # Casco / Cabeza
    shaded_rect(img, 10, 4, 12, 7, p["primary"], p["light"], p["dark"])
    hrect(img, 12, 6, 8, 3, p["skin"])
    px(img, 13, 7, p["eye"]); px(img, 17, 7, p["eye"])

    outline(img)
    return img

def draw_archer(p, faction, frame=0):
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Capucha y torso
    shaded_rect(img, 10, 11, 12, 11, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 11+l1dx, 22+l1dy, 4, 7, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 17+l2dx, 22+l2dy, 4, 7, p["primary"], p["light"], p["dark"])

    # Capucha
    shaded_rect(img, 10, 4, 12, 8, p["secondary"], p["accent"], p["dark"])
    hrect(img, 12, 7, 8, 4, p["skin"])
    px(img, 13, 8, p["eye"]); px(img, 17, 8, p["eye"])

    # Arco tenso según facción — el arco/cuerda quedan fijos, la flecha/jabalina
    # (lo que realmente "sale disparado") se anima con el ciclo de ataque.
    if faction == "dawnguard":
        # Arco de madera sagrada y flecha dorada
        for y in range(8, 24): px(img, 4, y, p["secondary"])
        px(img, 3, 9, p["accent"]); px(img, 3, 22, p["accent"])
        rect(img, 4, 15+wdy, 14+wdx, 1, p["accent"]) # Flecha nocked
        px(img, 17+wdx, 14+wdy, WHT)
    elif faction == "ironbone":
        # Arco de huesos y flecha venenosa verde
        for y in range(8, 24): px(img, 4, y, p["skin"])
        px(img, 3, 8, p["secondary"]); px(img, 3, 23, p["secondary"])
        rect(img, 4, 15+wdy, 14+wdx, 1, p["secondary"])
        px(img, 17+wdx, 14+wdy, p["accent"])
    elif faction == "ashclaw":
        # Cazador lanzando jabalina
        shaded_rect(img, 3+wdx, 14+wdy, 16, 2, p["secondary"], p["accent"], p["dark"])
        px(img, 18+wdx, 13+wdy, p["primary"]); px(img, 19+wdx, 14+wdy, p["accent"]) # Punta de fuego
    else:
        # Arco de runas violeta y flecha plasma cian
        for y in range(8, 24): px(img, 4, y, p["secondary"])
        rect(img, 4, 15+wdy, 14+wdx, 1, p["primary"])
        px(img, 17+wdx, 14+wdy, p["light"]); px(img, 18+wdx, 14+wdy, WHT)

    # Carcaj a la espalda
    shaded_rect(img, 22, 12, 4, 9, p["secondary"], p["accent"], p["dark"])
    for i in range(3): px(img, 23+i, 11, p["accent"])

    outline(img)
    return img

def draw_mage(p, faction, frame=0):
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    # El mago no tiene piernas visibles (tunica larga) — el "paso" de caminata
    # se traduce en un leve balanceo horizontal del ruedo, como un flotar/deslizar.
    hem_shift = l1dx - l2dx
    img = img32()
    # Túnica de mago
    shaded_rect(img, 8, 12, 16, 14, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 6+hem_shift, 22, 20, 6, p["primary"], p["light"], p["dark"])

    # Cabeza y Sombrero / Tocado
    shaded_rect(img, 11, 6, 10, 7, p["skin"], p["light"], p["dark"])
    circle(img, 13, 9, 1, p["eye"])
    circle(img, 19, 9, 1, p["eye"])

    if faction == "dawnguard":
        # Capucha de Capellán con aureola dorada arriba
        shaded_rect(img, 10, 2, 12, 5, p["secondary"], p["accent"], p["dark"])
        circle(img, 16, 0, 3, p["accent"]) # Aureola
        circle(img, 16, 0, 1, WHT)
    elif faction == "ironbone":
        # Corona de Liche sobre cráneo
        hrect(img, 10, 2, 12, 4, p["secondary"])
        for cx in [11, 14, 17, 20]: px(img, cx, 1, p["accent"])
    elif faction == "ashclaw":
        # Máscara de cráneo de dragón de Chamán
        shaded_rect(img, 10, 3, 12, 6, p["skin"], WHT, p["dark"])
        px(img, 12, 5, p["eye"]); px(img, 18, 5, p["eye"])
    else:
        # Sombrero puntiagudo arcano con runas
        shaded_rect(img, 10, 1, 12, 5, p["secondary"], p["accent"], p["dark"])
        px(img, 15, 0, p["primary"])

    # Báculo Mágico según facción — se anima con el ciclo de ataque (empuje/casteo)
    shaded_rect(img, 24+wdx, 5+wdy, 2, 21, p["secondary"], p["accent"], p["dark"])
    if faction == "dawnguard":
        # Orbe de luz sagrada
        circle(img, 25+wdx, 4+wdy, 3, p["accent"])
        px(img, 25+wdx, 4+wdy, WHT)
    elif faction == "ironbone":
        # Cráneo con fuego verde
        circle(img, 25+wdx, 4+wdy, 3, p["skin"])
        circle(img, 25+wdx, 2+wdy, 2, p["secondary"])
    elif faction == "ashclaw":
        # Gema de fuego tribal
        circle(img, 25+wdx, 4+wdy, 3, p["primary"])
        px(img, 25+wdx, 4+wdy, p["accent"])
    else:
        # Cristal violáceo flotante
        circle(img, 25+wdx, 4+wdy, 3, p["accent"])
        circle(img, 25+wdx, 4+wdy, 1, WHT)

    outline(img)
    return img

def draw_tank(p, faction, frame=0):
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Piernas pesadas tipo tanque
    shaded_rect(img, 6+l1dx, 21+l1dy, 8, 8, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 18+l2dx, 21+l2dy, 8, 8, p["primary"], p["light"], p["dark"])

    # Pecho y armadura masiva
    shaded_rect(img, 4, 9, 24, 13, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 7, 11, 18, 9, p["secondary"], p["accent"], p["dark"])

    # Hombreas gigantes (Pauldrons)
    shaded_rect(img, 1, 8, 6, 8, p["secondary"], p["accent"], p["dark"])
    shaded_rect(img, 25, 8, 6, 8, p["secondary"], p["accent"], p["dark"])

    # Casco de fortaleza
    shaded_rect(img, 8, 2, 16, 8, p["primary"], p["light"], p["dark"])
    rect(img, 10, 6, 12, 2, p["eye"]) # Ranura de visera encendida

    # Armas / Escudos masivos
    if faction == "dawnguard":
        # Escudo de torre dorado masivo
        shaded_rect(img, 0, 10, 6, 16, p["secondary"], p["accent"], p["dark"])
        rect(img, 2, 14, 2, 8, p["primary"])
    elif faction == "ironbone":
        # Escudo de lápida/huesos con ácido
        shaded_rect(img, 0, 10, 6, 16, p["skin"], WHT, p["dark"])
        px(img, 3, 16, p["secondary"])
    elif faction == "ashclaw":
        # Cuernos de orco en hombreras y maza de piedra — la maza se anima con el ataque
        px(img, 2, 6, p["secondary"]); px(img, 29, 6, p["secondary"])
        shaded_rect(img, 27+wdx, 4+wdy, 4, 18, p["secondary"], p["accent"], p["dark"])
        circle(img, 29+wdx, 4+wdy, 4, p["dark"])
    else:
        # Núcleo de Golem con cristales flotantes
        circle(img, 16, 15, 3, p["accent"])
        px(img, 16, 15, WHT)

    outline(img)
    return img

def draw_hero(p, faction, frame=0):
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Capa legendaria / Alas a la espalda
    if faction == "dawnguard":
        # Alas blancas de Arcángel
        for i in range(6):
            rect(img, 2+i, 4+i, 4, 14, WHT)
            rect(img, 26-i, 4+i, 4, 14, WHT)
    elif faction == "ironbone":
        # Alas de murciélago de Dreadlord
        shaded_rect(img, 2, 5, 6, 15, p["dark"], p["primary"], BLK)
        shaded_rect(img, 24, 5, 6, 15, p["dark"], p["primary"], BLK)
    elif faction == "ashclaw":
        # Estandartes de guerra rojos en la espalda
        rect(img, 4, 2, 3, 16, p["primary"])
        rect(img, 25, 2, 3, 16, p["primary"])
        px(img, 5, 3, p["accent"]); px(img, 26, 3, p["accent"])
    else:
        # Aura de tormenta eléctrica brillante
        circle(img, 16, 16, 13, (*p["light"][:3], 80))

    # Torso de héroe
    shaded_rect(img, 8, 11, 16, 12, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 10, 12, 12, 9, p["secondary"], p["accent"], p["dark"])

    # Gemas y detalles
    circle(img, 16, 16, 2, p["accent"])
    px(img, 16, 16, WHT)

    # Piernas
    shaded_rect(img, 9+l1dx, 22+l1dy, 5, 8, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 18+l2dx, 22+l2dy, 5, 8, p["primary"], p["light"], p["dark"])

    # Cabeza y Corona / Yelmo
    shaded_rect(img, 10, 4, 12, 7, p["skin"], p["light"], p["dark"])
    circle(img, 13, 7, 2, p["eye"])
    circle(img, 19, 7, 2, p["eye"])

    # Corona brillante
    shaded_rect(img, 9, 1, 14, 4, p["secondary"], p["accent"], p["dark"])
    for cx in [10, 13, 16, 19, 21]: px(img, cx, 0, p["accent"])

    # Espada mítica en la diestra — se anima con el ciclo de ataque
    shaded_rect(img, 24+wdx, 6+wdy, 4, 18, p["accent"], WHT, p["secondary"])
    circle(img, 26+wdx, 6+wdy, 3, p["eye"])
    px(img, 26+wdx, 6+wdy, WHT)

    outline(img)
    return img

def draw_armored(p, faction, frame=0):
    """Enemy-only elite variant (UNIT_ARMORED): immune to all physical damage.
    Fully plated, no visible weak points, no held weapon (it never attacks)."""
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Heavy plated legs, fully encased
    shaded_rect(img, 8+l1dx, 22+l1dy, 7, 8, p["dark"], p["light"], BLK)
    shaded_rect(img, 17+l2dx, 22+l2dy, 7, 8, p["dark"], p["light"], BLK)

    # Massive fused chest plate, no gaps
    shaded_rect(img, 5, 10, 22, 13, p["dark"], p["light"], BLK)
    shaded_rect(img, 8, 12, 16, 9, p["secondary"], p["accent"], p["dark"])
    for rx in (7, 24):
        for ry in range(11, 21, 3):
            px(img, rx, ry, p["accent"])  # rivets along the seams

    # Fully-enclosed domed helm, only a thin eye slit
    shaded_rect(img, 9, 2, 14, 9, p["dark"], p["light"], BLK)
    rect(img, 11, 7, 10, 2, p["eye"])

    if faction == "dawnguard":
        px(img, 16, 1, p["accent"])                 # holy crest tip
    elif faction == "ironbone":
        rect(img, 14, 0, 4, 2, p["secondary"])       # bone crown nub
    elif faction == "ashclaw":
        px(img, 9, 3, p["secondary"]); px(img, 22, 3, p["secondary"])  # stubby horns
    else:
        circle(img, 16, 1, 1, p["accent"])           # arcane rune-stud

    outline(img)
    return img

def draw_warded(p, faction, frame=0):
    """Enemy-only elite variant (UNIT_WARDED): immune to magic damage.
    A hooded ward-bearer with a visible glyph/barrier instead of a weapon."""
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    hem_shift = l1dx - l2dx
    img = img32()
    # Flowing robe (glides rather than steps, like the mage)
    shaded_rect(img, 8, 12, 16, 12, p["primary"], p["light"], p["dark"])
    shaded_rect(img, 6+hem_shift, 22, 20, 6, p["primary"], p["light"], p["dark"])

    # Ward glyph glowing on the chest
    circle(img, 16, 17, 3, p["secondary"])
    px(img, 16, 17, WHT)

    # Hood
    shaded_rect(img, 10, 4, 12, 8, p["dark"], p["primary"], BLK)
    px(img, 13, 8, p["eye"]); px(img, 19, 8, p["eye"])

    # Floating ward-halo — the "resists magic" tell, present on every faction
    circle(img, 16, 1, 3, (*p["secondary"][:3], 160))

    # Raised hand with a shimmering barrier orb instead of a weapon
    circle(img, 25+wdx, 14+wdy, 3, (*p["secondary"][:3], 200))
    px(img, 25+wdx, 14+wdy, WHT)

    if faction == "dawnguard":
        px(img, 16, 0, p["accent"])
    elif faction == "ironbone":
        px(img, 14, 8, WHT); px(img, 18, 8, WHT)  # hollow bone-white eyes
    elif faction == "ashclaw":
        rect(img, 12, 3, 8, 2, p["secondary"])     # tribal ward-band
    else:
        px(img, 16, 2, WHT)

    outline(img)
    return img

def draw_flyer(p, faction, frame=0):
    """Enemy-only elite variant (UNIT_FLYER): immune to melee damage only.
    Small, hovers above the path — wings use the leg-offset animation slots
    (a flap cycle) instead of a walk cycle, since it never touches ground."""
    l1dx, l1dy, l2dx, l2dy, wdx, wdy = anim_offsets(frame)
    img = img32()
    # Lean floating body, legs tucked (no ground contact)
    shaded_rect(img, 12, 14, 8, 10, p["primary"], p["light"], p["dark"])

    # Wings spread, flapping via the repurposed leg-offset slots
    shaded_rect(img, 2+l1dx, 10+l1dy, 10, 6, p["secondary"], p["accent"], p["dark"])
    shaded_rect(img, 20+l2dx, 10+l2dy, 10, 6, p["secondary"], p["accent"], p["dark"])

    # Small head
    circle(img, 16, 10, 4, p["skin"])
    px(img, 14, 9, p["eye"]); px(img, 18, 9, p["eye"])

    # Faint hover shadow beneath, since it flies over the path rather than
    # walking it — a quiet visual cue that melee can't reach it.
    for x in range(12, 20):
        px(img, x, 27, (0, 0, 0, 60))

    if faction == "dawnguard":
        px(img, 16, 6, p["accent"])                  # small halo glint
    elif faction == "ironbone":
        px(img, 13, 12, p["secondary"]); px(img, 19, 12, p["secondary"])  # tattered wing-tips
    elif faction == "ashclaw":
        px(img, 16, 22, p["secondary"])              # spiked tail-tip
    else:
        circle(img, 16, 22, 1, p["accent"])           # trailing arcane wisp

    outline(img)
    return img

def draw_projectile(p, faction, size=16):
    img = Image.new("RGBA", (size, size), T)
    c = size // 2

    if faction == "dawnguard":
        # Proyectil Holy Bolt: Estrella dorada brillante con centro blanco
        circle(img, c, c, 6, (*p["proj"][:3], 140))
        circle(img, c, c, 4, p["proj"])
        circle(img, c, c, 2, WHT)
        # Destellos en cruz
        for i in range(1, 7):
            px(img, c+i, c, p["accent"])
            px(img, c-i, c, p["accent"])
            px(img, c, c+i, p["accent"])
            px(img, c, c-i, p["accent"])
    elif faction == "ironbone":
        # Cráneo volador con fuego tóxico verde
        circle(img, c, c, 5, p["skin"])
        circle(img, c, c-1, 3, WHT)
        px(img, c-1, c, BLK); px(img, c+1, c, BLK) # cuencas de ojos
        # Cola de fuego verde
        for y in range(c+2, size):
            px(img, c-1, y, p["secondary"])
            px(img, c+1, y, p["secondary"])
    elif faction == "ashclaw":
        # Lanza/Bola de fuego giratoria de fuego naranja
        circle(img, c, c, 5, p["primary"])
        circle(img, c, c, 3, p["proj"])
        circle(img, c, c, 1, WHT)
        # Chispas de fuego
        px(img, c-4, c-3, p["accent"]); px(img, c+4, c+3, p["accent"])
    else:
        # Orbe de plasma arcano cian con arcos eléctricos violeta
        circle(img, c, c, 5, (*p["primary"][:3], 150))
        circle(img, c, c, 3, p["light"])
        circle(img, c, c, 1, WHT)
        # Arcos eléctricos
        px(img, c-3, c-2, p["accent"]); px(img, c+3, c+2, p["accent"])
        px(img, c+2, c-3, p["accent"]); px(img, c-2, c+3, p["accent"])

    outline(img, (*BLK[:3], 180))
    return img

# ===========================================================================
# SPRITE SHEET BUILDER
# ===========================================================================

DRAWERS = [draw_scout, draw_warrior, draw_archer, draw_mage, draw_tank, draw_hero,
           draw_armored, draw_warded, draw_flyer]
TYPE_NAMES = ["scout", "warrior", "archer", "mage", "tank", "hero",
              "armored", "warded", "flyer"]

def make_unit_sheet(drawer, p, faction):
    """Arma la tira horizontal de FRAMES_TOTAL frames (idle+walk+attack) para
    una unidad. Este PNG (no la pose suelta) es el que termina en el juego."""
    sheet = Image.new("RGBA", (FRAME_W * FRAMES_TOTAL, FRAME_H), T)
    for i in range(FRAMES_TOTAL):
        frame_img = drawer(p, faction, frame=i)
        sheet.paste(frame_img, (i * FRAME_W, 0), frame_img)
    return sheet

def make_sheet():
    """Hoja de referencia (solo pose idle, para tener un vistazo rapido de
    las 24 unidades + proyectiles). La animacion completa de cada una vive en
    su PNG individual (ver make_unit_sheet), no aca."""
    cols = len(TYPE_NAMES) + 1
    rows = len(FACTION_ORDER)
    cell = 34
    pad  = 4
    sw = cols * cell + pad * 2
    sh = rows * cell + pad * 2 + 20

    sheet = Image.new("RGBA", (sw, sh), (20, 20, 35, 255))

    for fi, fname in enumerate(FACTION_ORDER):
        p = PALETTES[fname]
        for ti, drawer in enumerate(DRAWERS):
            spr = drawer(p, fname, frame=IDLE_FRAME)
            ox = pad + ti * cell
            oy = pad + fi * cell
            sheet.paste(spr, (ox, oy), spr)
        # Proyectil al final de la fila
        proj = draw_projectile(p, fname)
        ox = pad + len(DRAWERS) * cell
        oy = pad + fi * cell + 8
        sheet.paste(proj, (ox, oy), proj)

    sheet.save(f"{OUT_DIR}/sprite_sheet.png")
    preview = sheet.resize((sw * 3, sh * 3), Image.NEAREST)
    preview.save(f"{PREVIEW_DIR}/sprite_sheet_preview.png")
    print(f"  [OK] sprite_sheet.png  ({sw}x{sh})")

# ===========================================================================
# EXECUTION
# ===========================================================================

if __name__ == "__main__":
    print("Generando sprite sheets animados de alta calidad por facción...\n")
    print(f"Cada unidad: {FRAMES_TOTAL} frames de {FRAME_W}x{FRAME_H}"
          f" (1 idle + 4 walk + 3 attack) = {FRAME_W*FRAMES_TOTAL}x{FRAME_H} px final.\n")

    for fname in FACTION_ORDER:
        p = PALETTES[fname]
        print(f"  [{fname.upper()}]")
        for ti, (drawer, tname) in enumerate(zip(DRAWERS, TYPE_NAMES)):
            sheet = make_unit_sheet(drawer, p, fname)
            save(sheet, f"{fname}_{tname}")
        proj = draw_projectile(p, fname)
        save(proj, f"{fname}_projectile")
        print()

    print("Construyendo hoja de referencia (solo idle)...")
    make_sheet()

    print(f"\n¡Completado! {len(FACTION_ORDER) * (len(TYPE_NAMES)+1)} sprites generados en {OUT_DIR}")
    print(f"Previsualizaciones (amplificadas 4x) en {PREVIEW_DIR}")
