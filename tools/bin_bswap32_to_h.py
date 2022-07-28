#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import sys
import os

def trans(data, var_name : str, file_timestamp : int) -> str:
    '''big end to little end and trans to the C header'''

    out = []
    if not var_name:
        var_name = 'ap8288_bin'
    # Header File Safeguard
    out.append('#ifndef __{var_name}_H__'.format(var_name = var_name.upper()))
    out.append('#define __{var_name}_H__'.format(var_name = var_name.upper()))
    out.append('')
    # file modify timestamp
    if file_timestamp:
        line = 'const unsigned int {var_name}_timestamp = {timestamp:d};'
        out.append(line.format(var_name = var_name, timestamp = file_timestamp))
    # bin file size
    data_len = len(data)
    out.append('const unsigned int {var_name}_len = {data_len};'.format(
        var_name = var_name, data_len = data_len))
    # bin to hex
    out.append('const unsigned char {var_name}[] = {{'.format(var_name = var_name))
    data = data + b'\xff\xff\xff\xff'
    data_len = (data_len + 3) & (~3)
    tmp = [b'0' for x in range(0, 4)]
    line = []
    for i in range(0, data_len, 4):
        tmp[3] = data[i]
        tmp[2] = data[i + 1]
        tmp[1] = data[i + 2]
        tmp[0] = data[i + 3]

        if (i > 0) and ((i % 16) == 0):
            # new line
            out.append('    ' + ''.join(line))
            line = []
        for j in range(0, 4):
            line.append('0x{val:02X}, '.format(val = tmp[j]))
            # line.append(str.format('0x%02X, ' % tmp[j]))
    # last line
    if len(line) > 0:
        out.append('    ' + ''.join(line))

    out.append('};\n')
    out.append('#endif // end of __{var_name}_H__'.format(var_name = var_name.upper()))
    out.append('')
    return '\n'.join(out)

def main():
    ret = 1
    parser = argparse.ArgumentParser(description='Generate binary header output')
    parser.add_argument('-i', '--input', required=True, help='Input file')
    parser.add_argument('-o', '--out', required=True, help='Output file')
    parser.add_argument('-v', '--var', required=False, help='Variable name to use in file')

    args = parser.parse_args()
    if not args:
        return ret

    if os.path.exists(args.input) and os.path.isfile(args.input):
        ret = 0
        with open(args.input, 'rb') as f:
            data = f.read()

        out = trans(data, args.var, int(os.stat(args.input).st_mtime))
        if len(out) > 0:
            with open(args.out, 'w') as f:
                f.write(out)

    return ret


if __name__ == '__main__':
    sys.exit(main())