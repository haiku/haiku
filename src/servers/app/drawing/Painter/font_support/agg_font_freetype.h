//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// See implementation agg_font_freetype.cpp
//
//----------------------------------------------------------------------------

#ifndef AGG_FONT_FREETYPE_INCLUDED
#define AGG_FONT_FREETYPE_INCLUDED

#include <ft2build.h>
#include FT_FREETYPE_H


#include "agg_scanline_storage_aa.h"
#include "agg_scanline_storage_bin.h"
#include "agg_scanline_u.h"
#include "agg_scanline_bin.h"
#include "agg_path_storage_integer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_conv_curve.h"
#include "agg_trans_affine.h"
#include "agg_font_cache_manager.h"

class ServerFont;

namespace agg
{


    //-----------------------------------------------font_engine_freetype_base
    class font_engine_freetype_base
    {
    public:
        //--------------------------------------------------------------------
        typedef serialized_scanlines_adaptor_aa<int8u>    gray8_adaptor_type;
        typedef serialized_scanlines_adaptor_bin          mono_adaptor_type;
        typedef scanline_storage_aa8                      scanlines_aa_type;
        typedef scanline_storage_bin                      scanlines_bin_type;

        //--------------------------------------------------------------------
        ~font_engine_freetype_base();
        font_engine_freetype_base(bool flag32, FT_Library library, unsigned max_faces = 32);

        // Set font parameters
        //--------------------------------------------------------------------
        void resolution(unsigned dpi);
        bool load_font(const ServerFont &font, glyph_rendering ren_type,
        			   double size);
        bool attach(const char* file_name);
        bool char_map(FT_Encoding map);
        bool size(double size);
		void transform(const trans_affine& mtx);
        void transform(double xx, double xy, double yx, double yy);
        void hinting(bool h);
        void flip_y(bool f);

        // Set Gamma
        //--------------------------------------------------------------------
        template<class GammaF> void gamma(const GammaF& f)
        {
            m_rasterizer.gamma(f);
        }

        // Accessors
        //--------------------------------------------------------------------
        int         last_error()   const { return m_last_error; }
        unsigned    resolution()   const { return m_resolution; }
        unsigned    cur_id()       const { return m_cur_id;     }
        unsigned    num_faces()    const;
        FT_Encoding char_map()     const { return m_char_map;   }
        double      size()         const { return double(m_size) / 64.0; }
        bool        hinting()      const { return m_hinting;    }
        bool        flip_y()       const { return m_flip_y;     }


        // Interface mandatory to implement for font_cache_manager
        //--------------------------------------------------------------------
        const char*     font_signature() const { return m_signature;    }
        int             change_stamp()   const { return m_change_stamp; }

        bool            prepare_glyph(unsigned glyph_code);
        unsigned        glyph_index() const { return m_glyph_index; }
        unsigned        data_size()   const { return m_data_size;   }
        glyph_data_type data_type()   const { return m_data_type;   }
        const rect_i&   bounds()      const { return m_bounds;      }
        double          advance_x()   const { return m_advance_x;   }
        double          advance_y()   const { return m_advance_y;   }
        void            write_glyph_to(int8u* data) const;
        bool            add_kerning(unsigned first, unsigned second,
                                    double* x, double* y);

    private:
        font_engine_freetype_base(const font_engine_freetype_base&);
        const font_engine_freetype_base& operator = (const font_engine_freetype_base&);

        void update_char_size();
        void update_signature();
        void update_transform();

        bool            m_flag32;
        int             m_change_stamp;
        int             m_last_error;
        unsigned        m_cur_id;
        unsigned        m_face_index;
        FT_Encoding     m_char_map;
        char*           m_signature;
        unsigned        m_size;
        FT_Matrix       m_matrix;
        bool            m_hinting;
        bool            m_flip_y;
        bool            m_library_initialized;
        FT_Library      m_library;    // handle to library    
        FT_Face         m_cur_face;  // handle to the current face object
        int             m_resolution;
        glyph_rendering m_glyph_rendering;
        unsigned        m_glyph_index;
        unsigned        m_data_size;
        glyph_data_type m_data_type;
        rect_i          m_bounds;
        double          m_advance_x;
        double          m_advance_y;

        path_storage_integer<int16, 6>              m_path16;
        path_storage_integer<int32, 6>              m_path32;
        conv_curve<path_storage_integer<int16, 6> > m_curves16;
        conv_curve<path_storage_integer<int32, 6> > m_curves32;
        scanline_u8              m_scanline_aa;
        scanline_bin             m_scanline_bin;
        scanlines_aa_type        m_scanlines_aa;
        scanlines_bin_type       m_scanlines_bin;
        rasterizer_scanline_aa<> m_rasterizer;
    };




    //------------------------------------------------font_engine_freetype_int16
    // This class uses values of type int16 (10.6 format) for the vector cache. 
    // The vector cache is compact, but when rendering glyphs of height
    // more that 200 there integer overflow can occur.
    //
    class font_engine_freetype_int16 : public font_engine_freetype_base
    {
    public:
        typedef serialized_integer_path_adaptor<int16, 6>     path_adaptor_type;
        typedef font_engine_freetype_base::gray8_adaptor_type gray8_adaptor_type;
        typedef font_engine_freetype_base::mono_adaptor_type  mono_adaptor_type;
        typedef font_engine_freetype_base::scanlines_aa_type  scanlines_aa_type;
        typedef font_engine_freetype_base::scanlines_bin_type scanlines_bin_type;

        font_engine_freetype_int16(FT_Library library, unsigned max_faces = 32) : 
            font_engine_freetype_base(false, library, max_faces) {}
    };

    //------------------------------------------------font_engine_freetype_int32
    // This class uses values of type int32 (26.6 format) for the vector cache. 
    // The vector cache is twice larger than in font_engine_freetype_int16, 
    // but it allows you to render glyphs of very large sizes.
    //
    class font_engine_freetype_int32 : public font_engine_freetype_base
    {
    public:
        typedef serialized_integer_path_adaptor<int32, 6>     path_adaptor_type;
        typedef font_engine_freetype_base::gray8_adaptor_type gray8_adaptor_type;
        typedef font_engine_freetype_base::mono_adaptor_type  mono_adaptor_type;
        typedef font_engine_freetype_base::scanlines_aa_type  scanlines_aa_type;
        typedef font_engine_freetype_base::scanlines_bin_type scanlines_bin_type;

        font_engine_freetype_int32(FT_Library library, unsigned max_faces = 32) : 
            font_engine_freetype_base(true, library, max_faces) {}
    };


}

#endif
