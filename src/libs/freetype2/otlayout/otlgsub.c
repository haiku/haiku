#include "otlgsub.h"
#include "otlcommn.h"

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 1                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_gsub_lookup1_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 2                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_seq_validate( OTL_Bytes      table,
                    OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    /* check glyph indices */
  }


  static void
  otl_gsub_lookup2_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, seq_count;

          OTL_CHECK( 4 );
          coverage  = OTL_NEXT_USHORT( p );
          seq_count = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( seq_count*2 );
          for ( ; seq_count > 0; seq_count-- )
            otl_seq_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 3                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_alternate_set_validate( OTL_Bytes      table,
                              OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    /* XXX: check glyph indices */
  }


  static void
  otl_gsub_lookup3_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_alternate_set_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 4                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_ligature_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_UInt  glyph_id, count;

    OTL_CHECK( 4 );
    glyph_id = OTL_NEXT_USHORT( p );
    count    = OTL_NEXT_USHORT( p );

    if ( count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( 2*(count-1) );
    /* XXX: check glyph indices */
  }


  static void
  otl_ligature_set_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_ligature_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gsub_lookup4_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_ligature_set_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 5                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  static void
  otl_sub_rule_validate( OTL_Bytes      table,
                         OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   glyph_count, subst_count;

    OTL_CHECK( 4 );
    glyph_count = OTL_NEXT_USHORT( p );
    subst_count = OTL_NEXT_USHORT( p );

    if ( glyph_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( (glyph_count-1)*2 + substcount*4 );

    /* XXX: check glyph indices and subst lookups */
  }


  static void
  otl_sub_rule_set_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_sub_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_sub_class_rule_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_UInt  glyph_count, subst_count;

    OTL_CHECK( 4 );
    glyph_count = OTL_NEXT_USHORT( p );
    subst_count = OTL_NEXT_USHORT( p );

    if ( glyph_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( (glyph_count-1)*2 + substcount*4 );

    /* XXX: check glyph indices and subst lookups */
  }


  static void
  otl_sub_class_rule_set_validate( OTL_Bytes      table,
                                   OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_sub_class_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gsub_lookup5_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_sub_rule_set_validate( table + coverage, valid );
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, class_def, count;

          OTL_CHECK( 6 );
          coverage  = OTL_NEXT_USHORT( p );
          class_def = OTL_NEXT_USHORT( p );
          count     = OTL_NEXT_USHORT( p );

          otl_coverage_validate        ( table + coverage, valid );
          otl_class_definition_validate( table + class_def, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_sub_class_rule_set_validate( table + coveragen valid );
        }
        break;

      case 3:
        {
          OTL_UInt  glyph_count, subst_count, count;

          OTL_CHECK( 4 );
          glyph_count = OTL_NEXT_USHORT( p );
          subst_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*glyph_count + 4*subst_count );
          for ( count = glyph_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 6                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  static void
  otl_chain_sub_rule_validate( OTL_Bytes      table,
                               OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   back_count, input_count, ahead_count, subst_count, count;

    OTL_CHECK( 2 );
    back_count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*back_count+2 );
    p += 2*back_count;

    input_count = OTL_NEXT_USHORT( p );
    if ( input_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( 2*input_count );
    p += 2*(input_count-1);

    ahead_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 2*ahead_count + 2 );
    p += 2*ahead_count;

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 4*count );

    /* XXX: check glyph indices and subst lookups */
  }


  static void
  otl_chain_sub_rule_set_validate( OTL_Bytes      table,
                                   OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_chain_sub_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_chain_sub_class_rule_validate( OTL_Bytes      table,
                                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   back_count, input_count, ahead_count, subst_count, count;

    OTL_CHECK( 2 );
    back_count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*back_count+2 );
    p += 2*back_count;

    input_count = OTL_NEXT_USHORT( p );
    if ( input_count == 0 )
      OTL_INVALID_DATA;

    OTL_CHECK( 2*input_count );
    p += 2*(input_count-1);

    ahead_count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 2*ahead_count + 2 );
    p += 2*ahead_count;

    count = OTL_NEXT_USHORT( p );
    OTL_CHECK( 4*count );

    /* XXX: check class indices and subst lookups */
  }



  static void
  otl_chain_sub_class_set_validate( OTL_Bytes      table,
                                    OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   count;

    OTL_CHECK( 2 );
    count = OTL_NEXT_USHORT( p );

    OTL_CHECK( 2*count );
    for ( ; count > 0; count-- )
      otl_chain_sub_rule_validate( table + OTL_NEXT_USHORT( p ), valid );
  }


  static void
  otl_gsub_lookup6_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt  coverage, count;

          OTL_CHECK( 4 );
          coverage = OTL_NEXT_USHORT( p );
          count    = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_chain_sub_rule_set_validate( table + coverage, valid );
        }
        break;

      case 2:
        {
          OTL_UInt  coverage, back_class, input_class, ahead_class, count;

          OTL_CHECK( 10 );
          coverage    = OTL_NEXT_USHORT( p );
          back_class  = OTL_NEXT_USHORT( p );
          input_class = OTL_NEXT_USHORT( p );
          ahead_class = OTL_NEXT_USHORT( p );
          count       = OTL_NEXT_USHORT( p );

          otl_coverage_validate( table + coverage, valid );

          otl_class_definition_validate( table + back_class,  valid );
          otl_class_definition_validate( table + input_class, valid );
          otl_class_definition_validate( table + ahead_class, valid );

          OTL_CHECK( 2*count );
          for ( ; count > 0; count-- )
            otl_chain_sub_class_set( table + OTL_NEXT_USHORT( p ), valid );
        }
        break;

      case 3:
        {
          OTL_UInt  back_count, input_count, ahead_count, subst_count, count;

          OTL_CHECK( 2 );
          back_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*back_count+2 );
          for ( count = back_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          input_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*input_count+2 );
          for ( count = input_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          ahead_count = OTL_NEXT_USHORT( p );

          OTL_CHECK( 2*ahead_count+2 );
          for ( count = ahead_count; count > 0; count-- )
            otl_coverage_validate( table + OTL_NEXT_USHORT( p ), valid );

          subst_count = OTL_NEXT_USHORT( p );
          OTL_CHECK( subst_count*4 );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                 GSUB LOOKUP TYPE 6                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  static void
  otl_gsub_lookup7_validate( OTL_Bytes      table,
                             OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   format, coverage;

    OTL_CHECK( 2 );
    format = OTL_NEXT_USHORT( p );
    switch ( format )
    {
      case 1:
        {
          OTL_UInt          lookup_type, lookup_offset;
          OTL_ValidateFunc  validate;

          OTL_CHECK( 6 );
          lookup_type   = OTL_NEXT_USHORT( p );
          lookup_offset = OTL_NEXT_ULONG( p );

          if ( lookup_type == 0 || lookup_type >= 7 )
            OTL_INVALID_DATA;

          validate = otl_gsub_validate_funcs[ lookup_type-1 ];
          validate( table + lookup_offset, valid );
        }
        break;

      default:
        OTL_INVALID_DATA;
    }
  }


  static const OTL_ValidateFunc  otl_gsub_validate_funcs[ 7 ] =
  {
    otl_gsub_lookup1_validate,
    otl_gsub_lookup2_validate,
    otl_gsub_lookup3_validate,
    otl_gsub_lookup4_validate,
    otl_gsub_lookup5_validate,
    otl_gsub_lookup6_validate
  };

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                         GSUB TABLE                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


  OTL_LOCALDEF( void )
  otl_gsub_validate( OTL_Bytes      table,
                     OTL_Validator  valid )
  {
    OTL_Bytes  p = table;
    OTL_UInt   scripts, features, lookups;

    OTL_CHECK( 10 );

    if ( OTL_NEXT_USHORT( p ) != 0x10000UL )
      OTL_INVALID_DATA;

    scripts  = OTL_NEXT_USHORT( p );
    features = OTL_NEXT_USHORT( p );
    lookups  = OTL_NEXT_USHORT( p );

    otl_script_list_validate ( table + scripts, valid );
    otl_feature_list_validate( table + features, valid );

    otl_lookup_list_validate( table + lookups, 7, otl_gsub_validate_funcs,
                              valid );
  }
