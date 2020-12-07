#!/usr/bin/env python3

# -*- coding: utf8 -*-

import datetime

import sys
import random
import string

# MAX_VALUE_LEN = 2**64 - 1
MAX_VALUE = 18446744073709552000

def generate_random_value():
    return random.randint(0, MAX_VALUE)

def main():
    if len(sys.argv) < 3:
        print("Usage: {} <output dir> <rows> <cols>".format(sys.argv[0]))
        sys.exit(1)

    output_dir = sys.argv[1]
    rows = int(sys.argv[2])
    cols = int(sys.argv[3])

    output_filename = "big_test.t"
    with open( output_filename, 'w') as foutput :
        foutput.write("{} {}\n".format(rows, cols))
        for _ in range( 0, rows ) :
            for _ in range( 0, cols ) :
                value = generate_random_value()
                foutput.write("{} ".format(value))
            foutput.write("\n")

if __name__ == "__main__":
    main()
