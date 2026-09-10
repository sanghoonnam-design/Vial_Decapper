"""Approximate, unitless geometry traced from the two supplied equipment views.

Y is vertical; the head projects toward negative Z. These are display dimensions,
not manufacturing dimensions or machine travel limits.
"""

from math import cos, sin, tau


def build_device_mesh(carriage_position=0.0):
    """Position 0 is under the head; 1 is forward near the fixed sensor."""
    travel = -65 * max(0.0, min(1.0, carriage_position))
    faces = []

    def box(center, size, color):
        x, y, z = center
        w, h, d = (v / 2 for v in size)
        vertices = [(x + a*w, y + b*h, z + c*d)
                    for a, b, c in ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),
                                    (-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1))]
        for indices in ((0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5)):
            faces.append((tuple(vertices[i] for i in indices), color))

    def cylinder(center, radius, height, color, segments=20):
        x, y, z = center
        rings = [[(x + radius*cos(tau*i/segments), y + offset,
                   z + radius*sin(tau*i/segments)) for i in range(segments)]
                 for offset in (-height/2, height/2)]
        faces.append((tuple(reversed(rings[0])), color))
        faces.append((tuple(rings[1]), color))
        for i in range(segments):
            j = (i+1) % segments
            faces.append(((rings[0][i], rings[0][j], rings[1][j], rings[1][i]), color))

    silver, dark, steel, gold = '#b8c4ce', '#353e48', '#738592', '#d9ad38'
    # Base, feet, rear upright and paired linear guides.
    box((0, 8, -25), (150, 16, 230), silver)
    for x in (-58, 58):
        for z in (-68, 68):
            box((x, -3, z), (24, 8, 24), dark)
    # Fixed front vial-presence sensor, facing back toward the vial holder.
    box((0, 20, -127), (28, 8, 18), silver)
    box((0, 86, -127), (12, 128, 10), dark)
    box((0, 143, -120), (8, 10, 5), '#e5a343')
    # Fore/aft rails remain fixed to the base.
    for x in (-23, 23):
        box((x, 20, -35), (7, 8, 174), steel)
    box((0, 138, 54), (84, 246, 18), silver)
    box((0, 125, 40), (44, 214, 14), dark)
    for x in (-30, 30):
        box((x, 131, 32), (8, 226, 9), steel)
        for y in range(35, 240, 32):
            box((x, y, 26), (4, 4, 3), dark)
    cylinder((0, 133, 26), 4, 224, gold)
    # The complete lower holder travels on the base rails independently of the head.
    holder_z = -37 + travel
    box((0, 29, holder_z), (58, 10, 42), silver)
    box((0, 82, holder_z), (40, 96, 34), dark)
    box((0, 134, holder_z), (48, 8, 42), silver)
    cylinder((0, 141, holder_z), 13, 6, dark)
    # Moving carriage and the projecting head support.
    box((0, 192, 24), (82, 70, 14), silver)
    box((0, 208, -5), (76, 14, 75), silver)
    box((0, 249, -12), (100, 10, 115), '#396c9e')
    box((0, 257, -12), (100, 9, 115), silver)
    for x in (-41, 41):
        box((x, 283, -12), (10, 48, 100), silver)
    # Circular decapping assembly, jaws, spindle and upper drive.
    cylinder((0, 275, -37), 35, 26, steel)
    cylinder((0, 292, -37), 31, 8, silver)
    cylinder((0, 298, -37), 20, 7, dark)
    for x in (-15, 15):
        box((x, 307, -37), (11, 20, 27), silver)
        box((x, 315, -37), (7, 7, 15), dark)
    cylinder((0, 229, -37), 18, 33, steel)
    cylinder((0, 207, -37), 15, 12, gold)
    cylinder((0, 196, -37), 10, 10, dark)
    box((0, 306, 28), (38, 76, 38), dark)
    box((0, 268, 28), (44, 8, 44), silver)
    for x in (-12, -4, 4, 12):
        box((x, 309, 8), (2, 63, 2), steel)
    # Side actuator and base-mounted drive visible in the side reference.
    box((55, 180, 40), (32, 55, 36), dark)
    box((55, 212, 40), (36, 9, 40), silver)
    cylinder((55, 231, 40), 8, 28, steel)
    box((51, 32, 60), (36, 32, 49), dark)
    box((51, 32, 32), (40, 36, 7), silver)
    return faces
