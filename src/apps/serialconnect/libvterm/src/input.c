#include "vterm_internal.h"

#include <stdio.h>

#include "utf8.h"

void vterm_input_push_char(VTerm *vt, VTermModifier mod, uint32_t c)
{
  int needs_CSIu;
  /* The shift modifier is never important for Unicode characters
   * apart from Space
   */
  if(c != ' ')
    mod &= ~VTERM_MOD_SHIFT;
  /* However, since Shift-Space is too easy to mistype accidentally, remove
   * shift if it's the only modifier
   */
  else if(mod == VTERM_MOD_SHIFT)
    mod = 0;

  if(mod == 0) {
    // Normal text - ignore just shift
    char str[6];
    int seqlen = fill_utf8(c, str);
    vterm_push_output_bytes(vt, str, seqlen);
    return;
  }

  switch(c) {
    /* Special Ctrl- letters that can't be represented elsewise */
    case 'h': case 'i': case 'j': case 'm': case '[':
      needs_CSIu = 1;
      break;
    /* Ctrl-\ ] ^ _ don't need CSUu */
    case '\\': case ']': case '^': case '_':
      needs_CSIu = 0;
      break;
    /* All other characters needs CSIu except for letters a-z */
    default:
      needs_CSIu = (c < 'a' || c > 'z');
  }

  /* ALT we can just prefix with ESC; anything else requires CSI u */
  if(needs_CSIu && (mod & ~VTERM_MOD_ALT)) {
    vterm_push_output_sprintf(vt, "\e[%d;%du", c, mod+1);
    return;
  }

  if(mod & VTERM_MOD_CTRL)
    c &= 0x1f;

  vterm_push_output_sprintf(vt, "%s%c", mod & VTERM_MOD_ALT ? "\e" : "", c);
}

typedef struct {
  enum {
    KEYCODE_NONE,
    KEYCODE_LITERAL,
    KEYCODE_TAB,
    KEYCODE_ENTER,
    KEYCODE_CSI,
    KEYCODE_CSI_CURSOR,
    KEYCODE_CSINUM,
  } type;
  char literal;
  int csinum;
} keycodes_s;

keycodes_s keycodes[] = {
  { KEYCODE_NONE }, // NONE

  { KEYCODE_ENTER,   '\r'   }, // ENTER
  { KEYCODE_TAB,     '\t'   }, // TAB
  { KEYCODE_LITERAL, '\x7f' }, // BACKSPACE == ASCII DEL
  { KEYCODE_LITERAL, '\e'   }, // ESCAPE

  { KEYCODE_CSI_CURSOR, 'A' }, // UP
  { KEYCODE_CSI_CURSOR, 'B' }, // DOWN
  { KEYCODE_CSI_CURSOR, 'D' }, // LEFT
  { KEYCODE_CSI_CURSOR, 'C' }, // RIGHT

  { KEYCODE_CSINUM, '~', 2 },  // INS
  { KEYCODE_CSINUM, '~', 3 },  // DEL
  { KEYCODE_CSI_CURSOR, 'H' }, // HOME
  { KEYCODE_CSI_CURSOR, 'F' }, // END
  { KEYCODE_CSINUM, '~', 5 },  // PAGEUP
  { KEYCODE_CSINUM, '~', 6 },  // PAGEDOWN

  { KEYCODE_NONE },            // F0 - shouldn't happen
  { KEYCODE_CSI_CURSOR, 'P' }, // F1
  { KEYCODE_CSI_CURSOR, 'Q' }, // F2
  { KEYCODE_CSI_CURSOR, 'R' }, // F3
  { KEYCODE_CSI_CURSOR, 'S' }, // F4
  { KEYCODE_CSINUM, '~', 15 }, // F5
  { KEYCODE_CSINUM, '~', 17 }, // F6
  { KEYCODE_CSINUM, '~', 18 }, // F7
  { KEYCODE_CSINUM, '~', 19 }, // F8
  { KEYCODE_CSINUM, '~', 20 }, // F9
  { KEYCODE_CSINUM, '~', 21 }, // F10
  { KEYCODE_CSINUM, '~', 23 }, // F11
  { KEYCODE_CSINUM, '~', 24 }, // F12
};

void vterm_input_push_key(VTerm *vt, VTermModifier mod, VTermKey key)
{
  keycodes_s k;
  /* Since Shift-Enter and Shift-Backspace are too easy to mistype
   * accidentally, remove shift if it's the only modifier
   */
  if((key == VTERM_KEY_ENTER || key == VTERM_KEY_BACKSPACE) && mod == VTERM_MOD_SHIFT)
    mod = 0;

  if(key == VTERM_KEY_NONE || key >= VTERM_KEY_MAX)
    return;

  if(key >= sizeof(keycodes)/sizeof(keycodes[0]))
    return;

  k = keycodes[key];

  switch(k.type) {
  case KEYCODE_NONE:
    break;

  case KEYCODE_TAB:
    /* Shift-Tab is CSI Z but plain Tab is 0x09 */
    if(mod == VTERM_MOD_SHIFT)
      vterm_push_output_sprintf(vt, "\e[Z");
    else if(mod & VTERM_MOD_SHIFT)
      vterm_push_output_sprintf(vt, "\e[1;%dZ", mod+1);
    else
      goto literal;
    break;

  case KEYCODE_ENTER:
    /* Enter is CRLF in newline mode, but just LF in linefeed */
    if(vt->state->mode.newline)
      vterm_push_output_sprintf(vt, "\r\n");
    else
      goto literal;
    break;

literal:
  case KEYCODE_LITERAL:
    if(mod & (VTERM_MOD_SHIFT|VTERM_MOD_CTRL))
      vterm_push_output_sprintf(vt, "\e[%d;%du", k.literal, mod+1);
    else
      vterm_push_output_sprintf(vt, mod & VTERM_MOD_ALT ? "\e%c" : "%c", k.literal);
    break;

  case KEYCODE_CSI_CURSOR:
    if(vt->state->mode.cursor && mod == 0) {
      vterm_push_output_sprintf(vt, "\eO%c", k.literal);
      break;
    }
    /* else FALLTHROUGH */
  case KEYCODE_CSI:
    if(mod == 0)
      vterm_push_output_sprintf(vt, "\e[%c", k.literal);
    else
      vterm_push_output_sprintf(vt, "\e[1;%d%c", mod + 1, k.literal);
    break;

  case KEYCODE_CSINUM:
    if(mod == 0)
      vterm_push_output_sprintf(vt, "\e[%d%c", k.csinum, k.literal);
    else
      vterm_push_output_sprintf(vt, "\e[%d;%d%c", k.csinum, mod + 1, k.literal);
    break;
  }
}
