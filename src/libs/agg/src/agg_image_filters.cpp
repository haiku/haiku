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
// Filtering class image_filter_base implemantation
//
//----------------------------------------------------------------------------


#include "agg_image_filters.h"


namespace agg
{

    //--------------------------------------------------------------------
    image_filter_base::~image_filter_base()
    {
        delete [] m_weight_array_int;
        delete [] m_weight_array_dbl;
    }


    //--------------------------------------------------------------------
    image_filter_base::image_filter_base(unsigned dimension) :
        m_dimension(dimension),
        m_start(-int(dimension / 2 - 1)),
        m_weight_array_dbl(new double [dimension << image_subpixel_shift]),
        m_weight_array_int(new int [dimension << image_subpixel_shift])
    {
    }


    //--------------------------------------------------------------------
    void image_filter_base::weight(unsigned idx, double val)
    {
        m_weight_array_dbl[idx] = val;
        m_weight_array_int[idx] = int(val * image_filter_size);
    }

    //--------------------------------------------------------------------
    double image_filter_base::calc_x(unsigned idx) const
    {
        return double(idx) / double(image_subpixel_size) - 
               double(m_dimension / 2);
    }

    //--------------------------------------------------------------------
    // This function normalizes integer values and corrects the rounding 
    // errors. It doesn't do anything with the source floating point values
    // (m_weight_array_dbl), it corrects only integers according to the rule 
    // of 1.0 which means that any sum of pixel weights must be equal to 1.0.
    // So, the filter function must produce a graph of the proper shape.
    //--------------------------------------------------------------------
    void image_filter_base::normalize()
    {
        unsigned i;
        int flip = 1;

        for(i = 0; i < image_subpixel_size; i++)
        {
            for(;;)
            {
                int sum = 0;
                unsigned j;
                for(j = 0; j < m_dimension; j++)
                {
                    sum += m_weight_array_int[j * image_subpixel_size + i];
                }
                sum -= image_filter_size;

                if(sum == 0) break;

                int inc = (sum > 0) ? -1 : 1;

                for(j = 0; j < m_dimension && sum; j++)
                {
                    flip ^= 1;
                    unsigned idx = flip ? m_dimension/2 + j/2 : m_dimension/2 - j/2;
                    int v = m_weight_array_int[idx * image_subpixel_size + i];
                    if(v < image_filter_size)
                    {
                        m_weight_array_int[idx * image_subpixel_size + i] += inc;
                        sum += inc;
                    }
                }
            }
        }
    }

}

