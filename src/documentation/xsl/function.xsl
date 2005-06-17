<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:strip-space elements="requires effects postconditions returns throws 
                             complexity notes rationale purpose"/>

  <!-- When true, the stylesheet will emit compact definitions of
       functions when the function does not have any detailed
       description. -->
  <xsl:param name="boost.compact.function">1</xsl:param>

  <!-- BoostBook documentation mode. Can be "compact" or
       "standardese", for now -->
  <xsl:param name="boost.generation.mode">compact</xsl:param>

  <!-- The longest type length that is considered "short" for the
       layout of function return types. When the length of the result type
       and any storage specifiers is greater than this length, they will be
       placed on a separate line from the function name and parameters
       unless everything fits on a single line. -->
  <xsl:param name="boost.short.result.type">12</xsl:param>

  <!-- Display a function declaration -->
  <xsl:template name="function">
    <xsl:param name="indentation"/>
    <xsl:param name="is-reference"/>

    <!-- Whether or not we should include parameter names in the output -->
    <xsl:param name="include-names" select="$is-reference"/>

    <!-- What type of link the function name should have. This shall
         be one of 'anchor' (the function output will be the target of
         links), 'link' (the function output will link to a definition), or
         'none' (the function output will not be either a link or a link
         target) -->
    <xsl:param name="link-type">
      <xsl:choose>
        <xsl:when test="$is-reference">
          <xsl:text>anchor</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>link</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:param>

    <!-- The id we should link to or anchor as -->
    <xsl:param name="link-to">
      <xsl:call-template name="generate.id"/>
    </xsl:param>

    <!-- If we are printing a constructor -->
    <xsl:param name="constructor-for"/>

    <!-- If we are printing a destructor -->
    <xsl:param name="destructor-for"/>

    <!-- If we are printing a copy assignment operator -->
    <xsl:param name="copy-assign-for"/>

    <!-- The name of this function -->
    <xsl:param name="name" select="@name"/>

    <!-- True if this is the function's separate documentation -->
    <xsl:param name="standalone" select="false()"/>

    <!-- True if we should suppress the template header -->
    <xsl:param name="suppress-template" select="false()"/>

    <!-- Calculate the specifiers -->
    <xsl:variable name="specifiers">
      <xsl:if test="@specifiers">
        <xsl:value-of select="concat(@specifiers, ' ')"/>
      </xsl:if>
    </xsl:variable>

    <!-- Calculate the type -->
    <xsl:variable name="type">
      <xsl:value-of select="$specifiers"/>

      <xsl:choose>
        <!-- Conversion operators have an empty type, because the return
             type is part of the name -->
        <xsl:when test="$name='conversion-operator'"/>

        <!-- Constructors and destructors have no return type -->
        <xsl:when test="$constructor-for or $destructor-for"/>

        <!-- Copy assignment operators return a reference to the class
             they are in, unless another type has been explicitly
             provided in the element. -->
        <xsl:when test="$copy-assign-for and not(type)">
          <xsl:value-of select="concat($copy-assign-for, '&amp; ')"/>
        </xsl:when>

        <xsl:otherwise>
          <xsl:apply-templates select="type" mode="annotation"/>
          <xsl:text> </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Build the function name with return type -->
    <xsl:variable name="function-name">
      <xsl:choose>
        <xsl:when test="$constructor-for">
          <xsl:value-of select="$constructor-for"/>
        </xsl:when>
        <xsl:when test="$destructor-for">
          <xsl:value-of select="concat('~',$destructor-for)"/>
        </xsl:when>
        <xsl:when test="$copy-assign-for">
          <xsl:value-of select="'operator='"/>
        </xsl:when>
        <xsl:when test="$name='conversion-operator'">
          <xsl:text>operator </xsl:text>
          <xsl:apply-templates select="type" mode="annotation"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$name"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="not ($standalone) or 
                  (local-name(.)='signature' and (position() &gt; 1))
                  or $suppress-template">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <!-- Indent this declaration -->
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    
    <!-- Build the template header -->
    <xsl:variable name="template-length">
      <xsl:choose>
        <xsl:when test="$suppress-template">
          0
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="template.synopsis.length"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
        
    <!-- Build a full parameter string (without line breaks) -->
    <xsl:variable name="param-string">
      <xsl:text>(</xsl:text>
      <xsl:call-template name="function-parameters">
        <xsl:with-param name="include-names" select="$include-names"/>
        <xsl:with-param name="wrap" select="false()"/>
      </xsl:call-template>
      <xsl:text>)</xsl:text>
    </xsl:variable>

    <!-- Build the text that follows the declarator-->
    <xsl:variable name="postdeclarator">
      <xsl:if test="@cv">
        <xsl:text> </xsl:text>
        <xsl:value-of select="@cv"/>
      </xsl:if>
    </xsl:variable>

    <!-- Build the full declaration text -->
    <xsl:variable name="decl-string" 
      select="concat($type, $function-name, $param-string, $postdeclarator)"/>
    <xsl:variable name="end-column" 
      select="$template-length + string-length($decl-string) + $indentation"/>
    
    <xsl:choose>
      <!-- Check if we should put the template header on its own line to
           save horizontal space. -->
      <xsl:when test="($template-length &gt; 0) and 
                      ($end-column &gt; $max-columns)">
        <!-- Emit template header on its own line -->
        <xsl:apply-templates select="template" mode="synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:apply-templates>
        
        <!-- Emit the rest of the function declaration (without the
             template header) indented two extra spaces. -->
        <xsl:call-template name="function">
          <xsl:with-param name="indentation" select="$indentation + 2"/>
          <xsl:with-param name="is-reference" select="$is-reference"/>
          <xsl:with-param name="include-names" select="$include-names"/>
          <xsl:with-param name="link-type" select="$link-type"/>
          <xsl:with-param name="link-to" select="$link-to"/>
          <xsl:with-param name="constructor-for" select="$constructor-for"/>
          <xsl:with-param name="destructor-for" select="$destructor-for"/>
          <xsl:with-param name="copy-assign-for" select="$copy-assign-for"/>
          <xsl:with-param name="name" select="$name"/>
          <xsl:with-param name="standalone" select="$standalone"/>
          <xsl:with-param name="suppress-template" select="true()"/>
        </xsl:call-template>
      </xsl:when>

      <!-- Check if we can put the entire declaration on a single
           line. -->
      <xsl:when test="not($end-column &gt; $max-columns)">
        <!-- Emit template header, if not suppressed -->
        <xsl:if test="not($suppress-template)">
          <xsl:apply-templates select="template" mode="synopsis">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="wrap" select="false()"/>
            <xsl:with-param name="highlight" select="true()"/>
          </xsl:apply-templates>
        </xsl:if>

        <!-- Emit specifiers -->
        <xsl:call-template name="source-highlight">
          <xsl:with-param name="text" select="$specifiers"/>
        </xsl:call-template>

        <!-- Emit type, if any -->
        <xsl:choose>
          <!-- Conversion operators have an empty type, because the return
               type is part of the name -->
          <xsl:when test="$name='conversion-operator'"/>

          <!-- Constructors and destructors have no return type -->
          <xsl:when test="$constructor-for or $destructor-for"/>

          <!-- Copy assignment operators return a reference to the class
               they are in, unless another type has been explicitly
               provided in the element. -->
          <xsl:when test="$copy-assign-for and not(type)">
            <xsl:value-of select="concat($copy-assign-for, '&amp; ')"/>
          </xsl:when>

          <xsl:otherwise>
            <xsl:apply-templates select="type" mode="highlight"/>
            <xsl:text> </xsl:text>
          </xsl:otherwise>
        </xsl:choose>

        <xsl:call-template name="link-or-anchor">
          <xsl:with-param name="to" select="$link-to"/>
          <xsl:with-param name="text" select="$function-name"/>
          <xsl:with-param name="link-type" select="$link-type"/>
          <xsl:with-param name="highlight" select="true()"/>
        </xsl:call-template>

        <xsl:text>(</xsl:text>
        <xsl:call-template name="function-parameters">
          <xsl:with-param name="include-names" select="$include-names"/>
          <xsl:with-param name="indentation" 
            select="$indentation + $template-length + string-length($type)
                    + string-length($function-name) + 1"/>
          <xsl:with-param name="final" select="true()"/>
        </xsl:call-template>                
        <xsl:text>)</xsl:text>

        <xsl:call-template name="source-highlight">
          <xsl:with-param name="text" select="$postdeclarator"/>
        </xsl:call-template>
        <xsl:text>;</xsl:text>    
      </xsl:when>

      <!-- This declaration will take multiple lines -->
      <xsl:otherwise>
        <!-- Emit specifiers -->
        <xsl:call-template name="source-highlight">
          <xsl:with-param name="text" select="$specifiers"/>
        </xsl:call-template>

        <!-- Emit type, if any -->
        <xsl:choose>
          <!-- Conversion operators have an empty type, because the return
               type is part of the name -->
          <xsl:when test="$name='conversion-operator'"/>

          <!-- Constructors and destructors have no return type -->
          <xsl:when test="$constructor-for or $destructor-for"/>

          <!-- Copy assignment operators return a reference to the class
               they are in, unless another type has been explicitly
               provided in the element. -->
          <xsl:when test="$copy-assign-for and not(type)">
            <xsl:value-of select="concat($copy-assign-for, '&amp; ')"/>
          </xsl:when>

          <xsl:otherwise>
            <xsl:apply-templates select="type" mode="highlight"/>
            <xsl:text> </xsl:text>
          </xsl:otherwise>
        </xsl:choose>

        <xsl:if test="string-length($type) &gt; $boost.short.result.type">
          <xsl:text>&#10;</xsl:text>
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
        </xsl:if>

        <!-- Determine how many columns the type and storage
             specifiers take on the same line as the function name. -->
        <xsl:variable name="type-length">
          <xsl:choose>
            <xsl:when test="string-length($type) &gt; $boost.short.result.type">
              0
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="string-length($type)"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:call-template name="link-or-anchor">
          <xsl:with-param name="to" select="$link-to"/>
          <xsl:with-param name="text" select="$function-name"/>
          <xsl:with-param name="link-type" select="$link-type"/>
          <xsl:with-param name="highlight" select="true()"/>
        </xsl:call-template>
        <xsl:text>(</xsl:text>
        <xsl:call-template name="function-parameters">
          <xsl:with-param name="include-names" select="$include-names"/>
          <xsl:with-param name="indentation" 
            select="$indentation + $type-length 
                    + string-length($function-name) + 1"/>
          <xsl:with-param name="final" select="true()"/>
        </xsl:call-template>                
        <xsl:text>)</xsl:text>
        <xsl:call-template name="source-highlight">
          <xsl:with-param name="text" select="$postdeclarator"/>
        </xsl:call-template>
        <xsl:text>;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>    

  <!-- Synopsis of function parameters, e.g., "(const T&, int x = 5)" -->
  <xsl:template name="function-parameters">
    <!-- Indentation level of this parameter list -->
    <xsl:param name="indentation"/>

    <!-- True if we should include parameter names -->
    <xsl:param name="include-names" select="true()"/>

    <!-- True if we should wrap function parameters to the next line -->
    <xsl:param name="wrap" select="true()"/>

    <!-- True if we are printing the final output -->
    <xsl:param name="final" select="false()"/>

    <!-- Internal: The prefix to emit before a parameter -->
    <xsl:param name="prefix" select="''"/>

    <!-- Internal: The list of parameters -->
    <xsl:param name="parameters" select="parameter"/>

    <!-- Internal: The column we are on -->
    <xsl:param name="column" select="$indentation"/>

    <!-- Internal: Whether this is the first parameter on this line or not -->
    <xsl:param name="first-on-line" select="true()"/>

    <xsl:if test="$parameters">
      <!-- Information for this parameter -->
      <xsl:variable name="parameter" select="$parameters[position()=1]"/>
      <xsl:variable name="name">
        <xsl:if test="$include-names">
          <xsl:text> </xsl:text><xsl:value-of select="$parameter/@name"/>
        </xsl:if>
      </xsl:variable>

      <xsl:variable name="type" select="string($parameter/paramtype)"/>

      <xsl:variable name="default">
        <xsl:choose>
          <xsl:when test="$parameter/@default">
            <xsl:text> = </xsl:text>
            <xsl:value-of select="$parameter/@default"/>
          </xsl:when>
          <xsl:when test="$parameter/default">
            <xsl:text> = </xsl:text>
            <xsl:choose>
              <xsl:when test="$final">
                <xsl:apply-templates
                  select="$parameter/default/*|$parameter/default/text()"
                  mode="annotation"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="string($parameter/default)"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="text" select="concat($type, $name, $default)"/>
      
      <xsl:variable name="end-column" 
        select="$column + string-length($prefix) + string-length($text)"/>

      <xsl:choose>
        <!-- Parameter goes on this line -->
        <xsl:when test="$first-on-line or ($end-column &lt; $max-columns) 
                        or not($wrap)">
          <xsl:choose>
            <xsl:when test="$final">
              <xsl:value-of select="$prefix"/>
              <xsl:apply-templates 
                select="$parameter/paramtype/*|$parameter/paramtype/text()"
                mode="annotation">
                <xsl:with-param name="highlight" select="true()"/>
              </xsl:apply-templates>
              <xsl:value-of select="$name"/>
              <xsl:copy-of select="$default"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($prefix, $text)"/>
            </xsl:otherwise>
          </xsl:choose>

          <xsl:call-template name="function-parameters">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="include-names" select="$include-names"/>
            <xsl:with-param name="wrap" select="$wrap"/>
            <xsl:with-param name="final" select="$final"/>
            <xsl:with-param name="parameters" 
              select="$parameters[position()!=1]"/>
            <xsl:with-param name="prefix" select="', '"/>
            <xsl:with-param name="column" select="$end-column"/>
            <xsl:with-param name="first-on-line" select="false()"/>
          </xsl:call-template>
        </xsl:when>
        <!-- Parameter goes on next line -->
        <xsl:otherwise>
          <!-- The comma goes on this line -->
          <xsl:value-of select="$prefix"/><xsl:text>&#10;</xsl:text>

          <!-- Indent and print the parameter -->
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
          <xsl:choose>
            <xsl:when test="$final">
              <xsl:apply-templates 
                select="$parameter/paramtype/*|$parameter/paramtype/text()"
                mode="annotation">
                <xsl:with-param name="highlight" select="true()"/>
              </xsl:apply-templates>
              <xsl:value-of select="$name"/>
              <xsl:value-of select="$default"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($prefix, $text)"/>
            </xsl:otherwise>
          </xsl:choose>

          <!-- Emit next parameter -->
          <xsl:call-template name="function-parameters">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="include-names" select="$include-names"/>
            <xsl:with-param name="wrap" select="$wrap"/>
            <xsl:with-param name="final" select="$final"/>
            <xsl:with-param name="parameters" 
              select="$parameters[position()!=1]"/>
            <xsl:with-param name="prefix" select="', '"/>
            <xsl:with-param name="column" 
              select="1 + string-length($text) + $indentation"/>
            <xsl:with-param name="first-on-line" select="false()"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- Function synopsis -->
  <xsl:template match="function|method" mode="synopsis">
    <xsl:param name="indentation"/>

    <!-- True if we should compact this function -->
    <xsl:variable name="compact"
      select="not (para|description|requires|effects|postconditions|returns|
                   throws|complexity|notes|rationale) and 
              ($boost.compact.function='1') and
              not (local-name(.)='method')"/>

    <xsl:choose>
      <xsl:when test="$compact">
        <xsl:if test="purpose">
          <!-- Compact display outputs the purpose as a comment (if
               there is one) and the entire function declaration. -->
          <xsl:text>&#10;&#10;</xsl:text>
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>

          <xsl:call-template name="highlight-comment">
            <xsl:with-param name="text">
              <xsl:text>// </xsl:text>
              <xsl:apply-templates select="purpose/*|purpose/text()"
                mode="purpose"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:if>

        <xsl:call-template name="function">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="is-reference" select="true()"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="function">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="is-reference" select="false()"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="overloaded-function|overloaded-method" mode="synopsis">
    <xsl:param name="indentation"/>

    <xsl:variable name="name" select="@name"/>

    <!-- True if we should compact this function -->
    <xsl:variable name="compact"
      select="not (para|description|requires|effects|postconditions|returns|
                   throws|complexity|notes|rationale) and 
              ($boost.compact.function='1') and
              not (local-name(.)='overloaded-method')"/>

    <xsl:choose>
      <xsl:when test="$compact">
        <xsl:if test="purpose">
          <!-- Compact display outputs the purpose as a comment (if
               there is one) and the entire function declaration. -->
          <xsl:text>&#10;</xsl:text>
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
          
          <xsl:call-template name="highlight-comment">
            <xsl:with-param name="text">
              <xsl:text>// </xsl:text>
              <xsl:apply-templates select="purpose" mode="annotation"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:if>

        <xsl:for-each select="signature">
          <xsl:call-template name="function">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="is-reference" select="true()"/>
            <xsl:with-param name="name" select="$name"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="local-name(..)='namespace'">
        <xsl:variable name="link-to">
          <xsl:call-template name="generate.id"/>
        </xsl:variable>

        <xsl:for-each select="signature">
          <xsl:call-template name="function">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="is-reference" select="false()"/>
            <xsl:with-param name="name" select="$name"/>
            <xsl:with-param name="link-to" select="$link-to"/>
          </xsl:call-template>
        </xsl:for-each>        
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="signature">
          <xsl:call-template name="function">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="is-reference" select="false()"/>
            <xsl:with-param name="name" select="$name"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Group free functions together under a category name (header synopsis)-->
  <xsl:template match="free-function-group" mode="header-synopsis">
    <xsl:param name="class"/>
    <xsl:param name="indentation"/>
    <xsl:apply-templates select="function|overloaded-function" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Constructors, destructor, and assignment operators -->
  <xsl:template name="construct-copy-destruct-synopsis">
    <xsl:param name="indentation"/>
    <xsl:if test="constructor|copy-assignment|destructor">
      <xsl:if test="typedef|static-constant">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
      <xsl:text>&#10;</xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation"/>      
      </xsl:call-template>
      <emphasis>
        <xsl:text>// </xsl:text>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id"/>
            <xsl:text>construct-copy-destruct</xsl:text>
          </xsl:with-param>
          <xsl:with-param name="text" select="'construct/copy/destruct'"/>
        </xsl:call-template>
      </emphasis>
      <xsl:apply-templates select="constructor" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:apply-templates>
      <xsl:apply-templates select="copy-assignment" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:apply-templates>
      <xsl:apply-templates select="destructor" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:apply-templates>
    </xsl:if>
  </xsl:template>

  <xsl:template name="construct-copy-destruct-reference">
    <xsl:if test="constructor|copy-assignment|destructor">
      <xsl:call-template name="member-documentation">
        <xsl:with-param name="name">
          <xsl:call-template name="anchor">
            <xsl:with-param name="to">
              <xsl:call-template name="generate.id"/>
              <xsl:text>construct-copy-destruct</xsl:text>
            </xsl:with-param>
            <xsl:with-param name="text" select="''"/>
          </xsl:call-template>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text" select="@name"/>
          </xsl:call-template>
          <xsl:text> construct/copy/destruct</xsl:text>
        </xsl:with-param>
        <xsl:with-param name="text">
          <orderedlist>
            <xsl:apply-templates select="constructor" mode="reference"/>
            <xsl:apply-templates select="copy-assignment" mode="reference"/>
            <xsl:apply-templates select="destructor" mode="reference"/>
          </orderedlist>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="constructor" mode="reference">
    <xsl:call-template name="function.documentation">
      <xsl:with-param name="text">
        <para>
          <xsl:call-template name="preformatted">
            <xsl:with-param name="text">
              <xsl:call-template name="function">
                <xsl:with-param name="indentation" select="0"/>
                <xsl:with-param name="is-reference" select="true()"/>
                <xsl:with-param name="constructor-for" select="../@name"/>
                <xsl:with-param name="standalone" select="true()"/>
              </xsl:call-template>
            </xsl:with-param>
          </xsl:call-template>
        </para>
        <xsl:call-template name="function-requirements"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="copy-assignment" mode="reference">
    <xsl:call-template name="function.documentation">
      <xsl:with-param name="text">
        <para>
          <xsl:call-template name="preformatted">
            <xsl:with-param name="text">
              <xsl:call-template name="function">
                <xsl:with-param name="indentation" select="0"/>
                <xsl:with-param name="is-reference" select="true()"/>
                <xsl:with-param name="copy-assign-for" select="../@name"/>
                <xsl:with-param name="standalone" select="true()"/>
              </xsl:call-template>
            </xsl:with-param>
          </xsl:call-template>
        </para>
        <xsl:call-template name="function-requirements"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="destructor" mode="reference">
    <xsl:call-template name="function.documentation">
      <xsl:with-param name="text">
        <para>
          <xsl:call-template name="preformatted">
            <xsl:with-param name="text">
              <xsl:call-template name="function">
                <xsl:with-param name="indentation" select="0"/>
                <xsl:with-param name="is-reference" select="true()"/>
                <xsl:with-param name="destructor-for" select="../@name"/>
                <xsl:with-param name="standalone" select="true()"/>
              </xsl:call-template>
            </xsl:with-param>
          </xsl:call-template>
        </para>
        <xsl:call-template name="function-requirements"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Templates for functions -->
  <xsl:template name="function.documentation">
    <xsl:param name="text"/>

    <xsl:choose>
      <xsl:when test="$boost.generation.mode='compact'">
        <xsl:call-template name="function.documentation.compact">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$boost.generation.mode='standardese'">
        <xsl:call-template name="function.documentation.standardese">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>Error: invalid value '</xsl:text>
        <xsl:value-of select="$boost.generation.mode"/>
        <xsl:text>' for stylesheet parameter boost.generation.mode.</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="function.documentation.compact">
    <xsl:param name="text"/>
    
    <xsl:choose>
      <xsl:when test="count(ancestor::free-function-group) &gt; 0
                      or count(ancestor::method-group) &gt; 0
                      or local-name(.)='constructor'
                      or local-name(.)='copy-assignment'
                      or local-name(.)='destructor'">
        <listitem><xsl:copy-of select="$text"/></listitem>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="function.documentation.standardese">
    <xsl:param name="text"/>
    <!--
    <formalpara>
      <title>
        <xsl:call-template name="fully-qualified-name">
          <xsl:with-param name="node" select="."/>
        </xsl:call-template>
      </title>
      <xsl:copy-of select="$text"/>
    </formalpara>
-->
    <listitem><xsl:copy-of select="$text"/></listitem>
  </xsl:template>

  <!-- Semantic descriptions of functions -->
  <xsl:template name="function-requirements">
    <xsl:param name="namespace-reference" select="false()"/>

    <xsl:if test="$namespace-reference=false()">
      <xsl:apply-templates select="purpose/*"/>
    </xsl:if>

    <xsl:apply-templates select="description/*"/>

    <!-- Document parameters -->
    <xsl:if test="parameter/description|signature/parameter/description">
      <variablelist spacing="compact">
        <title>Parameters</title>
        <xsl:for-each select="parameter|signature/parameter">
		  <xsl:sort select="attribute::name"/>
          <xsl:if test="description">
            <varlistentry>
              <term><xsl:value-of select="@name"/></term>
              <listitem>
                <xsl:apply-templates select="description/*"/>
              </listitem>
            </varlistentry>
          </xsl:if>
        </xsl:for-each>
      </variablelist>
    </xsl:if>

    <xsl:if test="para">
      <xsl:message>
        <xsl:text>Warning: Use of 'para' elements in a function is deprecated.&#10;</xsl:text>
        <xsl:text>  Wrap them in a 'description' element.</xsl:text>
      </xsl:message>
      <xsl:call-template name="print.warning.context"/>
      <xsl:apply-templates select="para"/>
    </xsl:if>

    <xsl:if test="requires|effects|postconditions|returns|throws|complexity|
                  notes|rationale">
      <xsl:choose>
        <xsl:when test="$boost.generation.mode='compact'">
          <xsl:call-template name="function.requirements.compact"/>
        </xsl:when>
        <xsl:when test="$boost.generation.mode='standardese'">
          <xsl:call-template name="function.requirements.standardese"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>Error: invalid value '</xsl:text>
          <xsl:value-of select="$boost.generation.mode"/>
          <xsl:text>' for stylesheet parameter boost.generation.mode.</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <xsl:template name="function.requirement.name">
    <xsl:param name="node" select="."/>
    <xsl:choose>
      <xsl:when test="local-name($node)='requires'">Requires</xsl:when>
      <xsl:when test="local-name($node)='effects'">Effects</xsl:when>
      <xsl:when test="local-name($node)='postconditions'">
        <xsl:text>Postconditions</xsl:text>
      </xsl:when>
      <xsl:when test="local-name($node)='returns'">Returns</xsl:when>
      <xsl:when test="local-name($node)='throws'">Throws</xsl:when>
      <xsl:when test="local-name($node)='complexity'">Complexity</xsl:when>
      <xsl:when test="local-name($node)='notes'">Notes</xsl:when>
      <xsl:when test="local-name($node)='rationale'">Rationale</xsl:when>
      <xsl:otherwise>
        <xsl:message>
          <xsl:text>Error: unhandled node type `</xsl:text>
          <xsl:value-of select="local-name($node)"/>
          <xsl:text>' in template function.requirement.name.</xsl:text>
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The "compact" function requirements mode uses variable lists -->
  <xsl:template name="function.requirements.compact">
    <variablelist spacing="boost">
      <xsl:apply-templates 
        select="requires|effects|postconditions|returns|throws|complexity|
                notes|rationale"
        mode="function.requirements.compact"/>
    </variablelist>
  </xsl:template>

  <xsl:template match="*" mode="function.requirements.compact">
    <varlistentry>
      <term><xsl:call-template name="function.requirement.name"/></term>
      <listitem>
        <xsl:apply-templates select="./*|./text()" mode="annotation"/>
      </listitem>
    </varlistentry>
  </xsl:template>

  <!-- The "standardese" function requirements mode uses ordered lists -->
  <xsl:template name="function.requirements.standardese">
    <orderedlist spacing="compact" numeration="arabic">
      <xsl:apply-templates 
        select="requires|effects|postconditions|returns|throws|complexity|
                notes|rationale"
        mode="function.requirements.standardese"/>
    </orderedlist>
  </xsl:template>

  <xsl:template match="*" mode="function.requirements.standardese">
    <listitem>
      <xsl:apply-templates select="./*|./text()" 
        mode="function.requirements.injection">
        <xsl:with-param name="requirement">
          <xsl:call-template name="function.requirement.name"/>
        </xsl:with-param>
      </xsl:apply-templates>
    </listitem>
  </xsl:template>

  <!-- Handle paragraphs in function requirement clauses -->
  <xsl:template match="para|simpara" mode="function.requirements.injection">
    <xsl:param name="requirement"/>

    <xsl:element name="{local-name(.)}">
      <xsl:for-each select="./@*">
        <xsl:attribute name="{name(.)}">
          <xsl:value-of select="."/>
        </xsl:attribute>
      </xsl:for-each>

      <xsl:if test="position()=1">
        <emphasis role="bold">
          <xsl:copy-of select="$requirement"/>
          <xsl:text>:&#160;</xsl:text>
        </emphasis>
      </xsl:if>
      <xsl:apply-templates/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="text()" mode="function.requirements.injection">
    <xsl:param name="requirement"/>

    <simpara>
      <xsl:if test="position()=1">
        <emphasis role="bold">
          <xsl:copy-of select="$requirement"/>
          <xsl:text>:&#160;</xsl:text>
        </emphasis>
      </xsl:if>
      <xsl:copy-of select="."/>
      <xsl:apply-templates/>
    </simpara>
  </xsl:template>

  <!-- Function reference -->
  <xsl:template match="function|method" mode="reference">
    <!-- True if we should compact this function -->
    <xsl:variable name="compact"
      select="not (para|description|requires|effects|postconditions|returns|
                   throws|complexity|notes|rationale) and 
              ($boost.compact.function='1') and
              not (local-name(.)='method')"/>

    <xsl:if test="not ($compact)">
      <xsl:call-template name="function.documentation">
        <xsl:with-param name="text">
          <para>
            <xsl:call-template name="preformatted">
              <xsl:with-param name="text">
                <xsl:call-template name="function">
                  <xsl:with-param name="indentation" select="0"/>
                  <xsl:with-param name="is-reference" select="true()"/>
                  <xsl:with-param name="link-type" select="'anchor'"/>
                  <xsl:with-param name="standalone" select="true()"/>
                </xsl:call-template>
              </xsl:with-param>
            </xsl:call-template>
          </para>
          <xsl:call-template name="function-requirements"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Reference for functions at namespace level -->
  <xsl:template match="function" mode="namespace-reference">
    <!-- True if we should compact this function -->
    <xsl:variable name="compact"
      select="not (para|description|requires|effects|postconditions|returns|
                   throws|complexity|notes|rationale) and 
              ($boost.compact.function='1')"/>

    <xsl:if test="not ($compact)">
      <xsl:call-template name="reference-documentation">
        <xsl:with-param name="name">
          <xsl:text>Function </xsl:text>
          <xsl:if test="template">
            <xsl:text>template </xsl:text>
          </xsl:if>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text" select="@name"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="refname">
          <xsl:call-template name="fully-qualified-name">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
        <xsl:with-param name="anchor">
          <xsl:call-template name="generate.id"/>
        </xsl:with-param>
        <xsl:with-param name="synopsis">
          <xsl:call-template name="function">
            <xsl:with-param name="indentation" select="0"/>
            <xsl:with-param name="is-reference" select="true()"/>
            <xsl:with-param name="link-type" select="'none'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="text">
          <xsl:call-template name="function-requirements">
            <xsl:with-param name="namespace-reference" select="true()"/>
          </xsl:call-template>
        </xsl:with-param>
      </xsl:call-template> 
    </xsl:if>   
  </xsl:template>

  <xsl:template match="overloaded-function" mode="reference">
    <xsl:variable name="name" select="@name"/>

    <!-- True if we should compact this function -->
    <xsl:variable name="compact"
      select="not (para|description|requires|effects|postconditions|returns|
                   throws|complexity|notes|rationale) and 
              ($boost.compact.function='1')"/>
    
    <xsl:if test="not ($compact)">
      <xsl:call-template name="function.documentation">
        <xsl:with-param name="text">
          <para>
            <xsl:attribute name="id">
              <xsl:call-template name="generate.id"/>
            </xsl:attribute>
            
            <xsl:call-template name="preformatted">
              <xsl:with-param name="text">
                <xsl:for-each select="signature">
                  <xsl:call-template name="function">
                    <xsl:with-param name="indentation" select="0"/>
                    <xsl:with-param name="is-reference" select="true()"/>
                    <xsl:with-param name="name" select="$name"/>
                    <xsl:with-param name="standalone" select="true()"/>
                  </xsl:call-template>
                </xsl:for-each>
              </xsl:with-param>
            </xsl:call-template>
          </para>
          <xsl:call-template name="function-requirements"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="overloaded-function" mode="namespace-reference">
    <!-- True if we should compact this function -->
    <xsl:variable name="compact"
      select="not (para|description|requires|effects|postconditions|returns|
                   throws|complexity|notes|rationale) and 
              ($boost.compact.function='1')"/>

    <xsl:variable name="name" select="@name"/>

    <xsl:if test="not ($compact)">
      <xsl:call-template name="reference-documentation">
        <xsl:with-param name="name">
          <xsl:text>Function </xsl:text>
          <xsl:if test="template">
            <xsl:text>template </xsl:text>
          </xsl:if>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text" select="@name"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="refname">
          <xsl:call-template name="fully-qualified-name">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
        <xsl:with-param name="anchor">
          <xsl:call-template name="generate.id"/>
        </xsl:with-param>
        <xsl:with-param name="synopsis">
          <xsl:for-each select="signature">
            <xsl:call-template name="function">
              <xsl:with-param name="indentation" select="0"/>
              <xsl:with-param name="is-reference" select="true()"/>
              <xsl:with-param name="link-type" select="'none'"/>
              <xsl:with-param name="name" select="$name"/>
            </xsl:call-template>
          </xsl:for-each>
        </xsl:with-param>
        <xsl:with-param name="text">
          <xsl:call-template name="function-requirements">
            <xsl:with-param name="namespace-reference" select="true()"/>
          </xsl:call-template>
        </xsl:with-param>
      </xsl:call-template> 
    </xsl:if>   
  </xsl:template>

  <xsl:template match="overloaded-method" mode="reference">
    <xsl:variable name="name" select="@name"/>

    <xsl:call-template name="function.documentation">
      <xsl:with-param name="text">
        <para>
          <xsl:attribute name="id">
            <xsl:call-template name="generate.id"/>
          </xsl:attribute>
          
          <xsl:call-template name="preformatted">
            <xsl:with-param name="text">
              <xsl:for-each select="signature">
                <xsl:call-template name="function">
                  <xsl:with-param name="indentation" select="0"/>
                  <xsl:with-param name="is-reference" select="true()"/>
                  <xsl:with-param name="name" select="$name"/>
                  <xsl:with-param name="standalone" select="true()"/>
                </xsl:call-template>
              </xsl:for-each>
            </xsl:with-param>
          </xsl:call-template>
        </para>
        <xsl:call-template name="function-requirements"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Group member functions together under a category name (synopsis)-->
  <xsl:template match="method-group" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <emphasis>
      <xsl:text>// </xsl:text>
      <xsl:call-template name="internal-link">
        <xsl:with-param name="to">
          <xsl:call-template name="generate.id"/>
        </xsl:with-param>
        <xsl:with-param name="text" select="string(@name)"/>
      </xsl:call-template>
    </emphasis>
    <xsl:apply-templates select="method|overloaded-method" 
      mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Group member functions together under a category name (reference)-->
  <xsl:template match="method-group" mode="reference">
    <xsl:call-template name="member-documentation">
      <xsl:with-param name="name">
        <xsl:call-template name="anchor">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id"/>
          </xsl:with-param>
          <xsl:with-param name="text" select="''"/>
        </xsl:call-template>
        <xsl:call-template name="monospaced">
          <xsl:with-param name="text" select="../@name"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>
        <xsl:value-of select="@name"/>
      </xsl:with-param>
      <xsl:with-param name="text">
        <orderedlist>
          <xsl:apply-templates select="method|overloaded-method"
            mode="reference"/>
        </orderedlist>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template> 

  <!-- Group free functions together under a category name (synopsis)-->
  <xsl:template match="free-function-group" mode="synopsis">
    <xsl:param name="class"/>
    <xsl:param name="indentation"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <emphasis>
      <xsl:text>// </xsl:text>
      <xsl:call-template name="internal-link">
        <xsl:with-param name="to">
          <xsl:call-template name="generate.id"/>
        </xsl:with-param>
        <xsl:with-param name="text" select="string(@name)"/>
      </xsl:call-template>
    </emphasis>
    <xsl:apply-templates select="function|overloaded-function" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Group free functions together under a category name (reference)-->
  <xsl:template match="free-function-group" mode="reference">
    <xsl:param name="class"/>
    <xsl:call-template name="member-documentation">
      <xsl:with-param name="name">
        <xsl:call-template name="anchor">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id"/>
          </xsl:with-param>
          <xsl:with-param name="text" select="''"/>
        </xsl:call-template>
        <xsl:call-template name="monospaced">
          <xsl:with-param name="text" select="$class"/>
        </xsl:call-template>
        <xsl:value-of select="concat(' ',@name)"/>
      </xsl:with-param>
      <xsl:with-param name="text">
        <orderedlist>
          <xsl:apply-templates select="function|overloaded-function"
            mode="reference"/>
        </orderedlist>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>  
</xsl:stylesheet>
