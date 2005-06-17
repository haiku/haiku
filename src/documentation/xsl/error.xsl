<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:template name="print.warning.context">
    <xsl:message>
      <xsl:text>  In </xsl:text>
      <xsl:call-template name="fully-qualified-name">
        <xsl:with-param name="node" select="."/>
      </xsl:call-template>
    </xsl:message>
  </xsl:template>
</xsl:stylesheet>
