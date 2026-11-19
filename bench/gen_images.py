#!/usr/bin/env python3
"""Generate small synthetic 4K JPEG test images (smooth/texture/text/noise).
Run: python3 -I bench/gen_images.py
Outputs RGB PNG ground truth + JPEG q80 into bench/images/.
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "images")
os.makedirs(OUT, exist_ok=True)
W, H = 3840, 2160
rng = np.random.default_rng(12345)
yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)

def save(name, rgb, sub=2):
    im = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8), "RGB")
    im.save(os.path.join(OUT, name + ".png"))
    im.save(os.path.join(OUT, name + ".jpg"), quality=80, subsampling=sub)


# 1. smooth: low-frequency blobs / gradient
r = 128 + 90*np.sin(xx/900.0) * np.cos(yy/700.0)
g = 128 + 80*np.sin((xx+yy)/1100.0 + 1.3)
b = 128 + 70*np.cos((xx*0.7 - yy)/800.0 + 2.1)
smooth = np.stack([r, g, b], -1)
save("smooth4k", smooth)

# 2. texture: band-limited multi-scale texture (photo-like)
tex = np.zeros((H, W), np.float32)
for k in range(6):
    fx, fy = rng.uniform(0.002, 0.05, 2)
    ph = rng.uniform(0, 2*np.pi)
    amp = 60.0 / (1 + k)
    tex += amp*np.sin(xx*fx + yy*fy + ph)
tex += 18*np.sin(xx*0.31 + 1.0)*np.sin(yy*0.27)
t = 128 + tex
texture = np.stack([t, t*0.96 + 6, t*0.9 + 14], -1)
save("texture4k", texture)

# 3. text: white page with black text-ish strokes
page = np.full((H, W, 3), 245, np.uint8)
img = Image.fromarray(page, "RGB")
d = ImageDraw.Draw(img)
try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 34)
except Exception:
    font = ImageFont.load_default()
for row in range(40):
    y = 40 + row*52
    line = ("The quick brown fox jumps over the lazy dog 0123456789  "
            "JPEG IDCT AAN fast algorithm and chroma upsampling table test line %02d" % row)
    d.text((50, y), line, fill=(10, 10, 10), font=font)
    if row % 5 == 0:
        d.rectangle([50, y+38, 50 + 300 + (row*97) % 1200, y+44], fill=(60,60,60))
save("text4k", np.asarray(img))

# 4. noise: flat-ish noise, entropy-heavy
noise = 128 + rng.normal(0, 30, (H, W, 3)).astype(np.float32)
save("noise4k", noise)
print("wrote images to", OUT)

# 4:4:4 copies (no chroma subsampling) for isolating the IDCT path
save("smooth4k_444", smooth, sub=0)
save("texture4k_444", texture, sub=0)
save("noise4k_444", noise, sub=0)
