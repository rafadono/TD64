"""
tools/downsample_images.py — Algoritmo de Downsampling y Conversión a N64

Procesa imágenes de alta resolución (ej. generadas por IA) y las convierte a
las restricciones estrictas de hardware de la Nintendo 64:
  - Reducción de escala mediante Pixel-Art Nearest-Neighbor / Grid Averaging.
  - Extracción de fondo transparente para sprites (remueve fondo negro #000000).
  - Cuantización de color a paleta N64 (RGBA16 / 16-32 colores).
  - Generación de archivos finales en assets/sprites/ listos para mksprite.

Uso:
  python3 tools/downsample_images.py <ruta_imagen_entrada> <tipo_asset> [nombre_salida]

Tipos de Asset:
  - unit        -> 32x32 px (Sprite estático de unidad)
  - sheet       -> 256x32 px (Sprite sheet animado de 8 frames)
  - projectile  -> 16x16 px (Proyectil)
  - background  -> 320x240 px (Fondo de pantalla / Menú VI mode)
"""

import sys
import os
from PIL import Image, ImageOps

# Directorios de trabajo
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(BASE_DIR)
ASSETS_DIR = os.path.join(PROJECT_DIR, "assets", "sprites")
PREVIEW_DIR = os.path.join(ASSETS_DIR, "previews")

os.makedirs(ASSETS_DIR, exist_ok=True)
os.makedirs(PREVIEW_DIR, exist_ok=True)

# Presets de dimensiones para N64
PRESETS = {
    "unit": (32, 32),
    "sheet": (256, 32),
    "projectile": (16, 16),
    "background": (320, 240)
}

def remove_black_background(img: Image.Image, threshold: int = 25) -> Image.Image:
    """Convierte píxeles negros/casi negros (#000000) en transparentes (Alpha=0)."""
    img = img.convert("RGBA")
    datas = img.getdata()
    new_data = []
    for r, g, b, a in datas:
        if r <= threshold and g <= threshold and b <= threshold:
            new_data.append((0, 0, 0, 0))
        else:
            new_data.append((r, g, b, a))
    img.putdata(new_data)
    return img

def quantize_n64_rgba16(img: Image.Image) -> Image.Image:
    """
    Cuantiza la imagen al formato RGBA16 de la N64 (5 bits R, 5 bits G, 5 bits B, 1 bit A).
    Reduce los canales de 256 niveles (8-bit) a 32 niveles (5-bit).
    """
    img = img.convert("RGBA")
    datas = img.getdata()
    quantized_data = []
    
    for r, g, b, a in datas:
        if a < 128:
            quantized_data.append((0, 0, 0, 0))
        else:
            # Cuantizar a 5 bits por canal (pasos de 8)
            r5 = (r >> 3) << 3
            g5 = (g >> 3) << 3
            b5 = (b >> 3) << 3
            quantized_data.append((r5, g5, b5, 255))
            
    img.putdata(quantized_data)
    return img

def downsample_image(input_path: str, asset_type: str, output_name: str = None) -> str:
    if not os.path.exists(input_path):
        print(f"Error: El archivo de entrada '{input_path}' no existe.")
        return None

    if asset_type not in PRESETS:
        print(f"Error: Tipo de asset '{asset_type}' no válido. Opciones: {list(PRESETS.keys())}")
        return None

    target_w, target_h = PRESETS[asset_type]
    
    if not output_name:
        output_name = os.path.splitext(os.path.basename(input_path))[0]
        if not output_name.endswith(".png"):
            output_name += ".png"
    elif not output_name.endswith(".png"):
        output_name += ".png"

    output_path = os.path.join(ASSETS_DIR, output_name)
    preview_path = os.path.join(PREVIEW_DIR, output_name.replace(".png", "_preview.png"))

    print(f"Procesando: {input_path}")
    print(f"Target N64: {target_w}x{target_h} px ({asset_type})")

    # 1. Cargar imagen original
    img = Image.open(input_path).convert("RGBA")

    # 2. Si es sprite o proyectil, extraer fondo negro a transparente
    if asset_type in ["unit", "sheet", "projectile"]:
        img = remove_black_background(img)

    # 3. Downsampling usando Nearest Neighbor para preservar bordes pixel-art sin desenfoque
    resampled = img.resize((target_w, target_h), Image.Resampling.NEAREST)

    # 4. Cuantizar paleta de colores al formato N64 RGBA16
    final_img = quantize_n64_rgba16(resampled)

    # 5. Guardar PNG optimizado para mksprite en assets/sprites/
    final_img.save(output_path, "PNG")
    print(f"  [Guardado] Sprite N64: {output_path}")

    # 6. Guardar Preview ampliada (4x) para inspección visual
    preview_scale = 4 if asset_type != "background" else 2
    pw, ph = target_w * preview_scale, target_h * preview_scale
    preview_img = final_img.resize((pw, ph), Image.Resampling.NEAREST)
    preview_img.save(preview_path, "PNG")
    print(f"  [Guardado] Preview: {preview_path}")

    return output_path

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python3 tools/downsample_images.py <ruta_imagen> <tipo_asset: unit|sheet|projectile|background> [nombre_salida]")
        sys.exit(1)

    in_file = sys.argv[1]
    a_type = sys.argv[2]
    out_file = sys.argv[3] if len(sys.argv) > 3 else None
    
    downsample_image(in_file, a_type, out_file)
