<?xml version="1.0" encoding="utf-8"?>
<axsl:stylesheet xmlns:axsl="http://www.w3.org/1999/XSL/Transform" xmlns:w="http://schemas.microsoft.com/office/word/2003/wordml" xmlns:v="urn:schemas-microsoft-com:vml" xmlns:w10="urn:schemas-microsoft-com:office:word" xmlns:sl="http://schemas.microsoft.com/schemaLibrary/2003/core" xmlns:aml="http://schemas.microsoft.com/aml/2001/core" xmlns:wx="http://schemas.microsoft.com/office/word/2003/auxHint" xmlns:o="urn:schemas-microsoft-com:office:office" xmlns:dt="uuid:C2F41010-65B3-11d1-A29F-00AA00C14882" xmlns:st1="urn:schemas-microsoft-com:office:smarttags" version="1.0">
<!--====================================-->
<!--=                                  =-->
<!--=   DO NOT EDIT THIS STYLESHEET    =-->
<!--=                                  =-->
<!--= This stylesheet is generated     =-->
<!--= by makeSections.xsl and a        =-->
<!--= mapping specification.           =-->
<!--=                                  =-->
<!--= Revision history:                =-->
<!--=  1.0 2005-11-08 SRB              =-->
<!--=   Initial version.               =-->
<!--=                                  =-->
<!--= $Id: wordml-blocks.xsl,v 1.1 2005/11/11 05:18:16 balls Exp $ =-->
<!--=                                  =-->
<!--====================================-->
  <axsl:output indent="yes" encoding="utf-8"/>
  <axsl:strip-space elements="*"/>
  <axsl:preserve-space elements="w:t"/>
  <axsl:template match="wx:sub-section">
    <axsl:variable name="subsections" select="w:p[w:pPr/w:pStyle/@w:val = &quot;bibliography&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliography-title&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary-title&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset-title&quot;]"/>
    <axsl:copy>
      <axsl:apply-templates select="@*"/>
      <axsl:choose>
        <axsl:when test="$subsections">
          <axsl:apply-templates select="$subsections[1]/preceding-sibling::node()"/>
          <axsl:apply-templates select="$subsections[1]" mode="subsections">
            <axsl:with-param name="subsections" select="$subsections[position() != 1]"/>
          </axsl:apply-templates>
        </axsl:when>
        <axsl:when test="wx:sub-section">
          <axsl:apply-templates select="wx:sub-section[1]/preceding-sibling::node()"/>
          <axsl:apply-templates select="wx:sub-section"/>
        </axsl:when>
        <axsl:otherwise>
          <axsl:apply-templates/>
        </axsl:otherwise>
      </axsl:choose>
    </axsl:copy>
    <axsl:choose>
      <axsl:when test="following-sibling::wx:sub-section | following-sibling::w:p[w:pPr/w:pStyle/@w:val = &quot;bibliography&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliography-title&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary-title&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset-title&quot;]">
        <axsl:variable name="nextComponent" select="following-sibling::*[self::wx:sub-section|self::w:p[w:pPr/w:pStyle/@w:val = &quot;bibliography&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliography-title&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary-title&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset-title&quot;]][1]"/>
        <axsl:apply-templates select="following-sibling::*[generate-id(following-sibling::*[self::wx:sub-section|self::w:p[w:pPr/w:pStyle/@w:val = &quot;bibliography&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliography-title&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary-title&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset-title&quot;]][1]) = generate-id($nextComponent)]"/>
      </axsl:when>
      <axsl:otherwise>
        <axsl:apply-templates select="following-sibling::*"/>
      </axsl:otherwise>
    </axsl:choose>
  </axsl:template>
  <axsl:template match="w:p" mode="subsections">
    <axsl:param name="subsections" select="/.."/>
    <axsl:choose>
      <axsl:when test="w:pPr/w:pStyle/@w:val = &quot;bibliography&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliography-title&quot;">
        <wx:sub-section class="bibliography">
          <axsl:call-template name="copy"/>
          <axsl:variable name="bibliodivs" select="following-sibling::w:p[w:pPr/w:pStyle/@w:val = &quot;bibliodiv&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliodiv-title&quot;]"/>
          <axsl:choose>
            <axsl:when test="$bibliodivs">
              <axsl:apply-templates select="following-sibling::*[1]" mode="bibliodivs">
                <axsl:with-param name="nextSubsection" select="$subsections[1]"/>
                <axsl:with-param name="bibliodivs" select="$bibliodivs[position() != 1]"/>
              </axsl:apply-templates>
            </axsl:when>
            <axsl:otherwise>
              <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
                <axsl:with-param name="nextSubsection" select="$subsections[1]"/>
              </axsl:apply-templates>
            </axsl:otherwise>
          </axsl:choose>
        </wx:sub-section>
      </axsl:when>
      <axsl:when test="w:pPr/w:pStyle/@w:val = &quot;glossary&quot; or w:pPr/w:pStyle/@w:val = &quot;glossary-title&quot;">
        <wx:sub-section class="glossary">
          <axsl:call-template name="copy"/>
          <axsl:variable name="glossdivs" select="following-sibling::w:p[w:pPr/w:pStyle/@w:val = &quot;glossdiv&quot; or w:pPr/w:pStyle/@w:val = &quot;glossdiv-title&quot;]"/>
          <axsl:choose>
            <axsl:when test="$glossdivs">
              <axsl:apply-templates select="following-sibling::*[1]" mode="glossdivs">
                <axsl:with-param name="nextSubsection" select="$subsections[1]"/>
                <axsl:with-param name="glossdivs" select="$glossdivs[position() != 1]"/>
              </axsl:apply-templates>
            </axsl:when>
            <axsl:otherwise>
              <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
                <axsl:with-param name="nextSubsection" select="$subsections[1]"/>
              </axsl:apply-templates>
            </axsl:otherwise>
          </axsl:choose>
        </wx:sub-section>
      </axsl:when>
      <axsl:when test="w:pPr/w:pStyle/@w:val = &quot;qandaset&quot; or w:pPr/w:pStyle/@w:val = &quot;qandaset-title&quot;">
        <wx:sub-section class="qandaset">
          <axsl:call-template name="copy"/>
          <axsl:variable name="qandadivs" select="following-sibling::w:p[w:pPr/w:pStyle/@w:val = &quot;qandadiv&quot; or w:pPr/w:pStyle/@w:val = &quot;qandadiv-title&quot;]"/>
          <axsl:choose>
            <axsl:when test="$qandadivs">
              <axsl:apply-templates select="following-sibling::*[1]" mode="qandadivs">
                <axsl:with-param name="nextSubsection" select="$subsections[1]"/>
                <axsl:with-param name="qandadivs" select="$qandadivs[position() != 1]"/>
              </axsl:apply-templates>
            </axsl:when>
            <axsl:otherwise>
              <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
                <axsl:with-param name="nextSubsection" select="$subsections[1]"/>
              </axsl:apply-templates>
            </axsl:otherwise>
          </axsl:choose>
        </wx:sub-section>
      </axsl:when>
    </axsl:choose>
  </axsl:template>
  <axsl:template match="*" mode="subsections">
    <axsl:param name="subsections" select="/.."/>
    <axsl:copy>
      <axsl:apply-templates select="@*"/>
      <axsl:apply-templates mode="subsections"/>
    </axsl:copy>
  </axsl:template>
  <axsl:template match="w:p" mode="bibliodivs">
    <axsl:param name="nextSubsection" select="/.."/>
    <axsl:param name="bibliodivs" select="/.."/>
    <axsl:choose>
      <axsl:when test="generate-id() = generate-id($nextSubsection)"/>
      <axsl:when test="w:pPr/w:pStyle/@w:val = &quot;bibliodiv&quot; or w:pPr/w:pStyle/@w:val = &quot;bibliodiv-title&quot;">
        <wx:sub-section class="bibliodiv">
          <axsl:call-template name="copy"/>
          <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
            <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
            <axsl:with-param name="nextbibliodiv" select="$bibliodivs[1]"/>
          </axsl:apply-templates>
        </wx:sub-section>
        <axsl:choose>
          <axsl:when test="$nextSubsection and         $bibliodivs and         count($nextSubsection/preceding-sibling::* | $bibliodivs[1]) = count($nextSubsection/preceding-sibling::*)">
            <axsl:apply-templates select="$bibliodivs[1]" mode="bibliodivs">
              <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
              <axsl:with-param name="bibliodivs" select="$bibliodivs[position() != 1]"/>
            </axsl:apply-templates>
          </axsl:when>
          <axsl:when test="$bibliodivs">
            <axsl:apply-templates select="$bibliodivs[1]" mode="bibliodivs">
              <axsl:with-param name="bibliodivs" select="$bibliodivs[position() != 1]"/>
            </axsl:apply-templates>
          </axsl:when>
        </axsl:choose>
      </axsl:when>
      <axsl:otherwise>
        <axsl:call-template name="copy"/>
        <axsl:apply-templates select="following-sibling::*[1]" mode="bibliodivs">
          <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
        </axsl:apply-templates>
      </axsl:otherwise>
    </axsl:choose>
  </axsl:template>
  <axsl:template match="w:p" mode="glossdivs">
    <axsl:param name="nextSubsection" select="/.."/>
    <axsl:param name="glossdivs" select="/.."/>
    <axsl:choose>
      <axsl:when test="generate-id() = generate-id($nextSubsection)"/>
      <axsl:when test="w:pPr/w:pStyle/@w:val = &quot;glossdiv&quot; or w:pPr/w:pStyle/@w:val = &quot;glossdiv-title&quot;">
        <wx:sub-section class="glossdiv">
          <axsl:call-template name="copy"/>
          <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
            <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
            <axsl:with-param name="nextglossdiv" select="$glossdivs[1]"/>
          </axsl:apply-templates>
        </wx:sub-section>
        <axsl:choose>
          <axsl:when test="$nextSubsection and         $glossdivs and         count($nextSubsection/preceding-sibling::* | $glossdivs[1]) = count($nextSubsection/preceding-sibling::*)">
            <axsl:apply-templates select="$glossdivs[1]" mode="glossdivs">
              <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
              <axsl:with-param name="glossdivs" select="$glossdivs[position() != 1]"/>
            </axsl:apply-templates>
          </axsl:when>
          <axsl:when test="$glossdivs">
            <axsl:apply-templates select="$glossdivs[1]" mode="glossdivs">
              <axsl:with-param name="glossdivs" select="$glossdivs[position() != 1]"/>
            </axsl:apply-templates>
          </axsl:when>
        </axsl:choose>
      </axsl:when>
      <axsl:otherwise>
        <axsl:call-template name="copy"/>
        <axsl:apply-templates select="following-sibling::*[1]" mode="glossdivs">
          <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
        </axsl:apply-templates>
      </axsl:otherwise>
    </axsl:choose>
  </axsl:template>
  <axsl:template match="w:p" mode="qandadivs">
    <axsl:param name="nextSubsection" select="/.."/>
    <axsl:param name="qandadivs" select="/.."/>
    <axsl:choose>
      <axsl:when test="generate-id() = generate-id($nextSubsection)"/>
      <axsl:when test="w:pPr/w:pStyle/@w:val = &quot;qandadiv&quot; or w:pPr/w:pStyle/@w:val = &quot;qandadiv-title&quot;">
        <wx:sub-section class="qandadiv">
          <axsl:call-template name="copy"/>
          <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
            <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
            <axsl:with-param name="nextqandadiv" select="$qandadivs[1]"/>
          </axsl:apply-templates>
        </wx:sub-section>
        <axsl:choose>
          <axsl:when test="$nextSubsection and         $qandadivs and         count($nextSubsection/preceding-sibling::* | $qandadivs[1]) = count($nextSubsection/preceding-sibling::*)">
            <axsl:apply-templates select="$qandadivs[1]" mode="qandadivs">
              <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
              <axsl:with-param name="qandadivs" select="$qandadivs[position() != 1]"/>
            </axsl:apply-templates>
          </axsl:when>
          <axsl:when test="$qandadivs">
            <axsl:apply-templates select="$qandadivs[1]" mode="qandadivs">
              <axsl:with-param name="qandadivs" select="$qandadivs[position() != 1]"/>
            </axsl:apply-templates>
          </axsl:when>
        </axsl:choose>
      </axsl:when>
      <axsl:otherwise>
        <axsl:call-template name="copy"/>
        <axsl:apply-templates select="following-sibling::*[1]" mode="qandadivs">
          <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
        </axsl:apply-templates>
      </axsl:otherwise>
    </axsl:choose>
  </axsl:template>
  <axsl:template match="*" mode="terminal">
    <axsl:param name="nextSubsection" select="/.."/>
    <axsl:param name="nextbibliodiv" select="/.."/>
    <axsl:param name="nextglossdiv" select="/.."/>
    <axsl:param name="nextqandadiv" select="/.."/>
    <axsl:choose>
      <axsl:when test="generate-id() = generate-id($nextSubsection)"/>
      <axsl:when test="generate-id() = generate-id($nextbibliodiv)"/>
      <axsl:when test="generate-id() = generate-id($nextglossdiv)"/>
      <axsl:when test="generate-id() = generate-id($nextqandadiv)"/>
      <axsl:otherwise>
        <axsl:call-template name="copy"/>
        <axsl:apply-templates select="following-sibling::*[1]" mode="terminal">
          <axsl:with-param name="nextSubsection" select="$nextSubsection"/>
          <axsl:with-param name="nextbibliodiv" select="$nextbibliodiv"/>
          <axsl:with-param name="nextglossdiv" select="$nextglossdiv"/>
          <axsl:with-param name="nextqandadiv" select="$nextqandadiv"/>
        </axsl:apply-templates>
      </axsl:otherwise>
    </axsl:choose>
  </axsl:template>
  <axsl:template match="*">
    <axsl:copy>
      <axsl:apply-templates select="@*"/>
      <axsl:apply-templates/>
    </axsl:copy>
  </axsl:template>
  <axsl:template name="copy">
    <axsl:copy>
      <axsl:apply-templates select="@*"/>
      <axsl:apply-templates/>
    </axsl:copy>
  </axsl:template>
  <axsl:template match="@*">
    <axsl:copy/>
  </axsl:template>
</axsl:stylesheet>
