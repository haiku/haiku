<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<!-- ********************************************************************
     $Id: refentry.xsl,v 1.14 2006/04/26 04:38:28 xmldoc Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

  <xsl:template match="refnamediv">
    <xsl:choose>
      <xsl:when test="preceding-sibling::refnamediv">
        <!-- * No title on secondary refnamedivs! -->
        <!-- * Just put a single line break instead -->
        <xsl:text>.br&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="mark.subheading"/>
        <xsl:text>.SH "</xsl:text>
        <xsl:apply-templates select="." mode="title.markup"/>
        <xsl:text>"</xsl:text>
        <xsl:text>&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="mark.subheading"/>
    <!-- * if we have multiple Refname instances, separate the names -->
    <!-- * with commas -->
    <xsl:for-each select="refname">
      <xsl:if test="position()>1">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:value-of select="."/>
    </xsl:for-each>
    <!-- * The man(7) man pages says: -->
    <!-- * -->
    <!-- *   The only required heading is NAME, which should be the -->
    <!-- *   first section and be followed on the next line by a one -->
    <!-- *   line description of the program: -->
    <!-- * -->
    <!-- *      .SH NAME chess \- the game of chess -->
    <!-- * -->
    <!-- *   It is extremely important that this format is followed, -->
    <!-- *   and that there is a backslash before the single dash -->
    <!-- *   which follows the command name.  This syntax is used by -->
    <!-- *   the makewhatis(8) program to create a database of short -->
    <!-- *   command descriptions for the whatis(1) and apropos(1) -->
    <!-- *   commands. -->
    <!-- * -->
    <!-- * So why don't we precede the hyphen with a backslash here? -->
    <!-- * Well, because it's added later, by the apply-string-subst-map -->
    <!-- * template, before we generate final output -->
    <xsl:text> - </xsl:text>
    <xsl:value-of select="normalize-space (refpurpose)"/>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="refsynopsisdiv">
    <xsl:call-template name="mark.subheading"/>
    <xsl:text>.SH "</xsl:text>
    <xsl:apply-templates select="." mode="title.markup"/>
    <xsl:text>"&#10;</xsl:text>
    <xsl:call-template name="mark.subheading"/>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="refsect1|refentry/refsection">
    <xsl:call-template name="mark.subheading"/>
    <xsl:text>.SH "</xsl:text>
    <xsl:apply-templates select="." mode="title.markup"/>
    <xsl:text>"&#10;</xsl:text>
    <xsl:call-template name="mark.subheading"/>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="refsect2|refentry/refsection/refsection">
    <xsl:call-template name="mark.subheading"/>
    <xsl:variable name="title">
      <xsl:value-of select="(info/title
                            |refsectioninfo/title
                            |refsect1info/title
                            |title)[1]"/>
    </xsl:variable>
    <xsl:text>.SS "</xsl:text>
    <xsl:value-of select="$title"/>
    <xsl:text>"&#10;</xsl:text>
    <xsl:call-template name="mark.subheading"/>
    <xsl:choose>
      <!-- * If default-indentation adjustment is on, then indent the -->
      <!-- * child content of this Refsect2 -->
      <xsl:when test="not($man.indent.refsect = 0)">
        <xsl:text>.RS&#10;</xsl:text>
        <xsl:apply-templates/>
        <xsl:text>.RE&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <!-- * If default-indentation adjustment is on, then do not -->
        <!-- * indent the child content of thie Refsect2, because -->
        <!-- * the title is already "sticking out to the left" -->
        <!-- * (as the groff_man(7) man page describes it), which -->
        <!-- * actually means the title is indented by the value of -->
        <!-- * the SN register, which appears by default to be -->
        <!-- * about half of the default indentation value -->
        <xsl:apply-templates/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>.\" end of SS subsection "</xsl:text>
    <xsl:value-of select="$title"/>
    <xsl:text>"&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="refsect3|refentry/refsection/refsection/refsection">
    <xsl:variable name="title">
      <xsl:value-of select="(info/title
                            |refsectioninfo/title
                            |refsect1info/title
                            |title)[1]"/>
    </xsl:variable>
    <xsl:choose>
      <!-- * If default-indentation adjustment is on, then indent the -->
      <!-- * child content of this Refsect3 or Refsection. -->
      <xsl:when test="not($man.indent.refsect != 0)">
        <xsl:call-template name="nested-section-title"/>
        <xsl:text>.RS&#10;</xsl:text>
        <xsl:apply-templates/>
        <xsl:text>.RE&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <!-- * If default-indentation adjustment is on, then do not -->
        <!-- * indent the child content of thie Refsect2, because -->
        <!-- * the title is already "sticking out to the left" -->
        <!-- * (as the groff_man(7) man page describes it), which -->
        <!-- * actually means the title is indented by the value of -->
        <!-- * the SN register, which appears by default to be -->
        <!-- * about half of the default indentation value -->
        <xsl:text>.ti (\n(SNu * 5u / 3u)&#10;</xsl:text>
        <xsl:call-template name="nested-section-title"/>
        <xsl:apply-templates/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>.\" end of subsection "</xsl:text>
    <xsl:value-of select="$title"/>
    <xsl:text>"&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="refsection">
    <!-- * This template is used for a nested Refsection that is -->
    <!-- * is a child of a Refsect3-level section (The numberd -->
    <!-- * Refsect hierarchy in DocBook ends with Refsect3, so -->
    <!-- * there is not actually a Refsect4 element.) -->
    <xsl:variable name="title">
      <xsl:value-of select="(info/title
                            |refsectioninfo/title
                            |refsect1info/title
                            |title)[1]"/>
    </xsl:variable>
    <xsl:variable name="indent-width">
      <xsl:if test="not($man.indent.refsect = 0)">
        <!-- * If default-indentation adjustment is on, then indent the -->
        <!-- * child content of this Refsect3 or Refsection. -->
        <xsl:text>(\n(SNu)&#10;</xsl:text>
      </xsl:if>
    </xsl:variable>
    <xsl:call-template name="nested-section-title"/>
    <xsl:text>.RS (\n(SNu)&#10;</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>.RE&#10;</xsl:text>
    <xsl:text>.\" end of subsection "</xsl:text>
    <xsl:value-of select="$title"/>
    <xsl:text>"&#10;</xsl:text>
  </xsl:template>

  <!-- ==================================================================== -->

  <!-- * Use uppercase to render titles of all instances of Refsect1 or -->
  <!-- * top-level Refsection, including in cross-references -->
  <xsl:template match="refsect1|refentry/refsection"
                mode="title.markup">
    <xsl:variable name="title" select="(info/title
                                       |refsectioninfo/title
                                       |refsect1info/title
                                       |title)[1]"/>
    <xsl:call-template name="string.upper">
      <xsl:with-param name="string">
        <xsl:apply-templates select="$title" mode="title.markup"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- * Output of Titles from Xref with Endterm needs to be handled -->
  <!-- * separately from output for Endterm-less Xref -->
  <xsl:template match="refsect1/title
                       |refentry/refsection/title
                       |refsynopsisdiv/title"
                mode="endterm">
    <xsl:call-template name="string.upper">
      <xsl:with-param name="string">
        <xsl:apply-templates/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- * Use uppercase to render titles of all instances of Refsynopsisdiv, -->
  <!-- * including in cross-references -->
  <xsl:template match="refsynopsisdiv" mode="title.markup">
    <xsl:param name="allow-anchors" select="0"/>
    <xsl:call-template name="string.upper">
      <xsl:with-param name="string">
        <xsl:choose>
          <xsl:when test="info/title
                          |refsynopsisdivinfo/title
                          |title">
            <xsl:apply-templates
                select="(info/title
                        |refsynopsisdivinfo/title
                        |title)[1]" mode="title.markup">
              <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'RefSynopsisDiv'"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- * Use uppercase to render titles of all instances of Refnamediv, -->
  <!-- * including in cross-references -->
  <xsl:template match="refnamediv" mode="title.markup">
    <xsl:call-template name="string.upper">
      <xsl:with-param name="string">
        <xsl:call-template name="gentext">
          <xsl:with-param name="key" select="'RefName'"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="refnamediv" mode="xref-to">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:template>

  <!-- ==================================================================== -->

  <!-- * suppress any title we don't otherwise process elsewhere -->

  <xsl:template match="title"/>

</xsl:stylesheet>
