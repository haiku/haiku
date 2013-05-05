#include "vterm_internal.h"

#include <stdio.h>

#include "rect.h"
#include "utf8.h"

#define UNICODE_SPACE 0x20
#define UNICODE_LINEFEED 0x0a

/* State of the pen at some moment in time, also used in a cell */
typedef struct
{
  /* After the bitfield */
  VTermColor   fg, bg;

  unsigned int bold      : 1;
  unsigned int underline : 2;
  unsigned int italic    : 1;
  unsigned int blink     : 1;
  unsigned int reverse   : 1;
  unsigned int strike    : 1;
  unsigned int font      : 4; /* 0 to 9 */
} ScreenPen;

/* Internal representation of a screen cell */
typedef struct
{
  uint32_t chars[VTERM_MAX_CHARS_PER_CELL];
  ScreenPen pen;
} ScreenCell;

struct VTermScreen
{
  VTerm *vt;
  VTermState *state;

  const VTermScreenCallbacks *callbacks;
  void *cbdata;

  VTermDamageSize damage_merge;
  /* start_row == -1 => no damage */
  VTermRect damaged;
  VTermRect pending_scrollrect;
  int pending_scroll_downward, pending_scroll_rightward;

  int rows;
  int cols;
  int global_reverse;

  /* Primary and Altscreen. buffers[1] is lazily allocated as needed */
  ScreenCell *buffers[2];

  /* buffer will == buffers[0] or buffers[1], depending on altscreen */
  ScreenCell *buffer;

  ScreenPen pen;
};

static inline ScreenCell *getcell(VTermScreen *screen, int row, int col)
{
  /* TODO: Bounds checking */
  return screen->buffer + (screen->cols * row) + col;
}

static ScreenCell *realloc_buffer(VTermScreen *screen, ScreenCell *buffer, int new_rows, int new_cols)
{
  ScreenCell *new_buffer = vterm_allocator_malloc(screen->vt, sizeof(ScreenCell) * new_rows * new_cols);
  int row, col;

  for(row = 0; row < new_rows; row++) {
    for(col = 0; col < new_cols; col++) {
      ScreenCell *new_cell = new_buffer + row*new_cols + col;

      if(buffer && row < screen->rows && col < screen->cols)
        *new_cell = buffer[row * screen->cols + col];
      else {
        new_cell->chars[0] = 0;
        new_cell->pen = screen->pen;
      }
    }
  }

  if(buffer)
    vterm_allocator_free(screen->vt, buffer);

  return new_buffer;
}

static void damagerect(VTermScreen *screen, VTermRect rect)
{
  VTermRect emit;

  switch(screen->damage_merge) {
  case VTERM_DAMAGE_CELL:
    /* Always emit damage event */
    emit = rect;
    break;

  case VTERM_DAMAGE_ROW:
    /* Emit damage longer than one row. Try to merge with existing damage in
     * the same row */
    if(rect.end_row > rect.start_row + 1) {
      // Bigger than 1 line - flush existing, emit this
      vterm_screen_flush_damage(screen);
      emit = rect;
    }
    else if(screen->damaged.start_row == -1) {
      // None stored yet
      screen->damaged = rect;
      return;
    }
    else if(rect.start_row == screen->damaged.start_row) {
      // Merge with the stored line
      if(screen->damaged.start_col > rect.start_col)
        screen->damaged.start_col = rect.start_col;
      if(screen->damaged.end_col < rect.end_col)
        screen->damaged.end_col = rect.end_col;
      return;
    }
    else {
      // Emit the currently stored line, store a new one
      emit = screen->damaged;
      screen->damaged = rect;
    }
    break;

  case VTERM_DAMAGE_SCREEN:
  case VTERM_DAMAGE_SCROLL:
    /* Never emit damage event */
    if(screen->damaged.start_row == -1)
      screen->damaged = rect;
    else {
      rect_expand(&screen->damaged, &rect);
    }
    return;

  default:
    fprintf(stderr, "TODO: Maybe merge damage for level %d\n", screen->damage_merge);
    return;
  }

  if(screen->callbacks && screen->callbacks->damage)
    (*screen->callbacks->damage)(emit, screen->cbdata);
}

static void damagescreen(VTermScreen *screen)
{
  VTermRect rect = {
    .start_row = 0,
    .end_row   = screen->rows,
    .start_col = 0,
    .end_col   = screen->cols,
  };

  damagerect(screen, rect);
}

static int putglyph(const uint32_t chars[], int width, VTermPos pos, void *user)
{
  VTermScreen *screen = user;
  ScreenCell *cell = getcell(screen, pos.row, pos.col);
  int i;
  int col;

  VTermRect rect = {
    .start_row = pos.row,
    .end_row   = pos.row+1,
    .start_col = pos.col,
    .end_col   = pos.col+width,
  };

  for(i = 0; i < VTERM_MAX_CHARS_PER_CELL && chars[i]; i++) {
    cell->chars[i] = chars[i];
    cell->pen = screen->pen;
  }
  if(i < VTERM_MAX_CHARS_PER_CELL)
    cell->chars[i] = 0;

  for(col = 1; col < width; col++)
    getcell(screen, pos.row, pos.col + col)->chars[0] = (uint32_t)-1;

  damagerect(screen, rect);

  return 1;
}

static void copycell(VTermPos dest, VTermPos src, void *user)
{
  VTermScreen *screen = user;
  ScreenCell *destcell = getcell(screen, dest.row, dest.col);
  ScreenCell *srccell = getcell(screen, src.row, src.col);

  *destcell = *srccell;
}

static int moverect_internal(VTermRect dest, VTermRect src, void *user)
{
  VTermScreen *screen = user;

  vterm_copy_cells(dest, src, &copycell, screen);

  return 1;
}

static int moverect_user(VTermRect dest, VTermRect src, void *user)
{
  VTermScreen *screen = user;

  if(screen->callbacks && screen->callbacks->moverect) {
    if(screen->damage_merge != VTERM_DAMAGE_SCROLL)
      // Avoid an infinite loop
      vterm_screen_flush_damage(screen);

    if((*screen->callbacks->moverect)(dest, src, screen->cbdata))
      return 1;
  }

  damagerect(screen, dest);

  return 1;
}

static int erase_internal(VTermRect rect, void *user)
{
  VTermScreen *screen = user;
  int row, col;

  for(row = rect.start_row; row < rect.end_row; row++)
    for(col = rect.start_col; col < rect.end_col; col++) {
      ScreenCell *cell = getcell(screen, row, col);
      cell->chars[0] = 0;
      cell->pen = screen->pen;
    }

  return 1;
}

static int erase_user(VTermRect rect, void *user)
{
  VTermScreen *screen = user;

  damagerect(screen, rect);

  return 1;
}

static int erase(VTermRect rect, void *user)
{
  erase_internal(rect, user);
  return erase_user(rect, user);
}

static int scrollrect(VTermRect rect, int downward, int rightward, void *user)
{
  VTermScreen *screen = user;

  vterm_scroll_rect(rect, downward, rightward,
      moverect_internal, erase_internal, screen);

  if(screen->damage_merge == VTERM_DAMAGE_SCROLL) {
    if(screen->damaged.start_row != -1 &&
       !rect_intersects(&rect, &screen->damaged)) {
      vterm_screen_flush_damage(screen);
    }

    if(screen->pending_scrollrect.start_row == -1) {
      screen->pending_scrollrect = rect;
      screen->pending_scroll_downward  = downward;
      screen->pending_scroll_rightward = rightward;
    }
    else if(rect_equal(&screen->pending_scrollrect, &rect) &&
       ((screen->pending_scroll_downward  == 0 && downward  == 0) ||
        (screen->pending_scroll_rightward == 0 && rightward == 0))) {
      screen->pending_scroll_downward  += downward;
      screen->pending_scroll_rightward += rightward;
    }
    else {
      vterm_screen_flush_damage(screen);

      screen->pending_scrollrect = rect;
      screen->pending_scroll_downward  = downward;
      screen->pending_scroll_rightward = rightward;
    }

    if(screen->damaged.start_row != -1) {
      if(rect_contains(&rect, &screen->damaged)) {
        vterm_rect_move(&screen->damaged, -downward, -rightward);
        rect_clip(&screen->damaged, &rect);
      }
      else {
        fprintf(stderr, "TODO: scrollrect split damage\n");
      }
    }

    return 1;
  }

  vterm_screen_flush_damage(screen);

  vterm_scroll_rect(rect, downward, rightward,
      moverect_user, erase_user, screen);

  return 1;
}

static int movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
  VTermScreen *screen = user;

  if(screen->callbacks && screen->callbacks->movecursor)
    return (*screen->callbacks->movecursor)(pos, oldpos, visible, screen->cbdata);

  return 0;
}

static int setpenattr(VTermAttr attr, VTermValue *val, void *user)
{
  VTermScreen *screen = user;

  switch(attr) {
  case VTERM_ATTR_BOLD:
    screen->pen.bold = val->boolean;
    return 1;
  case VTERM_ATTR_UNDERLINE:
    screen->pen.underline = val->number;
    return 1;
  case VTERM_ATTR_ITALIC:
    screen->pen.italic = val->boolean;
    return 1;
  case VTERM_ATTR_BLINK:
    screen->pen.blink = val->boolean;
    return 1;
  case VTERM_ATTR_REVERSE:
    screen->pen.reverse = val->boolean;
    return 1;
  case VTERM_ATTR_STRIKE:
    screen->pen.strike = val->boolean;
    return 1;
  case VTERM_ATTR_FONT:
    screen->pen.font = val->number;
    return 1;
  case VTERM_ATTR_FOREGROUND:
    screen->pen.fg = val->color;
    return 1;
  case VTERM_ATTR_BACKGROUND:
    screen->pen.bg = val->color;
    return 1;
  }

  return 0;
}

static int settermprop(VTermProp prop, VTermValue *val, void *user)
{
  VTermScreen *screen = user;

  switch(prop) {
  case VTERM_PROP_ALTSCREEN:
    if(val->boolean && !screen->buffers[1])
      return 0;

    screen->buffer = val->boolean ? screen->buffers[1] : screen->buffers[0];
    /* only send a damage event on disable; because during enable there's an
     * erase that sends a damage anyway
     */
    if(!val->boolean)
      damagescreen(screen);
    break;
  case VTERM_PROP_REVERSE:
    screen->global_reverse = val->boolean;
    damagescreen(screen);
    break;
  default:
    ; /* ignore */
  }

  if(screen->callbacks && screen->callbacks->settermprop)
    return (*screen->callbacks->settermprop)(prop, val, screen->cbdata);

  return 1;
}

static int setmousefunc(VTermMouseFunc func, void *data, void *user)
{
  VTermScreen *screen = user;

  if(screen->callbacks && screen->callbacks->setmousefunc)
    return (*screen->callbacks->setmousefunc)(func, data, screen->cbdata);

  return 0;
}

static int bell(void *user)
{
  VTermScreen *screen = user;

  if(screen->callbacks && screen->callbacks->bell)
    return (*screen->callbacks->bell)(screen->cbdata);

  return 0;
}

static int resize(int new_rows, int new_cols, void *user)
{
  VTermScreen *screen = user;
  int old_rows, old_cols;

  int is_altscreen = (screen->buffers[1] && screen->buffer == screen->buffers[1]);

  screen->buffers[0] = realloc_buffer(screen, screen->buffers[0], new_rows, new_cols);
  if(screen->buffers[1])
    screen->buffers[1] = realloc_buffer(screen, screen->buffers[1], new_rows, new_cols);

  screen->buffer = is_altscreen ? screen->buffers[1] : screen->buffers[0];

  old_rows = screen->rows;
  old_cols = screen->cols;

  screen->rows = new_rows;
  screen->cols = new_cols;

  if(new_cols > old_cols) {
    VTermRect rect = {
      .start_row = 0,
      .end_row   = old_rows,
      .start_col = old_cols,
      .end_col   = new_cols,
    };
    damagerect(screen, rect);
  }

  if(new_rows > old_rows) {
    VTermRect rect = {
      .start_row = old_rows,
      .end_row   = new_rows,
      .start_col = 0,
      .end_col   = new_cols,
    };
    damagerect(screen, rect);
  }

  if(screen->callbacks && screen->callbacks->resize)
    return (*screen->callbacks->resize)(new_rows, new_cols, screen->cbdata);

  return 1;
}

static VTermStateCallbacks state_cbs = {
  .putglyph     = &putglyph,
  .movecursor   = &movecursor,
  .scrollrect   = &scrollrect,
  .erase        = &erase,
  .setpenattr   = &setpenattr,
  .settermprop  = &settermprop,
  .setmousefunc = &setmousefunc,
  .bell         = &bell,
  .resize       = &resize,
};

static VTermScreen *screen_new(VTerm *vt)
{
  VTermState *state = vterm_obtain_state(vt);
  VTermScreen *screen;
  int rows, cols;

  if(!state)
    return NULL;

  screen = vterm_allocator_malloc(vt, sizeof(VTermScreen));

  vterm_get_size(vt, &rows, &cols);

  screen->vt = vt;
  screen->state = state;

  screen->damage_merge = VTERM_DAMAGE_CELL;
  screen->damaged.start_row = -1;
  screen->pending_scrollrect.start_row = -1;

  screen->rows = rows;
  screen->cols = cols;

  screen->buffers[0] = realloc_buffer(screen, NULL, rows, cols);

  screen->buffer = screen->buffers[0];

  vterm_state_set_callbacks(screen->state, &state_cbs, screen);

  return screen;
}

void vterm_screen_free(VTermScreen *screen)
{
  vterm_allocator_free(screen->vt, screen->buffers[0]);
  if(screen->buffers[1])
    vterm_allocator_free(screen->vt, screen->buffers[1]);

  vterm_allocator_free(screen->vt, screen);
}

void vterm_screen_reset(VTermScreen *screen, int hard)
{
  screen->damaged.start_row = -1;
  screen->pending_scrollrect.start_row = -1;
  vterm_state_reset(screen->state, hard);
  vterm_screen_flush_damage(screen);
}

static size_t _get_chars(VTermScreen *screen, const int utf8, void *buffer, size_t len, const VTermRect rect)
{
  size_t outpos = 0;
  int padding = 0;
  int row, col;
  int i;

#define PUT(c)                                             \
  if(utf8) {                                               \
    size_t thislen = utf8_seqlen(c);                       \
    if(buffer && outpos + thislen <= len)                  \
      outpos += fill_utf8((c), (char *)buffer + outpos);   \
    else                                                   \
      outpos += thislen;                                   \
  }                                                        \
  else {                                                   \
    if(buffer && outpos + 1 <= len)                        \
      ((uint32_t*)buffer)[outpos++] = (c);                 \
    else                                                   \
      outpos++;                                            \
  }

  for(row = rect.start_row; row < rect.end_row; row++) {
    for(col = rect.start_col; col < rect.end_col; col++) {
      ScreenCell *cell = getcell(screen, row, col);

      if(cell->chars[0] == 0)
        // Erased cell, might need a space
        padding++;
      else if(cell->chars[0] == (uint32_t)-1)
        // Gap behind a double-width char, do nothing
        ;
      else {
        while(padding) {
          PUT(UNICODE_SPACE);
          padding--;
        }
        for(i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell->chars[i]; i++) {
          PUT(cell->chars[i]);
        }
      }
    }

    if(row < rect.end_row - 1) {
      PUT(UNICODE_LINEFEED);
      padding = 0;
    }
  }

  return outpos;
}

size_t vterm_screen_get_chars(VTermScreen *screen, uint32_t *chars, size_t len, const VTermRect rect)
{
  return _get_chars(screen, 0, chars, len, rect);
}

size_t vterm_screen_get_text(VTermScreen *screen, char *str, size_t len, const VTermRect rect)
{
  return _get_chars(screen, 1, str, len, rect);
}

/* Copy internal to external representation of a screen cell */
void vterm_screen_get_cell(VTermScreen *screen, VTermPos pos, VTermScreenCell *cell)
{
  ScreenCell *intcell = getcell(screen, pos.row, pos.col);
  int i;

  for(i = 0; ; i++) {
    cell->chars[i] = intcell->chars[i];
    if(!intcell->chars[i])
      break;
  }

  cell->attrs.bold      = intcell->pen.bold;
  cell->attrs.underline = intcell->pen.underline;
  cell->attrs.italic    = intcell->pen.italic;
  cell->attrs.blink     = intcell->pen.blink;
  cell->attrs.reverse   = intcell->pen.reverse ^ screen->global_reverse;
  cell->attrs.strike    = intcell->pen.strike;
  cell->attrs.font      = intcell->pen.font;

  cell->fg = intcell->pen.fg;
  cell->bg = intcell->pen.bg;

  if(pos.col < (screen->cols - 1) &&
     getcell(screen, pos.row, pos.col + 1)->chars[0] == (uint32_t)-1)
    cell->width = 2;
  else
    cell->width = 1;
}

int vterm_screen_is_eol(VTermScreen *screen, VTermPos pos)
{
  /* This cell is EOL if this and every cell to the right is black */
  for(; pos.col < screen->cols; pos.col++) {
    ScreenCell *cell = getcell(screen, pos.row, pos.col);
    if(cell->chars[0] != 0)
      return 0;
  }

  return 1;
}

VTermScreen *vterm_obtain_screen(VTerm *vt)
{
  VTermScreen *screen;
  if(vt->screen)
    return vt->screen;

  screen = screen_new(vt);
  vt->screen = screen;

  return screen;
}

void vterm_screen_enable_altscreen(VTermScreen *screen, int altscreen)
{

  if(!screen->buffers[1] && altscreen) {
    int rows, cols;
    vterm_get_size(screen->vt, &rows, &cols);

    screen->buffers[1] = realloc_buffer(screen, NULL, rows, cols);
  }
}

void vterm_screen_set_callbacks(VTermScreen *screen, const VTermScreenCallbacks *callbacks, void *user)
{
  screen->callbacks = callbacks;
  screen->cbdata = user;
}

void vterm_screen_flush_damage(VTermScreen *screen)
{
  if(screen->pending_scrollrect.start_row != -1) {
    vterm_scroll_rect(screen->pending_scrollrect, screen->pending_scroll_downward, screen->pending_scroll_rightward,
        moverect_user, erase_user, screen);

    screen->pending_scrollrect.start_row = -1;
  }

  if(screen->damaged.start_row != -1) {
    if(screen->callbacks && screen->callbacks->damage)
      (*screen->callbacks->damage)(screen->damaged, screen->cbdata);

    screen->damaged.start_row = -1;
  }
}

void vterm_screen_set_damage_merge(VTermScreen *screen, VTermDamageSize size)
{
  vterm_screen_flush_damage(screen);
  screen->damage_merge = size;
}
