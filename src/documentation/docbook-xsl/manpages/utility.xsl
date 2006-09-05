<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:saxon="http://icl.com/saxon"
                exclude-result-prefixes="exsl dyn saxon"
                version='1.0'>

<!-- ********************************************************************
     $Id: utility.xsl,v 1.15 2006/04/24 02:29:31 xmldoc Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<!-- * This file contains "utility" templates that are called multiple -->
<!-- * times per each Refentry. -->

<!-- ==================================================================== -->

  <!-- * NOTE TO DEVELOPERS: For ease of maintenance, the current -->
  <!-- * manpages stylesheets use the mode="bold" and mode="italic" -->
  <!-- * templates for *anything and everything* that needs to get -->
  <!-- * boldfaced or italicized.   -->
  <!-- * -->
  <!-- * So if you add anything that needs bold or italic character -->
  <!-- * formatting, try to apply these templates to it rather than -->
  <!-- * writing separate code to format it. This can be a little odd if -->
  <!-- * the content you want to format is not element content; in those -->
  <!-- * cases, you need to turn it into element content before applying -->
  <!-- * the template; see examples of this in the existing code. -->

  <xsl:template mode="bold" match="*">
    <xsl:for-each select="node()">
      <xsl:text>\fB</xsl:text>
      <xsl:apply-templates select="."/>
      <xsl:text>\fR</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <xsl:template mode="italic" match="*">
    <xsl:for-each select="node()">
      <xsl:text>\fI</xsl:text>
      <xsl:apply-templates select="."/>
      <xsl:text>\fR</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- ================================================================== -->

  <!-- * NOTE TO DEVELOPERS: For ease of maintenance, the current -->
  <!-- * manpages stylesheets use the mode="prevent.line.breaking" -->
  <!-- * templates for *anything and everything* that needs to have -->
  <!-- * embedded spaces turned into no-break spaces in output - in -->
  <!-- * order to prevent that output from getting broken across lines -->
  <!-- * -->
  <!-- * So if you add anything that whose output, try to apply this -->
  <!-- * template to it rather than writing separate code to format -->
  <!-- * it. This can be a little odd if the content you want to -->
  <!-- * format is not element content; in those cases, you need to -->
  <!-- * turn it into element content before applying the template; -->
  <!-- * see examples of this in the existing code. -->
  <!-- * -->
  <!-- * This template is currently called by the funcdef and paramdef -->
  <!-- * and group/arg templates. -->
  <xsl:template mode="prevent.line.breaking" match="*">
    <xsl:variable name="rcontent">
      <xsl:apply-templates/>
    </xsl:variable>
    <xsl:variable name="content">
      <xsl:value-of select="normalize-space($rcontent)"/>
    </xsl:variable>
    <xsl:call-template name="string.subst">
      <xsl:with-param name="string" select="$content"/>
      <xsl:with-param name="target" select="' '"/>
      <!-- * We output a real nobreak space here (rather than, "\ ", -->
      <!-- * the roff nobreak space) because, when we do character-map -->
      <!-- * processing before final output, the character-map will -->
      <!-- * handle conversion of the &#160; to "\ " for us -->
      <xsl:with-param name="replacement" select="'&#160;'"/>
    </xsl:call-template>
  </xsl:template>

  <!-- ================================================================== -->

  <xsl:template name="suppress.hyphenation">
    <!-- * we need to suppress hyphenation inline only if hyphenation is -->
    <!-- * actually on, and even then only outside of Cmdsynopsis and -->
    <!-- * Funcsynopsis, where it is already always turned off -->
    <xsl:if test="$man.hyphenate != 0 and
                  not(ancestor::cmdsynopsis) and
                  not(ancestor::funcsynopsis)">
      <xsl:text>\%</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- ================================================================== -->

  <!-- * The nested-section-title template is called for refsect3, and any -->
  <!-- * refsection nested more than 2 levels deep. -->
  <xsl:template name="nested-section-title">
    <!-- * The next few lines are some arcane roff code to control line -->
    <!-- * spacing after headings. -->
    <xsl:text>.sp&#10;</xsl:text>
    <xsl:text>.it 1 an-trap&#10;</xsl:text>
    <xsl:text>.nr an-no-space-flag 1&#10;</xsl:text>
    <xsl:text>.nr an-break-flag 1&#10;</xsl:text>
    <xsl:text>.br&#10;</xsl:text>
    <!-- * make title wrapper so that we can use mode="bold" template to -->
    <!-- * apply character formatting to it -->
    <xsl:variable name="title.wrapper">
      <bold><xsl:choose>
        <xsl:when test="title">
          <xsl:value-of select="normalize-space(title[1])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="." mode="object.title.markup.textonly"/>
        </xsl:otherwise>
      </xsl:choose></bold>
    </xsl:variable>
    <xsl:call-template name="mark.subheading"/>
    <xsl:apply-templates mode="bold" select="exsl:node-set($title.wrapper)"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="mark.subheading"/>
  </xsl:template>

  <!-- ================================================================== -->

  <!-- * The mixed-block template jumps through a few hoops to deal with -->
  <!-- * mixed-content blocks, so that we don't end up munging verbatim -->
  <!-- * environments or lists and so that we don't gobble up whitespace -->
  <!-- * when we shouldn't -->
  <xsl:template name="mixed-block">
    <xsl:for-each select="node()">
      <xsl:choose>
        <!-- * Check to see if this node is a verbatim environment. -->
        <!-- * If so, put a line of space before it. -->
        <!-- * -->
        <!-- * Yes, address and synopsis are vertabim environments. -->
        <!-- * -->
        <!-- * The code here previously also treated informaltable as a -->
        <!-- * verbatim, presumably to support some kludge; I removed it -->
        <xsl:when test="self::address|self::literallayout|self::programlisting|
                        self::screen|self::synopsis">
          <xsl:text>&#10;</xsl:text>
          <xsl:text>.sp&#10;</xsl:text>
          <xsl:apply-templates select="."/>
        </xsl:when>
        <!-- * Check to see if this node is a list; if it is, we don't -->
        <!-- * want to normalize-space(), so we just apply-templates. -->
        <!-- * Do same for all admonitions -->
        <xsl:when test="(self::itemizedlist|self::orderedlist|
                        self::variablelist|self::glosslist|
                        self::simplelist[@type !='inline']|
                        self::segmentedlist|
                        self::caution|self::important|
                        self::note|self::tip|self::warning)">
          <xsl:apply-templates select="."/>
        </xsl:when>
        <xsl:when test="self::text()">
          <!-- * Check to see if this is a text node. -->
          <!-- * -->
          <!-- * If so, replace all whitespace at the beginning or end of it -->
          <!-- * with a single linebreak. -->
          <!-- * -->
          <xsl:variable name="content">
            <xsl:apply-templates select="."/>
          </xsl:variable>
          <xsl:if
              test="starts-with(translate(.,'&#9;&#10;&#13; ','    '), ' ')
                    and preceding-sibling::node()[1][name(.)!='']
                    and normalize-space($content) != ''
                    and not(
                    preceding-sibling::*[1][
                    self::caution or
                    self::important or
                    self::note or
                    self::tip or
                    self::warning or
                    self::variablelist or
                    self::glosslistlist or
                    self::itemizedlist or
                    self::orderededlist or
                    self::segmentedlist or
                    self::procedure or
                    self::address or
                    self::literallayout or
                    self::programlisting or
                    self::screen
                    ]
                    )
                    ">
            <xsl:text>&#10;</xsl:text>
          </xsl:if>
          <xsl:value-of select="normalize-space($content)"/>
          <xsl:if
              test="(translate(substring(., string-length(.), 1),'&#9;&#10;&#13; ','    ')  = ' '
                    and following-sibling::node()[1][name(.)!=''])
                    or following-sibling::node()[1][self::comment()]
                    or following-sibling::node()[1][self::processing-instruction()]
                    ">
            <xsl:if test="normalize-space($content) != ''
                          or concat(normalize-space($content), ' ') = ' '">
              <xsl:text>&#10;</xsl:text>
            </xsl:if>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <!-- * At this point, we know that this node is not a verbatim -->
          <!-- * environment, list, admonition, or text node; so we can -->
          <!-- * safely normalize-space() it. -->
          <xsl:variable name="content">
            <xsl:apply-templates select="."/>
          </xsl:variable>
          <xsl:value-of select="normalize-space($content)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!-- ================================================================== -->

  <xsl:template name="make.adjusted.man.filename">
    <xsl:param name="name"/>
    <xsl:param name="section"/>
    <xsl:param name="dirname">
      <xsl:if test="not($man.output.in.separate.dir = 0)">
        <xsl:choose>
          <xsl:when test="not($man.output.subdirs.enabled = 0)">
            <xsl:value-of
                select="concat($man.output.base.dir, 'man', normalize-space($section), '/')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$man.output.base.dir"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:param>
    <xsl:call-template name="string.subst">
      <!-- * Replace any spaces in filename with underscores & then append -->
      <!-- * a dot plus a section number to create the man filename -->
      <xsl:with-param name="string"
                      select="concat($dirname,
                              normalize-space($name),
                              '.', normalize-space($section))"/>
      <xsl:with-param name="target" select="' '"/>
      <xsl:with-param name="replacement" select="'_'"/>
    </xsl:call-template>
  </xsl:template>
  
  <!-- ================================================================== -->

  <!-- * Put a horizontal rule or other divider around section titles -->
  <!-- * in roff source (just to make things easier to read). -->
  <xsl:template name="mark.subheading">
    <xsl:if test="$man.subheading.divider.enabled != 0">
      <xsl:text>.\" </xsl:text>
      <xsl:value-of select="$man.subheading.divider"/>
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
