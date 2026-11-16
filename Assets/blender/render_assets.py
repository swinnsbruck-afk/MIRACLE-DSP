# MIRACLE DSP – renders the panel artwork used by the plugin interface.
#
#   /Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup \
#       -P Assets/blender/render_assets.py -- Assets/renders [preview]
#
# Coordinates are design pixels of the 1360 x 452 px plugin window (x right,
# y down), mapped 1:1 to Blender units with y flipped. Everything is rendered
# top-down with an orthographic camera, so the images line up with the GUI.

import math
import os
import sys

import bpy

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = os.path.abspath(args[0] if args else "renders")
PREVIEW = len(args) > 1 and args[1] == "preview"
os.makedirs(OUT, exist_ok=True)

SCALE = 2          # render pixels per design pixel for the panel
SPRITE_SCALE = 4   # render pixels per design pixel for small parts
SAMPLES = 16 if PREVIEW else 96

W, H = 1360, 452
LEFT = 280
EDIT_H = 318


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        prefs.compute_device_type = "METAL"
        prefs.get_devices()
        for device in prefs.devices:
            device.use = True
        scene.cycles.device = "GPU"
    except Exception:
        scene.cycles.device = "CPU"
    scene.cycles.samples = SAMPLES
    scene.cycles.use_denoising = True
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"

    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.55, 0.57, 0.6, 1.0)
    bg.inputs[1].default_value = 0.18
    return scene


def lin(hex_colour):
    """sRGB hex -> linear RGBA"""
    def channel(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    h = hex_colour.lstrip("#")
    return (channel(int(h[0:2], 16)), channel(int(h[2:4], 16)), channel(int(h[4:6], 16)), 1.0)


def material(name, hex_colour, roughness=0.5, grain=0.0, grain_scale=2.0, metallic=0.0, coat=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = lin(hex_colour)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    if "Coat Weight" in bsdf.inputs:
        bsdf.inputs["Coat Weight"].default_value = coat
    if grain > 0.0:
        # fine moulded-plastic texture
        coords = nodes.new("ShaderNodeTexCoord")
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = grain_scale
        noise.inputs["Detail"].default_value = 6.0
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = grain
        bump.inputs["Distance"].default_value = 0.2
        links.new(coords.outputs["Object"], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def box(name, x0, y0, x1, y1, z0, z1, mat, bevel=0.0, segments=3):
    """Axis-aligned box in design pixels (y down)."""
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (x1 - x0, y1 - y0, z1 - z0)
    obj.location = ((x0 + x1) / 2.0, -(y0 + y1) / 2.0, (z0 + z1) / 2.0)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if mat is not None:
        obj.data.materials.append(mat)
    if bevel > 0.0:
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = segments
        mod.limit_method = "ANGLE"
    return obj


def cut(target, cutter):
    """Boolean difference applied before the bevel, cutter removed."""
    mod = target.modifiers.new("Cut", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.object = cutter
    mod.solver = "EXACT"
    bpy.context.view_layer.objects.active = target
    while target.modifiers.find("Cut") > 0:
        bpy.ops.object.modifier_move_up(modifier="Cut")
    bpy.ops.object.modifier_apply(modifier="Cut")
    bpy.data.objects.remove(cutter, do_unlink=True)


def sun(rx=-35.0, ry=-18.0, strength=2.4, angle=6.0):
    bpy.ops.object.light_add(type="SUN")
    light = bpy.context.active_object
    light.data.energy = strength
    light.data.angle = math.radians(angle)
    light.rotation_euler = (math.radians(rx), math.radians(ry), 0.0)


def ortho_camera(cx, cy, width_px, height_px, scale):
    bpy.ops.object.camera_add(location=(cx, -cy, 1000.0), rotation=(0.0, 0.0, 0.0))
    cam = bpy.context.active_object
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = max(width_px, height_px)
    cam.data.clip_end = 5000.0
    scene = bpy.context.scene
    scene.camera = cam
    scene.render.resolution_x = int(round(width_px * scale))
    scene.render.resolution_y = int(round(height_px * scale))
    scene.render.resolution_percentage = 100


def render(path, transparent):
    scene = bpy.context.scene
    scene.render.film_transparent = transparent
    scene.render.filepath = os.path.join(OUT, path)
    bpy.ops.render.render(write_still=True)
    print("rendered", path)


def screw(x, y, mat):
    bpy.ops.mesh.primitive_cylinder_add(radius=4.2, depth=2.0, location=(x, -y, 1.2), vertices=40)
    head = bpy.context.active_object
    head.data.materials.append(mat)
    bev = head.modifiers.new("Bevel", "BEVEL")
    bev.width = 0.8
    bev.segments = 4
    for angle in (45.0, -45.0):
        slot = box("Slot", -3.2, -0.45, 3.2, 0.45, -0.7, 0.7, None)
        slot.rotation_euler = (0.0, 0.0, math.radians(angle))
        slot.location = (x, -y, 2.3)
        cut(head, slot)


# ---------------------------------------------------------------- the panel
def build_panel(total_h):
    slate = material("Slate", "#30363c", roughness=0.62, grain=0.35, grain_scale=1.6)
    slate_light = material("SlateLight", "#363c43", roughness=0.58, grain=0.35, grain_scale=1.6)
    fascia = material("Fascia", "#262422", roughness=0.78, grain=0.25, grain_scale=3.0)
    well = material("Well", "#101214", roughness=0.85)
    deep = material("Deep", "#050506", roughness=0.9)
    screw_mat = material("Screw", "#8a9096", roughness=0.35, metallic=0.9)
    rubber = material("Rubber", "#1b1d20", roughness=0.9, grain=0.2, grain_scale=4.0)

    body = box("Body", 0, 0, W, total_h, -30, 0, slate, bevel=2.0)

    # recesses in the left column: disk tray, drive, wheel well; keyboard bed
    cut(body, box("TrayCut", 18, 14, 262, 218, -12, 5, None))
    cut(body, box("DriveCut", 18, 226, 262, 256, -6, 5, None))
    cut(body, box("WheelCut", 18, 264, 262, H - 18, -4, 5, None))
    cut(body, box("KeysCut", LEFT + 6, 216, W - 14, H - 12, -14, 5, None))

    box("TrayFloor", 18, 14, 262, 218, -13, -11.5, well)
    box("DriveFace", 18, 226, 262, 256, -7, -5.5, deep)
    box("DriveSlot", 26, 246, 222, 249, -5.6, -5.0, well, bevel=0.6)
    box("Eject", 232, 236, 252, 246, -5.6, -3.2, rubber, bevel=1.2)
    box("WheelFloor", 18, 264, 262, H - 18, -5, -3.5, fascia)
    box("KeysFloor", LEFT + 6, 216, W - 14, H - 12, -15, -13.5, deep)

    box("CheekL", 0, 0, 9, H, -30, 3.0, slate_light, bevel=2.5)
    box("CheekR", W - 12, 0, W, H, -30, 3.0, slate_light, bevel=2.5)

    box("Strip", LEFT, 0, W - 12, 35, -2, 1.8, slate_light, bevel=1.2)
    box("Bar", LEFT, 192, W - 12, 216, -2, 1.8, slate_light, bevel=1.2)
    box("Panel", LEFT, 36, W - 12, 191, -2, 0.4, fascia, bevel=0.6)

    for (x, y) in ((LEFT + 12, 26), (W - 26, 26), (LEFT + 12, 204), (W - 26, 204)):
        screw(x, y, screw_mat)

    if total_h > H:
        box("EditPlate", 6, H + 4, W - 6, total_h - 4, -2, 0.6, slate, bevel=1.5)

    sun()


def render_panel():
    reset_scene()
    build_panel(H)
    ortho_camera(W / 2.0, H / 2.0, W, H, SCALE)
    render("panel.png", transparent=False)

    reset_scene()
    build_panel(H + EDIT_H)
    ortho_camera(W / 2.0, H + EDIT_H / 2.0, W, EDIT_H, SCALE)
    render("edit_panel.png", transparent=False)


# ---------------------------------------------------------------- keyboard keys
WHITE_W = (W - LEFT - 30) / 36.0
KEY_H = H - (36 + 156 + 26) - 16
BLACK_W = WHITE_W * 0.7
BLACK_H = KEY_H * 0.64


def render_keys():
    for down in (False, True):
        state = "down" if down else "up"

        reset_scene()
        ivory = material("Ivory", "#f1ecdf", roughness=0.32, coat=0.2)
        key = box("White", 0.6, 0, WHITE_W - 0.6, KEY_H, -14, 0, ivory, bevel=1.4, segments=4)
        if down:   # pressed keys pivot down at the back
            key.rotation_euler = (math.radians(-2.2), 0.0, 0.0)
            key.location.z -= 1.0
        sun(rx=-28.0, ry=-12.0, strength=3.0, angle=8.0)
        ortho_camera(WHITE_W / 2.0, KEY_H / 2.0, WHITE_W, KEY_H, SPRITE_SCALE)
        render("key_white_%s.png" % state, transparent=True)

        reset_scene()
        black = material("BlackKey", "#030303", roughness=0.28, coat=0.25)
        key = box("Black", 0, 0, BLACK_W, BLACK_H, -8, 0, black, bevel=1.2, segments=4)
        cx = BLACK_W / 2.0
        for v in key.data.vertices:   # narrower top face like a real black key
            if v.co.z > -1.0:
                v.co.x = cx + (v.co.x - cx) * 0.82
        if down:
            key.rotation_euler = (math.radians(-2.5), 0.0, 0.0)
            key.location.z -= 1.2
        sun(rx=-28.0, ry=-12.0, strength=3.0, angle=8.0)
        ortho_camera(BLACK_W / 2.0, BLACK_H / 2.0, BLACK_W, BLACK_H, SPRITE_SCALE)
        render("key_black_%s.png" % state, transparent=True)


# ---------------------------------------------------------------- panel keys, fader caps, wheel
def render_keycaps():
    colours = {"amber": "#f5b42a", "grey": "#8e908d", "white": "#e8e5da", "black": "#1a1b1d"}
    for name, hex_colour in colours.items():
        reset_scene()
        cap = material("Cap", hex_colour, roughness=0.42, grain=0.08, grain_scale=6.0)
        box("Cap", 0, 0, 34, 14, -4, 0, cap, bevel=1.4, segments=4)
        sun(rx=-30.0, ry=-15.0, strength=3.0, angle=10.0)
        ortho_camera(17.0, 7.0, 34.0, 14.0, SPRITE_SCALE)
        render("keycap_%s.png" % name, transparent=True)

    for name, (w, h) in {"large": (28.0, 15.0), "small": (20.0, 11.0)}.items():
        reset_scene()
        plastic = material("FaderCap", "#141516", roughness=0.4, grain=0.05, grain_scale=6.0)
        line = material("Line", "#f2f2f2", roughness=0.5)
        cap = box("FaderCap", 0, 0, w, h, -6, 0, plastic, bevel=1.6, segments=4)
        cut(cap, box("Groove", 1.0, h / 2.0 - 1.0, w - 1.0, h / 2.0 + 1.0, -0.6, 1.0, None))
        box("Paint", 1.2, h / 2.0 - 0.9, w - 1.2, h / 2.0 + 0.9, -0.62, -0.4, line)
        sun(rx=-30.0, ry=-15.0, strength=3.0, angle=10.0)
        ortho_camera(w / 2.0, h / 2.0, w, h, SPRITE_SCALE)
        render("fader_%s.png" % name, transparent=True)


def render_wheel_frames():
    # one rib spacing (10 degrees) in 12 frames, repeated by the GUI while the wheel turns
    wheel_w, wheel_h = 24.0, 124.0
    radius = wheel_h / 2.0
    frames = 12
    for frame in range(frames):
        reset_scene()
        rubber = material("Wheel", "#17181a", roughness=0.7, grain=0.15, grain_scale=5.0)
        bpy.ops.mesh.primitive_cylinder_add(radius=radius, depth=wheel_w, vertices=128,
                                            location=(wheel_w / 2.0, -wheel_h / 2.0, -radius),
                                            rotation=(0.0, math.radians(90.0), 0.0))
        bpy.context.active_object.data.materials.append(rubber)
        for i in range(36):
            angle = math.radians(i * 10.0 + frame * 10.0 / frames)
            bpy.ops.mesh.primitive_cube_add(size=1.0)
            rib = bpy.context.active_object
            rib.scale = (wheel_w - 3.0, 1.3, 1.6)
            rib.location = (wheel_w / 2.0, -wheel_h / 2.0 + radius * math.sin(angle), -radius + radius * math.cos(angle))
            rib.rotation_euler = (-angle, 0.0, 0.0)
            rib.data.materials.append(rubber)
        sun(rx=-30.0, ry=-10.0, strength=3.2, angle=12.0)
        ortho_camera(wheel_w / 2.0, wheel_h / 2.0, wheel_w, wheel_h, SPRITE_SCALE)
        render("wheel_%02d.png" % frame, transparent=True)


only = set(args[2:])
for job in (render_panel, render_keys, render_keycaps, render_wheel_frames):
    if not only or job.__name__ in only:
        job()
print("done")
