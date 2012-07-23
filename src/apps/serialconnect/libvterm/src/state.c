#include "vterm_internal.h"

#include <stdio.h>
#include <string.h>

#define strneq(a,b,n) (strncmp(a,b,n)==0)

#include "utf8.h"

#ifdef DEBUG
# define DEBUG_GLYPH_COMBINE
#endif

#define MOUSE_WANT_DRAG 0x01
#define MOUSE_WANT_MOVE 0x02

/* Some convenient wrappers to make callback functions easier */

static void putglyph(VTermState *state, const uint32_t chars[], int width, VTermPos pos)
{
  if(state->callbacks && state->callbacks->putglyph)
    if((*state->callbacks->putglyph)(chars, width, pos, state->cbdata))
      return;

  fprintf(stderr, "libvterm: Unhandled putglyph U+%04x at (%d,%d)\n", chars[0], pos.col, pos.row);
}

static void updatecursor(VTermState *state, VTermPos *oldpos, int cancel_phantom)
{
  if(state->pos.col == oldpos->col && state->pos.row == oldpos->row)
    return;

  if(cancel_phantom)
    state->at_phantom = 0;

  if(state->callbacks && state->callbacks->movecursor)
    if((*state->callbacks->movecursor)(state->pos, *oldpos, state->mode.cursor_visible, state->cbdata))
      return;
}

static void erase(VTermState *state, VTermRect rect)
{
  if(state->callbacks && state->callbacks->erase)
    if((*state->callbacks->erase)(rect, state->cbdata))
      return;
}

static VTermState *vterm_state_new(VTerm *vt)
{
  VTermState *state = vterm_allocator_malloc(vt, sizeof(VTermState));

  state->vt = vt;

  state->rows = vt->rows;
  state->cols = vt->cols;

  // 90% grey so that pure white is brighter
  state->default_fg.red = state->default_fg.green = state->default_fg.blue = 240;
  state->default_bg.red = state->default_bg.green = state->default_bg.blue = 0;

  state->bold_is_highbright = 0;

  return state;
}

void vterm_state_free(VTermState *state)
{
  vterm_allocator_free(state->vt, state->combine_chars);
  vterm_allocator_free(state->vt, state);
}

static void scroll(VTermState *state, VTermRect rect, int downward, int rightward)
{
  if(!downward && !rightward)
    return;

  if(state->callbacks && state->callbacks->scrollrect)
    if((*state->callbacks->scrollrect)(rect, downward, rightward, state->cbdata))
      return;

  if(state->callbacks)
    vterm_scroll_rect(rect, downward, rightward,
        state->callbacks->moverect, state->callbacks->erase, state->cbdata);
}

static void linefeed(VTermState *state)
{
  if(state->pos.row == SCROLLREGION_END(state) - 1) {
    VTermRect rect = {
      .start_row = state->scrollregion_start,
      .end_row   = SCROLLREGION_END(state),
      .start_col = 0,
      .end_col   = state->cols,
    };

    scroll(state, rect, 1, 0);
  }
  else if(state->pos.row < state->rows-1)
    state->pos.row++;
}

static void grow_combine_buffer(VTermState *state)
{
  size_t    new_size = state->combine_chars_size * 2;
  uint32_t *new_chars = vterm_allocator_malloc(state->vt, new_size * sizeof(new_chars[0]));

  memcpy(new_chars, state->combine_chars, state->combine_chars_size * sizeof(new_chars[0]));

  vterm_allocator_free(state->vt, state->combine_chars);
  state->combine_chars = new_chars;
}

static void set_col_tabstop(VTermState *state, int col)
{
  unsigned char mask = 1 << (col & 7);
  state->tabstops[col >> 3] |= mask;
}

static void clear_col_tabstop(VTermState *state, int col)
{
  unsigned char mask = 1 << (col & 7);
  state->tabstops[col >> 3] &= ~mask;
}

static int is_col_tabstop(VTermState *state, int col)
{
  unsigned char mask = 1 << (col & 7);
  return state->tabstops[col >> 3] & mask;
}

static void tab(VTermState *state, int count, int direction)
{
  while(count--)
    while(state->pos.col >= 0 && state->pos.col < state->cols-1) {
      state->pos.col += direction;

      if(is_col_tabstop(state, state->pos.col))
        break;
    }
}

static int on_text(const char bytes[], size_t len, void *user)
{
  VTermState *state = user;
  uint32_t* chars;

  VTermPos oldpos = state->pos;

  // We'll have at most len codepoints
  uint32_t codepoints[len];
  int npoints = 0;
  size_t eaten = 0;
  int i = 0;

  VTermEncodingInstance *encoding =
    !(bytes[eaten] & 0x80) ? &state->encoding[state->gl_set] :
    state->vt->is_utf8     ? &state->encoding_utf8 :
                             &state->encoding[state->gr_set];

  (*encoding->enc->decode)(encoding->enc, encoding->data,
      codepoints, &npoints, len, bytes, &eaten, len);

  /* This is a combining char. that needs to be merged with the previous
   * glyph output */
  if(vterm_unicode_is_combining(codepoints[i])) {
    /* See if the cursor has moved since */
    if(state->pos.row == state->combine_pos.row && state->pos.col == state->combine_pos.col + state->combine_width) {
      size_t saved_i = 0;
#ifdef DEBUG_GLYPH_COMBINE
      int printpos;
      printf("DEBUG: COMBINING SPLIT GLYPH of chars {");
      for(printpos = 0; state->combine_chars[printpos]; printpos++)
        printf("U+%04x ", state->combine_chars[printpos]);
      printf("} + {");
#endif

      /* Find where we need to append these combining chars */
      while(state->combine_chars[saved_i])
        saved_i++;

      /* Add extra ones */
      while(i < npoints && vterm_unicode_is_combining(codepoints[i])) {
        if(saved_i >= state->combine_chars_size)
          grow_combine_buffer(state);
        state->combine_chars[saved_i++] = codepoints[i++];
      }
      if(saved_i >= state->combine_chars_size)
        grow_combine_buffer(state);
      state->combine_chars[saved_i] = 0;

#ifdef DEBUG_GLYPH_COMBINE
      for(; state->combine_chars[printpos]; printpos++)
        printf("U+%04x ", state->combine_chars[printpos]);
      printf("}\n");
#endif

      /* Now render it */
      putglyph(state, state->combine_chars, state->combine_width, state->combine_pos);
    }
    else {
      fprintf(stderr, "libvterm: TODO: Skip over split char+combining\n");
    }
  }

  for(; i < npoints; i++) {
    // Try to find combining characters following this
    int glyph_starts = i;
    int glyph_ends;
    int width = 0;

    for(glyph_ends = i + 1; glyph_ends < npoints; glyph_ends++)
      if(!vterm_unicode_is_combining(codepoints[glyph_ends]))
        break;

    chars = alloca(sizeof(uint32_t) * (glyph_ends - glyph_starts + 1));

    for( ; i < glyph_ends; i++) {
      chars[i - glyph_starts] = codepoints[i];
      width += vterm_unicode_width(codepoints[i]);
    }

    chars[glyph_ends - glyph_starts] = 0;
    i--;

#ifdef DEBUG_GLYPH_COMBINE
	{
    int printpos;
    printf("DEBUG: COMBINED GLYPH of %d chars {", glyph_ends - glyph_starts);
    for(printpos = 0; printpos < glyph_ends - glyph_starts; printpos++)
      printf("U+%04x ", chars[printpos]);
    printf("}, onscreen width %d\n", width);
	}
#endif

    if(state->at_phantom) {
      linefeed(state);
      state->pos.col = 0;
      state->at_phantom = 0;
    }

    if(state->mode.insert) {
      /* TODO: This will be a little inefficient for large bodies of text, as
       * it'll have to 'ICH' effectively before every glyph. We should scan
       * ahead and ICH as many times as required
       */
      VTermRect rect = {
        .start_row = state->pos.row,
        .end_row   = state->pos.row + 1,
        .start_col = state->pos.col,
        .end_col   = state->cols,
      };
      scroll(state, rect, 0, -1);
    }
    putglyph(state, chars, width, state->pos);

    if(i == npoints - 1) {
      /* End of the buffer. Save the chars in case we have to combine with
       * more on the next call */
      unsigned int save_i;
      for(save_i = 0; chars[save_i]; save_i++) {
        if(save_i >= state->combine_chars_size)
          grow_combine_buffer(state);
        state->combine_chars[save_i] = chars[save_i];
      }
      if(save_i >= state->combine_chars_size)
        grow_combine_buffer(state);
      state->combine_chars[save_i] = 0;
      state->combine_width = width;
      state->combine_pos = state->pos;
    }

    if(state->pos.col + width >= state->cols) {
      if(state->mode.autowrap)
        state->at_phantom = 1;
    }
    else {
      state->pos.col += width;
    }
  }

  updatecursor(state, &oldpos, 0);

  return eaten;
}

static int on_control(unsigned char control, void *user)
{
  VTermState *state = user;

  VTermPos oldpos = state->pos;

  switch(control) {
  case 0x07: // BEL - ECMA-48 8.3.3
    if(state->callbacks && state->callbacks->bell)
      (*state->callbacks->bell)(state->cbdata);
    break;

  case 0x08: // BS - ECMA-48 8.3.5
    if(state->pos.col > 0)
      state->pos.col--;
    break;

  case 0x09: // HT - ECMA-48 8.3.60
    tab(state, 1, +1);
    break;

  case 0x0a: // LF - ECMA-48 8.3.74
  case 0x0b: // VT
  case 0x0c: // FF
    linefeed(state);
    if(state->mode.newline)
      state->pos.col = 0;
    break;

  case 0x0d: // CR - ECMA-48 8.3.15
    state->pos.col = 0;
    break;

  case 0x0e: // LS1 - ECMA-48 8.3.76
    state->gl_set = 1;
    break;

  case 0x0f: // LS0 - ECMA-48 8.3.75
    state->gl_set = 0;
    break;

  case 0x84: // IND - DEPRECATED but implemented for completeness
    linefeed(state);
    break;

  case 0x85: // NEL - ECMA-48 8.3.86
    linefeed(state);
    state->pos.col = 0;
    break;

  case 0x88: // HTS - ECMA-48 8.3.62
    set_col_tabstop(state, state->pos.col);
    break;

  case 0x8d: // RI - ECMA-48 8.3.104
    if(state->pos.row == state->scrollregion_start) {
      VTermRect rect = {
        .start_row = state->scrollregion_start,
        .end_row   = SCROLLREGION_END(state),
        .start_col = 0,
        .end_col   = state->cols,
      };

      scroll(state, rect, -1, 0);
    }
    else if(state->pos.row > 0)
        state->pos.row--;
    break;

  default:
    return 0;
  }

  updatecursor(state, &oldpos, 1);

  return 1;
}

static void output_mouse(VTermState *state, int code, int pressed, int modifiers, int col, int row)
{
  modifiers <<= 2;

  switch(state->mouse_protocol) {
  case MOUSE_X10:
    if(col + 0x21 > 0xff)
      col = 0xff - 0x21;
    if(row + 0x21 > 0xff)
      row = 0xff - 0x21;

    if(!pressed)
      code = 3;

    vterm_push_output_sprintf(state->vt, "\e[M%c%c%c",
        (code | modifiers) + 0x20, col + 0x21, row + 0x21);
    break;

  case MOUSE_UTF8:
    {
      char utf8[18]; size_t len = 0;

      if(!pressed)
        code = 3;

      len += fill_utf8((code | modifiers) + 0x20, utf8 + len);
      len += fill_utf8(col + 0x21, utf8 + len);
      len += fill_utf8(row + 0x21, utf8 + len);

      vterm_push_output_sprintf(state->vt, "\e[M%s", utf8);
    }
    break;

  case MOUSE_SGR:
    vterm_push_output_sprintf(state->vt, "\e[<%d;%d;%d%c",
        code | modifiers, col + 1, row + 1, pressed ? 'M' : 'm');
    break;

  case MOUSE_RXVT:
    if(!pressed)
      code = 3;

    vterm_push_output_sprintf(state->vt, "\e[%d;%d;%dM",
        code | modifiers, col + 1, row + 1);
    break;
  }
}

static void mousefunc(int col, int row, int button, int pressed, int modifiers, void *data)
{
  VTermState *state = data;

  int old_col     = state->mouse_col;
  int old_row     = state->mouse_row;
  int old_buttons = state->mouse_buttons;

  state->mouse_col = col;
  state->mouse_row = row;

  if(button > 0 && button <= 3) {
    if(pressed)
      state->mouse_buttons |= (1 << (button-1));
    else
      state->mouse_buttons &= ~(1 << (button-1));
  }

  modifiers &= 0x7;


  /* Most of the time we don't get button releases from 4/5 */
  if(state->mouse_buttons != old_buttons || button >= 4) {
    if(button < 4) {
      output_mouse(state, button-1, pressed, modifiers, col, row);
    }
    else if(button < 6) {
      output_mouse(state, button-4 + 0x40, pressed, modifiers, col, row);
    }
  }
  else if(col != old_col || row != old_row) {
    if((state->mouse_flags & MOUSE_WANT_DRAG && state->mouse_buttons) ||
       (state->mouse_flags & MOUSE_WANT_MOVE)) {
      int button = state->mouse_buttons & 0x01 ? 1 :
                   state->mouse_buttons & 0x02 ? 2 :
                   state->mouse_buttons & 0x04 ? 3 : 4;
      output_mouse(state, button-1 + 0x20, 1, modifiers, col, row);
    }
  }
}

static int settermprop_bool(VTermState *state, VTermProp prop, int v)
{
  VTermValue val;
  val.boolean = v;

#ifdef DEBUG
  if(VTERM_VALUETYPE_BOOL != vterm_get_prop_type(prop)) {
    fprintf(stderr, "Cannot set prop %d as it has type %d, not type BOOL\n",
        prop, vterm_get_prop_type(prop));
    return -1;
  }
#endif

  if(state->callbacks && state->callbacks->settermprop)
    if((*state->callbacks->settermprop)(prop, &val, state->cbdata))
      return 1;

  return 0;
}

static int settermprop_int(VTermState *state, VTermProp prop, int v)
{
  VTermValue val;
  val.number = v;

#ifdef DEBUG
  if(VTERM_VALUETYPE_INT != vterm_get_prop_type(prop)) {
    fprintf(stderr, "Cannot set prop %d as it has type %d, not type int\n",
        prop, vterm_get_prop_type(prop));
    return -1;
  }
#endif

  if(state->callbacks && state->callbacks->settermprop)
    if((*state->callbacks->settermprop)(prop, &val, state->cbdata))
      return 1;

  return 0;
}

static int settermprop_string(VTermState *state, VTermProp prop, const char *str, size_t len)
{
  char strvalue[len+1];
  VTermValue val;

  strncpy(strvalue, str, len);
  strvalue[len] = 0;

  val.string = strvalue;

#ifdef DEBUG
  if(VTERM_VALUETYPE_STRING != vterm_get_prop_type(prop)) {
    fprintf(stderr, "Cannot set prop %d as it has type %d, not type STRING\n",
        prop, vterm_get_prop_type(prop));
    return -1;
  }
#endif

  if(state->callbacks && state->callbacks->settermprop)
    if((*state->callbacks->settermprop)(prop, &val, state->cbdata))
      return 1;

  return 0;
}

static void savecursor(VTermState *state, int save)
{
  if(save) {
    state->saved.pos = state->pos;
    state->saved.mode.cursor_visible = state->mode.cursor_visible;
    state->saved.mode.cursor_blink   = state->mode.cursor_blink;
    state->saved.mode.cursor_shape   = state->mode.cursor_shape;

    vterm_state_savepen(state, 1);
  }
  else {
    VTermPos oldpos = state->pos;

    state->pos = state->saved.pos;
    state->mode.cursor_visible = state->saved.mode.cursor_visible;
    state->mode.cursor_blink   = state->saved.mode.cursor_blink;
    state->mode.cursor_shape   = state->saved.mode.cursor_shape;

    settermprop_bool(state, VTERM_PROP_CURSORVISIBLE, state->mode.cursor_visible);
    settermprop_bool(state, VTERM_PROP_CURSORBLINK,   state->mode.cursor_blink);
    settermprop_int (state, VTERM_PROP_CURSORSHAPE,   state->mode.cursor_shape);

    vterm_state_savepen(state, 0);

    updatecursor(state, &oldpos, 1);
  }
}

static void altscreen(VTermState *state, int alt)
{
  /* Only store that we're on the alternate screen if the usercode said it
   * switched */
  if(!settermprop_bool(state, VTERM_PROP_ALTSCREEN, alt))
    return;

  state->mode.alt_screen = alt;
  if(alt) {
    VTermRect rect = {
      .start_row = 0,
      .start_col = 0,
      .end_row = state->rows,
      .end_col = state->cols,
    };
    erase(state, rect);
  }
}

static int on_escape(const char *bytes, size_t len, void *user)
{
  VTermState *state = user;

  /* Easier to decode this from the first byte, even though the final
   * byte terminates it
   */
  switch(bytes[0]) {
  case '#':
    if(len != 2)
      return 0;

    switch(bytes[1]) {
      case '8': // DECALN
      {
        VTermPos pos;
        uint32_t E[] = { 'E', 0 };
        for(pos.row = 0; pos.row < state->rows; pos.row++)
          for(pos.col = 0; pos.col < state->cols; pos.col++)
            putglyph(state, E, 1, pos);
        break;
      }

      default:
        return 0;
    }
    return 2;

  case '(': case ')': case '*': case '+': // SCS
    if(len != 2)
      return 0;

    {
      int setnum = bytes[0] - 0x28;
      VTermEncoding *newenc = vterm_lookup_encoding(ENC_SINGLE_94, bytes[1]);

      if(newenc) {
        state->encoding[setnum].enc = newenc;

        if(newenc->init)
          (*newenc->init)(newenc, state->encoding[setnum].data);
      }
    }

    return 2;

  case '7': // DECSC
    savecursor(state, 1);
    return 1;

  case '8': // DECRC
    savecursor(state, 0);
    return 1;

  case '=': // DECKPAM
    state->mode.keypad = 1;
    return 1;

  case '>': // DECKPNM
    state->mode.keypad = 0;
    return 1;

  case 'c': // RIS - ECMA-48 8.3.105
  {
    VTermPos oldpos = state->pos;
    vterm_state_reset(state, 1);
    if(state->callbacks && state->callbacks->movecursor)
      (*state->callbacks->movecursor)(state->pos, oldpos, state->mode.cursor_visible, state->cbdata);
    return 1;
  }

  case 'n': // LS2 - ECMA-48 8.3.78
    state->gl_set = 2;
    return 1;

  case 'o': // LS3 - ECMA-48 8.3.80
    state->gl_set = 3;
    return 1;

  default:
    return 0;
  }
}

static void set_mode(VTermState *state, int num, int val)
{
  switch(num) {
  case 4: // IRM - ECMA-48 7.2.10
    state->mode.insert = val;
    break;

  case 20: // LNM - ANSI X3.4-1977
    state->mode.newline = val;
    break;

  default:
    fprintf(stderr, "libvterm: Unknown mode %d\n", num);
    return;
  }
}

static void set_dec_mode(VTermState *state, int num, int val)
{
  switch(num) {
  case 1:
    state->mode.cursor = val;
    break;

  case 5:
    settermprop_bool(state, VTERM_PROP_REVERSE, val);
    break;

  case 6: // DECOM - origin mode
    {
      VTermPos oldpos = state->pos;
      state->mode.origin = val;
      state->pos.row = state->mode.origin ? state->scrollregion_start : 0;
      state->pos.col = 0;
      updatecursor(state, &oldpos, 1);
    }
    break;

  case 7:
    state->mode.autowrap = val;
    break;

  case 12:
    state->mode.cursor_blink = val;
    settermprop_bool(state, VTERM_PROP_CURSORBLINK, val);
    break;

  case 25:
    state->mode.cursor_visible = val;
    settermprop_bool(state, VTERM_PROP_CURSORVISIBLE, val);
    break;

  case 1000:
  case 1002:
  case 1003:
    if(val) {
      state->mouse_col     = 0;
      state->mouse_row     = 0;
      state->mouse_buttons = 0;

      state->mouse_flags = 0;
      state->mouse_protocol = MOUSE_X10;

      if(num == 1002)
        state->mouse_flags |= MOUSE_WANT_DRAG;
      if(num == 1003)
        state->mouse_flags |= MOUSE_WANT_MOVE;
    }

    if(state->callbacks && state->callbacks->setmousefunc)
      (*state->callbacks->setmousefunc)(val ? mousefunc : NULL, state, state->cbdata);

    break;

  case 1005:
    state->mouse_protocol = val ? MOUSE_UTF8 : MOUSE_X10;
    break;

  case 1006:
    state->mouse_protocol = val ? MOUSE_SGR : MOUSE_X10;
    break;

  case 1015:
    state->mouse_protocol = val ? MOUSE_RXVT : MOUSE_X10;
    break;

  case 1047:
    altscreen(state, val);
    break;

  case 1048:
    savecursor(state, val);
    break;

  case 1049:
    altscreen(state, val);
    savecursor(state, val);
    break;

  default:
    fprintf(stderr, "libvterm: Unknown DEC mode %d\n", num);
    return;
  }
}

static int on_csi(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user)
{
  VTermState *state = user;
  int leader_byte = 0;
  int intermed_byte = 0;
  VTermPos oldpos;

  // Some temporaries for later code
  int count, val;
  int row, col;
  VTermRect rect;

  if(leader && leader[0]) {
    if(leader[1]) // longer than 1 char
      return 0;

    switch(leader[0]) {
    case '?':
    case '>':
      leader_byte = leader[0];
      break;
    default:
      return 0;
    }
  }

  if(intermed && intermed[0]) {
    if(intermed[1]) // longer than 1 char
      return 0;

    switch(intermed[0]) {
    case ' ':
      intermed_byte = intermed[0];
      break;
    default:
      return 0;
    }
  }

  oldpos = state->pos;

#define LEADER(l,b) ((l << 8) | b)
#define INTERMED(i,b) ((i << 16) | b)

  switch(intermed_byte << 16 | leader_byte << 8 | command) {
  case 0x40: // ICH - ECMA-48 8.3.64
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->pos.row;
    rect.end_row   = state->pos.row + 1;
    rect.start_col = state->pos.col;
    rect.end_col   = state->cols;

    scroll(state, rect, 0, -count);

    break;

  case 0x41: // CUU - ECMA-48 8.3.22
    count = CSI_ARG_COUNT(args[0]);
    state->pos.row -= count;
    state->at_phantom = 0;
    break;

  case 0x42: // CUD - ECMA-48 8.3.19
    count = CSI_ARG_COUNT(args[0]);
    state->pos.row += count;
    state->at_phantom = 0;
    break;

  case 0x43: // CUF - ECMA-48 8.3.20
    count = CSI_ARG_COUNT(args[0]);
    state->pos.col += count;
    state->at_phantom = 0;
    break;

  case 0x44: // CUB - ECMA-48 8.3.18
    count = CSI_ARG_COUNT(args[0]);
    state->pos.col -= count;
    state->at_phantom = 0;
    break;

  case 0x45: // CNL - ECMA-48 8.3.12
    count = CSI_ARG_COUNT(args[0]);
    state->pos.col = 0;
    state->pos.row += count;
    state->at_phantom = 0;
    break;

  case 0x46: // CPL - ECMA-48 8.3.13
    count = CSI_ARG_COUNT(args[0]);
    state->pos.col = 0;
    state->pos.row -= count;
    state->at_phantom = 0;
    break;

  case 0x47: // CHA - ECMA-48 8.3.9
    val = CSI_ARG_OR(args[0], 1);
    state->pos.col = val-1;
    state->at_phantom = 0;
    break;

  case 0x48: // CUP - ECMA-48 8.3.21
    row = CSI_ARG_OR(args[0], 1);
    col = argcount < 2 || CSI_ARG_IS_MISSING(args[1]) ? 1 : CSI_ARG(args[1]);
    // zero-based
    state->pos.row = row-1;
    state->pos.col = col-1;
    if(state->mode.origin)
      state->pos.row += state->scrollregion_start;
    state->at_phantom = 0;
    break;

  case 0x49: // CHT - ECMA-48 8.3.10
    count = CSI_ARG_COUNT(args[0]);
    tab(state, count, +1);
    break;

  case 0x4a: // ED - ECMA-48 8.3.39
    switch(CSI_ARG(args[0])) {
    case CSI_ARG_MISSING:
    case 0:
      rect.start_row = state->pos.row; rect.end_row = state->pos.row + 1;
      rect.start_col = state->pos.col; rect.end_col = state->cols;
      if(rect.end_col > rect.start_col)
        erase(state, rect);

      rect.start_row = state->pos.row + 1; rect.end_row = state->rows;
      rect.start_col = 0;
      if(rect.end_row > rect.start_row)
        erase(state, rect);
      break;

    case 1:
      rect.start_row = 0; rect.end_row = state->pos.row;
      rect.start_col = 0; rect.end_col = state->cols;
      if(rect.end_col > rect.start_col)
        erase(state, rect);

      rect.start_row = state->pos.row; rect.end_row = state->pos.row + 1;
                          rect.end_col = state->pos.col + 1;
      if(rect.end_row > rect.start_row)
        erase(state, rect);
      break;

    case 2:
      rect.start_row = 0; rect.end_row = state->rows;
      rect.start_col = 0; rect.end_col = state->cols;
      erase(state, rect);
      break;
    }
    break;

  case 0x4b: // EL - ECMA-48 8.3.41
    rect.start_row = state->pos.row;
    rect.end_row   = state->pos.row + 1;

    switch(CSI_ARG(args[0])) {
    case CSI_ARG_MISSING:
    case 0:
      rect.start_col = state->pos.col; rect.end_col = state->cols; break;
    case 1:
      rect.start_col = 0; rect.end_col = state->pos.col + 1; break;
    case 2:
      rect.start_col = 0; rect.end_col = state->cols; break;
    default:
      return 0;
    }

    if(rect.end_col > rect.start_col)
      erase(state, rect);

    break;

  case 0x4c: // IL - ECMA-48 8.3.67
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->pos.row;
    rect.end_row   = SCROLLREGION_END(state);
    rect.start_col = 0;
    rect.end_col   = state->cols;

    scroll(state, rect, -count, 0);

    break;

  case 0x4d: // DL - ECMA-48 8.3.32
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->pos.row;
    rect.end_row   = SCROLLREGION_END(state);
    rect.start_col = 0;
    rect.end_col   = state->cols;

    scroll(state, rect, count, 0);

    break;

  case 0x50: // DCH - ECMA-48 8.3.26
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->pos.row;
    rect.end_row   = state->pos.row + 1;
    rect.start_col = state->pos.col;
    rect.end_col   = state->cols;

    scroll(state, rect, 0, count);

    break;

  case 0x53: // SU - ECMA-48 8.3.147
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->scrollregion_start;
    rect.end_row   = SCROLLREGION_END(state);
    rect.start_col = 0;
    rect.end_col   = state->cols;

    scroll(state, rect, count, 0);

    break;

  case 0x54: // SD - ECMA-48 8.3.113
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->scrollregion_start;
    rect.end_row   = SCROLLREGION_END(state);
    rect.start_col = 0;
    rect.end_col   = state->cols;

    scroll(state, rect, -count, 0);

    break;

  case 0x58: // ECH - ECMA-48 8.3.38
    count = CSI_ARG_COUNT(args[0]);

    rect.start_row = state->pos.row;
    rect.end_row   = state->pos.row + 1;
    rect.start_col = state->pos.col;
    rect.end_col   = state->pos.col + count;

    erase(state, rect);
    break;

  case 0x5a: // CBT - ECMA-48 8.3.7
    count = CSI_ARG_COUNT(args[0]);
    tab(state, count, -1);
    break;

  case 0x60: // HPA - ECMA-48 8.3.57
    col = CSI_ARG_OR(args[0], 1);
    state->pos.col = col-1;
    state->at_phantom = 0;
    break;

  case 0x61: // HPR - ECMA-48 8.3.59
    count = CSI_ARG_COUNT(args[0]);
    state->pos.col += count;
    state->at_phantom = 0;
    break;

  case 0x63: // DA - ECMA-48 8.3.24
    val = CSI_ARG_OR(args[0], 0);
    if(val == 0)
      // DEC VT100 response
      vterm_push_output_sprintf(state->vt, "\e[?1;2c");
    break;

  case LEADER('>', 0x63): // DEC secondary Device Attributes
    vterm_push_output_sprintf(state->vt, "\e[>%d;%d;%dc", 0, 100, 0);
    break;

  case 0x64: // VPA - ECMA-48 8.3.158
    row = CSI_ARG_OR(args[0], 1);
    state->pos.row = row-1;
    if(state->mode.origin)
      state->pos.row += state->scrollregion_start;
    state->at_phantom = 0;
    break;

  case 0x65: // VPR - ECMA-48 8.3.160
    count = CSI_ARG_COUNT(args[0]);
    state->pos.row += count;
    state->at_phantom = 0;
    break;

  case 0x66: // HVP - ECMA-48 8.3.63
    row = CSI_ARG_OR(args[0], 1);
    col = argcount < 2 || CSI_ARG_IS_MISSING(args[1]) ? 1 : CSI_ARG(args[1]);
    // zero-based
    state->pos.row = row-1;
    state->pos.col = col-1;
    if(state->mode.origin)
      state->pos.row += state->scrollregion_start;
    state->at_phantom = 0;
    break;

  case 0x67: // TBC - ECMA-48 8.3.154
    val = CSI_ARG_OR(args[0], 0);

    switch(val) {
    case 0:
      clear_col_tabstop(state, state->pos.col);
      break;
    case 3:
    case 5:
      for(col = 0; col < state->cols; col++)
        clear_col_tabstop(state, col);
      break;
    case 1:
    case 2:
    case 4:
      break;
    /* TODO: 1, 2 and 4 aren't meaningful yet without line tab stops */
    default:
      return 0;
    }
    break;

  case 0x68: // SM - ECMA-48 8.3.125
    if(!CSI_ARG_IS_MISSING(args[0]))
      set_mode(state, CSI_ARG(args[0]), 1);
    break;

  case LEADER('?', 0x68): // DEC private mode set
    if(!CSI_ARG_IS_MISSING(args[0]))
      set_dec_mode(state, CSI_ARG(args[0]), 1);
    break;

  case 0x6a: // HPB - ECMA-48 8.3.58
    count = CSI_ARG_COUNT(args[0]);
    state->pos.col -= count;
    state->at_phantom = 0;
    break;

  case 0x6b: // VPB - ECMA-48 8.3.159
    count = CSI_ARG_COUNT(args[0]);
    state->pos.row -= count;
    state->at_phantom = 0;
    break;

  case 0x6c: // RM - ECMA-48 8.3.106
    if(!CSI_ARG_IS_MISSING(args[0]))
      set_mode(state, CSI_ARG(args[0]), 0);
    break;

  case LEADER('?', 0x6c): // DEC private mode reset
    if(!CSI_ARG_IS_MISSING(args[0]))
      set_dec_mode(state, CSI_ARG(args[0]), 0);
    break;

  case 0x6d: // SGR - ECMA-48 8.3.117
    vterm_state_setpen(state, args, argcount);
    break;

  case 0x6e: // DSR - ECMA-48 8.3.35
    val = CSI_ARG_OR(args[0], 0);

    switch(val) {
    case 0: case 1: case 2: case 3: case 4:
      // ignore - these are replies
      break;
    case 5:
      vterm_push_output_sprintf(state->vt, "\e[0n");
      break;
    case 6:
      vterm_push_output_sprintf(state->vt, "\e[%d;%dR", state->pos.row + 1, state->pos.col + 1);
      break;
    }
    break;

  case LEADER('!', 0x70): // DECSTR - DEC soft terminal reset
    vterm_state_reset(state, 0);
    break;

  case INTERMED(' ', 0x71): // DECSCUSR - DEC set cursor shape
    val = CSI_ARG_OR(args[0], 1);

    switch(val) {
    case 0: case 1:
      state->mode.cursor_blink = 1;
      state->mode.cursor_shape = VTERM_PROP_CURSORSHAPE_BLOCK;
      break;
    case 2:
      state->mode.cursor_blink = 0;
      state->mode.cursor_shape = VTERM_PROP_CURSORSHAPE_BLOCK;
      break;
    case 3:
      state->mode.cursor_blink = 1;
      state->mode.cursor_shape = VTERM_PROP_CURSORSHAPE_UNDERLINE;
      break;
    case 4:
      state->mode.cursor_blink = 0;
      state->mode.cursor_shape = VTERM_PROP_CURSORSHAPE_UNDERLINE;
      break;
    }

    settermprop_bool(state, VTERM_PROP_CURSORBLINK, state->mode.cursor_blink);
    settermprop_int (state, VTERM_PROP_CURSORSHAPE, state->mode.cursor_shape);
    break;

  case 0x72: // DECSTBM - DEC custom
    state->scrollregion_start = CSI_ARG_OR(args[0], 1) - 1;
    state->scrollregion_end = argcount < 2 || CSI_ARG_IS_MISSING(args[1]) ? -1 : CSI_ARG(args[1]);
    if(state->scrollregion_start == 0 && state->scrollregion_end == state->rows)
      state->scrollregion_end = -1;
    break;

  case 0x73: // ANSI SAVE
    savecursor(state, 1);
    break;

  case 0x75: // ANSI RESTORE
    savecursor(state, 0);
    break;

  default:
    return 0;
  }

#define LBOUND(v,min) if((v) < (min)) (v) = (min)
#define UBOUND(v,max) if((v) > (max)) (v) = (max)

  LBOUND(state->pos.col, 0);
  UBOUND(state->pos.col, state->cols-1);

  if(state->mode.origin) {
    LBOUND(state->pos.row, state->scrollregion_start);
    UBOUND(state->pos.row, state->scrollregion_end-1);
  }
  else {
    LBOUND(state->pos.row, 0);
    UBOUND(state->pos.row, state->rows-1);
  }

  updatecursor(state, &oldpos, 1);

  return 1;
}

static int on_osc(const char *command, size_t cmdlen, void *user)
{
  VTermState *state = user;

  if(cmdlen < 2)
    return 0;

  if(strneq(command, "0;", 2)) {
    settermprop_string(state, VTERM_PROP_ICONNAME, command + 2, cmdlen - 2);
    settermprop_string(state, VTERM_PROP_TITLE, command + 2, cmdlen - 2);
    return 1;
  }
  else if(strneq(command, "1;", 2)) {
    settermprop_string(state, VTERM_PROP_ICONNAME, command + 2, cmdlen - 2);
    return 1;
  }
  else if(strneq(command, "2;", 2)) {
    settermprop_string(state, VTERM_PROP_TITLE, command + 2, cmdlen - 2);
    return 1;
  }

  return 0;
}

static void request_status_string(VTermState *state, const char *command, size_t cmdlen)
{
  if(cmdlen == 1)
    switch(command[0]) {
      case 'r': // Query DECSTBM
        vterm_push_output_sprintf(state->vt, "\eP1$r%d;%dr\e\\", state->scrollregion_start+1, SCROLLREGION_END(state));
        return;
    }

  if(cmdlen == 2)
    if(strneq(command, " q", 2)) {
      int reply = 0;
      switch(state->mode.cursor_shape) {
        case VTERM_PROP_CURSORSHAPE_BLOCK:     reply = 2; break;
        case VTERM_PROP_CURSORSHAPE_UNDERLINE: reply = 4; break;
      }
      if(state->mode.cursor_blink)
        reply--;
      vterm_push_output_sprintf(state->vt, "\eP1$r%d q\e\\", reply);
      return;
    }

  vterm_push_output_sprintf(state->vt, "\eP0$r%.s\e\\", (int)cmdlen, command);
}

static int on_dcs(const char *command, size_t cmdlen, void *user)
{
  VTermState *state = user;

  if(cmdlen >= 2 && strneq(command, "$q", 2)) {
    request_status_string(state, command+2, cmdlen-2);
    return 1;
  }

  return 0;
}

static int on_resize(int rows, int cols, void *user)
{
  VTermState *state = user;
  VTermPos oldpos = state->pos;

  if(cols != state->cols) {
    unsigned char *newtabstops = vterm_allocator_malloc(state->vt, (cols + 7) / 8);

    /* TODO: This can all be done much more efficiently bytewise */
    int col;
    for(col = 0; col < state->cols && col < cols; col++) {
      unsigned char mask = 1 << (col & 7);
      if(state->tabstops[col >> 3] & mask)
        newtabstops[col >> 3] |= mask;
      else
        newtabstops[col >> 3] &= ~mask;
      }

    for( ; col < cols; col++) {
      unsigned char mask = 1 << (col & 7);
      if(col % 8 == 0)
        newtabstops[col >> 3] |= mask;
      else
        newtabstops[col >> 3] &= ~mask;
    }

    vterm_allocator_free(state->vt, state->tabstops);
    state->tabstops = newtabstops;
  }

  state->rows = rows;
  state->cols = cols;

  if(state->pos.row >= rows)
    state->pos.row = rows - 1;
  if(state->pos.col >= cols)
    state->pos.col = cols - 1;

  if(state->at_phantom && state->pos.col < cols-1) {
    state->at_phantom = 0;
    state->pos.col++;
  }

  if(state->callbacks && state->callbacks->resize)
    (*state->callbacks->resize)(rows, cols, state->cbdata);

  updatecursor(state, &oldpos, 1);

  return 1;
}

static const VTermParserCallbacks parser_callbacks = {
  .text    = on_text,
  .control = on_control,
  .escape  = on_escape,
  .csi     = on_csi,
  .osc     = on_osc,
  .dcs     = on_dcs,
  .resize  = on_resize,
};

VTermState *vterm_obtain_state(VTerm *vt)
{
  VTermState *state;
  if(vt->state)
    return vt->state;

  state = vterm_state_new(vt);
  vt->state = state;

  state->combine_chars_size = 16;
  state->combine_chars = vterm_allocator_malloc(state->vt, state->combine_chars_size * sizeof(state->combine_chars[0]));

  state->tabstops = vterm_allocator_malloc(state->vt, (state->cols + 7) / 8);

  state->encoding_utf8.enc = vterm_lookup_encoding(ENC_UTF8, 'u');
  if(*state->encoding_utf8.enc->init)
    (*state->encoding_utf8.enc->init)(state->encoding_utf8.enc, state->encoding_utf8.data);

  vterm_set_parser_callbacks(vt, &parser_callbacks, state);

  return state;
}

void vterm_state_reset(VTermState *state, int hard)
{
  int col, i;
  VTermEncoding *default_enc;

  state->scrollregion_start = 0;
  state->scrollregion_end = -1;

  state->mode.keypad         = 0;
  state->mode.cursor         = 0;
  state->mode.autowrap       = 1;
  state->mode.insert         = 0;
  state->mode.newline        = 0;
  state->mode.cursor_visible = 1;
  state->mode.cursor_blink   = 1;
  state->mode.cursor_shape   = VTERM_PROP_CURSORSHAPE_BLOCK;
  state->mode.alt_screen     = 0;
  state->mode.origin         = 0;

  for(col = 0; col < state->cols; col++)
    if(col % 8 == 0)
      set_col_tabstop(state, col);
    else
      clear_col_tabstop(state, col);

  if(state->callbacks && state->callbacks->initpen)
    (*state->callbacks->initpen)(state->cbdata);

  vterm_state_resetpen(state);

  default_enc = state->vt->is_utf8 ?
      vterm_lookup_encoding(ENC_UTF8,      'u') :
      vterm_lookup_encoding(ENC_SINGLE_94, 'B');

  for(i = 0; i < 4; i++) {
    state->encoding[i].enc = default_enc;
    if(default_enc->init)
      (*default_enc->init)(default_enc, state->encoding[i].data);
  }

  state->gl_set = 0;
  state->gr_set = 0;

  // Initialise the props
  settermprop_bool(state, VTERM_PROP_CURSORVISIBLE, state->mode.cursor_visible);
  settermprop_bool(state, VTERM_PROP_CURSORBLINK,   state->mode.cursor_blink);
  settermprop_int (state, VTERM_PROP_CURSORSHAPE,   state->mode.cursor_shape);

  if(hard) {
	VTermRect rect = { 0, state->rows, 0, state->cols };

    state->pos.row = 0;
    state->pos.col = 0;
    state->at_phantom = 0;

    erase(state, rect);
  }
}

void vterm_state_get_cursorpos(VTermState *state, VTermPos *cursorpos)
{
  *cursorpos = state->pos;
}

void vterm_state_set_callbacks(VTermState *state, const VTermStateCallbacks *callbacks, void *user)
{
  if(callbacks) {
    state->callbacks = callbacks;
    state->cbdata = user;

    if(state->callbacks && state->callbacks->initpen)
      (*state->callbacks->initpen)(state->cbdata);
  }
  else {
    state->callbacks = NULL;
    state->cbdata = NULL;
  }
}
