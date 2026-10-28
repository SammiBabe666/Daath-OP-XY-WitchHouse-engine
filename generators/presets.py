"""Writes presets/<name>.json from definition.json's presets. Run from the repository root:
    python generators/presets.py .
Sigil becomes default.json, the machine's init preset."""
import json, sys, copy, os
root = sys.argv[1] if len(sys.argv) > 1 else '.'
# A version 4 OP-XY preset, as the device saves one; engine params are filled in below.
TEMPLATE = json.loads('{"engine": {"bendrange": 8191, "highpass": 0, "modulation": {"aftertouch": {"amount": 16384, "target": 0}, "modwheel": {"amount": 16384, "target": 0}, "pitchbend": {"amount": 16384, "target": 0}, "velocity": {"amount": 16384, "target": 0}}, "params": [0, 0, 0, 0, 0, 0, 0, 0], "playmode": "poly", "portamento.amount": 0, "portamento.type": 0, "transpose": 0, "tuning.root": 0, "tuning.scale": 0, "velocity.sensitivity": 0, "volume": 16384, "width": 0}, "envelope": {"amp": {"attack": 0, "decay": 8192, "release": 6000, "sustain": 32767}, "filter": {"attack": 0, "decay": 0, "release": 0, "sustain": 32767}}, "fx": {"active": false, "params": [0, 0, 0, 0, 0, 32767, 0, 0], "type": "svf"}, "lfo": {"active": false, "params": [0, 0, 0, 0, 0, 0, 0, 0], "type": "value"}, "octave": 0, "platform": "OP-XY", "regions": [], "type": "daath", "version": 4}')
d = json.load(open(os.path.join(root, 'definition.json')))
ids = [p['id'] for p in d['parameters']]
# Bass envelopes: instant attack, full sustain; longer release where the sound drones.
release = {'Sigil': 6000, 'Crypt': 9000, 'Coven': 9000, 'Screwed': 12000, 'Hexed': 5000, 'Rotten': 6000}
names = {'Sigil': 'default'}
os.makedirs(os.path.join(root, 'presets'), exist_ok=True)
for name, values in d['presets'].items():
    p = copy.deepcopy(TEMPLATE)
    p['type'] = d['id']
    p['engine']['params'] = [round(values[i] * 32767) for i in ids] + [0] * (8 - len(ids))
    p['envelope']['amp'] = {'attack': 0, 'decay': 8192, 'release': release[name], 'sustain': 32767}
    path = os.path.join(root, 'presets', names.get(name, name.lower()) + '.json')
    with open(path, 'w') as f:
        json.dump(p, f, indent=2, sort_keys=True)
        f.write('\n')
