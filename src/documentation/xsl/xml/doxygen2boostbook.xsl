<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:import href="../lookup.xsl"/>

  <!-- Set this parameter to a space-separated list of headers that
       will be included in the output (all others are ignored). If this
       parameter is omitted or left as the empty string, all headers will
       be output. -->
  <xsl:param name="boost.doxygen.headers" select="''"/>

  <!-- The common prefix to all headers -->
  <xsl:param name="boost.doxygen.header.prefix" select="'boost'"/>

  <!-- The text that Doxygen places in overloaded functions. Damn them
       for forcing us to compare TEXT just to figure out what's overloaded
       and what isn't. -->
  <xsl:param name="boost.doxygen.overload">
    This is an overloaded member function, provided for convenience. It differs from the above function only in what argument(s) it accepts.
  </xsl:param>

  <!-- The namespace used to identify code that should not be
       processed at all. -->
  <xsl:param name="boost.doxygen.detailns">detail</xsl:param>

  <!-- The substring used to identify unspecified types that we can't
       mask from within Doxygen. This is a hack (big surprise). -->
  <xsl:param name="boost.doxygen.detail"><xsl:value-of select="$boost.doxygen.detailns"/>::</xsl:param>

  <!-- The title that will be used for the BoostBook library reference emitted. 
       If left blank, BoostBook will assign a default title. -->
  <xsl:param name="boost.doxygen.reftitle" select="''"/>
  
  <xsl:output method="xml" indent="yes" standalone="yes"/>

  <xsl:key name="compounds-by-kind" match="compounddef" use="@kind"/>
  <xsl:key name="compounds-by-id" match="compounddef" use="@id"/>
  <xsl:key name="inner-classes" match="compounddef[not(attribute::kind='namespace') and not(attribute::kind='file')]/innerclass" use="@refid"/>

  <xsl:strip-space elements="briefdescription detaileddescription"/>

  <xsl:template match="/">
    <xsl:apply-templates select="doxygen"/>
  </xsl:template>

  <xsl:template match="doxygen">
    <library-reference>
          <xsl:if test="string($boost.doxygen.reftitle) != ''">
            <title><xsl:copy-of select="$boost.doxygen.reftitle"/></title>
          </xsl:if>
      <xsl:apply-templates select="key('compounds-by-kind', 'file')"/>
    </library-reference>
  </xsl:template>

  <xsl:template match="compounddef">
    <!-- The set of innernamespace nodes that limits our search -->
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <xsl:choose>
      <!-- If the string INTERNAL ONLY is in the description, don't
           emit this entity. This hack is necessary because Doxygen doesn't
           tell us what is \internal and what isn't. -->
      <xsl:when test="contains(detaileddescription/para, 'INTERNAL ONLY')"/>
      <xsl:when test="contains(briefdescription/para, 'INTERNAL ONLY')"/>

      <xsl:when test="@kind='file'">
        <xsl:call-template name="file"/>
      </xsl:when>
      <xsl:when test="@kind='namespace'">
        <xsl:call-template name="namespace">
          <xsl:with-param name="with-namespace-refs" 
            select="$with-namespace-refs"/>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@kind='class'">
        <xsl:call-template name="class">
          <xsl:with-param name="class-key" select="'class'"/>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@kind='struct'">
        <xsl:call-template name="class">
          <xsl:with-param name="class-key" select="'struct'"/>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@kind='union'">
        <xsl:call-template name="class">
          <xsl:with-param name="class-key" select="'union'"/>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
Cannot handle compounddef with kind=<xsl:value-of select="@kind"/>
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="namespace">
    <!-- The set of innernamespace nodes that limits our search -->
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <xsl:variable name="fullname" select="string(compoundname)"/>

    <xsl:if test="$with-namespace-refs[string(text())=$fullname]
                  and not(contains($fullname, $boost.doxygen.detailns))">
      <!-- Namespace without the prefix -->
      <xsl:variable name="rest">
        <xsl:call-template name="strip-qualifiers">
          <xsl:with-param name="name" select="compoundname"/>
        </xsl:call-template>
      </xsl:variable>
      
      <!-- Grab only the namespace name, not any further nested namespaces -->
      <xsl:variable name="name">
        <xsl:choose>
          <xsl:when 
            test="contains($rest, '::')">
            <xsl:value-of select="substring-before($rest, '::')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$rest"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      
      <namespace>
        <xsl:attribute name="name">
          <xsl:value-of select="$name"/>
        </xsl:attribute>
        
        <xsl:apply-templates>
          <xsl:with-param name="with-namespace-refs" 
            select="$with-namespace-refs"/>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:apply-templates>
      </namespace>
    </xsl:if>
  </xsl:template>

  <xsl:template name="class">
    <xsl:param name="class-key"/>
    <xsl:param name="in-file"/>
    <xsl:param name="with-namespace-refs"/>

    <xsl:if test="contains(string(location/attribute::file), 
                           concat('/', $in-file)) and
                  not (key('inner-classes', @id))">
      <!-- The short name of this class -->
      <xsl:variable name="name-with-spec">
        <xsl:call-template name="strip-qualifiers">
          <xsl:with-param name="name" select="compoundname"/>
        </xsl:call-template>
      </xsl:variable>
      
      <xsl:variable name="name">
        <xsl:choose>
          <xsl:when test="contains($name-with-spec, '&lt;')">
            <xsl:value-of select="substring-before($name-with-spec, '&lt;')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$name-with-spec"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="specialization">
        <xsl:if test="contains($name-with-spec, '&lt;')">
          <xsl:variable name="spec-with-gt" 
            select="substring-after($name-with-spec, '&lt;')"/>
          <xsl:value-of select="substring($spec-with-gt, 1, 
                                          string-length($spec-with-gt)-1)"/>
        </xsl:if>
      </xsl:variable>

      <xsl:variable name="actual-class-key">
        <xsl:value-of select="$class-key"/>
        <xsl:if test="string-length($specialization) &gt; 0">
          <xsl:text>-specialization</xsl:text>
        </xsl:if>
      </xsl:variable>

      <xsl:element name="{$actual-class-key}">
        <xsl:attribute name="name">
          <xsl:value-of select="$name"/>
        </xsl:attribute>
        
        <xsl:apply-templates select="templateparamlist" mode="template"/>

        <xsl:if test="string-length($specialization) &gt; 0">
          <specialization>
            <xsl:call-template name="specialization">
              <xsl:with-param name="specialization" select="$specialization"/>
            </xsl:call-template>
          </specialization>
        </xsl:if>

        <xsl:apply-templates select="basecompoundref" mode="inherit"/>

        <xsl:apply-templates select="briefdescription" mode="passthrough"/>
        <xsl:apply-templates select="detaileddescription" mode="passthrough"/>
        <xsl:apply-templates/>
      </xsl:element>
    </xsl:if>
  </xsl:template>

  <xsl:template name="enum">
    <xsl:param name="in-file"/>

    <xsl:if test="contains(string(location/attribute::file), 
                           concat('/', $in-file))">
      <xsl:variable name="name">
        <xsl:call-template name="strip-qualifiers">
          <xsl:with-param name="name" select="name"/>
        </xsl:call-template>
      </xsl:variable>
      
      <enum>
        <xsl:attribute name="name">
          <xsl:value-of select="$name"/>
        </xsl:attribute>
        
        <xsl:apply-templates select="enumvalue"/>
        
        <xsl:apply-templates select="briefdescription" mode="passthrough"/>
        <xsl:apply-templates select="detaileddescription" mode="passthrough"/>
      </enum>
    </xsl:if>
  </xsl:template>

  <xsl:template match="enumvalue">
    <enumvalue>
      <xsl:attribute name="name">
        <xsl:value-of select="name"/>
      </xsl:attribute>

      <xsl:if test="initializer">
        <default>
          <xsl:apply-templates select="initializer" mode="passthrough"/>
        </default>
      </xsl:if>
    </enumvalue>
  </xsl:template>

  <xsl:template name="doxygen.include.header.rec">
    <xsl:param name="name"/>
    <xsl:param name="header-list" select="$boost.doxygen.headers"/>

    <xsl:choose>
      <xsl:when test="contains($header-list, ' ')">
        <xsl:variable name="header" 
          select="substring-before($header-list, ' ')"/>
        <xsl:variable name="rest" select="substring-after($header-list, ' ')"/>

        <xsl:choose>
          <xsl:when test="$name=$header">
            <xsl:text>yes</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="doxygen.include.header.rec">
              <xsl:with-param name="name" select="$name"/>
              <xsl:with-param name="header-list" select="$rest"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$name=$header-list">
        <xsl:text>yes</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="doxygen.include.header">
    <xsl:param name="name"/>
    
    <xsl:if test="$boost.doxygen.headers=''">
      <xsl:text>yes</xsl:text>
    </xsl:if>
    <xsl:if test="not($boost.doxygen.headers='')">
      <xsl:call-template name="doxygen.include.header.rec">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="file">
    <xsl:variable name="include-header">
      <xsl:call-template name="doxygen.include.header">
        <xsl:with-param name="name" select="string(compoundname)"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$include-header='yes'">
      <header>
        <xsl:attribute name="name">
          <xsl:call-template name="shorten.header.name">
            <xsl:with-param name="header" select="location/attribute::file"/>
          </xsl:call-template>
        </xsl:attribute>
        
        <xsl:if test="briefdescription/*|detaileddescription/*">
          <xsl:apply-templates select="briefdescription/*" mode="passthrough"/>
          <xsl:apply-templates select="detaileddescription/*" mode="passthrough"/>
        </xsl:if>
        
        <xsl:apply-templates mode="toplevel">
          <xsl:with-param name="with-namespace-refs"
            select="innernamespace"/>
          <xsl:with-param name="in-file" select="string(compoundname)"/>
        </xsl:apply-templates>
      </header>
    </xsl:if>
  </xsl:template>

  <xsl:template name="shorten.header.name">
    <xsl:param name="header"/>

    <xsl:variable name="prefix">
      <xsl:value-of select="concat($boost.doxygen.header.prefix, '/')"/>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="contains($header, $prefix)">
        <xsl:variable name="rest" select="substring-after($header, $prefix)"/>
        <xsl:choose>
          <xsl:when test="contains($rest, $prefix)">
            <xsl:call-template name="shorten.header.name">
              <xsl:with-param name="header" select="$rest"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$prefix"/>
            <xsl:value-of select="$rest"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$header"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <xsl:template match="innernamespace">
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <xsl:apply-templates select="key('compounds-by-id', @refid)">
      <xsl:with-param name="with-namespace-refs"
        select="$with-namespace-refs"/>
      <xsl:with-param name="in-file" select="$in-file"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="innernamespace" mode="toplevel">
    <!-- The set of innernamespace nodes that limits our search -->
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <!-- The full name of the namespace we are referring to -->
    <xsl:variable name="fullname" 
      select="string(key('compounds-by-id', @refid)/compoundname)"/>

    <!-- Only pass on top-level namespaces -->
    <xsl:if test="not(contains($fullname, '::'))">
      <xsl:apply-templates select="key('compounds-by-id', @refid)">
        <xsl:with-param name="with-namespace-refs" 
          select="$with-namespace-refs"/>
        <xsl:with-param name="in-file" select="$in-file"/>
      </xsl:apply-templates>
    </xsl:if>
  </xsl:template>

  <xsl:template match="memberdef" mode="toplevel">
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <xsl:if test="@kind='define'">
      <macro>
        <xsl:attribute name="name">
          <xsl:value-of select="name/text()"/>
        </xsl:attribute>

        <xsl:if test="param">
          <xsl:attribute name="kind">
            <xsl:value-of select="'functionlike'"/>
          </xsl:attribute>
        </xsl:if>
        
        <xsl:for-each select="param">
          <macro-parameter>
            <xsl:attribute name="name">
              <xsl:value-of select="defname/text()"/>
            </xsl:attribute>
          </macro-parameter>
        </xsl:for-each>

        <xsl:apply-templates select="briefdescription" mode="passthrough"/>
        <xsl:apply-templates select="detaileddescription" mode="passthrough"/>
      </macro>
    </xsl:if>
  </xsl:template>

  <xsl:template match="innerclass" mode="toplevel">
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <xsl:apply-templates select="key('compounds-by-id', @refid)">
      <xsl:with-param name="with-namespace-refs" 
        select="$with-namespace-refs"/>
      <xsl:with-param name="in-file" select="$in-file"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="innerclass">
    <xsl:param name="with-namespace-refs"/>
    <xsl:param name="in-file"/>

    <xsl:apply-templates select="key('compounds-by-id', @refid)">
      <xsl:with-param name="with-namespace-refs" 
        select="$with-namespace-refs"/>
      <xsl:with-param name="in-file" select="$in-file"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Classes -->
  <xsl:template match="templateparamlist" mode="template">
    <template>
      <xsl:apply-templates mode="template"/>
    </template>
  </xsl:template>

  <xsl:template match="param" mode="template">
    <xsl:choose>
      <xsl:when test="string(type)='class' or string(type)='typename'">
        <template-type-parameter>
          <xsl:attribute name="name">
            <xsl:value-of select="normalize-space(string(declname))"/>
          </xsl:attribute>
          <xsl:if test="defval">
            <default>
              <xsl:apply-templates select="defval/*|defval/text()" 
                mode="passthrough"/>
            </default>
          </xsl:if>
        </template-type-parameter>
      </xsl:when>
      <xsl:otherwise>
        <template-nontype-parameter>
          <xsl:attribute name="name">
            <xsl:value-of select="normalize-space(string(declname))"/>
          </xsl:attribute>
          <type>
            <xsl:apply-templates select="type"/>
          </type>
          <xsl:if test="defval">
            <default>
              <xsl:apply-templates select="defval/*|defval/text()" 
                mode="passthrough"/>
            </default>
          </xsl:if>
        </template-nontype-parameter>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="templateparamlist"/>

  <!-- "Parse" a specialization from part of a name -->
  <xsl:template name="specialization">
    <xsl:param name="specialization"/>

    <xsl:choose>
      <xsl:when test="contains($specialization, ',')">
        <template-arg>
          <xsl:value-of 
            select="normalize-space(substring-before($specialization, ','))"/>
        </template-arg>
        <xsl:call-template name="specialization">
          <xsl:with-param name="specialization" 
            select="substring-after($specialization, ',')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <template-arg>
          <xsl:value-of select="normalize-space($specialization)"/>
        </template-arg>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Inheritance -->
  <xsl:template match="basecompoundref" mode="inherit">
    <inherit>
      <!-- Access specifier for inheritance -->
      <xsl:attribute name="access">
        <xsl:value-of select="@prot"/>
      </xsl:attribute>
      <!-- TBD: virtual vs. non-virtual inheritance -->

      <xsl:apply-templates mode="passthrough"/>
    </inherit>
  </xsl:template>

  <xsl:template match="basecompoundref"/>

  <!-- Skip over sections: they aren't very useful at all -->
  <xsl:template match="sectiondef">
    <xsl:param name="in-file" select="''"/>

    <xsl:choose>
      <xsl:when test="@kind='public-static-func'">
        <!-- TBD: pass on the fact that these are static functions -->
        <method-group name="public static functions">
          <xsl:apply-templates>
            <xsl:with-param name="in-section" select="true()"/>
          </xsl:apply-templates>
        </method-group>
      </xsl:when>
      <xsl:when test="@kind='protected-static-func'">
        <!-- TBD: pass on the fact that these are static functions -->
        <method-group name="protected static functions">
          <xsl:apply-templates>
            <xsl:with-param name="in-section" select="true()"/>
          </xsl:apply-templates>
        </method-group>
      </xsl:when>
      <xsl:when test="@kind='private-static-func'">
        <!-- TBD: pass on the fact that these are static functions -->
        <method-group name="private static functions">
          <xsl:apply-templates>
            <xsl:with-param name="in-section" select="true()"/>
          </xsl:apply-templates>
        </method-group>
      </xsl:when>
      <xsl:when test="@kind='public-func'">
        <method-group name="public member functions">
          <xsl:apply-templates>
            <xsl:with-param name="in-section" select="true()"/>
          </xsl:apply-templates>
        </method-group>
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:when test="@kind='protected-func'">
        <method-group name="protected member functions">
          <xsl:apply-templates>
            <xsl:with-param name="in-section" select="true()"/>
          </xsl:apply-templates>
        </method-group>
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:when test="@kind='private-func'">
        <method-group name="private member functions">
          <xsl:apply-templates>
            <xsl:with-param name="in-section" select="true()"/>
          </xsl:apply-templates>
        </method-group>
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:when test="@kind='public-static-attrib' or @kind='public-attrib'">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:when test="@kind='public-type'">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:when test="@kind='func'">
        <xsl:apply-templates>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:apply-templates>
      </xsl:when>
      <xsl:when test="@kind='typedef'">
        <xsl:apply-templates>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:apply-templates>
      </xsl:when>
      <xsl:when test="@kind='var'">
        <xsl:apply-templates>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:apply-templates>
      </xsl:when>
      <xsl:when test="@kind='enum'">
        <xsl:apply-templates>
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:apply-templates>
      </xsl:when>
      <xsl:when test="@kind='user-defined'">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
Cannot handle sectiondef with kind=<xsl:value-of select="@kind"/>      
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Handle member definitions -->
  <xsl:template match="memberdef">
    <!-- True when we're inside a section -->
    <xsl:param name="in-section" select="false()"/>
    <xsl:param name="in-file" select="''"/>

    <xsl:choose>
      <!-- If the string INTERNAL ONLY is in the description, don't
           emit this entity. This hack is necessary because Doxygen doesn't
           tell us what is \internal and what isn't. -->
      <xsl:when test="contains(detaileddescription/para, 'INTERNAL ONLY')"/>
      <xsl:when test="contains(briefdescription/para, 'INTERNAL ONLY')"/>

      <xsl:when test="@kind='typedef'">
        <xsl:call-template name="typedef">
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@kind='function'">
        <xsl:choose>
          <xsl:when test="ancestor::compounddef/attribute::kind='namespace'">
            <xsl:call-template name="function">
              <xsl:with-param name="in-file" select="$in-file"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <!-- We are in a class -->
            <!-- The name of the class we are in -->
            <xsl:variable name="in-class">
              <xsl:call-template name="strip-qualifiers">
                <xsl:with-param name="name" 
                  select="string(ancestor::compounddef/compoundname/text())"/>
              </xsl:call-template>
            </xsl:variable>
            
            <xsl:choose>
              <xsl:when test="string(name/text())=$in-class">
                <xsl:if test="not ($in-section)">
                  <xsl:call-template name="constructor"/>
                </xsl:if>
              </xsl:when>
              <xsl:when test="string(name/text())=concat('~',$in-class)">
                <xsl:if test="not ($in-section)">
                  <xsl:call-template name="destructor"/>
                </xsl:if>
              </xsl:when>
              <xsl:when test="string(name/text())='operator='">
                <xsl:if test="not ($in-section)">
                  <xsl:call-template name="copy-assignment"/>
                </xsl:if>
              </xsl:when>
              <xsl:when test="normalize-space(string(type))=''
                              and contains(name/text(), 'operator ')">
                <xsl:if test="$in-section">
                  <xsl:call-template name="conversion-operator"/>
                </xsl:if>
              </xsl:when>
              <xsl:otherwise>
                <xsl:if test="$in-section">
                  <xsl:call-template name="method"/>
                </xsl:if>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="@kind='enum'">
        <xsl:call-template name="enum">
          <xsl:with-param name="in-file" select="$in-file"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@kind='variable'">
        <xsl:call-template name="variable"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
Cannot handle memberdef element with kind=<xsl:value-of select="@kind"/>
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Display typedefs -->
  <xsl:template name="typedef">
    <xsl:param name="in-file" select="''"/>

    <xsl:if test="contains(string(location/attribute::file), $in-file)">
      <!-- TBD: Handle public/protected/private -->
      <typedef>
        <!-- Name of the type -->
        <xsl:attribute name="name">
          <xsl:value-of select="name/text()"/>
        </xsl:attribute>
        
        <xsl:apply-templates select="briefdescription" mode="passthrough"/>
        <xsl:apply-templates select="detaileddescription" mode="passthrough"/>
        
        <type><xsl:apply-templates select="type"/></type>
      </typedef>
    </xsl:if>
  </xsl:template>

  <!-- Handle function parameters -->
  <xsl:template match="param" mode="function">
    <parameter>
      <!-- Parameter name -->
      <xsl:attribute name="name">
        <xsl:value-of select="normalize-space(declname/text())"/>
      </xsl:attribute>

      <!-- Parameter type -->
      <paramtype><xsl:apply-templates select="type"/></paramtype>

      <!-- Default argument -->
      <xsl:if test="defval">
        <default>
          <xsl:choose>
            <xsl:when test="contains(string(defval), $boost.doxygen.detail)">
              <emphasis>unspecified</emphasis>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="defval/*|defval/text()" 
                mode="passthrough"/>
            </xsl:otherwise>
          </xsl:choose>

        </default>
      </xsl:if>

      <!-- Parameter description -->
      <xsl:variable name="name">
        <xsl:value-of select="normalize-space(declname/text())"/>
      </xsl:variable>

      <xsl:apply-templates select="../detaileddescription/para/parameterlist[attribute::kind='param']/parameterdescription"
        mode="parameter.description">
        <xsl:with-param name="name">
          <xsl:value-of select="$name"/>
        </xsl:with-param>
      </xsl:apply-templates>
    </parameter>
  </xsl:template>

  <xsl:template match="parameterdescription" mode="parameter.description">
    <!-- The parameter name we are looking for -->
    <xsl:param name="name"/>

    <!-- The parametername node associated with this description -->
    <xsl:variable name="name-node" select="preceding-sibling::*[1]"/>

    <xsl:if test="string($name-node/text()) = $name">
      <description>
        <xsl:apply-templates select="para" mode="passthrough"/>
      </description>
    </xsl:if>
  </xsl:template>

  <!-- Handle function children -->
  <xsl:template name="function.children">
    <xsl:param name="is-overloaded" select="false()"/>

    <xsl:if test="not($is-overloaded)">
      <!-- Emit template header -->
      <xsl:apply-templates select="templateparamlist" mode="template"/>
      
      <!-- Emit function parameters -->
      <xsl:apply-templates select="param" mode="function"/>
    </xsl:if>

    <!-- The description -->
    <xsl:apply-templates select="briefdescription" mode="passthrough"/>
    <xsl:apply-templates select="detaileddescription" mode="passthrough"/>
      
    <xsl:apply-templates 
      select="detaileddescription/para/simplesect[@kind='pre']"
      mode="function-clauses"/>
    <xsl:apply-templates 
      select="detaileddescription/para/simplesect[@kind='post']"
      mode="function-clauses"/>
    <xsl:apply-templates 
      select="detaileddescription/para/simplesect[@kind='return']"
      mode="function-clauses"/>
    <xsl:if test="detaileddescription/para/parameterlist[@kind='exception']">
      <throws>
        <xsl:apply-templates 
          select="detaileddescription/para/parameterlist[@kind='exception']"
          mode="function-clauses"/>
      </throws>
    </xsl:if>
    <xsl:variable name="notes" select="detaileddescription/para/simplesect[@kind='note' or @kind='attention']"/>
    <xsl:if test="count($notes) &gt; 0"> 
      <notes>
        <xsl:apply-templates select="$notes" mode="function-clauses"/>
      </notes>
    </xsl:if>
  </xsl:template>

  <!-- Handle free functions -->
  <xsl:template name="function">
    <xsl:param name="in-file" select="''"/>

    <xsl:variable name="firstpara" 
      select="normalize-space(detaileddescription/para[1])"/>
    <xsl:if test="contains(string(location/attribute::file), $in-file)
                  and 
                  not($firstpara=normalize-space($boost.doxygen.overload))">

      <xsl:variable name="next-node" select="following-sibling::*[1]"/>
      <xsl:variable name="has-overload">
        <xsl:if test="not(local-name($next-node)='memberdef')">
          false
        </xsl:if>
        <xsl:if test="not(string($next-node/name/text())=string(name/text()))">
          false
        </xsl:if>
        <xsl:if 
          test="not(normalize-space($next-node/detaileddescription/para[1])
                    =normalize-space($boost.doxygen.overload))">
          false
        </xsl:if>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="not(contains($has-overload, 'false'))">
          <overloaded-function>
            <xsl:attribute name="name">
              <xsl:call-template name="normalize-name"/>
            </xsl:attribute>

            <xsl:call-template name="overload-signatures"/>
            <xsl:call-template name="function.children">
              <xsl:with-param name="is-overloaded" select="true()"/>
            </xsl:call-template>
          </overloaded-function>
        </xsl:when>
        <xsl:otherwise>
          <function>
            <xsl:attribute name="name">
              <xsl:call-template name="normalize-name"/>
            </xsl:attribute>
            
            <!-- Return type -->
            <type><xsl:apply-templates select="type"/></type>
            
            <xsl:call-template name="function.children"/>
          </function>          
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- Emit overload signatures -->
  <xsl:template name="overload-signatures">
    <xsl:param name="node" select="."/>
    <xsl:param name="name" select="string(name/text())"/>
    <xsl:param name="first" select="true()"/>

    <xsl:choose>
      <xsl:when test="not(local-name($node)='memberdef')"/>
      <xsl:when test="not(string($node/name/text())=$name)"/>
      <xsl:when test="not(normalize-space($node/detaileddescription/para[1])
                          =normalize-space($boost.doxygen.overload))
                      and not($first)"/>
      <xsl:otherwise>
        <signature>
          <type>
            <xsl:apply-templates select="$node/type"/>
          </type>
          <xsl:apply-templates select="$node/templateparamlist" 
            mode="template"/>
          <xsl:apply-templates select="$node/param" mode="function"/>
        </signature>

        <xsl:call-template name="overload-signatures">
          <xsl:with-param name="node" select="$node/following-sibling::*[1]"/>
          <xsl:with-param name="name" select="$name"/>
          <xsl:with-param name="first" select="false()"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Handle constructors -->
  <xsl:template name="constructor">
    <constructor>
      <xsl:call-template name="function.children"/>
    </constructor>
  </xsl:template>

  <!-- Handle Destructors -->
  <xsl:template name="destructor">
    <destructor>
      <xsl:call-template name="function.children"/>
    </destructor>
  </xsl:template>

  <!-- Handle Copy Assignment -->
  <xsl:template name="copy-assignment">
    <copy-assignment>
      <xsl:call-template name="function.children"/>
    </copy-assignment>
  </xsl:template>

  <!-- Handle conversion operator -->
  <xsl:template name="conversion-operator">
    <method>
      <xsl:attribute name="name">
        <xsl:text>conversion-operator</xsl:text>
      </xsl:attribute>

      <!-- CV Qualifiers -->
      <xsl:if test="not (@const='no' and @volatile='no')">
        <xsl:attribute name="cv">
          <xsl:if test="@const='yes'">
            <xsl:text>const</xsl:text>
          </xsl:if>
          <xsl:if test="@volatile='yes'">
            <xsl:if test="@const='yes'">
              <xsl:text> </xsl:text>
            </xsl:if>
            <xsl:text>volatile</xsl:text>
          </xsl:if>
        </xsl:attribute>
      </xsl:if>

      <!-- Conversion type -->
      <type>
        <xsl:value-of select="substring-after(name/text(), 'operator ')"/>
      </type>

      <xsl:call-template name="function.children"/>
    </method>
  </xsl:template>

  <!-- Handle methods -->
  <xsl:template name="method">
    <method>
      <xsl:attribute name="name">
        <xsl:value-of select="name/text()"/>
      </xsl:attribute>

      <!-- TBD: virtual and static functions -->

      <!-- CV Qualifiers -->
      <xsl:if test="not (@const='no' and @volatile='no')">
        <xsl:attribute name="cv">
          <xsl:if test="@const='yes'">
            <xsl:text>const</xsl:text>
          </xsl:if>
          <xsl:if test="@volatile='yes'">
            <xsl:if test="@const='yes'">
              <xsl:text> </xsl:text>
            </xsl:if>
            <xsl:text>volatile</xsl:text>
          </xsl:if>
        </xsl:attribute>
      </xsl:if>

      <!-- Return type -->
      <type><xsl:apply-templates select="type"/></type>

      <xsl:call-template name="function.children"/>
    </method>
  </xsl:template>

  <!-- Handle member variables -->
  <xsl:template name="variable">
    <data-member>
      <xsl:attribute name="name">
        <xsl:value-of select="name/text()"/>
      </xsl:attribute>

      <!-- Specifiers -->
      <xsl:if test="@static = 'yes'">
        <xsl:attribute name="specifiers">static</xsl:attribute>
      </xsl:if>
      <xsl:if test="@mutable = 'yes'">
        <xsl:attribute name="specifiers">mutable</xsl:attribute>
      </xsl:if>

      <type>
        <xsl:apply-templates select="type"/>
      </type>

      <xsl:apply-templates select="briefdescription" mode="passthrough"/>
      <xsl:apply-templates select="detaileddescription" mode="passthrough"/>
    </data-member>
  </xsl:template>

  <!-- Things we ignore directly -->
  <xsl:template match="compoundname" mode="toplevel"/>
  <xsl:template match="includes|includedby|incdepgraph|invincdepgraph" mode="toplevel"/>
  <xsl:template match="programlisting" mode="toplevel"/>
  <xsl:template match="text()" mode="toplevel"/>

  <xsl:template match="text()"/>

  <!-- Passthrough of text -->
  <xsl:template match="text()" mode="passthrough">
    <xsl:value-of select="."/>
  </xsl:template>
  <xsl:template match="para" mode="passthrough">
    <para>
      <xsl:apply-templates mode="passthrough"/>
    </para>
  </xsl:template>

  <xsl:template match="para/simplesect" mode="passthrough">
    <xsl:if test="not (@kind='pre') and
                  not (@kind='return') and 
                  not (@kind='post') and
                  not (@kind='attention') and
                  not (@kind='note')">
      <xsl:apply-templates mode="passthrough"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="computeroutput|orderedlist|itemizedlist|listitem" 
    mode="passthrough">
    <xsl:element name="{name(.)}">
      <xsl:apply-templates mode="passthrough"/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="parameterlist" mode="passthrough"/>

  <xsl:template match="bold" mode="passthrough">
    <emphasis role="bold">
      <xsl:apply-templates mode="passthrough"/>
    </emphasis>
  </xsl:template>

  <xsl:template match="briefdescription" mode="passthrough">
    <xsl:if test="text()|*">
      <purpose>
        <xsl:apply-templates mode="purpose"/>
      </purpose>
    </xsl:if>
  </xsl:template>

  <xsl:template match="detaileddescription" mode="passthrough">
    <xsl:if test="text()|*">
      <description>
        <xsl:apply-templates mode="passthrough"/>
      </description>
    </xsl:if>
  </xsl:template>

  <!-- Handle function clauses -->
  <xsl:template match="simplesect" mode="function-clauses">
    <xsl:if test="@kind='pre'">
      <requires>
        <xsl:apply-templates mode="passthrough"/>
      </requires>
    </xsl:if>
    <xsl:if test="@kind='return'">
      <returns>
        <xsl:apply-templates mode="passthrough"/>
      </returns>
    </xsl:if>
    <xsl:if test="@kind='post'">
      <postconditions>
        <xsl:apply-templates mode="passthrough"/>
      </postconditions>
    </xsl:if>
    <xsl:if test="@kind='note' or @kind='attention'">
      <xsl:apply-templates mode="passthrough"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="parameterlist" mode="function-clauses">
    <xsl:if test="@kind='exception'">
      <simpara>
        <xsl:choose>
          <xsl:when test="normalize-space(parametername/text())='nothrow'">
            <xsl:text>Will not throw.</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <classname>
              <xsl:value-of select="parametername/text()"/>
            </classname>
            <xsl:text> </xsl:text>
            <xsl:apply-templates 
              select="parameterdescription/para/text()
                      |parameterdescription/para/*"
              mode="passthrough"/>
          </xsl:otherwise>
        </xsl:choose>
      </simpara>
    </xsl:if>
  </xsl:template>

  <xsl:template match="type">
    <xsl:choose>
      <xsl:when test="contains(string(.), $boost.doxygen.detail)">
        <emphasis>unspecified</emphasis>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="type"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="ref" mode="type">
    <xsl:choose>
      <xsl:when test="@kindref='compound'">
        <classname>
          <xsl:value-of select="text()"/>
        </classname>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="text()"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*" mode="type">
    <xsl:value-of select="."/>
  </xsl:template>

  <!-- Normalize the names of functions, because Doxygen sometimes
       puts in an obnoixous space. -->
  <xsl:template name="normalize-name">
    <xsl:param name="name" select="name/text()"/>

    <xsl:choose>
      <xsl:when test="contains($name, ' ')">
        <xsl:value-of select="substring-before($name, ' ')"/>
        <xsl:value-of select="substring-after($name, ' ')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Convert HTML tables into DocBook format -->
  <xsl:template match="table" mode="passthrough">
    <informaltable>
      <tgroup>
        <xsl:attribute name="cols">
          <xsl:value-of select="@cols"/>
        </xsl:attribute>

        <tbody>
          <xsl:apply-templates mode="table"/>
        </tbody>
      </tgroup>
    </informaltable>
  </xsl:template>

  <xsl:template match="row" mode="table">
    <row>
      <xsl:apply-templates mode="table"/>
    </row>
  </xsl:template>
  
  <xsl:template match="entry" mode="table">
    <entry>
      <xsl:if test="para/center">
        <xsl:attribute name="valign">
          <xsl:value-of select="'middle'"/>
        </xsl:attribute>
        <xsl:attribute name="align">
          <xsl:value-of select="'center'"/>
        </xsl:attribute>
      </xsl:if>

      <xsl:choose>
        <xsl:when test="@thead='yes'">
          <emphasis role="bold">
            <xsl:call-template name="table-entry"/>
          </emphasis>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="table-entry"/>
        </xsl:otherwise>
      </xsl:choose>
    </entry>
  </xsl:template>

  <xsl:template name="table-entry">
    <xsl:choose>
      <xsl:when test="para/center">
        <xsl:apply-templates select="para/center/*|para/center/text()"
          mode="passthrough"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="para/*|para/text()" mode="passthrough"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Handle program listings -->
  <xsl:template match="programlisting" mode="passthrough">
    <programlisting>
      <xsl:apply-templates mode="programlisting"/>
    </programlisting>
  </xsl:template>

  <xsl:template match="highlight|codeline" mode="programlisting">
    <xsl:apply-templates mode="programlisting"/>
  </xsl:template>

  <xsl:template match="sp" mode="programlisting">
    <xsl:text> </xsl:text>
  </xsl:template>

  <xsl:template match="*" mode="programlisting">
    <xsl:apply-templates select="." mode="passthrough"/>
  </xsl:template>

  <!-- Replace top-level "para" elements with "simpara" elements in
       the purpose -->
  <xsl:template match="*" mode="purpose">
    <xsl:apply-templates mode="passthrough"/>
  </xsl:template>

  <xsl:template match="text()" mode="purpose">
    <xsl:apply-templates mode="passthrough"/>
  </xsl:template>

  <xsl:template match="para" mode="purpose">
    <xsl:apply-templates select="*|text()" mode="passthrough"/>
  </xsl:template>

 </xsl:stylesheet>
