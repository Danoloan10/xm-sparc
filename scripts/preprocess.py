#! /usr/bin/env python

import sys

def main(argv):
    buf = ''
    for line in sys.stdin.readlines():
        if line.count('=') > 0:
            b = line.strip('\n').split('=', 1)
            buf += '{0}=\"{1}\"\n'.format(b[0], ' '.join(b[1:]))
        else:
            buf += line
    print(buf)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
    
