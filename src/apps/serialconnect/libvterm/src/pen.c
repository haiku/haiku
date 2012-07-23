#include "vterm_internal.h"

#include <stdio.h>

static const VTermColor ansi_colors[] = {
  /* R    G    B */
  {   0,   0,   0 }, // black
  { 224,   0,   0 }, // red
  {   0, 224,   0 }, // green
  { 224, 224,   0 }, // yellow
  {   0,   0, 224 }, // blue
  { 224,   0, 224 }, // magenta
  {   0, 224, 224 }, // cyan
  { 224, 224, 224 }, // white == light grey

  // high intensity
  { 128, 128, 128 }, // black
  { 255,  64,  64 }, // red
  {  64, 255,  64 }, // green
  { 255, 255,  64 }, // yellow
  {  64,  64, 255 }, // blue
  { 255,  64, 255 }, // magenta
  {  64, 255, 255 }, // cyan
  { 255, 255, 255 }, // white for real
};

static int ramp6[] = {
  0x00, 0x33, 0x66, 0x99, 0xCC, 0xFF,
};

static int ramp24[] = {
  0x00, 0x0B, 0x16, 0x21, 0x2C, 0x37, 0x42, 0x4D, 0x58, 0x63, 0x6E, 0x79,
  0x85, 0x90, 0x9B, 0xA6, 0xB1, 0xBC, 0xC7, 0xD2, 0xDD, 0xE8, 0xF3, 0xFF,
};

static void lookup_colour_ansi(long index, char is_bg, VTermColor *col)
{
  if(index >= 0 && index < 16) {
    *col = ansi_colors[index];
  }
}

static int lookup_colour(int palette, const long args[], int argcount, char is_bg, VTermColor *col)
{
  long index;

  switch(palette) {
  case 2: // RGB mode - 3 args contain colour values directly
    if(argcount < 3)
      return argcount;

    col->red   = CSI_ARG(args[0]);
    col->green = CSI_ARG(args[1]);
    col->blue  = CSI_ARG(args[2]);

    return 3;

  case 5: // XTerm 256-colour mode
    index = argcount ? CSI_ARG_OR(args[0], -1) : -1;

    if(index >= 0 && index < 16) {
      // Normal 8 colours or high intensity - parse as palette 0
      lookup_colour_ansi(index, is_bg, col);
    }
    else if(index >= 16 && index < 232) {
      // 216-colour cube
      index -= 16;

      col->blue  = ramp6[index     % 6];
      col->green = ramp6[index/6   % 6];
      col->red   = ramp6[index/6/6 % 6];
    }
    else if(index >= 232 && index < 256) {
      // 24 greyscales
      index -= 232;

      col->red   = ramp24[index];
      col->green = ramp24[index];
      col->blue  = ramp24[index];
    }

    return argcount ? 1 : 0;

  default:
    fprintf(stderr, "Unrecognised colour palette %d\n", palette);
    return 0;
  }
}

// Some conveniences

static void setpenattr(VTermState *state, VTermAttr attr, VTermValueType type, VTermValue *val)
{
#ifdef DEBUG
  if(type != vterm_get_attr_type(attr)) {
    fprintf(stderr, "Cannot set attr %d as it has type %d, not type %d\n",
        attr, vterm_get_attr_type(attr), type);
    return;
  }
#endif
  if(state->callbacks && state->callbacks->setpenattr)
    (*state->callbacks->setpenattr)(attr, val, state->cbdata);
}

static void setpenattr_bool(VTermState *state, VTermAttr attr, int boolean)
{
  VTermValue val = { .boolean = boolean };
  setpenattr(state, attr, VTERM_VALUETYPE_BOOL, &val);
}

static void setpenattr_int(VTermState *state, VTermAttr attr, int number)
{
  VTermValue val = { .number = number };
  setpenattr(state, attr, VTERM_VALUETYPE_INT, &val);
}

static void setpenattr_col(VTermState *state, VTermAttr attr, VTermColor color)
{
  VTermValue val = { .color = color };
  setpenattr(state, attr, VTERM_VALUETYPE_COLOR, &val);
}

static void set_pen_col_ansi(VTermState *state, VTermAttr attr, long col)
{
  VTermColor *colp = (attr == VTERM_ATTR_BACKGROUND) ? &state->pen.bg : &state->pen.fg;

  lookup_colour_ansi(col, attr == VTERM_ATTR_BACKGROUND, colp);

  setpenattr_col(state, attr, *colp);
}

void vterm_state_resetpen(VTermState *state)
{
  state->pen.bold = 0;      setpenattr_bool(state, VTERM_ATTR_BOLD, 0);
  state->pen.underline = 0; setpenattr_int( state, VTERM_ATTR_UNDERLINE, 0);
  state->pen.italic = 0;    setpenattr_bool(state, VTERM_ATTR_ITALIC, 0);
  state->pen.blink = 0;     setpenattr_bool(state, VTERM_ATTR_BLINK, 0);
  state->pen.reverse = 0;   setpenattr_bool(state, VTERM_ATTR_REVERSE, 0);
  state->pen.strike = 0;    setpenattr_bool(state, VTERM_ATTR_STRIKE, 0);
  state->pen.font = 0;      setpenattr_int( state, VTERM_ATTR_FONT, 0);

  state->fg_ansi = -1;
  state->pen.fg = state->default_fg;  setpenattr_col(state, VTERM_ATTR_FOREGROUND, state->default_fg);
  state->pen.bg = state->default_bg;  setpenattr_col(state, VTERM_ATTR_BACKGROUND, state->default_bg);
}

void vterm_state_savepen(VTermState *state, int save)
{
  if(save) {
    state->saved.pen = state->pen;
  }
  else {
    state->pen = state->saved.pen;

    setpenattr_bool(state, VTERM_ATTR_BOLD,       state->pen.bold);
    setpenattr_int( state, VTERM_ATTR_UNDERLINE,  state->pen.underline);
    setpenattr_bool(state, VTERM_ATTR_ITALIC,     state->pen.italic);
    setpenattr_bool(state, VTERM_ATTR_BLINK,      state->pen.blink);
    setpenattr_bool(state, VTERM_ATTR_REVERSE,    state->pen.reverse);
    setpenattr_bool(state, VTERM_ATTR_STRIKE,     state->pen.strike);
    setpenattr_int( state, VTERM_ATTR_FONT,       state->pen.font);
    setpenattr_col( state, VTERM_ATTR_FOREGROUND, state->pen.fg);
    setpenattr_col( state, VTERM_ATTR_BACKGROUND, state->pen.bg);
  }
}

void vterm_state_set_default_colors(VTermState *state, VTermColor *default_fg, VTermColor *default_bg)
{
  state->default_fg = *default_fg;
  state->default_bg = *default_bg;
}

void vterm_state_set_bold_highbright(VTermState *state, int bold_is_highbright)
{
  state->bold_is_highbright = bold_is_highbright;
}

void vterm_state_setpen(VTermState *state, const long args[], int argcount)
{
  // SGR - ECMA-48 8.3.117

  int argi = 0;
  int value;

  while(argi < argcount) {
    // This logic is easier to do 'done' backwards; set it true, and make it
    // false again in the 'default' case
    int done = 1;

    long arg;
    switch(arg = CSI_ARG(args[argi])) {
    case CSI_ARG_MISSING:
    case 0: // Reset
      vterm_state_resetpen(state);
      break;

    case 1: // Bold on
      state->pen.bold = 1;
      setpenattr_bool(state, VTERM_ATTR_BOLD, 1);
      if(state->fg_ansi > -1 && state->bold_is_highbright)
        set_pen_col_ansi(state, VTERM_ATTR_FOREGROUND, state->fg_ansi + (state->pen.bold ? 8 : 0));
      break;

    case 3: // Italic on
      state->pen.italic = 1;
      setpenattr_bool(state, VTERM_ATTR_ITALIC, 1);
      break;

    case 4: // Underline single
      state->pen.underline = 1;
      setpenattr_int(state, VTERM_ATTR_UNDERLINE, 1);
      break;

    case 5: // Blink
      state->pen.blink = 1;
      setpenattr_bool(state, VTERM_ATTR_BLINK, 1);
      break;

    case 7: // Reverse on
      state->pen.reverse = 1;
      setpenattr_bool(state, VTERM_ATTR_REVERSE, 1);
      break;

    case 9: // Strikethrough on
      state->pen.strike = 1;
      setpenattr_bool(state, VTERM_ATTR_STRIKE, 1);
      break;

    case 10: case 11: case 12: case 13: case 14:
    case 15: case 16: case 17: case 18: case 19: // Select font
      state->pen.font = CSI_ARG(args[argi]) - 10;
      setpenattr_int(state, VTERM_ATTR_FONT, state->pen.font);
      break;

    case 21: // Underline double
      state->pen.underline = 2;
      setpenattr_int(state, VTERM_ATTR_UNDERLINE, 2);
      break;

    case 22: // Bold off
      state->pen.bold = 0;
      setpenattr_bool(state, VTERM_ATTR_BOLD, 0);
      break;

    case 23: // Italic and Gothic (currently unsupported) off
      state->pen.italic = 0;
      setpenattr_bool(state, VTERM_ATTR_ITALIC, 0);
      break;

    case 24: // Underline off
      state->pen.underline = 0;
      setpenattr_int(state, VTERM_ATTR_UNDERLINE, 0);
      break;

    case 25: // Blink off
      state->pen.blink = 0;
      setpenattr_bool(state, VTERM_ATTR_BLINK, 0);
      break;

    case 27: // Reverse off
      state->pen.reverse = 0;
      setpenattr_bool(state, VTERM_ATTR_REVERSE, 0);
      break;

    case 29: // Strikethrough off
      state->pen.strike = 0;
      setpenattr_bool(state, VTERM_ATTR_STRIKE, 0);
      break;

    case 30: case 31: case 32: case 33:
    case 34: case 35: case 36: case 37: // Foreground colour palette
      value = CSI_ARG(args[argi]) - 30;
      state->fg_ansi = value;
      if(state->pen.bold && state->bold_is_highbright)
        value += 8;
      set_pen_col_ansi(state, VTERM_ATTR_FOREGROUND, value);
      break;

    case 38: // Foreground colour alternative palette
      state->fg_ansi = -1;
      if(argcount - argi < 1)
        return;
      argi += 1 + lookup_colour(CSI_ARG(args[argi+1]), args+argi+2, argcount-argi-2, 0, &state->pen.fg);
      setpenattr_col(state, VTERM_ATTR_FOREGROUND, state->pen.fg);
      break;

    case 39: // Foreground colour default
      state->fg_ansi = -1;
      state->pen.fg = state->default_fg;
      setpenattr_col(state, VTERM_ATTR_FOREGROUND, state->pen.fg);
      break;

    case 40: case 41: case 42: case 43:
    case 44: case 45: case 46: case 47: // Background colour palette
      set_pen_col_ansi(state, VTERM_ATTR_BACKGROUND, CSI_ARG(args[argi]) - 40);
      break;

    case 48: // Background colour alternative palette
      if(argcount - argi < 1)
        return;
      argi += 1 + lookup_colour(CSI_ARG(args[argi+1]), args+argi+2, argcount-argi-2, 1, &state->pen.bg);
      setpenattr_col(state, VTERM_ATTR_BACKGROUND, state->pen.bg);
      break;

    case 49: // Default background
      state->pen.bg = state->default_bg;
      setpenattr_col(state, VTERM_ATTR_BACKGROUND, state->pen.bg);
      break;

    case 90: case 91: case 92: case 93:
    case 94: case 95: case 96: case 97: // Foreground colour high-intensity palette
      set_pen_col_ansi(state, VTERM_ATTR_FOREGROUND, CSI_ARG(args[argi]) - 90 + 8);
      break;

    case 100: case 101: case 102: case 103:
    case 104: case 105: case 106: case 107: // Background colour high-intensity palette
      set_pen_col_ansi(state, VTERM_ATTR_BACKGROUND, CSI_ARG(args[argi]) - 100 + 8);
      break;

    default:
      done = 0;
      break;
    }

    if(!done)
      fprintf(stderr, "libvterm: Unhandled CSI SGR %lu\n", arg);

    while(CSI_ARG_HAS_MORE(args[argi++]));
  }
}

int vterm_state_get_penattr(VTermState *state, VTermAttr attr, VTermValue *val)
{
  switch(attr) {
  case VTERM_ATTR_BOLD:
    val->boolean = state->pen.bold;
    return 1;

  case VTERM_ATTR_UNDERLINE:
    val->number = state->pen.underline;
    return 1;

  case VTERM_ATTR_ITALIC:
    val->boolean = state->pen.italic;
    return 1;

  case VTERM_ATTR_BLINK:
    val->boolean = state->pen.blink;
    return 1;

  case VTERM_ATTR_REVERSE:
    val->boolean = state->pen.reverse;
    return 1;

  case VTERM_ATTR_STRIKE:
    val->boolean = state->pen.strike;
    return 1;

  case VTERM_ATTR_FONT:
    val->number = state->pen.font;
    return 1;

  case VTERM_ATTR_FOREGROUND:
    val->color = state->pen.fg;
    return 1;

  case VTERM_ATTR_BACKGROUND:
    val->color = state->pen.bg;
    return 1;
  }

  return 0;
}
