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

#ifndef AGG_CONV_ADAPTOR_VPGEN_INCLUDED
#define AGG_CONV_ADAPTOR_VPGEN_INCLUDED

#include "agg_basics.h"
#include "agg_vertex_iterator.h"

namespace agg
{

    //======================================================conv_adaptor_vpgen
    template<class VertexSource, class VPGen> class conv_adaptor_vpgen
    {
    public:
        conv_adaptor_vpgen(VertexSource& source) : m_source(&source) {}

        void set_source(VertexSource& source) { m_source = &source; }

        VPGen& vpgen() { return m_vpgen; }
        const VPGen& vpgen() const { return m_vpgen; }

        void rewind(unsigned path_id);
        unsigned vertex(double* x, double* y);

        typedef conv_adaptor_vpgen<VertexSource, VPGen> source_type;
        typedef vertex_iterator<source_type> iterator;
        iterator begin(unsigned id) { return iterator(*this, id); }
        iterator end() { return iterator(path_cmd_stop); }

    private:
        conv_adaptor_vpgen(const conv_adaptor_vpgen<VertexSource, VPGen>&);
        const conv_adaptor_vpgen<VertexSource, VPGen>& 
            operator = (const conv_adaptor_vpgen<VertexSource, VPGen>&);

        VertexSource* m_source;
        VPGen         m_vpgen;
        double        m_start_x;
        double        m_start_y;
        unsigned      m_poly_flags;
    };



    //------------------------------------------------------------------------
    template<class VertexSource, class VPGen>
    void conv_adaptor_vpgen<VertexSource, VPGen>::rewind(unsigned path_id) 
    { 
        m_source->rewind(path_id);
        m_vpgen.reset();
        m_start_x    = 0;
        m_start_y    = 0;
        m_poly_flags = 0;
    }


    //------------------------------------------------------------------------
    template<class VertexSource, class VPGen>
    unsigned conv_adaptor_vpgen<VertexSource, VPGen>::vertex(double* x, double* y)
    {
        unsigned cmd = path_cmd_stop;
        for(;;)
        {
            cmd = m_vpgen.vertex(x, y);
            if(!is_stop(cmd)) break;

            if(m_poly_flags)
            {
                cmd = m_poly_flags;
                m_poly_flags = 0;
                break;
            }

            double tx, ty;
            cmd = m_source->vertex(&tx, &ty);
            if(is_vertex(cmd))
            {
                if(is_move_to(cmd)) 
                {
                    m_vpgen.move_to(tx, ty);
                    m_start_x = tx;
                    m_start_y = ty;
                }
                else 
                {
                    m_vpgen.line_to(tx, ty);
                }
            }
            else
            {
                if(is_end_poly(cmd))
                {
                    m_poly_flags = cmd;
                    if(is_closed(cmd))
                    {
                        m_vpgen.line_to(m_start_x, m_start_y);
                    }
                }
                else
                {
                    // The adaptor should be transparent to all unknown commands
                    break;
                }
            }
        }
        return cmd;
    }


}


#endif

