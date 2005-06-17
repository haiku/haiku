<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet        xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" version="1.0" indent="yes" standalone="yes" />
 
  <xsl:param name="doxygen.xml.path">./</xsl:param>
 
  <xsl:template match="/">
    <doxygen>
      <xsl:attribute name="version">
        <xsl:choose>
          <xsl:when test="doxygen">
            <xsl:value-of select="doxygen/attribute::version"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="doxygenindex/attribute::version"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <!-- Load all doxgen generated xml files -->
      <xsl:for-each select="doxygen/compound">
        <xsl:variable name="id">
          <xsl:choose>
            <xsl:when test="@refid">
              <xsl:value-of select="@refid"/>
            </xsl:when>
            <xsl:when test="@id">
              <xsl:value-of select="@id"/>
            </xsl:when>
          </xsl:choose>
        </xsl:variable>
        <xsl:if test="$id">
          <xsl:copy-of select="document( concat($doxygen.xml.path, '/',
                               $id, '.xml' ) )/doxygen/*" />
        </xsl:if>
      </xsl:for-each>
      <xsl:for-each select="doxygenindex/compound">
        <xsl:variable name="id">
          <xsl:choose>
            <xsl:when test="@refid">
              <xsl:value-of select="@refid"/>
            </xsl:when>
            <xsl:when test="@id">
              <xsl:value-of select="@id"/>
            </xsl:when>
          </xsl:choose>
        </xsl:variable>
        <xsl:if test="$id">
          <xsl:copy-of select="document( concat($doxygen.xml.path, '/',
                               $id, '.xml' ) )/doxygen/*" />
        </xsl:if>
      </xsl:for-each>
    </doxygen>
  </xsl:template>
</xsl:stylesheet>
