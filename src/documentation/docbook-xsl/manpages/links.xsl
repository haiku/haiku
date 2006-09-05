<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                version='1.0'>

<!-- ********************************************************************
     $Id: links.xsl,v 1.6 2006/04/25 16:07:48 xmldoc Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<!-- * Make per-Refentry sets of all links in each Refentry which do not -->
<!-- * have the same URL as any preceding link in that same Refentry -->
<!-- * -->
<!-- * We need this in order to have inline numbering match the -->
<!-- * numbering of the link list. In both places, for any link whose -->
<!-- * URL is a duplicate of that of a preceding link in the same -->
<!-- * Refentry, we assign it the number of that previous link. -->
<!-- * -->
<!-- * Note that we don't get links in *info sections or Refmeta or -->
<!-- * Refnamediv or Indexterm, because, in manpages output, contents -->
<!-- * of those are either suppressed or are displayed out of document -->
<!-- * order - for example, the Info/Author content gets moved to the -->
<!-- * end of the page. So, if we were to number links in the Author -->
<!-- * content, it would "throw off" the numbering at the beginning of -->
<!-- * the main text flow. -->
<!-- * -->
<!-- * And note especially that the reason we don't use xsl:key here -->
<!-- * is that the "match" value is an XSLT _pattern_, not an XSLT -->
<!-- * _expression_; XSLT patterns can't contain function calls, but -->
<!-- * XSLT expressions can. We need the calls to the generate-id() -->
<!-- * function in order to determine whether or not two links have -->
<!-- * the same Refentry parent -->
<xsl:variable name="get.all.links.with.unique.urls">
  <xsl:if test="$man.links.are.numbered != 0">
    <xsl:for-each select="//refentry">
      <refentry.link.set>
        <xsl:attribute name="idref">
          <xsl:value-of select="generate-id()"/>
        </xsl:attribute>
        <xsl:for-each
            select=".//ulink[node()
                    and not(ancestor::refentryinfo)
                    and not(ancestor::info)
                    and not(ancestor::docinfo)
                    and not(ancestor::refmeta)
                    and not(ancestor::refnamediv)
                    and not(ancestor::indexterm)
                    and not(@url =
                    preceding::ulink[node()
                    and not(ancestor::refentryinfo)
                    and not(ancestor::info)
                    and not(ancestor::docinfo)
                    and not(ancestor::refmeta)
                    and not(ancestor::refnamediv)
                    and not(ancestor::indexterm)
                    and (generate-id(ancestor::refentry)
                    = generate-id(current()))]/@url)]">
          <link>
            <xsl:attribute name="url">
              <xsl:value-of select="@url"/>
            </xsl:attribute>
            <xsl:copy>
              <xsl:copy-of select="node()"/>
            </xsl:copy>
          </link>
        </xsl:for-each>
      </refentry.link.set>
    </xsl:for-each>
  </xsl:if>
</xsl:variable>

<xsl:variable name="all.links.with.unique.urls"
              select="exsl:node-set($get.all.links.with.unique.urls)"/>

<!-- ==================================================================== -->

<!-- * We first find out if the link is empty; if it is, we just -->
<!-- * display the contents of its URL (that is, the value of its "Url" -->
<!-- * attribute), and stop there. -->
<!-- * -->
<!-- * On the other hand, if it is NON-empty, we need to display its -->
<!-- * contents AND (optionally) display its URL. We could display the -->
<!-- * URL inline, after the contents (as we did previously), but that -->
<!-- * ends up looking horrible if you have a lot of links. -->
<!-- * -->
<!-- * So, we instead need to display the URL out-of-line, in a way -->
<!-- * that associates it with the content. How to do that in a -->
<!-- * text-based output format that lacks hyperlinks? -->
<!-- * -->
<!-- * Here's how: Do it the way that most text/curses-based browsers -->
<!-- * (e.g., w3m and lynx) do in the "-dump" output: Put a number -->
<!-- * (in brackets) before the contents, and then put the URL, with -->
<!-- * the corresponding number, in a generated section -->
<!-- * at the end of the page. -->
<!-- * -->
<!-- * For any link whose URL is a duplicate of that of a preceding -->
<!-- * link in the same Refentry, we assign it the number of that -->
<!-- * previous link. -->
<xsl:template match="ulink">
  <xsl:variable name="get.links.with.unique.urls">
    <!-- * get the set of all unique links in the ancestor Refentry of -->
    <!-- * this Ulink -->
    <xsl:copy-of
        select="$all.links.with.unique.urls/refentry.link.set
                [@idref = generate-id(current()/ancestor::refentry)]/link"/>
  </xsl:variable>
  <xsl:variable name="links.with.unique.urls"
                select="exsl:node-set($get.links.with.unique.urls)"/>
  <!-- * store link URL in variable because we check it multiple times -->
  <xsl:variable name="url">
    <xsl:value-of select="@url"/>
  </xsl:variable>
  <!-- * $link is either the link contents (if the link is non-empty) or -->
  <!-- * the URL (if the link is empty -->
  <xsl:variable name="link">
    <xsl:choose>
      <!-- * check to see if the element is empty or not; if it's non-empty, -->
      <!-- * get the content -->
      <xsl:when test="node()">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
        <!-- * The element is empty, so we just get the value of the URL; -->
        <!-- * note that we don't number empty links -->
        <!-- * -->
        <!-- * Add hyphenation suppression in URL output only if -->
        <!-- * break.after.slash is also non-zero -->
        <xsl:if test="$man.hyphenate.urls = 0 and
                      $man.break.after.slash = 0">
          <xsl:call-template name="suppress.hyphenation"/>
          <xsl:text>\%</xsl:text>
        </xsl:if>
        <xsl:value-of select="$url"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <!-- * if link is non-empty AND user wants links numbered, output -->
  <!-- * a number for it -->
  <xsl:if test="node() and $man.links.are.numbered != 0">
    <!-- * We number links by checking the $links.with.unique.urls set -->
    <!-- * and finding the link whose URL matches the URL for this -->
    <!-- * link. If this is the only instance in this Refentry of a link -->
    <!-- * with this URL, then it gets a unique number. But if this is a -->
    <!-- * link for which there are multiple instances of links in this -->
    <!-- * Refentry that have the same URL as this link, then the number -->
    <!-- * assigned is the number of the _first_ instance of a link in -->
    <!-- * this Refentry with the URL for this link (which be the number -->
    <!-- * of this link itself, if it happens to be the first instance). -->
    <xsl:variable name="link.number">
      <xsl:apply-templates
          select="$links.with.unique.urls/link[@url = $url][1]"
          mode="link.number"/>
    </xsl:variable>
    <!-- * format the number by placing it in square brackets. This -->
    <!-- * formatting could be made user-configurable - something -->
    <!-- * other than square brackets. But what else would work? -->
    <!-- * <10> Angle brackets? {10} Braces? -->
    <xsl:text>[</xsl:text>
    <xsl:value-of select="$link.number"/>
    <xsl:text>]\&amp;</xsl:text>
    <!-- * Note that the reason for the \& after the closing bracket -->
    <!-- * is to prevent a hyphen character from being rendered -->
    <!-- * between the closing bracket and the following text - even -->
    <!-- * when the following text is preceded by a "hyphenation -->
    <!-- * character"; for example: -->
    <!-- * -->
    <!-- *  [26]\&\fI\%COUNTRY\fR\fR -->
    <!-- * -->
    <!-- * Where COUNTRY is marked up with as <envar>COUNTRY</envar> -->
    <!-- * in the source (we generate \% before all Envar output (and -->
    <!-- * all other "computer inlines") to prevent it from being -->
    <!-- * broken across lines. -->
    <!-- * -->
    <!-- * Without the \& after the closing bracket, if the -->
    <!-- * [26]\fI\%COUNTRY\fR\fR instance fell at the end of a line, -->
    <!-- * a hyphen could end up being inserted; for example: -->
    <!-- * -->
    <!-- *   if you are uncertain, check the value of the [26]- -->
    <!-- *   COUNTRY environment variable -->
    <!-- * -->
    <!-- * The \& causes [26]COUNTRY to be treated as a unit, -->
    <!-- * preventing insertion of the stray hyphen. -->
  </xsl:if>
  <xsl:choose>
    <!-- * if user wants links underlined, underline (ital) it -->
    <xsl:when test="$man.links.are.underlined != 0">
      <xsl:variable name="link.wrapper">
        <italic><xsl:value-of select="$link"/></italic>
      </xsl:variable>
      <xsl:apply-templates mode="italic" select="exsl:node-set($link.wrapper)"/>
    </xsl:when>
    <xsl:otherwise>
      <!-- * user doesn't want links underlined, so just display content -->
      <xsl:value-of select="$link"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="link.number">
  <!-- * Count all links in this Refentry which do not have the same -->
  <!-- * URL as any preceding link in this same Refentry -->
  <!-- * -->
  <!-- * Note that we don't get links in *info sections or Refmeta or -->
  <!-- * Refnamediv or Indexterm, because, in manpages output, -->
  <!-- * contents of those are either suppressed or are displayed out -->
  <!-- * of document order -->
  <!-- * -->
  <!-- * And note that the reason we don't just use xsl:number here -->
  <!-- * is that the "match" value is an XSLT _pattern_, not an XSLT -->
  <!-- * _expression_; XSLT patterns can't contain function calls, but -->
  <!-- * XSLT expressions can. We need the calls to the generate-id() -->
  <!-- * function in order to determine whether or not two links have -->
  <!-- * the same Refentry parent -->
  <xsl:value-of select="count(preceding::ulink[node()
                        and not(ancestor::refentryinfo)
                        and not(ancestor::info)
                        and not(ancestor::docinfo)
                        and not(ancestor::refmeta)
                        and not(ancestor::refnamediv)
                        and not(ancestor::indexterm)
                        and not(@url =
                        preceding::ulink[node()
                        and not(ancestor::refentryinfo)
                        and not(ancestor::info)
                        and not(ancestor::docinfo)
                        and not(ancestor::refmeta)
                        and not(ancestor::refnamediv)
                        and not(ancestor::indexterm)
                        and (generate-id(ancestor::refentry)
                        = generate-id(current()/ancestor::refentry))]/@url)]
                        [generate-id(ancestor::refentry)
                        = generate-id(current()/ancestor::refentry)]) + 1"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="links.list">
  <!-- * Get all links in this Refentry which do not have the same URL -->
  <!-- * as any preceding link in this same Refentry -->
  <!-- * -->
  <!-- * Note that we don't get links in *info sections or Refmeta or -->
  <!-- * Refnamediv or Indexterm, because, in manpages output, -->
  <!-- * contents of those are either suppressed or are displayed out -->
  <!-- * of document order -->
  <xsl:variable name="links"
                select=".//ulink[node()
                  and not(ancestor::refentryinfo)
                  and not(ancestor::info)
                  and not(ancestor::docinfo)
                  and not(ancestor::refmeta)
                  and not(ancestor::refnamediv)
                  and not(ancestor::indexterm)
                  and not(@url =
                  preceding::ulink[node()
                  and not(ancestor::refentryinfo)
                  and not(ancestor::info)
                  and not(ancestor::docinfo)
                  and not(ancestor::refmeta)
                  and not(ancestor::refnamediv)
                  and not(ancestor::indexterm)
                  and (generate-id(ancestor::refentry)
                  = generate-id(current()))]/@url)]"/>
  <!-- * check to see if we have actually found any links; if we have, -->
  <!-- * we generate the links list, if not, we do nothing -->
  <xsl:if test="$links/node()">
    <xsl:call-template name="format.links.list">
      <xsl:with-param name="links" select="$links"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="format.links.list">
  <xsl:param name="links"/>
  <!-- * The value of $padding.length is used for determining how much -->
  <!-- * to right-pad numbers in the LINKS list. So, for $length, we -->
  <!-- * count how many links there are, then take the number of digits -->
  <!-- * in that count, and add 2 to it. The reason we add 2 is that we -->
  <!-- * also prepend a dot and no-break space to each link number in -->
  <!-- * the list, so we need to consider what length to pad out to. -->
  <xsl:param name="padding.length">
    <xsl:choose>
      <xsl:when test="$man.links.are.numbered != 0">
        <xsl:value-of select="string-length(count($links)) + 2"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- * if links aren't numbered, just set padding to zero -->
        <xsl:value-of select="0"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:call-template name="mark.subheading"/>
  <!-- * make the link-list section heading (REFERENCES) -->
  <xsl:text>.SH "</xsl:text>
  <xsl:call-template name="string.upper">
    <xsl:with-param name="string">
      <xsl:choose>
        <!-- * if user has specified a heading, use that -->
        <xsl:when test="$man.links.list.heading != ''">
          <xsl:value-of select="$man.links.list.heading"/>
        </xsl:when>
        <xsl:otherwise>
          <!-- * otherwise, get localized heading from gentext -->
          <xsl:call-template name="gentext">
            <xsl:with-param name="key" select="'References'"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:with-param>
  </xsl:call-template>
  <xsl:text>"&#10;</xsl:text>
  <xsl:for-each select="$links">
    <!-- * number the link, in the form " 1." followed by a -->
    <!-- * non-breaking space; -->
    <xsl:variable name="link.number">
      <xsl:apply-templates select="." mode="link.number"/>
      <xsl:text>.&#160;</xsl:text>
    </xsl:variable>
    <!-- * make paragraph with hanging indent; length of indent is -->
    <!-- * same as value of $padding.length -->
    <xsl:text>.TP </xsl:text>
    <xsl:value-of select="$padding.length"/>
    <xsl:text>&#10;</xsl:text>
    <!-- * right-pad each number out to the correct length -->
    <!-- * So, if there are 10 or more links, numbers -->
    <!-- * 1 through 9 will have a space in front of them; if -->
    <!-- * there are 100 or more, 1 through 9 will have two -->
    <!-- * spaces, 10 through 99 will have one space, etc. -->
    <xsl:call-template name="prepend-pad">
      <xsl:with-param name="padVar" select="$link.number"/>
      <xsl:with-param name="length" select="$padding.length"/>
    </xsl:call-template>
    <!-- * Print the links contents -->
    <!-- * IMPORTANT: If there are multiple links in this Refentry -->
    <!-- * with the same URL, this gets the contents of the _first -->
    <!-- * instance_ of the link in this Refentry with that URL -->
    <xsl:variable name="link.contents">
      <xsl:value-of select="."/>
    </xsl:variable>
    <xsl:value-of select="normalize-space($link.contents)"/>
    <xsl:text>&#10;</xsl:text>
    <!-- * Print the link's URL -->
    <!-- * Add hyphenation suppression in URL output only if -->
    <!-- * break.after.slash is also non-zero -->
    <xsl:if test="$man.hyphenate.urls = 0 and
                  $man.break.after.slash = 0">
      <xsl:call-template name="suppress.hyphenation"/>
      <xsl:text>\%</xsl:text>
    </xsl:if>
    <xsl:value-of select="@url"/>
    <xsl:text>&#10;</xsl:text>
  </xsl:for-each>
</xsl:template>

  <!-- * ============================================================== -->
  <!-- *    Handle table footnotes                                      -->
  <!-- * ============================================================== -->
  <xsl:template match="footnote" mode="table.footnote.mode">
    <xsl:variable name="footnotes" select=".//footnote"/>
    <xsl:variable name="table.footnotes"
                  select=".//tgroup//footnote"/>
    <xsl:value-of select="$man.table.footnotes.divider"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>.br&#10;</xsl:text>
    <xsl:apply-templates select="*[1]" mode="footnote.body.number"/>
    <xsl:apply-templates select="*[position() &gt; 1]"/>
  </xsl:template>

  <!-- * The following template for footnote.body.number mode was just -->
  <!-- * lifted from the HTML stylesheets with some minor adjustments -->
  <xsl:template match="*"  mode="footnote.body.number">
    <xsl:variable name="name">
      <xsl:text>ftn.</xsl:text>
      <xsl:call-template name="object.id">
        <xsl:with-param name="object" select="ancestor::footnote"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="href">
      <xsl:text>#</xsl:text>
      <xsl:call-template name="object.id">
        <xsl:with-param name="object" select="ancestor::footnote"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="footnote.mark">
      <xsl:text>[</xsl:text>
      <xsl:apply-templates select="ancestor::footnote"
                           mode="footnote.number"/>
      <xsl:text>]&#10;</xsl:text>
    </xsl:variable>
    <xsl:variable name="html">
      <xsl:apply-templates select="."/>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="function-available('exsl:node-set')">
        <xsl:variable name="html-nodes" select="exsl:node-set($html)"/>
        <xsl:choose>
          <xsl:when test="$html-nodes//p">
            <xsl:apply-templates select="$html-nodes" mode="insert.html.p">
              <xsl:with-param name="mark" select="$footnote.mark"/>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="$html-nodes" mode="insert.html.text">
              <xsl:with-param name="mark" select="$footnote.mark"/>
            </xsl:apply-templates>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$html"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- * The HTML stylesheets output <sup><a>...</a></sup> around -->
  <!-- * footnote markers in tables -->
  <xsl:template match="th/sup">
    <xsl:apply-templates/>
  </xsl:template>
  <xsl:template match="a">
    <xsl:apply-templates/>
  </xsl:template>

</xsl:stylesheet>
