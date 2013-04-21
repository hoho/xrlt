<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:variable name="href">
            <xrl:include>
                <xrl:href>/test/href/1</xrl:href>
                <xrl:type>json</xrl:type>
                <xrl:success select="/" />
            </xrl:include>
        </xrl:variable>

        <xrl:variable name="par1">
            <xrl:include>
                <xrl:href>/test/href/2</xrl:href>
                <xrl:type>xml</xrl:type>
                <xrl:success select="/param/value" />
            </xrl:include>
        </xrl:variable>

        <xrl:variable name="par2">
            <xrl:include>
                <xrl:href>/test/href/3</xrl:href>
                <xrl:type>text</xrl:type>
            </xrl:include>
        </xrl:variable>

        <hi>yo</hi>

        <xrl:include>
            <xrl:href select="$href" />
            <xrl:type>json</xrl:type>
            <xrl:with-param name="p" select="concat($par1, $par2)" />
        </xrl:include>

        <bye>yep</bye>
    </xrl:response>
</xrl:requestsheet>
