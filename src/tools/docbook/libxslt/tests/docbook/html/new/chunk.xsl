<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
		version="1.1"
                exclude-result-prefixes="doc">

<!-- This stylesheet works with Saxon and Xalan; for XT use xtchunk.xsl -->
<!-- This stylesheet should also work for any processor that supports   -->
<!-- exslt:document() (see http://www.exslt.org/)                       -->

<xsl:import href="autoidx.xsl"/>
<xsl:include href="chunk-common.xsl"/>
<xsl:include href="chunker.xsl"/>

</xsl:stylesheet>
