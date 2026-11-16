#!/usr/bin/env python3
"""Builds the promotional graphics in Release/<version>/Mockups from editor screenshots.

Usage: python3 make_mockups.py <shots dir> <output dir>

<shots dir> must hold compact.png, edit.png and edit_reverse.png as rendered by
promo_shots.cpp (editor at 2x scale, 2720 px wide).
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont

SHOTS, OUT = Path(sys.argv[1]), Path(sys.argv[2])
OUT.mkdir(parents=True, exist_ok=True)

FONTS = Path("/System/Library/Fonts/Supplemental")
AMBER, WHITE, RED = (245, 180, 42), (236, 236, 232), (205, 40, 36)


def font(name, size):
    return ImageFont.truetype(str(FONTS / name), size)


compact = Image.open(SHOTS / "compact.png").convert("RGB")
edit = Image.open(SHOTS / "edit.png").convert("RGB")
edit_rev = Image.open(SHOTS / "edit_reverse.png").convert("RGB")

# regions of the 2720 x 1540 edit screenshot
TRAY = (32, 26, 523, 513)
KEYPAD = (565, 69, 2700, 381)
WAVE = (26, 914, 1269, 1205)
PANEL = (0, 905, 2720, 1540)


def background(w, h):
    """Dark charcoal with a soft light spot behind the product."""
    bg = Image.new("RGB", (w, h), (21, 23, 27))
    glow = Image.new("L", (w, h), 0)
    ImageDraw.Draw(glow).ellipse((w * 0.1, h * 0.15, w * 0.9, h * 0.95), fill=70)
    glow = glow.filter(ImageFilter.GaussianBlur(min(w, h) * 0.18))
    bg.paste(Image.new("RGB", (w, h), (52, 57, 66)), (0, 0), glow)
    return bg


def place(canvas, img, x, y, width, shadow=True):
    """Pastes img scaled to width at x/y with a drop shadow; returns the bottom y."""
    s = width / img.width
    im = img.resize((int(img.width * s), int(img.height * s)), Image.LANCZOS)
    if shadow:
        pad = 60
        sh = Image.new("L", (im.width + pad * 2, im.height + pad * 2), 0)
        ImageDraw.Draw(sh).rectangle((pad, pad + 14, pad + im.width, pad + im.height + 14), fill=190)
        sh = sh.filter(ImageFilter.GaussianBlur(26))
        canvas.paste(Image.new("RGB", sh.size, (0, 0, 0)), (int(x) - pad, int(y) - pad), sh)
    canvas.paste(im, (int(x), int(y)))
    return int(y) + im.height


def spaced(d, x, y, txt, f, fill, spacing=0):
    for ch in txt:
        d.text((x, y), ch, font=f, fill=fill)
        x += d.textlength(ch, font=f) + spacing
    return x


def logo(canvas, cx, y, size, badge=True, align="center", x=None):
    """ESS76 badge, MIRACLE DSP wordmark and subtitle. Returns the bottom y."""
    d = ImageDraw.Draw(canvas)
    big, small = font("Arial Black.ttf", size), font("Arial Black.ttf", int(size * 0.5))
    sub, ital = font("Arial.ttf", int(size * 0.24)), font("Arial Italic.ttf", int(size * 0.24))
    w1, w2 = d.textlength("MIRACLE", font=big), d.textlength("DSP", font=small)
    total = w1 + size * 0.12 + w2
    left = cx - total / 2 if align == "center" else x
    if badge:
        bf = font("Arial Bold.ttf", int(size * 0.2))
        bw = d.textlength("ESS76", font=bf) + size * 0.3
        bx = cx - bw / 2 if align == "center" else left
        d.rounded_rectangle((bx, y, bx + bw, y + size * 0.33), radius=4, outline=WHITE, width=max(2, size // 50))
        d.text((bx + bw / 2, y + size * 0.165), "ESS76", font=bf, fill=WHITE, anchor="mm")
        y += size * 0.55
    d.text((left, y), "MIRACLE", font=big, fill=AMBER)
    d.text((left + w1 + size * 0.12, y + size * 0.98), "DSP", font=small, fill=(21, 23, 27), anchor="ls",
           stroke_width=max(2, size // 45), stroke_fill=AMBER)
    y += size * 1.25
    s1, sp, gap = "Digital Sample Player", size * 0.045, size * 0.25
    w_s1 = sum(d.textlength(ch, font=sub) + sp for ch in s1) - sp
    w_s2 = d.textlength("just like 1985", font=ital)
    sx = cx - (w_s1 + gap + w_s2) / 2 if align == "center" else left
    spaced(d, sx, y, s1, sub, WHITE, sp)
    d.text((sx + w_s1 + gap, y), "just like 1985", font=ital, fill=AMBER)
    return int(y + size * 0.4)


def features(canvas, y, items, size, cx, gap):
    d = ImageDraw.Draw(canvas)
    hf, tf = font("Arial Black.ttf", size), font("Arial.ttf", int(size * 0.62))
    widths = [max(d.textlength(h, font=hf), d.textlength(t, font=tf)) for h, t in items]
    x = cx - (sum(widths) + gap * (len(items) - 1)) / 2
    for (h, t), w in zip(items, widths):
        d.text((x, y), h, font=hf, fill=AMBER)
        d.text((x, y + size * 1.25), t, font=tf, fill=(200, 202, 206))
        x += w + gap


def disclaimer(canvas, y):
    ImageDraw.Draw(canvas).text(
        (canvas.width / 2, y), "Inspired by a classic 1980s sampler. Not affiliated with Ensoniq or Creative Technology.",
        font=font("Arial.ttf", max(12, canvas.width // 120)), fill=(110, 114, 122), anchor="mm")


def centered(canvas, y, txt, size, fill=WHITE, name="Arial.ttf"):
    ImageDraw.Draw(canvas).text((canvas.width / 2, y), txt, font=font(name, size), fill=fill, anchor="mm")


FEATS = [("8-BIT", "8-33 kHz engine"), ("4-POLE", "resonant filter"), ("2 ZONES", "upper & lower"),
         ("TEMPO SYNC", "follows your DAW"), ("REVERSE", "play it backwards"), ("YOUR", "sample folders")]

# 01 hero 1920 x 1080
c = background(1920, 1080)
logo(c, 960, 40, 96)
place(c, compact, 160, 300, 1600)
features(c, 880, FEATS, 30, 960, 56)
disclaimer(c, 1050)
c.save(OUT / "01_hero_1920x1080.png", optimize=True)

# 02 banner 1500 x 500
c = background(1500, 500)
place(c, compact.crop((0, 0, 2000, 904)), 610, 50, 1000)
d = ImageDraw.Draw(c)
logo(c, 0, 90, 84, align="left", x=60)
d.line((60, 300, 560, 300), fill=RED, width=4)
d.text((60, 330), "NEW IN 1.4", font=font("Arial Black.ttf", 22), fill=AMBER)
d.text((60, 364), "Tempo sync · Reverse · Playhead", font=font("Arial.ttf", 24), fill=WHITE)
c.save(OUT / "02_banner_1500x500.png", optimize=True)

# 03 social 1080 x 1080
c = background(1080, 1080)
logo(c, 540, 50, 84)
place(c, edit.crop(KEYPAD), 40, 320, 1000)
y = place(c, edit.crop(WAVE), 150, 530, 780)
features(c, y + 50, FEATS[:4], 26, 540, 40)
centered(c, y + 180, "Point it at a folder. Scroll. Play.", 32)
disclaimer(c, 1050)
c.save(OUT / "03_social_1080x1080.png", optimize=True)

# 04 story 1080 x 1920
c = background(1080, 1920)
logo(c, 540, 120, 96)
place(c, edit.crop(KEYPAD), 30, 470, 1020)
centered(c, 760, "NEW IN 1.4", 34, AMBER, "Arial Black.ttf")
y = place(c, edit.crop(WAVE), 90, 820, 900)
centered(c, y + 40, "TEMPO SYNC  ·  the red key plays the loop in DAW tempo", 26)
y = place(c, edit_rev.crop(WAVE), 90, y + 100, 900)
centered(c, y + 40, "REVERSE  ·  a white line shows where the sample plays", 26)
centered(c, y + 180, "8-bit grit. 4-pole filter.", 40)
centered(c, y + 236, "Every sample folder you own.", 40)
disclaimer(c, 1880)
c.save(OUT / "04_story_1080x1920.png", optimize=True)

# 05 laptop 1600 x 1000: plugin window open over a DAW arrangement
c = background(1600, 1000)
d = ImageDraw.Draw(c)
d.rounded_rectangle((150, 60, 1450, 900), radius=36, fill=(12, 12, 14), outline=(90, 94, 100), width=3)
screen = (180, 90, 1420, 860)
d.rectangle(screen, fill=(36, 38, 43))
d.rectangle((screen[0], screen[1], screen[2], screen[1] + 26), fill=(52, 55, 61))
for i, col in enumerate([(232, 94, 84), (235, 182, 64), (98, 190, 88)]):
    d.ellipse((screen[0] + 12 + i * 22, screen[1] + 7, screen[0] + 26 + i * 22, screen[1] + 21), fill=col)
tracks = [("Drums", (70, 120, 190)), ("Bass", (120, 88, 180)), ("MIRACLE DSP", (196, 140, 40)),
          ("Pad", (60, 150, 130)), ("Vocal", (170, 80, 110)), ("FX", (90, 110, 140)), ("Keys", (160, 120, 70))]
tf = font("Arial.ttf", 15)
for i, (name, col) in enumerate(tracks):
    ty = screen[1] + 50 + i * 100
    d.text((screen[0] + 14, ty + 30), name, font=tf, fill=(200, 200, 205))
    for k, (a, b) in enumerate([(0, 0.32), (0.34, 0.66), (0.68, 1.0)]):
        if (i + k) % 4 == 3:
            continue
        x0 = screen[0] + 150 + a * 1060
        d.rounded_rectangle((x0, ty + 10, x0 + (b - a) * 1060 - 8, ty + 80), radius=5, fill=col)
d.polygon([(40, 900), (1560, 900), (1500, 940), (100, 940)], fill=(70, 73, 79))
d.rectangle((720, 900, 880, 910), fill=(45, 47, 52))
place(c, edit, 270, 230, 1060)
c.save(OUT / "05_laptop_1600x1000.png", optimize=True)

# 06 feature sheet 1600 x 1400
c = background(1600, 1400)
d = ImageDraw.Draw(c)
logo(c, 800, 40, 80, badge=False)
hf, tf = font("Arial Black.ttf", 28), font("Arial.ttf", 19)


def caption(x, y, head, txt):
    d.text((x, y), head, font=hf, fill=AMBER)
    d.text((x, y + 40), txt, font=tf, fill=(220, 222, 226))


y = place(c, edit.crop(PANEL), 50, 200, 1500)
caption(50, y + 22, "EDIT PANEL", "Every parameter: wavesample with loop points, two oscillators, LFO, filter, both envelopes, master.")
y2 = place(c, edit.crop(WAVE), 50, y + 120, 720)
caption(50, y2 + 20, "TEMPO SYNC", "Reads the DAW tempo and the loop tempo. The red key plays the loop in time.")
y3 = place(c, edit_rev.crop(WAVE), 830, y + 120, 720)
caption(830, y3 + 20, "REVERSE + PLAYHEAD", "Play samples backwards. A white line follows the sample.")
y4 = place(c, edit.crop(KEYPAD), 50, y2 + 120, 960)
caption(50, y4 + 20, "REAL KEYPAD WORKFLOW", "Type a parameter number, step the value, recall programs.")
ty = place(c, edit.crop(TRAY), 1150, y2 + 120, 330)
caption(1150, ty + 20, "DISK TRAY", "One folder per half.")
disclaimer(c, 1380)
c.save(OUT / "06_feature_sheet_1600x1400.png", optimize=True)

print("written to", OUT)
