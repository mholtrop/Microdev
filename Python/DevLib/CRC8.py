#!/usr/bin/env python
#
# This is a single function crc8 that computes the CRC-8 check
# for an 8-bit CRC with polynomial x8+x2+x+1
#
# See: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
#
def crc8(dat,tail=0):
    '''Caculcate the CRC8 for x8+x2+x+1 (10000111).
    parameters:
    ============
    dat  - input data as integer to calculate crc on. Expected 8 bits.
    tail - usually zero, but can be a crc to check. If correct return 0.

    returns: CRC code.'''

    generator = (0b100000111) & 0xFF   # Yes, this is 0x07
    crc=0

    if isinstance(dat,int):
        dats=[]
        while dat:
            dats.insert(0,dat&0xFF)  # We need high bytes to low bytes: insert 8 bits at [0]
            dat >>= 8                # Shift to the next 8 bits.
    elif isinstance(dat,list):
        dats=dat
    else:
        print("I do not understand the input:",dat)
        return(0)

    for d in dats:
        crc = crc ^ d

        for i in range(8):
            if crc & 0x80:
                crc = (((crc << 1)&0xFF) ^ generator)
            else:
                crc = ((crc << 1)&0xFF)

    return(crc)

#
# This algorithm is general but harder to use. It comes straight from
# https://en.wikipedia.org/wiki/Cyclic_redundancy_check
#
def crc_remainder(input_bitstring, polynomial_bitstring, initial_filler):
    '''
    Calculates the CRC remainder of a string of bits using a chosen polynomial.
    initial_filler should be '1' or '0'.
    '''
    polynomial_bitstring = polynomial_bitstring.lstrip('0')
    len_input = len(input_bitstring)
    initial_padding = initial_filler * (len(polynomial_bitstring) - 1)
    input_padded_array = list(input_bitstring + initial_padding)
    while '1' in input_padded_array[:len_input]:
        cur_shift = input_padded_array.index('1')
        for i in range(len(polynomial_bitstring)):
            input_padded_array[cur_shift + i] = str(int(polynomial_bitstring[i] != input_padded_array[cur_shift + i]))
    return ''.join(input_padded_array)[len_input:]

def crc_check(input_bitstring, polynomial_bitstring, check_value):
    '''
    Calculates the CRC check of a string of bits using a chosen polynomial.
    '''
    polynomial_bitstring = polynomial_bitstring.lstrip('0')
    len_input = len(input_bitstring)
    initial_padding = check_value
    input_padded_array = list(input_bitstring + initial_padding)
    while '1' in input_padded_array[:len_input]:
        cur_shift = input_padded_array.index('1')
        for i in range(len(polynomial_bitstring)):
            input_padded_array[cur_shift + i] = str(int(polynomial_bitstring[i] != input_padded_array[cur_shift + i]))
    return ('1' not in ''.join(input_padded_array)[len_input:])
