<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:w="http://schemas.microsoft.com/office/word/2003/wordml"
  xmlns:v="urn:schemas-microsoft-com:vml"
  xmlns:w10="urn:schemas-microsoft-com:office:word"
  xmlns:sl="http://schemas.microsoft.com/schemaLibrary/2003/core"
  xmlns:aml="http://schemas.microsoft.com/aml/2001/core"
  xmlns:wx="http://schemas.microsoft.com/office/word/2003/auxHint"
  xmlns:o="urn:schemas-microsoft-com:office:office"
  xmlns:dt="uuid:C2F41010-65B3-11d1-A29F-00AA00C14882">

  <xsl:output method='xml' indent="yes" encoding='UTF-8'/>

  <!-- ********************************************************************
       $Id: wordml-normalise.xsl,v 1.4 2004/12/24 04:28:48 balls Exp $
       ********************************************************************

       This file is part of the XSL DocBook Stylesheet distribution.
       See ../README or http://nwalsh.com/docbook/xsl/ for copyright
       and other information.

       ******************************************************************** -->

  <xsl:strip-space elements='*'/>
  <xsl:preserve-space elements='w:t'/>

  <xsl:template match="w:wordDocument">
    <xsl:copy>
      <xsl:apply-templates select='w:body'/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match='wx:sect|wx:sub-section|wx:pBdrGroup'>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match='wx:borders |
                       wx:margin-left'/>

  <xsl:template match='w:pStyle|w:rStyle'>
    <xsl:copy>
      <xsl:for-each select='@*'>
        <xsl:copy/>
      </xsl:for-each>
    </xsl:copy>
  </xsl:template>

  <xsl:template match='*'>
    <xsl:copy>
      <xsl:for-each select='@*'>
        <xsl:copy/>
      </xsl:for-each>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
