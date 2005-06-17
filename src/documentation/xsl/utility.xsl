<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY nbsp "&#160;">
]>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <!-- Indent the current line-->
  <xsl:template name="indent">
    <xsl:param name="indentation"/>
    <xsl:if test="$indentation > 0">
      <xsl:text>&nbsp;</xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation - 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
