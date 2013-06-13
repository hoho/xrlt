<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:variable name="tmp2">
        <xrl:include>
            <xrl:href>/tmp2</xrl:href>
            <xrl:type>text</xrl:type>
        </xrl:include>
    </xrl:variable>

    <xrl:response>
        <xrl:variable name="tmp">
            <a>aaa</a>
            <b>bbb</b>
            <c>ccc</c>
            <d>ddd</d>
        </xrl:variable>

        <xrl:for-each select="$tmp/*">
            <xrl:variable name="tmp3" select="position()" />
            <xrl:value-of select="$tmp3" />
            <xrl:text>. </xrl:text>
            <xrl:value-of select="local-name()" />
            <xrl:text>: </xrl:text>
            <xrl:value-of select="text()" />
            <xrl:if test="position() = last() and $tmp2 != ''">
                <xrl:text>, </xrl:text>
                <xrl:value-of select="position() = last()" />
            </xrl:if>
            <xrl:text>
</xrl:text>
        </xrl:for-each>
    </xrl:response>

</xrl:requestsheet>
