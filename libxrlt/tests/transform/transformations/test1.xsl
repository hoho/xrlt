<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

    <xsl:output method="html" />

    <xsl:param name="papapa1" />
    <xsl:param name="papapa2" />
    <xsl:param name="papapa3" />
    <xsl:param name="papapa4" select="57575" />

    <xsl:template match="/">
        <yoyo>
            <xsl:text>hihi|</xsl:text>

            <xsl:text>papapa1=</xsl:text>
            <xsl:value-of select="$papapa1" />

            <xsl:text>|papapa2=</xsl:text>
            <xsl:value-of select="$papapa2" />

            <xsl:text>|papapa3=</xsl:text>
            <xsl:value-of select="$papapa3" />

            <xsl:text>|papapa4=</xsl:text>
            <xsl:value-of select="$papapa4" />

            <xsl:text>|</xsl:text>
            <xsl:for-each select="*">
                <xsl:value-of select="concat(local-name(), ': ', text())" />
                <xsl:text>|</xsl:text>
            </xsl:for-each>
            <xsl:text>haha</xsl:text>
        </yoyo>
    </xsl:template>

</xsl:stylesheet>
