#!/usr/bin/env python3
"""Put a newly built player ROM into the Foundry page.
Usage: python3 embed-player.py foundry.html ngmc.zip neogeo.xml PLAYER_NUMBER out.html
(ngmc.zip and neogeo.xml are the FOUNDRY-INPUT pair from `make foundry-input`.)"""
import sys, re, json, base64
src, zp, xp, ver, out = sys.argv[1:6]
s = open(src, encoding='utf-8').read()
z = base64.b64encode(open(zp, 'rb').read()).decode()
x = open(xp, encoding='utf-8').read()
new = 'const PLAYER_ROM = { name: "ngmc.zip", version: %d, zip: "%s", xml: %s };' % (int(ver), z, json.dumps(x))
s2, n = re.subn(r'const PLAYER_ROM = \{.*?\};', lambda m: new, s, count=1, flags=re.S)
assert n == 1, 'PLAYER_ROM block not found'
open(out, 'w', encoding='utf-8').write(s2)
print('wrote', out, len(s2), 'bytes')
