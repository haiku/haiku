# PNG files for kernel debugger fonts.
# This makes the fonts easier to modify if you decide to do so.
#
# Convert the font to hex bytes that can be added to the corresponding .cpp files
# in src/system/kernel/debug/font{,_big}.cpp
#
# This uses the convert command from imagemagic to:
# - Store black as 1 and white as 0
# - Flip the image left-to-right (that's how our console code expects it)
# - Convert it to an uncompressed bitmap without header
# Then it uses xxd to convert it to an hexdump in the correct endianness.
convert kdlbig.png -depth 1 -negate -flop GRAY:kdlbig
xxd -g 2 -c 16 kdlbig > kdlbig.hex

convert kdlsmall.png -depth 1 -negate -flop GRAY:kdlsmall
xxd -g 1  -c 12 kdlsmall > kdlsmall.hex
