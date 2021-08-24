SystemPalette class
###################

This object does all the handling for system attribute colors and system
palette management.

Member Functions
================

_ Denotes a protected function

SystemPalette(void)
-------------------

1. Allocate the rgb_color[256] palette on the heap and call _GenerateSystemPalette()
2. Initialize attribute variables to the defaults

~SystemPalette(void)
--------------------

1) Free the palette array

void SetPalette(uint8 index, RGBColor col), void SetPalette(uint8 index, rgb_color col)
---------------------------------------------------------------------------------------

Sets the said index to the passed color value.

RGBColor GetPalette(uint8 index)
--------------------------------

Returns the color at said index in the palette.

void SetGUIColor(color_which which, RGBColor col)
-------------------------------------------------

RGBColor GetGUIColor(color_which which)
---------------------------------------

color_set GetGUIColors(void)
----------------------------

void SetGUIColors(color_set cset)
---------------------------------

These tweak or return the system attribute colors, one at a time or all at once.

protected: void \_GenerateSystemPalette(rgb_color \*palette)
------------------------------------------------------------

Sets the passed palette to the BeOS R5 system colors, which follows.

Grays:
......

0,0,0 -> 248,248,248 by increments of 8

Blues:
......

0,0,255

0,0,229

0,0,204

0,0,179

0,0,154

0,0,129

0,0,105

0,0,80

0,0,55

0,0,30

Reds:
.....

as per blues, but red values are 1 less

Greens:
.......

as per blues, but green values are 1 less

0,152,51

255,255,255

The following sets use [255, 203, 152, 102, 51, 0] for the blue values,
keeping the other colors the same:

203,255, [value]

152,255, [value]

102,255, [value]

51,255, [value]

255,152, [value]

0,102,255

0,102,203

203,203, [value]

152,255, [value]

102,255, [value]

51,255, [value]

255,102, [value]

0,102,152

0,102,102

203,152, [value]

152,152, [value]

102,152, [value]

51,152, [value]

230,134,0

255,51, [value excepting 255]

0,102,51

0,102,0

203,102, [value]

152,102, [value]

102,102, [value]

51,102, [value]

255,0, [value excepting 0]

255,175,19

0,51,255

0,51,203

203,51, [value]

152,51, [value]

102,51, [value]

51,51, [value]

255,203,102 -> 255,203,255, stepping in the [value] increments

0,51, [value, starting at 152]

203,0, [value, excepting 0]

255,227,70

152,0, [value]

102,0, [value]

51,0, [value]

255,203,51

255,203,0

255,255, [values in reverse]

protected: void _SetDefaultGUIColors(void)
-------------------------------------------

Sets the internal color_set to the defaults, which is the following:

- panel_background: 216,216, 216
- panel_text: 0,0,0
- document_background: 255,255,255
- document_text: 0,0,0
- control_background: 216,216,216
- control_text: 0,0,0
- control_border: 0,0,0
- control_highlight: 0,0,255
- tooltip_background:
- tooltip_text: 0,0,0
- menu_background: 216,216,216
- menu_selected_background: 160,160,160
- menu_text: 0,0,0
- menu_selected_text: 0,0,0
- menu_separator_high: 241,241,241
- menu_separator_low: 186,186,186
- menu_triggers: 0,0,0

Structures
==========

.. code-block:: cpp

    color_set {
        rgb_color panel_background
        rgb_color panel_text
        rgb_color document_background
        rgb_color document_text
        rgb_color control_background
        rgb_color control_text
        rgb_color control_border
        rgb_color control_highlight
        rgb_color tooltip_background
        rgb_color tooltip_text
        rgb_color menu_background
        rgb_color menu_selected_background
        rgb_color menu_text
        rgb_color menu_selected_text
        rgb_color menu_separator
        rgb_color menu_triggers
    }

