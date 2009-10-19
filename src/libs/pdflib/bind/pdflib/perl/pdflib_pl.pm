# $Id: pdflib_pl.pm 14574 2005-10-29 16:27:43Z bonefish $
package pdflib_pl;
require Exporter;
require DynaLoader;
$VERSION=5.0;
@ISA = qw(Exporter DynaLoader);
package pdflibc;
bootstrap pdflib_pl;
var_pdflib_init();
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package pdflib_pl;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package pdflib_pl;

# p_annots.c
*PDF_add_launchlink = *pdflibc::PDF_add_launchlink;
*PDF_add_locallink = *pdflibc::PDF_add_locallink;
*PDF_add_note = *pdflibc::PDF_add_note;
*PDF_add_pdflink = *pdflibc::PDF_add_pdflink;
*PDF_add_weblink = *pdflibc::PDF_add_weblink;
*PDF_attach_file = *pdflibc::PDF_attach_file;
*PDF_set_border_color = *pdflibc::PDF_set_border_color;
*PDF_set_border_dash = *pdflibc::PDF_set_border_dash;
*PDF_set_border_style = *pdflibc::PDF_set_border_style;
# p_basic.c
*PDF_begin_page = *pdflibc::PDF_begin_page;
*PDF_close = *pdflibc::PDF_close;
*PDF_delete = *pdflibc::PDF_delete;
*PDF_end_page = *pdflibc::PDF_end_page;
*PDF_get_apiname = *pdflibc::PDF_get_apiname;
*PDF_get_buffer = *pdflibc::PDF_get_buffer;
*PDF_get_errmsg = *pdflibc::PDF_get_errmsg;
*PDF_get_errnum = *pdflibc::PDF_get_errnum;
*PDF_new = *pdflibc::PDF_new;
*PDF_open_file = *pdflibc::PDF_open_file;
# p_block.c
*PDF_fill_imageblock = *pdflibc::PDF_fill_imageblock;
*PDF_fill_pdfblock = *pdflibc::PDF_fill_pdfblock;
*PDF_fill_textblock = *pdflibc::PDF_fill_textblock;
# p_color.c
*PDF_makespotcolor = *pdflibc::PDF_makespotcolor;
*PDF_setcolor = *pdflibc::PDF_setcolor;
*PDF_setgray = *pdflibc::PDF_setgray;
*PDF_setgray_fill = *pdflibc::PDF_setgray_fill;
*PDF_setgray_stroke = *pdflibc::PDF_setgray_stroke;
*PDF_setrgbcolor = *pdflibc::PDF_setrgbcolor;
*PDF_setrgbcolor_fill = *pdflibc::PDF_setrgbcolor_fill;
*PDF_setrgbcolor_stroke = *pdflibc::PDF_setrgbcolor_stroke;
# p_draw.c
*PDF_arc = *pdflibc::PDF_arc;
*PDF_arcn = *pdflibc::PDF_arcn;
*PDF_circle = *pdflibc::PDF_circle;
*PDF_clip = *pdflibc::PDF_clip;
*PDF_closepath = *pdflibc::PDF_closepath;
*PDF_closepath_fill_stroke = *pdflibc::PDF_closepath_fill_stroke;
*PDF_closepath_stroke = *pdflibc::PDF_closepath_stroke;
*PDF_curveto = *pdflibc::PDF_curveto;
*PDF_endpath = *pdflibc::PDF_endpath;
*PDF_fill = *pdflibc::PDF_fill;
*PDF_fill_stroke = *pdflibc::PDF_fill_stroke;
*PDF_lineto = *pdflibc::PDF_lineto;
*PDF_moveto = *pdflibc::PDF_moveto;
*PDF_rect = *pdflibc::PDF_rect;
*PDF_stroke = *pdflibc::PDF_stroke;
# p_encoding.c
*PDF_encoding_set_char = *pdflibc::PDF_encoding_set_char;
# p_font.c
*PDF_findfont = *pdflibc::PDF_findfont;
*PDF_load_font = *pdflibc::PDF_load_font;
*PDF_setfont = *pdflibc::PDF_setfont;
# p_gstate.c
*PDF_concat = *pdflibc::PDF_concat;
*PDF_initgraphics = *pdflibc::PDF_initgraphics;
*PDF_restore = *pdflibc::PDF_restore;
*PDF_rotate = *pdflibc::PDF_rotate;
*PDF_save = *pdflibc::PDF_save;
*PDF_scale = *pdflibc::PDF_scale;
*PDF_setdash = *pdflibc::PDF_setdash;
*PDF_setdashpattern = *pdflibc::PDF_setdashpattern;
*PDF_setflat = *pdflibc::PDF_setflat;
*PDF_setlinecap = *pdflibc::PDF_setlinecap;
*PDF_setlinejoin = *pdflibc::PDF_setlinejoin;
*PDF_setlinewidth = *pdflibc::PDF_setlinewidth;
*PDF_setmatrix = *pdflibc::PDF_setmatrix;
*PDF_setmiterlimit = *pdflibc::PDF_setmiterlimit;
*PDF_setpolydash = *pdflibc::PDF_setpolydash;
*PDF_skew = *pdflibc::PDF_skew;
*PDF_translate = *pdflibc::PDF_translate;
# p_hyper.c
*PDF_add_bookmark = *pdflibc::PDF_add_bookmark;
*PDF_add_nameddest = *pdflibc::PDF_add_nameddest;
*PDF_set_info = *pdflibc::PDF_set_info;
# p_icc.c
*PDF_load_iccprofile = *pdflibc::PDF_load_iccprofile;
# p_image.c
*PDF_add_thumbnail = *pdflibc::PDF_add_thumbnail;
*PDF_close_image = *pdflibc::PDF_close_image;
*PDF_fit_image = *pdflibc::PDF_fit_image;
*PDF_load_image = *pdflibc::PDF_load_image;
*PDF_open_CCITT = *pdflibc::PDF_open_CCITT;
*PDF_open_image = *pdflibc::PDF_open_image;
*PDF_open_image_file = *pdflibc::PDF_open_image_file;
*PDF_place_image = *pdflibc::PDF_place_image;
# p_parmas.c
*PDF_get_parameter = *pdflibc::PDF_get_parameter;
*PDF_get_value = *pdflibc::PDF_get_value;
*PDF_set_parameter = *pdflibc::PDF_set_parameter;
*PDF_set_value = *pdflibc::PDF_set_value;
# p_pattern.c
*PDF_begin_pattern = *pdflibc::PDF_begin_pattern;
*PDF_end_pattern = *pdflibc::PDF_end_pattern;
# p_pdi.c
*PDF_close_pdi = *pdflibc::PDF_close_pdi;
*PDF_close_pdi_page = *pdflibc::PDF_close_pdi_page;
*PDF_fit_pdi_page = *pdflibc::PDF_fit_pdi_page;
*PDF_get_pdi_parameter = *pdflibc::PDF_get_pdi_parameter;
*PDF_get_pdi_value = *pdflibc::PDF_get_pdi_value;
*PDF_open_pdi = *pdflibc::PDF_open_pdi;
*PDF_open_pdi_page = *pdflibc::PDF_open_pdi_page;
*PDF_place_pdi_page = *pdflibc::PDF_place_pdi_page;
*PDF_process_pdi = *pdflibc::PDF_process_pdi;
# p_resource.c
*PDF_create_pvf = *pdflibc::PDF_create_pvf;
*PDF_delete_pvf = *pdflibc::PDF_delete_pvf;
# p_shading.c
*PDF_shading = *pdflibc::PDF_shading;
*PDF_shading_pattern = *pdflibc::PDF_shading_pattern;
*PDF_shfill = *pdflibc::PDF_shfill;
# p_template.c
*PDF_begin_template = *pdflibc::PDF_begin_template;
*PDF_end_template = *pdflibc::PDF_end_template;
# p_text.c
*PDF_continue_text = *pdflibc::PDF_continue_text;
*PDF_fit_textline = *pdflibc::PDF_fit_textline;
*PDF_set_text_pos = *pdflibc::PDF_set_text_pos;
*PDF_show = *pdflibc::PDF_show;
*PDF_show_boxed = *pdflibc::PDF_show_boxed;
*PDF_show_xy = *pdflibc::PDF_show_xy;
*PDF_stringwidth = *pdflibc::PDF_stringwidth;
# p_type3.c
*PDF_begin_font = *pdflibc::PDF_begin_font;
*PDF_begin_glyph = *pdflibc::PDF_begin_glyph;
*PDF_end_font = *pdflibc::PDF_end_font;
*PDF_end_glyph = *pdflibc::PDF_end_glyph;
# p_xgstate.c
*PDF_create_gstate = *pdflibc::PDF_create_gstate;
*PDF_set_gstate = *pdflibc::PDF_set_gstate;


@EXPORT = qw( PDF_add_launchlink PDF_add_locallink PDF_add_note PDF_add_pdflink PDF_add_weblink PDF_attach_file PDF_set_border_color PDF_set_border_dash PDF_set_border_style PDF_begin_page PDF_close PDF_delete PDF_end_page PDF_get_apiname PDF_get_buffer PDF_get_errmsg PDF_get_errnum PDF_new PDF_open_file PDF_fill_imageblock PDF_fill_pdfblock PDF_fill_textblock PDF_makespotcolor PDF_setcolor PDF_setgray PDF_setgray_fill PDF_setgray_stroke PDF_setrgbcolor PDF_setrgbcolor_fill PDF_setrgbcolor_stroke PDF_arc PDF_arcn PDF_circle PDF_clip PDF_closepath PDF_closepath_fill_stroke PDF_closepath_stroke PDF_curveto PDF_endpath PDF_fill PDF_fill_stroke PDF_lineto PDF_moveto PDF_rect PDF_stroke PDF_encoding_set_char PDF_findfont PDF_load_font PDF_setfont PDF_concat PDF_initgraphics PDF_restore PDF_rotate PDF_save PDF_scale PDF_setdash PDF_setdashpattern PDF_setflat PDF_setlinecap PDF_setlinejoin PDF_setlinewidth PDF_setmatrix PDF_setmiterlimit PDF_setpolydash PDF_skew PDF_translate PDF_add_bookmark PDF_add_nameddest PDF_set_info PDF_load_iccprofile PDF_add_thumbnail PDF_close_image PDF_fit_image PDF_load_image PDF_open_CCITT PDF_open_image PDF_open_image_file PDF_place_image PDF_get_parameter PDF_get_value PDF_set_parameter PDF_set_value PDF_begin_pattern PDF_end_pattern PDF_close_pdi PDF_close_pdi_page PDF_fit_pdi_page PDF_get_pdi_parameter PDF_get_pdi_value PDF_open_pdi PDF_open_pdi_page PDF_place_pdi_page PDF_process_pdi PDF_create_pvf PDF_delete_pvf PDF_shading PDF_shading_pattern PDF_shfill PDF_begin_template PDF_end_template PDF_continue_text PDF_fit_textline PDF_set_text_pos PDF_show PDF_show_boxed PDF_show_xy PDF_stringwidth PDF_begin_font PDF_begin_glyph PDF_end_font PDF_end_glyph PDF_create_gstate PDF_set_gstate );

# ------- VARIABLE STUBS --------

package pdflib_pl;

1;
