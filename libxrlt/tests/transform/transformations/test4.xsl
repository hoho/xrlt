<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

    <xsl:output method="html" />

    <xsl:key name="kkk" match="/tmp5/item2/*/text()" use="local-name(..)" />

    <xsl:template match="/">
        <xsl:apply-templates select="/tmp4/item" />
    </xsl:template>

    <xsl:template match="item">
        <tmp key="{text()}"><xsl:value-of select="key('kkk', text())" /></tmp>
    </xsl:template>

</xsl:stylesheet>
