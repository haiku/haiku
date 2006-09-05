<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

  xmlns:sfa="http://developer.apple.com/namespaces/sfa"
  xmlns:sf="http://developer.apple.com/namespaces/sf"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:appsl="http://developer.apple.com/namespaces/sl"

  xmlns:w="http://schemas.microsoft.com/office/word/2003/wordml"
  xmlns:v="urn:schemas-microsoft-com:vml"
  xmlns:w10="urn:schemas-microsoft-com:office:word"
  xmlns:sl="http://schemas.microsoft.com/schemaLibrary/2003/core"
  xmlns:aml="http://schemas.microsoft.com/aml/2001/core"
  xmlns:wx="http://schemas.microsoft.com/office/word/2003/auxHint"
  xmlns:o="urn:schemas-microsoft-com:office:office"
  xmlns:dt="uuid:C2F41010-65B3-11d1-A29F-00AA00C14882"

  xmlns:node='http://xsltsl.org/node'
  xmlns:doc='http://www.oasis-open.org/docbook/xml/4.0'
  extension-element-prefixes='node'
  exclude-result-prefixes='doc sfa sf xsi appsl'>

  <xsl:import href='xsltsl/stdlib.xsl'/>

  <xsl:output method="xml" indent='yes'/>

  <!-- ********************************************************************
       $Id: pages-normalise.xsl,v 1.6 2005/12/13 23:05:24 balls Exp $
       ********************************************************************

       This file is part of the XSL DocBook Stylesheet distribution.
       See ../README or http://nwalsh.com/docbook/xsl/ for copyright
       and other information.

       ******************************************************************** -->

  <xsl:strip-space elements='*'/>
  <xsl:preserve-space elements='sf:span'/>

  <xsl:key name='styles'
	   match='sf:paragraphstyle[not(ancestor::appsl:section-prototypes)] |
		  sf:characterstyle[not(ancestor::appsl:section-prototypes)]'
	   use='@sf:ident|@sfa:ID'/>

  <xsl:template match='appsl:document'>
    <w:wordDocument>
      <!-- TODO: headers and footers -->
      <xsl:apply-templates select='sf:text-storage'/>
    </w:wordDocument>
  </xsl:template>

  <xsl:template match='sf:text-body'>
    <w:body>
      <xsl:apply-templates/>
    </w:body>
  </xsl:template>

  <xsl:template match='sf:text-storage'>
    <wx:sect>
      <wx:sub-section>
	<xsl:apply-templates/>
      </wx:sub-section>
    </wx:sect>
  </xsl:template>

  <xsl:template match='sf:p'>
    <w:p>
      <xsl:call-template name='find-style'/>

      <xsl:apply-templates/>
    </w:p>
  </xsl:template>

  <xsl:template match='sf:span'>
    <xsl:variable name='style-name'
		  select='key("styles", @sf:style)/self::sf:characterstyle/@sf:name'/>

    <xsl:choose>
      <xsl:when test='$style-name = "attribute-name"'>
	<xsl:if test='not(preceding-sibling::node()[not(self::text()) or (self::text() and normalize-space() != "")])'>
	  <aml:annotation aml:id='25' w:type='Word.Comment.Start'/>
	  <w:r>
	    <w:rPr>
	      <w:rStyle w:val='attributes'/>
	    </w:rPr>
	    <w:t>
	      <xsl:text> </xsl:text>
	    </w:t>
	  </w:r>
	  <aml:annotation aml:id='25' w:type='Word.Comment.End'/>
	  <w:r>
	    <w:rPr>
	      <w:rStyle w:val='CommentReference'/>
	    </w:rPr>
	    <aml:annotation aml:id='25' aml:author='DocBook' aml:createdate='2004-12-23T00:01:00' w:type='Word.Comment' w:initials='DBK'>
	      <aml:content>
		<w:p>
		  <w:pPr>
		    <w:pStyle w:val='CommentText'/>
		  </w:pPr>
                  <w:r>
                    <w:rPr>
                      <w:rStyle w:val="CommentReference"/>
                    </w:rPr>
                    <w:annotationRef/>
                  </w:r>
		  <xsl:call-template name='make-attributes'/>
		</w:p>
	      </aml:content>
	    </aml:annotation>
	  </w:r>
	</xsl:if>
      </xsl:when>
      <xsl:when test='$style-name = "attribute-value"'/>
      <xsl:otherwise>
	<w:r>
	  <xsl:call-template name='find-style'>
	    <xsl:with-param name='char-style-name' select='$style-name'/>
	  </xsl:call-template>

	  <xsl:apply-templates/>
	</w:r>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- Precondition: $node is a sf:span with style "attribute-name" -->
  <xsl:template name='make-attributes'>
    <xsl:param name='node' select='.'/>

    <xsl:choose>
      <xsl:when test='not($node)'/>
      <xsl:when test='$node/following-sibling::node()[1][self::sf:span]'>
	<xsl:if test='key("styles", $node/following-sibling::*[1]/@sf:style)/self::sf:characterstyle/@sf:name = "attribute-value"'>
	  <w:r>
	    <w:rPr>
	      <w:rStyle w:val='attribute-name'/>
	    </w:rPr>
	    <w:t>
	      <xsl:apply-templates select='$node' mode='textonly'/>
	    </w:t>
	  </w:r>
	  <w:r><w:t>=</w:t></w:r>
	  <w:r>
	    <w:rPr>
	      <w:rStyle w:val='attribute-value'/>
	    </w:rPr>
	    <w:t>
	      <xsl:apply-templates select='$node/following-sibling::*[1]'
				   mode='textonly'/>
	    </w:t>
	  </w:r>
	  <xsl:if test='$node/following-sibling::node()[2][self::sf:span] and
			key("styles", $node/following-sibling::*[2]/@sf:style)/self::sf:characterstyle/@sf:name = "attribute-name"'>
	    <xsl:call-template name='make-attributes'>
	      <xsl:with-param name='node' select='$node/following-sibling::*[2]'/>
	    </xsl:call-template>
	  </xsl:if>
	</xsl:if>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match='sf:br'/>
  <xsl:template match='sf:lnbr|sf:crbr'>
    <w:r>
      <w:br/>
    </w:r>
  </xsl:template>
  <xsl:template match='sf:tab'>
    <xsl:text>        </xsl:text>
  </xsl:template>
  <xsl:template match='sf:link'>
    <w:hlink w:dest='{@href}'>
      <xsl:apply-templates/>
    </w:hlink>
  </xsl:template>

  <xsl:template match='text()'>
    <xsl:choose>
      <xsl:when test='ancestor::sf:span'>
	<w:t>
	  <xsl:value-of select='.'/>
	</w:t>
      </xsl:when>
      <xsl:otherwise>
	<w:r>
	  <xsl:if test='ancestor::sf:link'>
	    <w:rPr>
	      <w:rStyle w:val='Hyperlink'/>
	    </w:rPr>
	  </xsl:if>
	  <w:t>
	    <xsl:value-of select='.'/>
	  </w:t>
	</w:r>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match='sf:section|sf:layout'>
    <xsl:apply-templates/>
  </xsl:template>
  <xsl:template match='sf:stylesheet|sf:stylesheet-ref |
		       sf:container-hint |
		       sf:page-start|sf:br |
		       sf:selection-start|sf:selection-end |
		       sf:insertion-point |
		       sf:ghost-text'/>

  <xsl:template match='*'>
    <xsl:message>element "<xsl:value-of select='name()'/>" not handled</xsl:message>
  </xsl:template>

  <xsl:template name='find-style'>
    <xsl:param name='ident' select='@sf:style'/>
    <xsl:param name='para-style-name'
	       select='key("styles", $ident)/self::sf:paragraphstyle/@sf:name'/>
    <xsl:param name='char-style-name'
	       select='key("styles", $ident)/self::sf:characterstyle/@sf:name'/>

    <xsl:choose>
      <xsl:when test='$ident = "paragraph-style-default"'>
	<w:pPr>
	  <w:pStyle w:val='Normal'/>
	</w:pPr>
      </xsl:when>
      <xsl:when test='$para-style-name != ""'>
	<w:pPr>
	  <xsl:if test='$para-style-name != ""'>
	    <w:pStyle w:val='{$para-style-name}'/>
	  </xsl:if>
	</w:pPr>
      </xsl:when>
      <xsl:when test='$char-style-name != "" or
		      key("styles", $ident)/self::sf:characterstyle/sf:property-map/*'>
	<w:rPr>
	  <xsl:if test='$char-style-name != ""'>
	    <w:rStyle w:val='{$char-style-name}'/>
	  </xsl:if>
	  <xsl:apply-templates select='key("styles", $ident)/self::sf:characterstyle/sf:property-map/*'
			       mode='styles'/>
	</w:rPr>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match='sf:bold' mode='styles'>
    <w:b/>
  </xsl:template>
  <xsl:template match='sf:italic' mode='styles'>
    <w:i/>
  </xsl:template>
  <xsl:template match='sf:underline' mode='styles'>
    <w:u/>
  </xsl:template>

</xsl:stylesheet>
