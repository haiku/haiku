# Utility functions to aid in converting from the special data types the HVIF
# (aka flat icon format) has

import math

# Example usage:
# >>> from FlatIconFormat import *
# >>> read_float_24(0x3ffffd)
def read_float_24(value):
	sign = (-1) ** ((value & 0x800000) >> 23)
	exponent = 2 ** (((value & 0x7e0000) >> 17) - 32)
	mantissa = (value & 0x01ffff) * 2 ** -17
	return sign * exponent * (1 + mantissa)

# Example usage:
# >>> hex(write_float_24(0.9999885559082031))
#
# Does not perform bounds checking. Do not input numbers that are too large.
def write_float_24(value):
	# TODO: does not differentiate between 0.0 and -0.0
	if value >= 0:
		sign = 0
	else:
		sign = 1

	if value != 0:
		# TODO: make sure exponent fits in 6 bits
		exponent = math.floor(math.log2(abs(value)))
	else:
		exponent = -32

	if value != 0:
		mantissa = abs(value) / 2**exponent - 1
	else:
		mantissa = 0

	return (sign << 23) + ((exponent+32) << 17) + math.floor(mantissa * 2 ** 17)
