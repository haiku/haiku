<?xml version='1.0' encoding='ISO-8859-1' standalone='yes' ?>
<tagfile>
  <compound kind="file">
    <name>curve.h</name>
    <path>/home/rlk/sandbox/print-5.1/include/gutenprintui2/</path>
    <filename>curve_8h</filename>
    <class kind="struct">_StpuiCurve</class>
    <class kind="struct">_StpuiCurveClass</class>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_CURVE</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a887bee281f88accbe674790e5e5b28e3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_CURVE</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a03272c56621cf16c2caa378231c7fa43</anchor>
      <arglist>(obj)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_CURVE_CLASS</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>ad788bc4e6a85701756b28d972a64b7f8</anchor>
      <arglist>(klass)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_IS_CURVE</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a92f2038dd52d2b3af7c03bdeb5918567</anchor>
      <arglist>(obj)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_IS_CURVE_CLASS</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>aee626496df16d25159b91982478b0df9</anchor>
      <arglist>(klass)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_CURVE_GET_CLASS</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a41b32938b1bfc087ca93b3655ef03ab6</anchor>
      <arglist>(obj)</arglist>
    </member>
    <member kind="typedef">
      <type>struct _StpuiCurve</type>
      <name>StpuiCurve</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a5c0213fede0b7d8a91b66e4679cc899a</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct _StpuiCurveClass</type>
      <name>StpuiCurveClass</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a057f1c40a70e84c7a187b423ebe4aada</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>StpuiCurveType</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a59cb8817385039367325d6dbb4a0996b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>STPUI_CURVE_TYPE_LINEAR</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a59cb8817385039367325d6dbb4a0996ba741f86dc6dfb50a67dd621a287014de1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>STPUI_CURVE_TYPE_SPLINE</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a59cb8817385039367325d6dbb4a0996ba6cef8a118ee2829db63f812874a412c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>STPUI_CURVE_TYPE_FREE</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a59cb8817385039367325d6dbb4a0996ba1a204ef2d450d762415a1c0c1c77db89</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>GType</type>
      <name>stpui_curve_get_type</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a233b21367f03cd5ba884d4e3c742c8d2</anchor>
      <arglist>(void) G_GNUC_CONST</arglist>
    </member>
    <member kind="function">
      <type>GtkWidget *</type>
      <name>stpui_curve_new</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>acbdc99aaef1f5672d0294d9742fdc398</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_curve_reset</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a466fe70debd1a7eca3988f91a109009f</anchor>
      <arglist>(StpuiCurve *curve)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_curve_set_gamma</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a956d80d857cf5927768a38103d8af705</anchor>
      <arglist>(StpuiCurve *curve, gfloat gamma_)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_curve_set_range</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>ad4dcd042b1e50672781895d886adad66</anchor>
      <arglist>(StpuiCurve *curve, gfloat min_x, gfloat max_x, gfloat min_y, gfloat max_y)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_curve_get_vector</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a43799d3f187018958cb78ece053dadda</anchor>
      <arglist>(StpuiCurve *curve, int veclen, gfloat vector[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_curve_set_vector</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>abe1c943341872697e6219d7053db2804</anchor>
      <arglist>(StpuiCurve *curve, int veclen, const gfloat vector[])</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_curve_set_curve_type</name>
      <anchorfile>curve_8h.html</anchorfile>
      <anchor>a196839b2b6e39ae4bd5ccc9aad4ff8f7</anchor>
      <arglist>(StpuiCurve *curve, StpuiCurveType type)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>gammacurve.h</name>
    <path>/home/rlk/sandbox/print-5.1/include/gutenprintui2/</path>
    <filename>gammacurve_8h</filename>
    <class kind="struct">_StpuiGammaCurve</class>
    <class kind="struct">_StpuiGammaCurveClass</class>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_GAMMA_CURVE</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>a9c72ddb3a35cadb3cc00b316cbb8e601</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_GAMMA_CURVE</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>a1799655d0ba34bbf0f856411399f222e</anchor>
      <arglist>(obj)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_GAMMA_CURVE_CLASS</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>a45aa6ff6c9db14c6de371c1c9662483b</anchor>
      <arglist>(klass)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_IS_GAMMA_CURVE</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>a9a10e0a1241d9f042e2e1a2cf14b70c3</anchor>
      <arglist>(obj)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_IS_GAMMA_CURVE_CLASS</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>ad5a762ee6059464c4d4d2f175d716c16</anchor>
      <arglist>(klass)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_GAMMA_CURVE_GET_CLASS</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>ae0813a802c9f9ee0a8fa4d3ff372ef0f</anchor>
      <arglist>(obj)</arglist>
    </member>
    <member kind="typedef">
      <type>struct _StpuiGammaCurve</type>
      <name>StpuiGammaCurve</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>a16a893a0c5c5908d5db1a36c91590d6c</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct _StpuiGammaCurveClass</type>
      <name>StpuiGammaCurveClass</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>aec66cf466c32b2ead30a56c3fb1a3e09</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>GType</type>
      <name>stpui_gamma_curve_get_type</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>ae52edc31b54a94c1e4e106f1966af6e4</anchor>
      <arglist>(void) G_GNUC_CONST</arglist>
    </member>
    <member kind="function">
      <type>GtkWidget *</type>
      <name>stpui_gamma_curve_new</name>
      <anchorfile>gammacurve_8h.html</anchorfile>
      <anchor>a39433a30aa8d9b5a4e3fd1aa4176eb2f</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>gutenprintui.h</name>
    <path>/home/rlk/sandbox/print-5.1/include/gutenprintui2/</path>
    <filename>gutenprintui_8h</filename>
    <includes id="curve_8h" name="curve.h" local="no" imported="no">gutenprintui2/curve.h</includes>
    <includes id="gammacurve_8h" name="gammacurve.h" local="no" imported="no">gutenprintui2/gammacurve.h</includes>
    <includes id="typebuiltins_8h" name="typebuiltins.h" local="no" imported="no">gutenprintui2/typebuiltins.h</includes>
    <class kind="struct">stpui_plist_t</class>
    <class kind="struct">stpui_image</class>
    <member kind="typedef">
      <type>struct stpui_image</type>
      <name>stpui_image_t</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a195aac96c77c6de3925cd3d13c6ce2f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>guchar *(*</type>
      <name>get_thumbnail_func_t</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a9b59c94766b4713803ec6e1daaa84e70</anchor>
      <arglist>)(void *data, gint *width, gint *height, gint *bpp, gint page)</arglist>
    </member>
    <member kind="enumeration">
      <name>orient_t</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ac33232f845969eb04b32e1006c8240a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>ORIENT_AUTO</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ac33232f845969eb04b32e1006c8240a0a06af5eabe5d18a247c02641283fe4481</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>ORIENT_PORTRAIT</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ac33232f845969eb04b32e1006c8240a0af81c23cf1763365fba8e36db87131ff5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>ORIENT_LANDSCAPE</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ac33232f845969eb04b32e1006c8240a0a4dc9735e5b3e4c86b60141acc08e5db1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>ORIENT_UPSIDEDOWN</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ac33232f845969eb04b32e1006c8240a0a3ca76babc9953cc92305856ac00a2350</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>ORIENT_SEASCAPE</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ac33232f845969eb04b32e1006c8240a0a7de33495add2d66cc743f2edcbb548e3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>command_t</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ab31350eb38b009cbd282027630a1ee10</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>COMMAND_TYPE_DEFAULT</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ab31350eb38b009cbd282027630a1ee10aa6a7840c2276bfe38dd68b4fe3a8babf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>COMMAND_TYPE_CUSTOM</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ab31350eb38b009cbd282027630a1ee10acb7cf14821d9fcbaa983ee33d7b8e926</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>COMMAND_TYPE_FILE</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ab31350eb38b009cbd282027630a1ee10a5e9a887a5dab88a425cb52c8b1e49a7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_name</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>af54293f58c474e133e4a60054779c9be</anchor>
      <arglist>(stpui_plist_t *p, const char *val)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_name_n</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a90a9ed90e03acd443d08350a62aa3762</anchor>
      <arglist>(stpui_plist_t *p, const char *val, int n)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_plist_get_name</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a342817c9f4311f0ac827c94b0e62cbbb</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_queue_name</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a8a4fa7000900cb4813000048c322dcdb</anchor>
      <arglist>(stpui_plist_t *p, const char *val)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_queue_name_n</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aec75fb2ec25a5a69d270d70ea33c81eb</anchor>
      <arglist>(stpui_plist_t *p, const char *val, int n)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_plist_get_queue_name</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aacf031afbe7e9682f7367f1ae0ef1895</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_output_filename</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a248eae3ae0a96506fa8c92807f70d457</anchor>
      <arglist>(stpui_plist_t *p, const char *val)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_output_filename_n</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aea9ddb061703368f7ea1e3b861b8550d</anchor>
      <arglist>(stpui_plist_t *p, const char *val, int n)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_plist_get_output_filename</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a856c534e664748c9a419104357312dfd</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_extra_printer_options</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a72ee8e1bb9bee2e13c15a9aecd9582bd</anchor>
      <arglist>(stpui_plist_t *p, const char *val)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_extra_printer_options_n</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a9899c88da60069b72d7ddb601bcad548</anchor>
      <arglist>(stpui_plist_t *p, const char *val, int n)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_plist_get_extra_printer_options</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a4956cdd55680ed20a92e991d4cf52ee8</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_custom_command</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>af127a39a7ea466ca73e5a511f36ae985</anchor>
      <arglist>(stpui_plist_t *p, const char *val)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_custom_command_n</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a567c7c6c13930e398def69f2829bd038</anchor>
      <arglist>(stpui_plist_t *p, const char *val, int n)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_plist_get_custom_command</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>abc624051678aac91a2ffa6c7c5393b8d</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_copy_count</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a02092bb706d3770870ed5f64efb1ea2a</anchor>
      <arglist>(stpui_plist_t *p, gint count)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>stpui_plist_get_copy_count</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a7d4511d9e9c69136fabf9f9c64734c51</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_current_standard_command</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a79e27ac73e0f082abcdde41ee76879ce</anchor>
      <arglist>(stpui_plist_t *p, const char *val)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_current_standard_command_n</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aa405ef9e03818fedcff5746b9701c3c3</anchor>
      <arglist>(stpui_plist_t *p, const char *val, int n)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_plist_get_current_standard_command</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>afdc574a731f6697c3e48c6001a51b3af</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_set_command_type</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aa920697330124635ca464662caf975d5</anchor>
      <arglist>(stpui_plist_t *p, command_t val)</arglist>
    </member>
    <member kind="function">
      <type>command_t</type>
      <name>stpui_plist_get_command_type</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a3c9ed1191c6a03edba14f7b98d03ef55</anchor>
      <arglist>(const stpui_plist_t *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_global_parameter</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aeb3ea1fd028cb28211bac9e88f9ca8ac</anchor>
      <arglist>(const char *param, const char *value)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_get_global_parameter</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a28c71e75188d60a243755deaab31b5fd</anchor>
      <arglist>(const char *param)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_plist_copy</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a01db968552106a84a49f76d7203bd3f5</anchor>
      <arglist>(stpui_plist_t *vd, const stpui_plist_t *vs)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>stpui_plist_add</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ab2e655748deab72a24c25bfaf4ee9052</anchor>
      <arglist>(const stpui_plist_t *key, int add_only)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_printer_initialize</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a54daa32f5d5d64c644131570eab01c2b</anchor>
      <arglist>(stpui_plist_t *printer)</arglist>
    </member>
    <member kind="function">
      <type>const stpui_plist_t *</type>
      <name>stpui_get_current_printer</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a26ebbd1948a457740cd75ff630969487</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>stpui_build_standard_print_command</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a97f0f11c41859d80d9bb3803f81a671f</anchor>
      <arglist>(const stpui_plist_t *plist, const stp_printer_t *printer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_printrc_file</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a443905f09bfd6cad7fd06aa029ac306c</anchor>
      <arglist>(const char *name)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_get_printrc_file</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a8faab74e1f9c2b372efa56ae6fea713e</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_printrc_load</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a54f67bb1819c7135a0ca1f005a22d28b</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_get_system_printers</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ada642fd5eeff02d4c6b84d3125ca2da2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_printrc_save</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a8c4bddac236a6557ec126fd659ceade0</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_image_filename</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a701289a198a313c5b6758ab1cd678c58</anchor>
      <arglist>(const char *)</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>stpui_get_image_filename</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a8bc64271197449021140ce2d17ad71e2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_errfunc</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>acc06b85ae6af8ca0003584da411213fb</anchor>
      <arglist>(stp_outfunc_t wfunc)</arglist>
    </member>
    <member kind="function">
      <type>stp_outfunc_t</type>
      <name>stpui_get_errfunc</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ada3d388496b4b7d689f4cf8a957c75b2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_errdata</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>ad013cb96f40b5da4bca5dd9485aaa213</anchor>
      <arglist>(void *errdata)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>stpui_get_errdata</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a15368f674e52b511ad665cb2be45576c</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>gint</type>
      <name>stpui_do_print_dialog</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aac1066fa59dc8a04b90415994587ec1a</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>gint</type>
      <name>stpui_compute_orientation</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a6932b380986d06d6dd7671439aaf93a0</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_image_dimensions</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aa52f1b5d60ea21d09cfbab998df7ff33</anchor>
      <arglist>(gint width, gint height)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_image_resolution</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a3fa4a6d1300a470016744badf81daa43</anchor>
      <arglist>(gdouble xres, gdouble yres)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_image_type</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a7b2f104989fb67ca20e1b9874e047293</anchor>
      <arglist>(const char *image_type)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_image_raw_channels</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a0d40ca93fed8c7d10dcc516e6fb61ea1</anchor>
      <arglist>(gint channels)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_image_channel_depth</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a845898656d91fbf462db9c9b3e7c976f</anchor>
      <arglist>(gint bit_depth)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_thumbnail_func</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>afcaf0ce6ff01c073cdfb2d2f80ede234</anchor>
      <arglist>(get_thumbnail_func_t)</arglist>
    </member>
    <member kind="function">
      <type>get_thumbnail_func_t</type>
      <name>stpui_get_thumbnail_func</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a74c1f441bf3fbb2198cf4cbaca8c23ef</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stpui_set_thumbnail_data</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a965e5950073e3ef6775f3355556517f6</anchor>
      <arglist>(void *)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>stpui_get_thumbnail_data</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>aee04dfc45b26093bd65d3025b817cfd5</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>stpui_print</name>
      <anchorfile>gutenprintui_8h.html</anchorfile>
      <anchor>a1ffdddfb6efd3353d403192b2b2c1e20</anchor>
      <arglist>(const stpui_plist_t *printer, stpui_image_t *im)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>t.h</name>
    <path>/home/rlk/sandbox/print-5.1/include/gutenprintui2/</path>
    <filename>t_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_ORIENT_T</name>
      <anchorfile>t_8h.html</anchorfile>
      <anchor>a7c68a632b60b65d497e2933fa1b78a77</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_COMMAND_T</name>
      <anchorfile>t_8h.html</anchorfile>
      <anchor>ab213314f887b8b4f48f9aa1531673a9c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_CURVE_TYPE</name>
      <anchorfile>t_8h.html</anchorfile>
      <anchor>af0561b03a3bfb203b40c37f9bb373ffb</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>G_BEGIN_DECLS GType</type>
      <name>orient_t_orient_t_get_type</name>
      <anchorfile>t_8h.html</anchorfile>
      <anchor>ab05fb37a6c79f2b6b417ff107d9bb881</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>GType</type>
      <name>command_t_command_t_get_type</name>
      <anchorfile>t_8h.html</anchorfile>
      <anchor>a5e6b50173b88263be23734e76f4a39f2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>GType</type>
      <name>stpui_curve_type_get_type</name>
      <anchorfile>t_8h.html</anchorfile>
      <anchor>a1252245c3967f9e655de3d62c3999230</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>typebuiltins.h</name>
    <path>/home/rlk/sandbox/print-5.1/include/gutenprintui2/</path>
    <filename>typebuiltins_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_ORIENT_T</name>
      <anchorfile>typebuiltins_8h.html</anchorfile>
      <anchor>a7c68a632b60b65d497e2933fa1b78a77</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_COMMAND_T</name>
      <anchorfile>typebuiltins_8h.html</anchorfile>
      <anchor>ab213314f887b8b4f48f9aa1531673a9c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STPUI_TYPE_CURVE_TYPE</name>
      <anchorfile>typebuiltins_8h.html</anchorfile>
      <anchor>af0561b03a3bfb203b40c37f9bb373ffb</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>G_BEGIN_DECLS GType</type>
      <name>orient_t_orient_t_get_type</name>
      <anchorfile>typebuiltins_8h.html</anchorfile>
      <anchor>ab05fb37a6c79f2b6b417ff107d9bb881</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>GType</type>
      <name>command_t_command_t_get_type</name>
      <anchorfile>typebuiltins_8h.html</anchorfile>
      <anchor>a5e6b50173b88263be23734e76f4a39f2</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>GType</type>
      <name>stpui_curve_type_get_type</name>
      <anchorfile>typebuiltins_8h.html</anchorfile>
      <anchor>a1252245c3967f9e655de3d62c3999230</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>_StpuiCurve</name>
    <filename>struct__StpuiCurve.html</filename>
    <member kind="variable">
      <type>GtkDrawingArea</type>
      <name>graph</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>ade1a0d11481ca5e94e31025b5815c88f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gint</type>
      <name>cursor_type</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a5a45de0c0dd843601eb678cf898b6588</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gfloat</type>
      <name>min_x</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>aa2e971c049f0f314e9459b21d591dde2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gfloat</type>
      <name>max_x</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a873e40a32b9a0e863ecf5af33233819d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gfloat</type>
      <name>min_y</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a035fd7e1c773ea3291dc40f6c2a95f07</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gfloat</type>
      <name>max_y</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>aa4fbc033c9dc17d11be58786f5bfe817</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GdkPixmap *</type>
      <name>pixmap</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>aa1e237c2fafc8ce7b06f73b8af401de1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>StpuiCurveType</type>
      <name>curve_type</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>ac88de71f409ac73ffe41489313cacff2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gint</type>
      <name>height</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>ac679cbbcaa013aa84ac1e799d7299479</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gint</type>
      <name>grab_point</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a30591159410734580ff586f4c62ac622</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gint</type>
      <name>last</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a4866f6481ca65da32e4beda66c71381b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gint</type>
      <name>num_points</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>ad8005c397affa21ad090cd0868e4fb60</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GdkPoint *</type>
      <name>point</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>af7e80ace854c3b42cfcb419eacd7766f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gint</type>
      <name>num_ctlpoints</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a875906e982189427a2a3dda0e7cc38ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>gfloat(*</type>
      <name>ctlpoint</name>
      <anchorfile>struct__StpuiCurve.html</anchorfile>
      <anchor>a05e34073df0ce5bf7575b2e7383577a8</anchor>
      <arglist>)[2]</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>_StpuiCurveClass</name>
    <filename>struct__StpuiCurveClass.html</filename>
    <member kind="variable">
      <type>GtkDrawingAreaClass</type>
      <name>parent_class</name>
      <anchorfile>struct__StpuiCurveClass.html</anchorfile>
      <anchor>a60fce1a46ad405750d42c7aff83707bb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>curve_type_changed</name>
      <anchorfile>struct__StpuiCurveClass.html</anchorfile>
      <anchor>a3098d1e480ba77e57785667ccf0b1c9f</anchor>
      <arglist>)(StpuiCurve *curve)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved1</name>
      <anchorfile>struct__StpuiCurveClass.html</anchorfile>
      <anchor>a97e1aa40263796ab3dada695e40193cb</anchor>
      <arglist>)(void)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved2</name>
      <anchorfile>struct__StpuiCurveClass.html</anchorfile>
      <anchor>a053157829bb7c073427955e16c93c863</anchor>
      <arglist>)(void)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved3</name>
      <anchorfile>struct__StpuiCurveClass.html</anchorfile>
      <anchor>a64b1fa1cf35f409932139752569ca1f9</anchor>
      <arglist>)(void)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved4</name>
      <anchorfile>struct__StpuiCurveClass.html</anchorfile>
      <anchor>af1b28ac82d8fa56fbc0a84f17d26d7b0</anchor>
      <arglist>)(void)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>_StpuiGammaCurve</name>
    <filename>struct__StpuiGammaCurve.html</filename>
    <member kind="variable">
      <type>GtkVBox</type>
      <name>vbox</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>a07e6d5e4479cbca4a927635cc6d02f26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GtkWidget *</type>
      <name>table</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>af4a067c4f9e19c57d080b07aca9252b3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GtkWidget *</type>
      <name>curve</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>aef8d7f12bd9a16784147cc3841d966c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GtkWidget *</type>
      <name>button</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>a6ec3a772b95ebde518266e23d2c6d351</anchor>
      <arglist>[5]</arglist>
    </member>
    <member kind="variable">
      <type>gfloat</type>
      <name>gamma</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>a7783842531471a1605b5d38c7a02a52e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GtkWidget *</type>
      <name>gamma_dialog</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>aef9135d931e5a99a0d35c40a8fc79b5b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>GtkWidget *</type>
      <name>gamma_text</name>
      <anchorfile>struct__StpuiGammaCurve.html</anchorfile>
      <anchor>aabe5eae4e5aa05c6a62aab34d236c26b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>_StpuiGammaCurveClass</name>
    <filename>struct__StpuiGammaCurveClass.html</filename>
    <member kind="variable">
      <type>GtkVBoxClass</type>
      <name>parent_class</name>
      <anchorfile>struct__StpuiGammaCurveClass.html</anchorfile>
      <anchor>ae784cb24d21ce7c707727c5996fed405</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved1</name>
      <anchorfile>struct__StpuiGammaCurveClass.html</anchorfile>
      <anchor>a11f61971dbf7791712b4ea447b00518d</anchor>
      <arglist>)(void)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved2</name>
      <anchorfile>struct__StpuiGammaCurveClass.html</anchorfile>
      <anchor>af471d7067cff6f4f8908c147f4d7c99c</anchor>
      <arglist>)(void)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved3</name>
      <anchorfile>struct__StpuiGammaCurveClass.html</anchorfile>
      <anchor>a337220ca7c1966c911dd856d8377c66d</anchor>
      <arglist>)(void)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>_gtk_reserved4</name>
      <anchorfile>struct__StpuiGammaCurveClass.html</anchorfile>
      <anchor>ad6007664d93b51b3f533998dfc836634</anchor>
      <arglist>)(void)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>stpui_image</name>
    <filename>structstpui__image.html</filename>
    <member kind="variable">
      <type>stp_image_t</type>
      <name>im</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>a00862540482f307175e9fbf252751320</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>transpose</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>a5f2a75c7187c6abcf20e15e2aad22549</anchor>
      <arglist>)(struct stpui_image *image)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>hflip</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>af2733a318654d50214c94fccd9a018b7</anchor>
      <arglist>)(struct stpui_image *image)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>vflip</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>ac378339fe87fa21d5f117fc11b3f5b6c</anchor>
      <arglist>)(struct stpui_image *image)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>rotate_ccw</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>a0a31280def8cd739184192dbe8d34fa8</anchor>
      <arglist>)(struct stpui_image *image)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>rotate_cw</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>ae111082f0f23096d89e024c4fde9c0d6</anchor>
      <arglist>)(struct stpui_image *image)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>rotate_180</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>aaaae08fdabffb3d31ac761dfe0624506</anchor>
      <arglist>)(struct stpui_image *image)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>crop</name>
      <anchorfile>structstpui__image.html</anchorfile>
      <anchor>a96e6cc155230793c8aa847e5a7cd7255</anchor>
      <arglist>)(struct stpui_image *image, int left, int top, int right, int bottom)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>stpui_plist_t</name>
    <filename>structstpui__plist__t.html</filename>
    <member kind="variable">
      <type>char *</type>
      <name>name</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a95d5e51f00ae03f4ca085120fc7b88e6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>command_t</type>
      <name>command_type</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>ac1243181065442af4c72c24c0d3901f3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>queue_name</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a34155aeda35a97423a70071478a94469</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>extra_printer_options</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>aff2ab5a8193a99dc8014d804281173c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>custom_command</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a783211034ef118e102c9c07eb9f9a4fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>current_standard_command</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>adde7851705ff38bb9b14c1ec11bb53cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>output_filename</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>aee538bc59fd77730a5f73d3ea7d51d18</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>float</type>
      <name>scaling</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a7325ec55acf12db60e799b4532d50d0d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>orient_t</type>
      <name>orientation</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a394eafe3058457db432eeecb49addf07</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>unit</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a55bd8ba78e4f839d2897100cabd1c62f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>auto_size_roll_feed_paper</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a9f02a46dd284eeaf56a643e0e4b17ffb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>invalid_mask</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>a49e854e792c73fee7bebcfb4de9ac78a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>stp_vars_t *</type>
      <name>v</name>
      <anchorfile>structstpui__plist__t.html</anchorfile>
      <anchor>afbdf87548a3132b7270aa88ffcf52a8a</anchor>
      <arglist></arglist>
    </member>
  </compound>
</tagfile>
