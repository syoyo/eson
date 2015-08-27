import eson
import struct

def packFloatList( floatList ):
    return struct.pack( '<%sf' % len( floatList ), *floatList )

d = {}

farr = [1.0, 2.0, 2.1, 3.0, 3.5]

d['aa'] = long(3)   # int() is still OK
d['bb'] = float(3)
d['cc'] = packFloatList(farr)

ed = eson.dumps(d)

f = open("test.eson", "wb")
f.write(ed)
f.close()
