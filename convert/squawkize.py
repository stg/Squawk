HIGH_SAMPLES = [3, 5, 7]

import protracker
import sys

class Squawkizer():
    def __init__(self):
        self.reader = protracker.ModuleReader()

    def render(self, data):
        song_data = self.reader.parse(data)
        patterns = song_data['patterns']
        order_list = song_data['order_list']

        # interleave channels
        data = []
        for pattern in patterns:
            for i in range(64):
                data.append(self.mangle(pattern[3][i]))
                data.append(self.mangle(pattern[2][i]))
                data.append(self.mangle(pattern[0][i], 1))
                data.append(self.mangle(pattern[1][i]))

        # Make a C array
        i = 0
        s = ''
        for row in data:
            if not i & 3:
                s += '\n  '
            s += row + ', '
            i += 1

        # print c arrays and order count
        print 'static const pattern_t pattern[] PROGMEM = {' + s[:-2] + '\n};'
        print 'static const uint8_t order[] PROGMEM = {' + ', '.join([str(x) for x in order_list]) + '};'
        print 'static uint8_t order_count = %d;' % len(order_list)

    def mangle(self, row, octavemod=0):
        try:
            row['note'] = row['note'][:2] + str(int(row['note'][2]) + octavemod)
        except ValueError:
            # empty step
            pass

        # Cxx Axy EAx EBx 5xx, 6xx volume scaling
        if row['effect'][0] in ['A', '5', '6']:
            x = int(row['effect'][1], 16) >> 1
            y = int(row['effect'][2], 16) >> 1
            row['effect'] = ('%s%x%x' % (row['effect'][0], x, y)).upper()
        elif row['effect'][:-1] in ['EB', 'EA']:
            x = int(row['effect'][2], 16) >> 1
            row['effect'] = row['effect'][:-1] + ('%x' % x).upper()
        elif row['effect'][0] == 'C':
            x = int(row['effect'][1:], 16) >> 1
            row['effect'] = ('C%02x' % x).upper()

        effect = '0x' + ('%03x' % int(row['effect'], 16)).upper()

        return '{ %s, %s }' % (row['note'].replace('-', '_').replace('#', 'X').replace('___', 'vvv'), effect)


sq = Squawkizer()

for arg in sys.argv[1:]:
    sq.render(file(arg).read())
