<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

    <xsl:output method="html" />

    <xsl:template match="/">
        <xsl:text>hihi|</xsl:text>
        <xsl:for-each select="*">
            <xsl:value-of select="concat(local-name(), ': ', text())" />
            <xsl:text>|</xsl:text>
        </xsl:for-each>
        <xsl:text>haha</xsl:text>
    </xsl:template>

</xsl:stylesheet>
