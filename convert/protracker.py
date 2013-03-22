TONES = {
        856: 'C-1', 808: 'C#1', 762: 'D-1', 720: 'D#1', 678: 'E-1', 640: 'F-1', 604: 'F#1', 570: 'G-1', 538: 'G#1', 508: 'A-1', 480: 'A#1', 453: 'B-1',
        428: 'C-2', 404: 'C#2', 381: 'D-2', 360: 'D#2', 339: 'E-2', 320: 'F-2', 302: 'F#2', 285: 'G-2', 269: 'G#2', 254: 'A-2', 240: 'A#2', 226: 'B-2',
        214: 'C-3', 202: 'C#3', 190: 'D-3', 180: 'D#3', 170: 'E-3', 160: 'F-3', 151: 'F#3', 143: 'G-3', 135: 'G#3', 127: 'A-3', 120: 'A#3', 113: 'B-3'
}

TONES_OPPOSITE = {}
for x in TONES:
    TONES_OPPOSITE[TONES[x]] = x

class ModuleReader():
    def __init__(self, data=''):
        self.data = data
        self.fp = 0

    def read8(self):
        byte = ord(self.data[self.fp])
        self.fp += 1
        return byte

    def read16(self):
        d = self.read8() << 8
        d |= self.read8()
        return d

    def read_string(self, length):
        s = ''
        for i in range(length):
            s += self.data[self.fp]
            self.fp += 1
        s = s.strip('\x00')
        return s

    def read_sample(self):
        name = self.read_string(22)
        length = self.read16()
        finetune = ((self.read8() ^ 0xf) + 1) & 0xf
        volume = self.read8()
        repeat_offset = self.read16()
        repeat_length = self.read16()
        return {'name': name, 'length': length, 'finetune': finetune, 'volume': volume, 'repeat_offset': repeat_offset, 'repeat_length': repeat_length}

    def read_note(self):
        a = self.read8()
        b = self.read8()
        c = self.read8()
        d = self.read8()

        sample = (a & 0xf0) + (c >> 4)
        effect = hex(((c & 0xf) << 8) + d)[2:].upper()
        while len(effect) < 3:
            effect = '0' + effect
        note = ((a & 0xf) << 8) + b

        return {'note': TONES.get(note, '---'), 'sample': sample, 'effect': effect}

    def read_pattern(self):
        pattern = [[], [], [], []]
        for i in range(256):
            pattern[i & 3].append(self.read_note())
        return pattern

    def parse(self, data=None):
        if not data is None:
            self.data = data
            self.fp = 0

        r = {}
        r['name'] = self.read_string(20)
        r['samples'] = []
        for i in range(1, 32):
            r['samples'].append(self.read_sample())

        length = self.read8()

        self.read8()

        r['order_list'] = []
        for i in range(128):
            r['order_list'].append(self.read8())

        r['order_list'] = r['order_list'][:length]

        r['format'] = self.read_string(4)

        r['patterns'] = []
        for i in range(max(r['order_list']) + 1):
            r['patterns'].append(self.read_pattern())

        for s in r['samples']:
            sample_data = ''
            for i in range(s['length']):
                sample_data += chr(self.read8())
            s['data'] = sample_data
            del s['length']

        return r

if __name__ == '__main__':
    pattern = ModuleReader().parse(file('lompakko.mod').read())['patterns'][0]
    for i in range(64):
        s = '%02d\t' % i
        for x in range(4):
            s += '%s %02d %s\t' % (pattern[x][i]['note'], pattern[x][i]['sample'], pattern[x][i]['effect'])
        print s

