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
// Image transformation filters,
// Filtering classes (image_filter_base, image_filter),
// Basic filter shape classes:
//    image_filter_bilinear,
//    image_filter_bicubic,
//    image_filter_spline16,
//    image_filter_spline36,
//    image_filter_sinc64,
//    image_filter_sinc144,
//    image_filter_sinc196,
//    image_filter_sinc256
//
//----------------------------------------------------------------------------
#ifndef AGG_IMAGE_FILTERS_INCLUDED
#define AGG_IMAGE_FILTERS_INCLUDED

#include <math.h>
#include "agg_basics.h"

namespace agg
{

    // See Implementation agg_image_filters.cpp 

    enum
    {
        image_filter_shift = 14,                      //----image_filter_shift
        image_filter_size  = 1 << image_filter_shift, //----image_filter_size 
        image_filter_mask  = image_filter_size - 1,   //----image_filter_mask 

        image_subpixel_shift = 8,                         //----image_subpixel_shift
        image_subpixel_size  = 1 << image_subpixel_shift, //----image_subpixel_size 
        image_subpixel_mask  = image_subpixel_size - 1    //----image_subpixel_mask 
    };


    //-----------------------------------------------------image_filter_base
    class image_filter_base
    {
    public:
        ~image_filter_base();

        image_filter_base(unsigned dimension);

        unsigned      dimension()        const { return m_dimension;        }
        int           start()            const { return m_start;            }
        const double* weight_array_dbl() const { return m_weight_array_dbl; }
        const int*    weight_array_int() const { return m_weight_array_int; }

    protected:
        void     weight(unsigned idx, double val);
        double   calc_x(unsigned idx) const;
        void     normalize();

    private:
        image_filter_base(const image_filter_base&);
        const image_filter_base& operator = (const image_filter_base&);

        unsigned m_dimension;
        int      m_start;
        double*  m_weight_array_dbl;
        int*     m_weight_array_int;
    };



    //--------------------------------------------------------image_filter
    template<class FilterF> class image_filter : public image_filter_base
    {
    public:
        image_filter() :
            image_filter_base(FilterF::dimension()),
            m_filter_function()
        {
            unsigned i;
            unsigned dim = dimension() << image_subpixel_shift;
            for(i = 0; i < dim; i++)
            {
                weight(i, m_filter_function.calc_weight(calc_x(i)));
            }
            normalize();
        }
    private:
        FilterF m_filter_function;
    };


    //-----------------------------------------------image_filter_bilinear
    class image_filter_bilinear
    {
    public:
        static unsigned dimension() { return 2; }

        static double calc_weight(double x)
        {
            return (x <= 0.0) ? x + 1.0 : 1.0 - x;
        }
    };


    
    //------------------------------------------------image_filter_bicubic
    class image_filter_bicubic
    {
        static double pow3(double x)
        {
            return (x <= 0.0) ? 0.0 : x * x * x;
        }

    public:
        static unsigned dimension() { return 4; }

        static double calc_weight(double x)
        {
            return
                (1.0/6.0) * 
                (pow3(x + 2) - 4 * pow3(x + 1) + 6 * pow3(x) - 4 * pow3(x - 1));
        }
    };




    //----------------------------------------------image_filter_spline16
    class image_filter_spline16
    {
    public:
        static unsigned dimension() { return 4; }
        static double calc_weight(double x)
        {
            if(x < 0.0) x = -x;
            if(x < 1.0)
            {
                return ((x - 9.0/5.0 ) * x - 1.0/5.0 ) * x + 1.0;
            }
            return ((-1.0/3.0 * (x-1) + 4.0/5.0) * (x-1) - 7.0/15.0 ) * (x-1);
        }
    };



    //---------------------------------------------image_filter_spline36
    class image_filter_spline36
    {
    public:
        static unsigned dimension() { return 6; }
        static double calc_weight(double x)
        {
           if(x < 0.0) x = -x;

           if(x < 1.0)
           {
              return ((13.0/11.0 * x - 453.0/209.0) * x - 3.0/209.0) * x + 1.0;
           }
           if(x < 2.0)
           {
              return ((-6.0/11.0 * (x-1) + 270.0/209.0) * (x-1) - 156.0/ 209.0) * (x-1);
           }
   
           return ((1.0/11.0 * (x-2) - 45.0/209.0) * (x-2) +  26.0/209.0) * (x-2);
        }
    };


    //-----------------------------------------------image_filter_sinc36
    class image_filter_sinc36
    {
    public:
        static unsigned dimension() { return 6; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x3 = x * (1.0/3.0);
           return (sin(x) / x) * (sin(x3) / x3);
        }
    };


    
    //------------------------------------------------image_filter_sinc64
    class image_filter_sinc64
    {
    public:
        static unsigned dimension() { return 8; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x4 = x * 0.25;
           return (sin(x) / x) * (sin(x4) / x4);
        }
    };


    //-----------------------------------------------image_filter_sinc100
    class image_filter_sinc100
    {
    public:
        static unsigned dimension() { return 10; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x5 = x * 0.2;
           return (sin(x) / x) * (sin(x5) / x5);
        }
    };


    //-----------------------------------------------image_filter_sinc144
    class image_filter_sinc144
    {
    public:
        static unsigned dimension() { return 12; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x6 = x * (1.0/6.0);
           return (sin(x) / x) * (sin(x6) / x6);
        }
    };



    //-----------------------------------------------image_filter_sinc196
    class image_filter_sinc196
    {
    public:
        static unsigned dimension() { return 14; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x7 = x * (1.0/7.0);
           return (sin(x) / x) * (sin(x7) / x7);
        }
    };



    //-----------------------------------------------image_filter_sinc256
    class image_filter_sinc256
    {
    public:
        static unsigned dimension() { return 16; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x8 = x * 0.125;
           return (sin(x) / x) * (sin(x8) / x8);
        }
    };


    //--------------------------------------------image_filter_blackman36
    class image_filter_blackman36
    {
    public:
        static unsigned dimension() { return 6; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x3 = x * (1.0/3.0);
           return (sin(x) / x) * (0.42 + 0.5*cos(x3) + 0.08*cos(2*x3));
        }
    };


    //--------------------------------------------image_filter_blackman64
    class image_filter_blackman64
    {
    public:
        static unsigned dimension() { return 8; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x4 = x * 0.25;
           return (sin(x) / x) * (0.42 + 0.5*cos(x4) + 0.08*cos(2*x4));
        }
    };


    //-------------------------------------------image_filter_blackman100
    class image_filter_blackman100
    {
    public:
        static unsigned dimension() { return 10; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x5 = x * 0.2;
           return (sin(x) / x) * (0.42 + 0.5*cos(x5) + 0.08*cos(2*x5));
        }
    };


    //-------------------------------------------image_filter_blackman144
    class image_filter_blackman144
    {
    public:
        static unsigned dimension() { return 12; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x6 = x * (1.0/6.0);
           return (sin(x) / x) * (0.42 + 0.5*cos(x6) + 0.08*cos(2*x6));
        }
    };



    //-------------------------------------------image_filter_blackman196
    class image_filter_blackman196
    {
    public:
        static unsigned dimension() { return 14; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x7 = x * (1.0/7.0);
           return (sin(x) / x) * (0.42 + 0.5*cos(x7) + 0.08*cos(2*x7));
        }
    };



    //-------------------------------------------image_filter_blackman256
    class image_filter_blackman256
    {
    public:
        static unsigned dimension() { return 16; }
        static double calc_weight(double x)
        {
           if(x == 0.0) return 1.0;
           x *= pi;
           double x8 = x * 0.125;
           return (sin(x) / x) * (0.42 + 0.5*cos(x8) + 0.08*cos(2*x8));
        }
    };


}

#endif
