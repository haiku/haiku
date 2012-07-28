#ifndef __VTERM_H__
#define __VTERM_H__

#include <stdint.h>
#include <stdlib.h>

#include "vterm_input.h"

typedef struct VTerm VTerm;
typedef struct VTermState VTermState;
typedef struct VTermScreen VTermScreen;

typedef struct {
  int row;
  int col;
} VTermPos;

/* some small utility functions; we can just keep these static here */

/* order points by on-screen flow order */
static inline int vterm_pos_cmp(VTermPos a, VTermPos b)
{
  return (a.row == b.row) ? a.col - b.col : a.row - b.row;
}

typedef struct {
  int start_row;
  int end_row;
  int start_col;
  int end_col;
} VTermRect;

/* true if the rect contains the point */
static inline int vterm_rect_contains(VTermRect r, VTermPos p)
{
  return p.row >= r.start_row && p.row < r.end_row &&
         p.col >= r.start_col && p.col < r.end_col;
}

/* move a rect */
static inline void vterm_rect_move(VTermRect *rect, int row_delta, int col_delta)
{
  rect->start_row += row_delta; rect->end_row += row_delta;
  rect->start_col += col_delta; rect->end_col += col_delta;
}

/* Flag to indicate non-final subparameters in a single CSI parameter.
 * Consider
 *   CSI 1;2:3:4;5a
 * 1 4 and 5 are final.
 * 2 and 3 are non-final and will have this bit set
 *
 * Don't confuse this with the final byte of the CSI escape; 'a' in this case.
 */
#define CSI_ARG_FLAG_MORE (1<<31)
#define CSI_ARG_MASK      (~(1<<31))

#define CSI_ARG_HAS_MORE(a) ((a) & CSI_ARG_FLAG_MORE)
#define CSI_ARG(a)          ((a) & CSI_ARG_MASK)

/* Can't use -1 to indicate a missing argument; use this instead */
#define CSI_ARG_MISSING ((1UL<<31)-1)

#define CSI_ARG_IS_MISSING(a) (CSI_ARG(a) == CSI_ARG_MISSING)
#define CSI_ARG_OR(a,def)     (CSI_ARG(a) == CSI_ARG_MISSING ? (def) : CSI_ARG(a))
#define CSI_ARG_COUNT(a)      (CSI_ARG(a) == CSI_ARG_MISSING || CSI_ARG(a) == 0 ? 1 : CSI_ARG(a))

typedef struct {
  int (*text)(const char *bytes, size_t len, void *user);
  int (*control)(unsigned char control, void *user);
  int (*escape)(const char *bytes, size_t len, void *user);
  int (*csi)(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user);
  int (*osc)(const char *command, size_t cmdlen, void *user);
  int (*dcs)(const char *command, size_t cmdlen, void *user);
  int (*resize)(int rows, int cols, void *user);
} VTermParserCallbacks;

typedef struct {
  uint8_t red, green, blue;
} VTermColor;

typedef enum {
  /* VTERM_VALUETYPE_NONE = 0 */
  VTERM_VALUETYPE_BOOL = 1,
  VTERM_VALUETYPE_INT,
  VTERM_VALUETYPE_STRING,
  VTERM_VALUETYPE_COLOR,
} VTermValueType;

typedef union {
  int boolean;
  int number;
  char *string;
  VTermColor color;
} VTermValue;

typedef enum {
  /* VTERM_ATTR_NONE = 0 */
  VTERM_ATTR_BOLD = 1,   // bool:   1, 22
  VTERM_ATTR_UNDERLINE,  // number: 4, 21, 24
  VTERM_ATTR_ITALIC,     // bool:   3, 23
  VTERM_ATTR_BLINK,      // bool:   5, 25
  VTERM_ATTR_REVERSE,    // bool:   7, 27
  VTERM_ATTR_STRIKE,     // bool:   9, 29
  VTERM_ATTR_FONT,       // number: 10-19
  VTERM_ATTR_FOREGROUND, // color:  30-39 90-97
  VTERM_ATTR_BACKGROUND, // color:  40-49 100-107
} VTermAttr;

typedef enum {
  /* VTERM_PROP_NONE = 0 */
  VTERM_PROP_CURSORVISIBLE = 1, // bool
  VTERM_PROP_CURSORBLINK,       // bool
  VTERM_PROP_ALTSCREEN,         // bool
  VTERM_PROP_TITLE,             // string
  VTERM_PROP_ICONNAME,          // string
  VTERM_PROP_REVERSE,           // bool
  VTERM_PROP_CURSORSHAPE,       // number
} VTermProp;

enum {
  VTERM_PROP_CURSORSHAPE_BLOCK = 1,
  VTERM_PROP_CURSORSHAPE_UNDERLINE,
};

typedef void (*VTermMouseFunc)(int x, int y, int button, int pressed, int modifiers, void *data);

typedef struct {
  int (*putglyph)(const uint32_t chars[], int width, VTermPos pos, void *user);
  int (*movecursor)(VTermPos pos, VTermPos oldpos, int visible, void *user);
  int (*scrollrect)(VTermRect rect, int downward, int rightward, void *user);
  int (*moverect)(VTermRect dest, VTermRect src, void *user);
  int (*erase)(VTermRect rect, void *user);
  int (*initpen)(void *user);
  int (*setpenattr)(VTermAttr attr, VTermValue *val, void *user);
  int (*settermprop)(VTermProp prop, VTermValue *val, void *user);
  int (*setmousefunc)(VTermMouseFunc func, void *data, void *user);
  int (*bell)(void *user);
  int (*resize)(int rows, int cols, void *user);
} VTermStateCallbacks;

typedef struct {
  int (*damage)(VTermRect rect, void *user);
  int (*moverect)(VTermRect dest, VTermRect src, void *user);
  int (*movecursor)(VTermPos pos, VTermPos oldpos, int visible, void *user);
  int (*settermprop)(VTermProp prop, VTermValue *val, void *user);
  int (*setmousefunc)(VTermMouseFunc func, void *data, void *user);
  int (*bell)(void *user);
  int (*resize)(int rows, int cols, void *user);
} VTermScreenCallbacks;

typedef struct {
  /* libvterm relies on this memory to be zeroed out before it is returned
   * by the allocator. */
  void *(*malloc)(size_t size, void *allocdata);
  void  (*free)(void *ptr, void *allocdata);
} VTermAllocatorFunctions;

VTerm *vterm_new(int rows, int cols);
VTerm *vterm_new_with_allocator(int rows, int cols, VTermAllocatorFunctions *funcs, void *allocdata);
void   vterm_free(VTerm* vt);

void vterm_get_size(VTerm *vt, int *rowsp, int *colsp);
void vterm_set_size(VTerm *vt, int rows, int cols);

void vterm_set_parser_callbacks(VTerm *vt, const VTermParserCallbacks *callbacks, void *user);

VTermState *vterm_obtain_state(VTerm *vt);

void vterm_state_reset(VTermState *state, int hard);
void vterm_state_set_callbacks(VTermState *state, const VTermStateCallbacks *callbacks, void *user);
void vterm_state_get_cursorpos(VTermState *state, VTermPos *cursorpos);
void vterm_state_set_default_colors(VTermState *state, VTermColor *default_fg, VTermColor *default_bg);
void vterm_state_set_bold_highbright(VTermState *state, int bold_is_highbright);
int  vterm_state_get_penattr(VTermState *state, VTermAttr attr, VTermValue *val);

VTermValueType vterm_get_attr_type(VTermAttr attr);
VTermValueType vterm_get_prop_type(VTermProp prop);

VTermScreen *vterm_obtain_screen(VTerm *vt);

void vterm_screen_enable_altscreen(VTermScreen *screen, int altscreen);
void vterm_screen_set_callbacks(VTermScreen *screen, const VTermScreenCallbacks *callbacks, void *user);

typedef enum {
  VTERM_DAMAGE_CELL,    /* every cell */
  VTERM_DAMAGE_ROW,     /* entire rows */
  VTERM_DAMAGE_SCREEN,  /* entire screen */
  VTERM_DAMAGE_SCROLL,  /* entire screen + scrollrect */
} VTermDamageSize;

void vterm_screen_flush_damage(VTermScreen *screen);
void vterm_screen_set_damage_merge(VTermScreen *screen, VTermDamageSize size);

void   vterm_screen_reset(VTermScreen *screen, int hard);
size_t vterm_screen_get_chars(VTermScreen *screen, uint32_t *chars, size_t len, const VTermRect rect);
size_t vterm_screen_get_text(VTermScreen *screen, char *str, size_t len, const VTermRect rect);

typedef struct {
#define VTERM_MAX_CHARS_PER_CELL 6
  uint32_t chars[VTERM_MAX_CHARS_PER_CELL];
  char     width;
  struct {
    unsigned int bold      : 1;
    unsigned int underline : 2;
    unsigned int italic    : 1;
    unsigned int blink     : 1;
    unsigned int reverse   : 1;
    unsigned int strike    : 1;
    unsigned int font      : 4; /* 0 to 9 */
  } attrs;
  VTermColor fg, bg;
} VTermScreenCell;

void vterm_screen_get_cell(VTermScreen *screen, VTermPos pos, VTermScreenCell *cell);

int vterm_screen_is_eol(VTermScreen *screen, VTermPos pos);

void vterm_input_push_char(VTerm *vt, VTermModifier state, uint32_t c);
void vterm_input_push_key(VTerm *vt, VTermModifier state, VTermKey key);

void vterm_parser_set_utf8(VTerm *vt, int is_utf8);
void vterm_push_bytes(VTerm *vt, const char *bytes, size_t len);

size_t vterm_output_bufferlen(VTerm *vt); /* deprecated */

size_t vterm_output_get_buffer_size(VTerm *vt);
size_t vterm_output_get_buffer_current(VTerm *vt);
size_t vterm_output_get_buffer_remaining(VTerm *vt);

size_t vterm_output_bufferread(VTerm *vt, char *buffer, size_t len);

void vterm_scroll_rect(VTermRect rect,
                       int downward,
                       int rightward,
                       int (*moverect)(VTermRect src, VTermRect dest, void *user),
                       int (*eraserect)(VTermRect rect, void *user),
                       void *user);

void vterm_copy_cells(VTermRect dest,
                      VTermRect src,
                      void (*copycell)(VTermPos dest, VTermPos src, void *user),
                      void *user);

#endif
