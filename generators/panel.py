# Generates panel.svg.fragment, keeping the SDK two-page header block. Run from the repository root: python generators/panel.py .
import math, re, sys
root = sys.argv[1]
old = open(root + '/panel.svg.fragment').read().splitlines()
header = []
for line in old[1:]:
    if 'daath_text_15' in line:
        header.append(line); break
    header.append(line)
# The SDK header bars move from nodes 2..9 to 4..11 (after four outlines).
if not any('daath_node_11' in l for l in header):
    header = [re.sub(r'daath_node_(\d+)', lambda m: 'daath_node_%d' % (int(m.group(1)) + 2), l) for l in header]

def n(v): 
    s = ('%.4f' % v).rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s
def path(points, close=True):
    d = 'M' + ' L'.join('%s,%s' % (n(x), n(y)) for x, y in points)
    return d + (' Z' if close else '')
def rect(x, y, w, h, fill, stroke, width):
    return '<path d="M%s,%s L%s,%s L%s,%s L%s,%s Z" fill="%s" stroke="%s" stroke-width="%s"/>' % (
        n(x), n(y), n(x + w), n(y), n(x + w), n(y + h), n(x), n(y + h), fill, stroke, n(width))
def mat(x, y): return 'matrix(1 0 0 1 %s %s)' % (n(x), n(y))

# Monochrome: page tones 1..6, ink 7..8, sigil tones 9..12, flame 13..14, moon 15.
pal = ['#000000', '#0E0E0E', '#1E1E1E', '#343434', '#5A5A5A', '#868686', '#B4B4B4', '#141414',
       '#6A6A6A', '#444444', '#161616', '#D8D8D8', '#FFFFFF', '#8A8A8A', '#F2F2F2', '#E8E8E8']
out = ['<g id="daath_panel">'] + header
add = out.append
SX, SY = 337, 129
# Leather cover and page block shadow (static).
add('<path d="M43,43 L437,43 L442,48 L442,213 L437,218 L43,218 L38,213 L38,48 Z" fill="#1C1C1C" stroke="#3A3A3A" stroke-width="1.5"/>')
add(rect(46, 50, 388, 164, '#0A0A0A', '#0A0A0A', 1))
# Pages (bound: Murk darkens them).
add('<path id="daath_node_12" d="M50,54 L95,50 L140,48.5 L190,50 L236,55 L236,209 L190,205 L140,203.5 L95,205 L50,209 Z" fill="#B4B4B4" stroke="#343434" stroke-width="1"/>')
add('<path id="daath_node_13" d="M244,55 L290,50 L340,48.5 L385,50 L430,54 L430,209 L385,205 L340,203.5 L290,205 L244,209 Z" fill="#B4B4B4" stroke="#343434" stroke-width="1"/>')
# Gutter and stitching (static).
add(rect(236, 53, 8, 158, '#161616', '#161616', 0.5))
add(rect(239.4, 53, 1.2, 158, '#000000', '#000000', 0.3))
# Ribbon bookmark (static).
add('<path d="M296,203 L304,203 L304,219 L300,215 L296,219 Z" fill="#4A4A4A" stroke="#262626" stroke-width="0.8"/>')
# Tears (bound, appear with Rot): authored in place.
tears = ['M50,86 L59,91 L50,97 Z', 'M430,148 L420,154 L430,161 Z',
         'M113,49.7 L120,58 L127,49.1 Z', 'M378,205.6 L385,195 L392,206 Z']
for i, d in enumerate(tears):
    add('<path id="daath_node_%d" transform="%s" d="%s" fill="#000000" stroke="#000000" stroke-width="0.8"/>' % (19 + i, mat(640, 320), d))
# Runes: eight closed glyphs, forty placements (bound).
glyphs = ['M0,-5 L4,4 L-4,4 Z', 'M0,-5 L3.5,0 L0,5 L-3.5,0 Z',
          'M0,-5 L4,-1 L1,-1 L1,5 L-1,5 L-1,-1 L-4,-1 Z', 'M-4,-5 L4,-5 L-4,5 L4,5 Z',
          'M-4,-4 L4,-4 L0,5 Z', 'M-1,-5 L1,-5 L1,-1 L4,1 L1,2 L1,5 L-1,5 Z',
          'M-4,-4 L4,4 L4,-4 L-4,4 Z', 'M-4,-4 L0,0 L4,-4 L4,-1 L0,4 L-4,-1 Z']
for k in range(40):
    r, c = divmod(k, 8)
    g = glyphs[(k * 5 + r * 3) % 8]
    add('<path id="daath_node_%d" transform="%s" d="%s" fill="none" stroke="#141414" stroke-width="1.1" stroke-linejoin="round"/>'
        % (23 + k, mat(72 + 19 * c, 122 + 14 * r), g))
# Moon (mutable outline 1).
def moon(grave):
    bulge = 0.83 - grave * 1.83
    pts = []
    for i in range(32):
        t = math.radians(-90 + 180 * (2 * i + 1) / 64)
        pts.append((22 * math.cos(t), 22 * math.sin(t)))
    for i in range(32):
        t = math.radians(90 - 180 * (2 * i + 1) / 64)
        pts.append((22 * bulge * math.cos(t), 22 * math.sin(t)))
    return pts
add('<path id="daath_node_1" transform="%s" d="%s" fill="none" stroke="#E8E8E8" stroke-width="1.4" stroke-linejoin="round"/>' % (mat(142, 82), path(moon(0.5))))
# Halo behind the sigil (bound, filled).
add('<circle id="daath_node_18" transform="%s" cx="0" cy="0" r="40" fill="#444444" stroke="#444444" stroke-width="1"/>' % mat(640, 320))
# Summoning ring (mutable outline 0).
ring = [(50 * math.cos(2 * math.pi * i / 64), 50 * math.sin(2 * math.pi * i / 64)) for i in range(64)]
add('<path id="daath_node_0" transform="%s" d="%s" fill="none" stroke="#161616" stroke-width="1.4" stroke-linejoin="round"/>' % (mat(SX, SY), path(ring)))
# Pentagrams: two ghosts under the centre sigil (bound, stroked).
star = [(30 * math.cos(math.radians(-90 + 144 * k)), 30 * math.sin(math.radians(-90 + 144 * k))) for k in range(5)]
for node, colour, width in ((15, '#444444', 1.2), (16, '#444444', 1.2), (14, '#161616', 1.6)):
    add('<path id="daath_node_%d" transform="%s" d="%s" fill="none" stroke="%s" stroke-width="%s" stroke-linejoin="round"/>' % (node, mat(SX, SY), path(star), colour, width))
add('<circle id="daath_node_17" transform="%s" cx="0" cy="0" r="33" fill="none" stroke="#161616" stroke-width="1.2"/>' % mat(SX, SY))
# Candles (static wax, bound flames).
for x in (22, 458):
    add('<path d="M%s,150 L%s,150 L%s,208 L%s,208 Z" fill="#CFCFCF" stroke="#727272" stroke-width="0.8"/>' % (n(x - 6), n(x + 6), n(x + 6), n(x - 6)))
    add('<path d="M%s,150 L%s,158 L%s,158 Z" fill="#E8E8E8" stroke="#E8E8E8" stroke-width="0.5"/>' % (n(x + 2), n(x + 4.5), n(x + 1.5)))
    add(rect(x - 10, 208, 20, 4, '#363636', '#202020', 0.6))
    add(rect(x - 0.5, 146, 1, 4, '#101010', '#101010', 0.4))
def flame(h, w):
    pts = []
    for i in range(64):
        a = 2 * math.pi * i / 64
        s, c = math.sin(a), math.cos(a)
        pts.append((w * s * (1 - c / 2), -h * c) if c > 0 else (w * s, -w * c))
    return pts
for node, x in ((2, 22), (3, 458)):
    add('<path id="daath_node_%d" transform="%s" d="%s" fill="none" stroke="#8A8A8A" stroke-width="1.3" stroke-linejoin="round"/>' % (node, mat(x, 147), path(flame(10, 5))))
for i, colour in enumerate(pal):
    add('<circle id="daath_palette_%d" cx="640" cy="0" r="1" fill="%s" stroke="%s" stroke-width="1.4"/>' % (i, colour, colour))
add('</g>')
open(root + '/panel.svg.fragment', 'w').write('\n'.join(out) + '\n')
print(len(out), 'lines')
