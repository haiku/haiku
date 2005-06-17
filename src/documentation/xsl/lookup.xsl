<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:param name="boost.max.id.length">26</xsl:param>

  <!-- Generate an ID for the entity referenced -->
  <xsl:template name="generate.id">
    <xsl:param name="node" select="."/>
    <xsl:variable name="id">
      <xsl:apply-templates select="$node" mode="generate.id"/>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="string-length($id) &gt; $boost.max.id.length">
        <xsl:value-of select="generate-id($node)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="string($id)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*" mode="generate.id">
    <xsl:value-of select="generate-id(.)"/>
    <xsl:text>-bb</xsl:text>
  </xsl:template>

  <!-- Strip the qualifiers off a qualified name and return the unqualified
       name. For instance, "boost::python::function" would become just
       "function". -->
  <xsl:template name="strip-qualifiers">
    <xsl:param name="name"/>
    <xsl:choose>
      <xsl:when test="contains($name, '::')">
        <xsl:call-template name="strip-qualifiers">
          <xsl:with-param name="name" select="substring-after($name, '::')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Build the fully-qualified name of the given node -->
  <xsl:template name="fully-qualified-name">
    <xsl:param name="node"/>
    <xsl:param name="separator" select="'::'"/>
    <xsl:apply-templates select="$node" mode="fully-qualified-name">
      <xsl:with-param name="separator" select="$separator"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Hack to make the node we are building the current node so that the
       ancestor:: syntax will work -->
  <xsl:template match="*" mode="fully-qualified-name">
    <xsl:param name="separator" select="'::'"/>
    <xsl:call-template name="build-fully-qualified-name">
      <xsl:with-param name="separator" select="$separator"/>
    </xsl:call-template>
  </xsl:template>

  <!-- The real routine that builds a fully-qualified name for the current
       node. -->
  <xsl:template name="build-fully-qualified-name">
    <xsl:param name="separator" select="'::'"/>

    <!-- The depth of qualified name element that we will print now-->
    <xsl:param name="depth" select="1"/>

    <!-- Determine the set of ancestor namespaces -->
    <xsl:variable name="ancestors" 
      select="ancestor::namespace|ancestor::class"/>

    <xsl:choose>
      <xsl:when test="$depth &gt; count($ancestors)">
        <xsl:apply-templates select="." mode="print-name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="name($ancestors[$depth])='namespace' or
                      count(ancestor::free-function-group)=0">
          <xsl:apply-templates select="$ancestors[$depth]" mode="print-name"/>
          <xsl:value-of select="$separator"/>
        </xsl:if>
        <xsl:call-template name="build-fully-qualified-name">
          <xsl:with-param name="separator" select="$separator"/>
          <xsl:with-param name="depth" select="$depth + 1"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Print the name of the current node -->
  <xsl:template match="*" mode="print-name">
    <xsl:value-of select="@name"/>
  </xsl:template>

  <xsl:template name="name-matches-node">
    <!-- The name we are looking for -->
    <xsl:param name="name"/>

    <!-- The name to display -->
    <xsl:param name="display-name" select="$name"/>

    <!-- The context in which this name occurs -->
    <xsl:param name="context"/>

    <!-- The node that we are checking against -->
    <xsl:param name="node"/>

    <!-- The mode we are in. Can be one of:
           matches: emits the matches as they are found (for debugging)
           link: link to the node that was found
         -->
    <xsl:param name="mode" select="'matches'"/>

    <!-- The index into the list of using directives for the context node -->
    <xsl:param name="index" select="1"/>

    <!-- The prefix we should append to the name when checking this node -->
    <xsl:param name="prefix" select="''"/>

    <xsl:choose>
      <xsl:when test="count($node) &gt; 1">
        <xsl:variable name="matches">
          <xsl:call-template name="count-matches">
            <xsl:with-param name="name" select="$name"/>
            <xsl:with-param name="context" select="$context"/>
            <xsl:with-param name="nodes" select="$node[position() = 1]"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:choose>
          <xsl:when test="$matches = 0">
            <xsl:call-template name="name-matches-node">
              <xsl:with-param name="name" select="$name"/>
              <xsl:with-param name="display-name" select="$display-name"/>
              <xsl:with-param name="context" select="$context"/>
              <xsl:with-param name="node" select="$node[position() &gt; 1]"/>
              <xsl:with-param name="mode" select="$mode"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="name-matches-node">
              <xsl:with-param name="name" select="$name"/>
              <xsl:with-param name="display-name" select="$display-name"/>
              <xsl:with-param name="context" select="$context"/>
              <xsl:with-param name="node" select="$node[position() = 1]"/>
              <xsl:with-param name="mode" select="$mode"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="count($node) = 1">
        <!-- The fully-qualified name of the node we are checking against -->
        <xsl:variable name="fully-qualified-name">
          <xsl:call-template name="fully-qualified-name">
            <xsl:with-param name="node" select="$node"/>
          </xsl:call-template>
        </xsl:variable>
        
        <!-- The set of using directives for this context node -->
        <xsl:variable name="directives"
          select="$context/ancestor::*/using-namespace |
                  $context/ancestor::namespace |
                  $context/ancestor::*/using-class |
                  $context/ancestor::class"/>
        
        <!-- The name of the current directive -->
        <xsl:variable name="this-context">
          <xsl:apply-templates select="$directives[$index]" mode="print-name"/>
        </xsl:variable>
        
        <!-- Check if we have a match -->
        <xsl:variable name="have-match" 
          select="$fully-qualified-name = concat($prefix, $name)"/>
        
        <xsl:if test="$have-match">
          <xsl:choose>
            <xsl:when test="$mode='matches'">
              Match in namespace ::<xsl:value-of select="$prefix"/>
            </xsl:when>
            <xsl:when test="$mode='link'">
              <xsl:call-template name="internal-link">
                <xsl:with-param name="to">
                  <xsl:call-template name="generate.id">
                    <xsl:with-param name="node" select="$node"/>
                  </xsl:call-template>
                </xsl:with-param>
                <xsl:with-param name="text" select="$display-name"/>
              </xsl:call-template>
            </xsl:when>
          </xsl:choose>
        </xsl:if>
        
        <xsl:if test="(not($index &gt; count($directives))) and
                      (not($have-match) or ($mode = 'matches'))">
          <xsl:variable name="first-branch">
            <xsl:if test="not ($prefix = '')">
              <!-- Recurse and append the current context node to the prefix -->
              <xsl:call-template name="name-matches-node">
                <xsl:with-param name="name" select="$name"/>
                <xsl:with-param name="display-name" select="$display-name"/>
                <xsl:with-param name="context" select="$context"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="mode" select="$mode"/>
                <xsl:with-param name="index" select="$index + 1"/>
                <xsl:with-param name="prefix"
                  select="concat($prefix, $this-context, '::')"/>
              </xsl:call-template>
            </xsl:if>
          </xsl:variable>
          
          <xsl:choose>
            <xsl:when test="string($first-branch) != ''">
              <xsl:copy-of select="$first-branch"/>
            </xsl:when>
            <xsl:otherwise>
              <!-- Recurse with just the current context node -->
              <xsl:call-template name="name-matches-node">
                <xsl:with-param name="name" select="$name"/>
                <xsl:with-param name="display-name" select="$display-name"/>
                <xsl:with-param name="context" select="$context"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="mode" select="$mode"/>
                <xsl:with-param name="index" select="$index + 1"/>
                <xsl:with-param name="prefix" 
                  select="concat($this-context, '::')"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>        
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- Count the number of nodes in the set that match the given name and
       lookup context -->
  <xsl:template name="count-matches">
    <xsl:param name="name"/>
    <xsl:param name="context"/>
    <xsl:param name="nodes"/>

    <xsl:variable name="match-string">
      <xsl:for-each select="$nodes">
        <xsl:variable name="does-match">
          <xsl:call-template name="name-matches-node">
            <xsl:with-param name="name" select="$name"/>
            <xsl:with-param name="context" select="$context"/>
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="not($does-match='')">X</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="string-length($match-string)"/>
  </xsl:template>

  <xsl:template name="cxx-link-name">
    <!-- The actual lookup node -->
    <xsl:param name="lookup"/> 

    <!-- The type of name to lookup (e.g., class) -->
    <xsl:param name="type"/> 

    <!-- The name we are looking for -->
    <xsl:param name="name"/> 

    <!-- The name we will display  -->
    <xsl:param name="display-name"/>

    <!-- The name we are looking for (unqualified)-->
    <xsl:param name="unqualified name"/> 

    <!-- The list of nodes that match the lookup node in both name and type -->
    <xsl:param name="nodes"/>

    <!-- Count the number of nodes that match -->
    <xsl:variable name="matches">
      <xsl:call-template name="count-matches">
        <xsl:with-param name="name" select="$name"/>
        <xsl:with-param name="context" select="$lookup"/>
        <xsl:with-param name="nodes" select="$nodes"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$matches = 0">
        <xsl:message>
          <xsl:text>Cannot find </xsl:text>
          <xsl:value-of select="$type"/>
          <xsl:text> named '</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>'</xsl:text>
        </xsl:message>
        <xsl:value-of select="$display-name"/>
      </xsl:when>
      <xsl:when test="$matches = 1">
        <xsl:for-each select="$nodes">
          <xsl:call-template name="name-matches-node">
            <xsl:with-param name="name" select="$name"/>
            <xsl:with-param name="display-name" select="$display-name"/>
            <xsl:with-param name="context" select="$lookup"/>
            <xsl:with-param name="node" select="."/>
            <xsl:with-param name="mode" select="'link'"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
          <xsl:text>Reference to </xsl:text>
          <xsl:value-of select="$type"/>
          <xsl:text> '</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>' is ambiguous. Found:</xsl:text>
          <xsl:for-each select="$nodes">
            <xsl:call-template name="name-matches-node">
              <xsl:with-param name="name" select="$name"/>
              <xsl:with-param name="context" select="$lookup"/>
              <xsl:with-param name="node" select="."/>
              <xsl:with-param name="mode" select="'matches'"/>
            </xsl:call-template>
          </xsl:for-each>
        </xsl:message>
        <xsl:call-template name="name-matches-node">
          <xsl:with-param name="name" select="$name"/>
          <xsl:with-param name="display-name" select="$display-name"/>
          <xsl:with-param name="context" select="$lookup"/>
          <xsl:with-param name="node" select="$nodes"/>
          <xsl:with-param name="mode" select="'link'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
