<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                exclude-result-prefixes="exsl"
                version='1.0'>

<!-- ********************************************************************
     $Id: synop.xsl,v 1.29 2006/04/25 08:02:20 xmldoc Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<xsl:variable name="arg.or.sep"> |</xsl:variable>

<!-- * Note: If you're looking for the *Synopsis* element, you won't -->
<!-- * find any code here for handling it. It's a "verbatim" -->
<!-- * environment; see the block.xsl file instead. -->

<xsl:template match="synopfragmentref">
  <xsl:variable name="target" select="key('id',@linkend)"/>
  <xsl:variable name="snum">
    <xsl:apply-templates select="$target" mode="synopfragment.number"/>
  </xsl:variable>
  <xsl:text>(</xsl:text>
  <xsl:value-of select="$snum"/>
  <xsl:text>)</xsl:text>
  <xsl:text>&#160;</xsl:text>
  <xsl:variable name="synopfragmentref">
    <FragRefContents><xsl:value-of select="normalize-space(.)"/></FragRefContents>
  </xsl:variable>
  <xsl:apply-templates select="exsl:node-set($synopfragmentref)" mode="italic"/>
</xsl:template>

<xsl:template match="synopfragment" mode="synopfragment.number">
  <xsl:number format="1"/>
</xsl:template>

<xsl:template match="synopfragment">
  <xsl:variable name="snum">
    <xsl:apply-templates select="." mode="synopfragment.number"/>
  </xsl:variable>
  <xsl:text>&#10;</xsl:text>
  <!-- * If we have a group of Synopgfragments, we only want to output a -->
  <!-- * line of space before the first; so when we find a Synopfragment -->
  <!-- * whose first preceding sibling is another Synopfragment, we back -->
  <!-- * up one line vertically to negate the line of vertical space -->
  <!-- * that's added by the .HP macro -->
  <xsl:if test="preceding-sibling::*[1][self::synopfragment]">
    <xsl:text>.sp -1n&#10;</xsl:text>
  </xsl:if>
  <xsl:text>.HP </xsl:text>
  <!-- * For each Synopfragment, make a hanging paragraph, with the -->
  <!-- * indent calculated from the length of the generated number -->
  <!-- * used as a reference + pluse 3 characters (for the open and -->
  <!-- * close parens around the number, plus a space). -->
  <xsl:value-of select="string-length (normalize-space ($snum)) + 3"/>
  <xsl:text>&#10;</xsl:text>
  <xsl:text>(</xsl:text>
  <xsl:value-of select="$snum"/>
  <xsl:text>)</xsl:text>
  <xsl:text> </xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="group|arg" name="group-or-arg">
  <xsl:variable name="choice" select="@choice"/>
  <xsl:variable name="rep" select="@rep"/>
  <xsl:variable name="sepchar">
    <xsl:choose>
      <xsl:when test="ancestor-or-self::*/@sepchar">
        <xsl:value-of select="ancestor-or-self::*/@sepchar"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:if test="position()>1 and
                not(preceding-sibling::*[1][self::sbr])"
          ><xsl:value-of select="$sepchar"/></xsl:if>
  <xsl:choose>
    <xsl:when test="$choice='plain'">
      <!-- * do nothing -->
    </xsl:when>
    <xsl:when test="$choice='req'">
      <xsl:value-of select="$arg.choice.req.open.str"/>
    </xsl:when>
    <xsl:when test="$choice='opt'">
      <xsl:value-of select="$arg.choice.opt.open.str"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$arg.choice.def.open.str"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:variable name="arg">
    <xsl:apply-templates/>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="local-name(.) = 'arg' and not(ancestor::arg)">
      <!-- * Prevent arg contents from getting wrapped and broken up -->
      <xsl:variable name="arg.wrapper">
        <Arg><xsl:value-of select="normalize-space($arg)"/></Arg>
      </xsl:variable>
      <xsl:apply-templates mode="prevent.line.breaking"
                           select="exsl:node-set($arg.wrapper)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$arg"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:choose>
    <xsl:when test="$rep='repeat'">
      <xsl:value-of select="$arg.rep.repeat.str"/>
    </xsl:when>
    <xsl:when test="$rep='norepeat'">
      <xsl:value-of select="$arg.rep.norepeat.str"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$arg.rep.def.str"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:choose>
    <xsl:when test="$choice='plain'">
      <xsl:if test='arg'>
      <xsl:value-of select="$arg.choice.plain.close.str"/>
      </xsl:if>
    </xsl:when>
    <xsl:when test="$choice='req'">
      <xsl:value-of select="$arg.choice.req.close.str"/>
    </xsl:when>
    <xsl:when test="$choice='opt'">
      <xsl:value-of select="$arg.choice.opt.close.str"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$arg.choice.def.close.str"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="group/arg">
  <xsl:variable name="choice" select="@choice"/>
  <xsl:variable name="rep" select="@rep"/>
  <xsl:if test="position()>1"><xsl:value-of select="$arg.or.sep"/></xsl:if>
  <xsl:call-template name="group-or-arg"/>
</xsl:template>

<xsl:template match="sbr">
  <xsl:text>&#10;</xsl:text>
  <xsl:text>.br&#10;</xsl:text>
</xsl:template>

<xsl:template match="cmdsynopsis">
  <!-- * if justification is enabled by default, turn it off temporarily -->
  <xsl:if test="$man.justify != 0">
    <xsl:text>.ad l&#10;</xsl:text>
  </xsl:if>
  <!-- * if hyphenation is enabled by default, turn it off temporarily -->
  <xsl:if test="$man.hyphenate != 0">
    <xsl:text>.hy 0&#10;</xsl:text>
  </xsl:if>
  <xsl:text>.HP </xsl:text>
  <xsl:value-of select="string-length (normalize-space (command)) + 1"/>
  <xsl:text>&#10;</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>&#10;</xsl:text>
  <!-- * if justification is enabled by default, turn it back on -->
  <xsl:if test="$man.justify != 0">
    <xsl:text>.ad&#10;</xsl:text>
  </xsl:if>
  <!-- * if hyphenation is enabled by default, turn it back on -->
  <xsl:if test="$man.hyphenate != 0">
    <xsl:text>.hy&#10;</xsl:text>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->
<!-- *  Funcsynopis hierarchy starts here -->
<!-- ==================================================================== -->

<!-- * Note: If you're looking for the *Funcsynopsisinfo* element, -->
<!-- * you won't find any code here for handling it. It's a "verbatim" -->
<!-- * environment; see the block.xsl file instead. -->

<!-- * Within funcsynopis output, disable hyphenation, and use -->
<!-- * left-aligned filling for the duration of the synopsis, so that -->
<!-- * line breaks only occur between separate paramdefs. -->
<xsl:template match="funcsynopsis">
  <!-- * if justification is enabled by default, turn it off temporarily -->
  <xsl:if test="$man.justify != 0">
    <xsl:text>.ad l&#10;</xsl:text>
  </xsl:if>
  <!-- * if hyphenation is enabled by default, turn it off temporarily -->
  <xsl:if test="$man.hyphenate != 0">
    <xsl:text>.hy 0&#10;</xsl:text>
  </xsl:if>
  <xsl:apply-templates/>
  <!-- * if justification is enabled by default, turn it back on -->
  <xsl:if test="$man.justify != 0">
    <xsl:text>.ad&#10;</xsl:text>
  </xsl:if>
  <!-- * if hyphenation is enabled by default, turn it back on -->
  <xsl:if test="$man.hyphenate != 0">
    <xsl:text>.hy&#10;</xsl:text>
  </xsl:if>
</xsl:template>

<!-- * All Funcprototype content is by default rendered in bold, -->
<!-- * because the man(7) man page says this: -->
<!-- * -->
<!-- *   For functions, the arguments are always specified using -->
<!-- *   italics, even in the SYNOPSIS section, where the rest of -->
<!-- *   the function is specified in bold -->
<!-- * -->
<!-- * Look through the contents of the man/man2 and man3 directories -->
<!-- * on your system, and you'll see that most existing pages do follow -->
<!-- * this "bold everything in function synopsis" rule. -->
<!-- * -->
<!-- * Users who don't want the bold output can choose to adjust the -->
<!-- * man.font.funcprototype parameter on their own. So even if you -->
<!-- * don't personally like the way it looks, please don't change the -->
<!-- * default to be non-bold - because it's a convention that's -->
<!-- * followed is the vast majority of existing man pages that document -->
<!-- * functions, and we need to follow it by default, like it or no. -->
<xsl:template match="funcprototype">
  <xsl:variable name="funcprototype.string.value">
    <xsl:value-of select="funcdef"/>
  </xsl:variable>
  <xsl:variable name="funcprototype">
    <xsl:apply-templates select="funcdef"/>
  </xsl:variable>
  <xsl:text>.HP </xsl:text>
  <!-- * Hang Paragraph by length of string value of <funcdef> + 1 -->
  <!-- * (because funcdef is always followed by one open paren char) -->
  <xsl:value-of select="string-length (normalize-space ($funcprototype.string.value)) + 1"/>
  <xsl:text>&#10;</xsl:text>
  <xsl:text>.</xsl:text>
  <xsl:value-of select="$man.font.funcprototype"/>
  <xsl:text> </xsl:text>
  <!-- * The following quotation mark (and the one further below) are -->
  <!-- * needed to properly delimit the parts of the Funcprototype that -->
  <!-- * should be rendered in the prevailing font (either Bold or Roman) -->
  <!-- * from Parameter output that needs to be alternately rendered in -->
  <!-- * italic. -->
  <xsl:text>"</xsl:text>
  <xsl:value-of select="normalize-space($funcprototype)"/>
  <xsl:text>(</xsl:text>
  <xsl:apply-templates select="*[local-name() != 'funcdef']"/>
  <xsl:text>"</xsl:text>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template match="funcdef">
  <xsl:apply-templates mode="prevent.line.breaking"/>
</xsl:template>

<xsl:template match="funcdef/function">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="void">
  <xsl:text>void);</xsl:text>
</xsl:template>

<xsl:template match="varargs">
  <xsl:text>...);</xsl:text>
</xsl:template>

<xsl:template match="paramdef">
  <xsl:apply-templates mode="prevent.line.breaking" select="."/>
  <xsl:choose>
    <xsl:when test="following-sibling::*">
      <xsl:text>, </xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>);</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="paramdef/parameter">
  <!-- * x2008 is a "punctuation space"; we use it here because if we -->
  <!-- * were to just use a normal space, it would get replaced with a -->
  <!-- * non-breaking space when we run the whole Paramdef through the -->
  <!-- * prevent.line.breaking template. And as far as why we're -->
  <!-- * inserting the space an quotation marks around each Parameter to -->
  <!-- * begin with, the reason is that we need to because we are -->
  <!-- * outputting Funcsynopsis in either the "BI" or "RI" font, and -->
  <!-- * the space and quotation marks delimit the text as the -->
  <!-- * "alternate" or "I" text that needs to be rendered in italic. -->
  <xsl:text>"&#x2008;"</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>"&#x2008;"</xsl:text>
</xsl:template>

<xsl:template match="funcparams">
  <text>(</text>
  <xsl:apply-templates/>
  <text>)</text>
</xsl:template>

</xsl:stylesheet>
