<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:param name="boost.syntax.highlight">1</xsl:param>

  <xsl:template name="source-highlight">
    <xsl:param name="text" select="."/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Perform C++ keyword highlighting on the given text -->
  <xsl:template name="highlight-text">
    <xsl:param name="text" select="."/>
    <xsl:param name="keywords"
      select="'asm auto bool break case catch char class const const_cast continue default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new operator private protected public register reinterpret_cast return short signed sizeof static static_cast struct switch template this throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while'"/>
    <xsl:param name="best-match" select="''"/>
    
    <!-- Determine the current keyword -->
    <xsl:variable name="keyword">
      <xsl:choose>
        <xsl:when test="contains($keywords, ' ')">
          <xsl:value-of select="substring-before($keywords, ' ')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$keywords"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Determine the set of keywords that are left -->
    <xsl:variable name="keywords-left">
      <xsl:if test="contains($keywords, ' ')">
        <xsl:value-of select="substring-after($keywords, ' ')"/>
      </xsl:if>
    </xsl:variable>

    <!-- The set of characters that can be identifiers -->
    <xsl:variable name="id-chars" select="'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_'"/>

    <xsl:variable name="X" select="'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'"/>

    <xsl:choose>
      <!-- Have we exhausted all keywords without finding any to highlight? -->
      <xsl:when test="$keyword='' and $best-match=''">
        <!-- Just copy the text -->
        <xsl:copy-of select="$text"/>
      </xsl:when>

      <!-- Have we exhausted all keywords, but have one to highlight? If so,
           make sure we didn't just find part of an identifier. -->
      <xsl:when 
        test="$keyword='' and
              not (starts-with(translate(substring-after($text, $best-match), 
                                         $id-chars, $X), 'X')) and
              not (substring(translate(substring-before($text, $best-match),
                                       $id-chars, $X),
                             string-length(substring-before($text, 
                                                            $best-match)),
                             1) = 'X')">
        <!-- Copy text before this keyword -->
        <xsl:value-of select="substring-before($text, $best-match)"/>

        <!-- Highlight the keyword -->
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$best-match"/>
        </xsl:call-template>

        <!-- Recurse on the rest of the text -->
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" 
            select="substring-after($text, $best-match)"/>
        </xsl:call-template>
      </xsl:when>

      <!-- We thought we had a keyword to highlight, but it was part of an 
           identifier. So output all of the text up to (but not including!)
           the last letter of the identifier, and try again to
           highlight. -->
      <xsl:when test="$keyword=''">
        <xsl:value-of select="substring-before($text, $best-match)"/>
        <xsl:value-of 
          select="substring($best-match, 1, string-length($best-match)-1)"/>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text"
            select="concat(substring($best-match, string-length($best-match), 
                           1), substring-after($text, $best-match))"/>
        </xsl:call-template>
      </xsl:when>
      
      <!-- Does the text contain this keyword with a better match than we
           previously had? -->
      <xsl:when 
        test="contains($text, $keyword) and
              (($best-match = '') or 
               (string-length(substring-before($text, $keyword)) &lt;
                string-length(substring-before($text, $best-match))))">
        <!-- Recurse with the current keyword as the new best match -->
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="keywords" select="$keywords-left"/>
          <xsl:with-param name="best-match" select="$keyword"/>
        </xsl:call-template>
      </xsl:when>

      <!-- Text does not contain this keyword. Just recurse normally -->
      <xsl:otherwise>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="keywords" select="$keywords-left"/>
          <xsl:with-param name="best-match" select="$best-match"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*" mode="highlight">
    <xsl:element name="{name(.)}">
      <xsl:for-each select="./@*">
        <xsl:choose>
          <xsl:when test="local-name(.)='last-revision'">
            <xsl:attribute
              name="rev:last-revision"
              namespace="http://www.cs.rpi.edu/~gregod/boost/tools/doc/revision"
>
              <xsl:value-of select="."/>
            </xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="{name(.)}">
              <xsl:value-of select="."/>
            </xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:apply-templates mode="highlight"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="text()" mode="highlight">
    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="classname|methodname|functionname|libraryname|enumname|
                       conceptname|macroname" mode="highlight">
    <xsl:apply-templates select="." mode="annotation"/>
  </xsl:template>
</xsl:stylesheet>
