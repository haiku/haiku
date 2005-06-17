<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:strip-space elements="inherit purpose"/>

  <!-- When true, the stylesheet will emit compact definitions of
       enumerations when the enumeration does not have any detailed
       description. A compact definition renders the enum definition along
       with a comment for the purpose of the enum (if it exists) directly
       within the synopsis. A non-compact definition will create a
       separate refentry element for the enum. -->
  <xsl:param name="boost.compact.enum">1</xsl:param>

  <xsl:template match="class|struct|union" mode="generate.id">
    <xsl:choose>
      <xsl:when test="count(key('named-entities', @name))=1">
        <xsl:value-of select="@name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="fully-qualified-name">
          <xsl:with-param name="node" select="."/>
          <xsl:with-param name="separator" select="'.'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="enum" mode="generate.id">
    <xsl:text>enum.</xsl:text>
    <xsl:call-template name="fully-qualified-name">
      <xsl:with-param name="node" select="."/>
      <xsl:with-param name="separator" select="'.'"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Display the full name of the current node, e.g., "Class
       template function". -->
  <xsl:template name="type.display.name">
    <xsl:choose>
      <xsl:when test="contains(local-name(.), 'class')">
        <xsl:text>Class </xsl:text>        
      </xsl:when>
      <xsl:when test="contains(local-name(.), 'struct')">
        <xsl:text>Struct </xsl:text>
      </xsl:when>
      <xsl:when test="contains(local-name(.), 'union')">
        <xsl:text>Union </xsl:text>
      </xsl:when>
      <xsl:when test="local-name(.)='enum'">
        <xsl:text>Type </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
Unknown type element "<xsl:value-of select="local-name(.)"/>" in type.display.name
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="template and count(template/*) &gt; 0">
      <xsl:text>template </xsl:text>
    </xsl:if>
    <xsl:call-template name="monospaced">
      <xsl:with-param name="text">
        <xsl:value-of select="@name"/>
        <xsl:apply-templates select="specialization"/>
      </xsl:with-param>
    </xsl:call-template>    
  </xsl:template>

  <!-- Determine the class key for the given node -->
  <xsl:template name="type.class.key">
    <xsl:param name="node" select="."/>
    <xsl:choose>
      <xsl:when test="contains(local-name($node), '-specialization')">
        <xsl:value-of select="substring-before(local-name($node), '-')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="local-name($node)"/>
      </xsl:otherwise>
    </xsl:choose>    
  </xsl:template>

  <!-- Emit class synopsis -->
  <xsl:template match="class|class-specialization|
                       struct|struct-specialization|
                       union|union-specialization" mode="synopsis">
    <xsl:param name="indentation"/>
   
    <!-- The keyword used to declare this class type, e.g., class,
         struct, or union. -->
    <xsl:variable name="class-key">
      <xsl:call-template name="type.class.key"/>
    </xsl:variable>

    <!-- Spacing -->
    <xsl:if test="not (local-name(preceding-sibling::*[position()=1])=local-name(.)) and (position() &gt; 1)">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <xsl:text>&#10;</xsl:text>

    <!-- Calculate how long this declaration would be if we put it all
         on one line -->
    <xsl:variable name="full-decl-string">
      <xsl:apply-templates select="template" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation"/>
        <xsl:with-param name="wrap" select="false()"/>
      </xsl:apply-templates>
      <xsl:value-of select="$class-key"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:apply-templates select="specialization"/>
      <xsl:text>;</xsl:text>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="(string-length($full-decl-string) + 
                       string-length($indentation)) &lt; $max-columns">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <xsl:apply-templates select="template" mode="synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:apply-templates>

        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$class-key"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id"/>
          </xsl:with-param>
          <xsl:with-param name="text">
            <xsl:value-of select="@name"/>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:apply-templates select="specialization"/>
        <xsl:text>;</xsl:text>
      </xsl:when>

      <xsl:otherwise>
        <!-- Template header -->
        <xsl:if test="template">
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
          <xsl:apply-templates select="template" mode="synopsis">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:apply-templates>
          <xsl:text>&#10;</xsl:text>
          
          <!-- Indent class templates' names in the synopsis -->
          <xsl:text>  </xsl:text>
        </xsl:if>
        
        <!-- Class name -->
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$class-key"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="string(@name)"/>
        </xsl:call-template>
        <xsl:apply-templates select="specialization"/>
        <xsl:text>;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>

    <!-- Free functions associated with the class -->
    <xsl:apply-templates select="free-function-group" mode="header-synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="class" select="@name"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Emit a typedef synopsis -->
  <xsl:template name="type.typedef.display.aligned">
    <xsl:param name="indentation"/>
    <xsl:param name="max-type-length"/>
    <xsl:param name="max-name-length"/>

    <!-- Padding for the typedef types -->
    <xsl:variable name="type-padding">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$max-type-length"/>
      </xsl:call-template>
    </xsl:variable>

    <!-- Padding for the typedef names -->
    <xsl:variable name="name-padding">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$max-name-length"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:text>&#10;</xsl:text>
    <xsl:choose>
      <!-- Create a vertical ellipsis -->
      <xsl:when test="@name = '...'">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation + 3"/>      
        </xsl:call-template>        
        <xsl:text>.&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation + 3"/>      
        </xsl:call-template>
        <xsl:text>.&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation + 3"/>      
        </xsl:call-template>
        <xsl:text>.</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>      
        </xsl:call-template>
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'typedef'"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>

        <!-- Length of the type -->
        <xsl:variable name="type-length">
          <xsl:choose>
            <xsl:when test="@type">
              <xsl:message>
                <xsl:text>Warning: `type' attribute of `typedef' element is deprecated. Use 'type' element instead.</xsl:text>
              </xsl:message>
              <xsl:call-template name="print.warning.context"/>
              <xsl:value-of select="string-length(@type)"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="string-length(type)"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <!-- Emit the type -->
        <xsl:choose>
          <xsl:when test="@type">
            <xsl:value-of select="@type"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="type/*|type/text()" 
              mode="highlight"/>
            <!--
            <xsl:call-template name="source-highlight">
              <xsl:with-param name="text">
                <xsl:apply-templates select="type/*|type/text()"/>
              </xsl:with-param>
            </xsl:call-template> 
            -->
          </xsl:otherwise>
        </xsl:choose>

        <xsl:choose>
          <xsl:when test="$max-type-length &gt; 0">
            <xsl:value-of select="substring($type-padding, 1, 
                                            $max-type-length - $type-length)"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="substring(concat(@name, ';', $name-padding), 
                                            1, $max-name-length)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat(' ',@name,';')"/>
          </xsl:otherwise>
        </xsl:choose>

        <xsl:if test="purpose">
          <xsl:text>  </xsl:text>
          <xsl:call-template name="highlight-comment">
            <xsl:with-param name="text">
              <xsl:text>// </xsl:text>
              <xsl:apply-templates select="purpose" mode="comment"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="typedef" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:param name="max-type-length" select="0"/>
    <xsl:param name="max-name-length" select="0"/>
    
    <!-- Spacing -->
    <xsl:if test="not (local-name(preceding-sibling::*[position()=1])=local-name(.)) and (position() &gt; 1)">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <xsl:call-template name="type.typedef.display.aligned">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="max-type-length" select="$max-type-length"/>
      <xsl:with-param name="max-name-length" select="$max-name-length"/>
    </xsl:call-template>
  </xsl:template>

  <!-- TBD: Implement this -->
  <xsl:template match="typedef" mode="reference"/>

  <!-- Emit a list of static constants -->
  <xsl:template match="static-constant" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:text>&#10;</xsl:text>

    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text" select="'static const '"/>
    </xsl:call-template>

    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text">
        <xsl:apply-templates select="type/*|type/text()"/>    
      </xsl:with-param>
    </xsl:call-template>
    
    <xsl:if test="not(@name = '')">
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
    </xsl:if>

    <xsl:text> = </xsl:text>

    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text">
        <xsl:apply-templates select="default/*|default/text()"/>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:text>;</xsl:text>

    <xsl:if test="purpose">
      <xsl:text>  </xsl:text>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text">
          <xsl:text>// </xsl:text>
          <xsl:apply-templates select="purpose" mode="comment"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Format base classes on a single line -->
  <xsl:template name="print.base.classes.single">
    <xsl:apply-templates select="inherit"/>
  </xsl:template>

  <xsl:template name="print.base.classes.multi">
    <xsl:param name="indentation"/>
    
    <xsl:variable name="n" select="count(inherit)"/>
    <xsl:for-each select="inherit">
      <!-- Indentation -->
      <xsl:if test="position() &gt; 1">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:if>

      <!-- Output the access specifier -->
      <xsl:variable name="access">
        <xsl:choose>
          <xsl:when test="@access">
            <xsl:value-of select="@access"/>
          </xsl:when>
          <xsl:when test="parent::class|parent::class-specialization">
            <xsl:text>private</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>public</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:call-template name="highlight-keyword">
        <xsl:with-param name="keyword" select="@access"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>

      <!-- Output the type -->
      <xsl:choose>
        <xsl:when test="type">
          <xsl:apply-templates select="type/*|type/text()" mode="annotation"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates mode="annotation"/>
        </xsl:otherwise>
      </xsl:choose>

      <!-- Output a comma if not at the end -->
      <xsl:if test="position() &lt; $n">
        <xsl:text>,</xsl:text>
      </xsl:if>

      <!-- Output a comment if we have one -->
      <xsl:if test="purpose">
        <xsl:choose>
          <xsl:when test="position() &lt; $n">
            <xsl:text>  </xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>   </xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="highlight-comment">
          <xsl:with-param name="text">
            <xsl:text>// </xsl:text>
            <xsl:apply-templates select="purpose/*|purpose/text()"
              mode="annotation"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:if>

      <xsl:if test="position() &lt; $n">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="print.base.classes">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="base-indentation" select="0"/>

    <xsl:variable name="single-line-candidate" select="not(inherit/purpose)"/>
    <xsl:variable name="single-line">
      <xsl:if test="$single-line-candidate">
        <xsl:call-template name="print.base.classes.single"/>
      </xsl:if>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$single-line-candidate and 
                      (string-length($single-line) + $indentation + 3
                        &lt; $max-columns)">
        <xsl:call-template name="print.base.classes.single"/>
        <xsl:text> {</xsl:text>
      </xsl:when>
      <xsl:when test="$single-line-candidate and
                      (string-length($single-line) + $base-indentation + 5
                        &lt; $max-columns)">
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$base-indentation"/>
        </xsl:call-template>
        <xsl:text>  : </xsl:text>
        <xsl:call-template name="print.base.classes.single"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$base-indentation"/>
        </xsl:call-template>
        <xsl:text>{</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> : </xsl:text>
        <xsl:call-template name="print.base.classes.multi">
          <xsl:with-param name="indentation" select="$indentation + 3"/>
        </xsl:call-template>
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$base-indentation"/>
        </xsl:call-template>
        <xsl:text>{</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Emit a list of base classes without comments and on a single line -->
  <xsl:template match="inherit">
    <xsl:choose>
      <xsl:when test="position()=1">
        <xsl:text> : </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>, </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="@access"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <xsl:apply-templates mode="annotation">
      <xsl:with-param name="highlight" select="true()"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Find the maximum length of the types in typedefs -->
  <xsl:template name="find-max-type-length">
    <xsl:param name="typedefs" select="typedef"/>
    <xsl:param name="max-length" select="0"/>
    <xsl:param name="want-name" select="false()"/>

    <xsl:choose>
      <xsl:when test="$typedefs">
        <xsl:variable name="typedef" select="$typedefs[position()=1]"/>
        <xsl:variable name="rest" select="$typedefs[position()!=1]"/>

        <!-- Find the length of the type -->
        <xsl:variable name="length">
          <xsl:choose>
            <xsl:when test="$typedef/@name != '...'">
              <xsl:choose>
                <xsl:when test="$want-name">
                  <xsl:value-of select="string-length($typedef/@name) + 1"/>
                </xsl:when>
                <xsl:when test="$typedef/@type">
                  <xsl:value-of select="string-length($typedef/@type)"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="string-length($typedef/type)"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:when>
            <xsl:otherwise>
              0
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <!-- Pass on the larger length -->
        <xsl:choose>
          <xsl:when test="$length &gt; $max-length">
            <xsl:call-template name="find-max-type-length">
              <xsl:with-param name="typedefs" select="$rest"/>
              <xsl:with-param name="max-length" select="$length"/>
              <xsl:with-param name="want-name" select="$want-name"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="find-max-type-length">
              <xsl:with-param name="typedefs" select="$rest"/>
              <xsl:with-param name="max-length" select="$max-length"/>
              <xsl:with-param name="want-name" select="$want-name"/>
            </xsl:call-template>            
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$max-length"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>  

  <xsl:template match="constructor" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:call-template name="function">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="context" select="../@name"/>
      <xsl:with-param name="is-reference" select="false()"/>
      <xsl:with-param name="constructor-for" select="../@name"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="copy-assignment" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:call-template name="function">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="context" select="../@name"/>
      <xsl:with-param name="is-reference" select="false()"/>
      <xsl:with-param name="copy-assign-for" select="../@name"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="destructor" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:call-template name="function">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="context" select="../@name"/>
      <xsl:with-param name="is-reference" select="false()"/>
      <xsl:with-param name="destructor-for" select="../@name"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="class-type-synopsis">
    <xsl:param name="indentation" select="0"/>

    <!-- The keyword used to declare this class type, e.g., class,
         struct, or union. -->
    <xsl:variable name="class-key">
      <xsl:call-template name="type.class.key"/>
    </xsl:variable>

    <xsl:if test="ancestor::class|ancestor::class-specialization|
                  ancestor::struct|ancestor::struct-specialization|
                  ancestor::union|ancestor::union-specialization">
      <xsl:text>&#10;&#10;</xsl:text>

      <!-- If this nested class has a "purpose" element, use it as a
           comment. -->
      <xsl:if test="purpose">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-comment">
          <xsl:with-param name="text">
            <xsl:text>// </xsl:text>
            <xsl:apply-templates select="purpose" mode="comment"/>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
    </xsl:if>

    <!-- Template header -->
    <xsl:if test="template">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:call-template>
      <xsl:apply-templates select="template" mode="reference">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:apply-templates>
    </xsl:if>
    <xsl:text>&#10;</xsl:text>
    
    <!-- Class name -->
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="$class-key"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:apply-templates select="specialization"/>
    
    <xsl:choose>
      <xsl:when test="inherit">
        <!-- Base class list (with opening brace) -->
        <xsl:call-template name="print.base.classes">
          <xsl:with-param name="indentation" 
            select="string-length($class-key) + string-length(@name) 
                    + $indentation + 1"/>
          <xsl:with-param name="base-indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <!-- Opening brace -->
        <xsl:text> {</xsl:text>
      </xsl:otherwise>
    </xsl:choose>

    <!-- Public designator -->
    <xsl:if test="contains(local-name(.), 'class')">
      <xsl:text>&#10;</xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-keyword">
        <xsl:with-param name="keyword" select="'public'"/>
      </xsl:call-template>
      <xsl:text>:</xsl:text>
    </xsl:if>

    <!-- Typedefs -->
    <xsl:if test="typedef">
      <xsl:text>&#10;</xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text" select="'// types'"/>
      </xsl:call-template>
      
      <xsl:variable name="max-type-length">
        <xsl:call-template name="find-max-type-length"/>
      </xsl:variable>
      <xsl:variable name="max-name-length">
        <xsl:call-template name="find-max-type-length">
          <xsl:with-param name="want-name" select="true()"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:apply-templates select="typedef" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
        <xsl:with-param name="max-type-length" 
          select="$max-type-length"/>
        <xsl:with-param name="max-name-length" 
          select="$max-name-length"/>
      </xsl:apply-templates>
    </xsl:if>
    
    <!-- Static constants -->
    <xsl:if test="static-constant">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="typedef">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text" select="'// static constants'"/>
      </xsl:call-template>
      <xsl:apply-templates select="static-constant" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:apply-templates>
    </xsl:if>
    
    <!-- Nested classes/structs/unions -->
    <xsl:apply-templates select="class|class-specialization|
                                 struct|struct-specialization|
                                 union|union-specialization"
      mode="reference">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>
    
    <!-- Construct/Copy/Destruct -->
    <xsl:call-template name="construct-copy-destruct-synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:call-template>
    
    <!-- Member functions -->
    <xsl:apply-templates 
      select="method-group|method|overloaded-method" 
      mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>
    
    <!-- Data members -->
    <xsl:apply-templates select="data-member" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>
    
    <!-- Closing brace -->
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:text>};</xsl:text>
  </xsl:template>

  <!-- Emit nested class reference documentation -->
  <xsl:template match="class|class-specialization|
                       struct|struct-specialization|
                       union|union-specialization" mode="reference">
    <xsl:param name="indentation"/>

    <xsl:call-template name="class-type-synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Emit namespace-level class reference documentation -->
  <xsl:template match="class|class-specialization|
                       struct|struct-specialization|
                       union|union-specialization" mode="namespace-reference">
    <xsl:param name="indentation" select="0"/>

    <xsl:call-template name="separator"/>
    <xsl:call-template name="reference-documentation">
      <xsl:with-param name="refname">
        <xsl:call-template name="fully-qualified-name">
          <xsl:with-param name="node" select="."/>
        </xsl:call-template>
        <xsl:apply-templates select="specialization"/>
      </xsl:with-param>
      <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
      <xsl:with-param name="anchor">
        <xsl:call-template name="generate.id">
          <xsl:with-param name="node" select="."/>
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="name">
        <xsl:call-template name="type.display.name"/>
      </xsl:with-param>
      <xsl:with-param name="synopsis">
        <xsl:call-template name="class-type-synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <!-- Associated free functions -->
        <xsl:apply-templates select="ancestor-or-self::*/free-function-group"
          mode="synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="class" select="@name"/>
        </xsl:apply-templates>
      </xsl:with-param>
      <xsl:with-param name="text">
        <!-- Paragraphs go into the top of the "Description" section. -->
        <xsl:if test="para">
          <xsl:message>
            <xsl:text>Warning: Use of 'para' elements in 'class' element is deprecated.&#10;Wrap them in a 'description' element.</xsl:text>
          </xsl:message>
          <xsl:call-template name="print.warning.context"/>
          <xsl:apply-templates select="para" mode="annotation"/>
        </xsl:if>
        <xsl:apply-templates select="description"/>
        
        <xsl:call-template name="construct-copy-destruct-reference"/>
        <xsl:apply-templates 
          select="method-group|method|overloaded-method"
          mode="reference"/>
        <xsl:apply-templates select="free-function-group" mode="reference">
          <xsl:with-param name="class" select="@name"/>
        </xsl:apply-templates>
        
        <!-- Specializations of this class -->
        <!-- TBD: fix this. We should key off the class name and match
             fully-qualified names -->
        <xsl:variable name="name" select="@name"/>
        <xsl:if test="local-name(.)='class' and 
                      ../class-specialization[@name=$name]">
          <refsect2>
            <title>Specializations</title>
            <itemizedlist>
              <xsl:apply-templates 
                select="../class-specialization[@name=$name]"
                mode="specialization-list"/>
            </itemizedlist>
          </refsect2>
        </xsl:if>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Output a link to a specialization -->
  <xsl:template match="class-specialization|
                       struct-specialization|
                       union-specialization" mode="specialization-list">
    <listitem>
      <para>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text">
            <xsl:call-template name="type.display.name"/>
          </xsl:with-param>
        </xsl:call-template>
      </para>
    </listitem>
  </xsl:template>

  <!-- Data member synopsis -->
  <xsl:template match="data-member" mode="synopsis">
    <xsl:param name="indentation"/>

    <!-- Spacing -->
    <xsl:if 
      test="not(local-name(preceding-sibling::*[position()=1])=local-name(.))">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <!-- Indent -->
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>

    <xsl:if test="@specifiers">
      <xsl:call-template name="highlight-keyword">
        <xsl:with-param name="keyword" select="@specifiers"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
    </xsl:if>

    <xsl:apply-templates select="type/*|type/text()" mode="annotation"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>;</xsl:text>
  </xsl:template>

  <!-- Enumeration synopsis -->
  <xsl:template match="enum" mode="synopsis">
    <xsl:param name="indentation"/>

    <!-- Spacing -->
    <xsl:if 
      test="(not (local-name(preceding-sibling::*[position()=1])=local-name(.))
             and (position() &gt; 1)) or 
            not (para or description or not ($boost.compact.enum=1))">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <!-- Indent -->
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>

    <xsl:choose>
      <!-- When there is a detailed description, we only put the
           declaration in the synopsis and will put detailed documentation
           in either a <refentry/> or in class documentation. -->
      <xsl:when test="para or description or not ($boost.compact.enum=1)">
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'enum'"/>
        </xsl:call-template>

        <xsl:text> </xsl:text>

        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="string(@name)"/>
          <xsl:with-param name="higlhight" select="false()"/>
        </xsl:call-template>
      </xsl:when>
      <!-- When there is no detailed description, we put the enum
           definition within the synopsis. The purpose of the enum (if
           available) is formatted as a comment prior to the
           definition. This way, we do not create a separate block of text
           for what is generally a very small amount of information. -->
      <xsl:otherwise>
        <xsl:if test="purpose">
          <xsl:call-template name="highlight-comment">
            <xsl:with-param name="text">
              <xsl:text>// </xsl:text>
              <xsl:apply-templates select="purpose/*" mode="annotation"/>
            </xsl:with-param>
          </xsl:call-template>

          <xsl:text>&#10;</xsl:text>
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
        </xsl:if>

        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'enum'"/>
        </xsl:call-template>

        <xsl:text> </xsl:text>
        
        <xsl:call-template name="anchor">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="@name"/>
          <xsl:with-param name="higlhight" select="false()"/>
        </xsl:call-template>

        <xsl:text> { </xsl:text>
        <xsl:call-template name="type.enum.list.compact">
          <xsl:with-param name="indentation"
            select="$indentation + string-length(@name) + 8"/>
        </xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>;</xsl:text>
  </xsl:template>

  <!-- Enumeration reference at namespace level -->
  <xsl:template match="enum" mode="namespace-reference">
    <xsl:if test="para or description or not ($boost.compact.enum=1)">
      <xsl:call-template name="reference-documentation">
        <xsl:with-param name="name">
          <xsl:call-template name="type.display.name"/>
        </xsl:with-param>
        <xsl:with-param name="refname">
          <xsl:call-template name="fully-qualified-name">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
        <xsl:with-param name="anchor">
          <xsl:call-template name="generate.id">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="synopsis">
          <xsl:call-template name="type.enum.display"/>
        </xsl:with-param>
        <xsl:with-param name="text">
          <xsl:apply-templates select="para"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Output an enumeration along with its values -->
  <xsl:template name="type.enum.display">
    <!-- Spacing -->
    <xsl:if test="position() &gt; 1">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <xsl:text>&#10;</xsl:text>    

    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="'enum'"/>
    </xsl:call-template>

    <!-- Header -->
    <xsl:variable name="header" select="concat(' ', @name, ' { ')"/>
    <xsl:value-of select="$header"/>

    <!-- Print the enumeration values -->
    <xsl:call-template name="type.enum.list.compact">
      <xsl:with-param name="indentation" select="4 + string-length($header)"/>
    </xsl:call-template>

    <xsl:text> };</xsl:text>    
  </xsl:template>

  <!-- List enumeration values in a compact form e.g.,
       enum Name { value1 = foo, value2 = bar, ... }; 
       This routine prints only the enumeration values; the caller is 
       responsible for printing everything outside the braces
       (inclusive). -->
  <xsl:template name="type.enum.list.compact">
    <xsl:param name="indentation"/>

    <!-- Internal: The column we are on -->
    <xsl:param name="column" select="$indentation"/>

    <!-- Internal: The list of enumeration values -->
    <xsl:param name="values" select="enumvalue"/>

    <!-- Internal: a prefix that we need to print prior to printing
         this value. -->
    <xsl:param name="prefix" select="''"/>

    <xsl:if test="$values">
      <xsl:variable name="value" select="$values[position()=1]"/>
      <xsl:variable name="rest" select="$values[position() &gt; 1]"/>

      <!-- Compute the string to be printed for this value -->
      <xsl:variable name="result">
        <xsl:value-of select="$prefix"/>
        <xsl:value-of select="$value/attribute::name"/>

        <xsl:if test="$values/attribute">
          <xsl:text> = </xsl:text>
          <xsl:value-of select="string($values/attribute)"/>
        </xsl:if>
      </xsl:variable>

      <!-- The column we will end on, assuming that this value fits on
           this line -->
      <xsl:variable name="end" select="$column + string-length($result)"/>

      <xsl:choose>
        <!-- If the enumeration value fits on this line, put it there -->
        <xsl:when test="$end &lt; $max-columns">
          <xsl:value-of select="$prefix"/>
          <xsl:value-of select="$value/attribute::name"/>
          <xsl:if test="$value/default">
            <xsl:text> = </xsl:text>
            <xsl:apply-templates 
              select="$value/default/*|$value/default/text()"/>
          </xsl:if>

          <xsl:call-template name="type.enum.list.compact">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="column" select="$end"/>
            <xsl:with-param name="values" select="$rest"/>
            <xsl:with-param name="prefix" select="', '"/>
          </xsl:call-template>
        </xsl:when>
        <!-- This enumeration value doesn't fit on this line, so print
             it on the next line -->
        <xsl:otherwise>
          <xsl:value-of select="$prefix"/>
          <xsl:text>&#10;</xsl:text>

          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>

          <xsl:value-of select="$value/attribute::name"/>
          <xsl:if test="$value/default">
            <xsl:text> = </xsl:text>
            <xsl:apply-templates 
              select="$value/default/*|$value/default/text()"/>
          </xsl:if>

          <xsl:variable name="end2" select="$indentation 
                                           + string-length($result) 
                                           - string-length($prefix)"/>

          <xsl:call-template name="type.enum.list.compact">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="column" select="$end2"/>
            <xsl:with-param name="values" select="$rest"/>
            <xsl:with-param name="prefix" select="', '"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
