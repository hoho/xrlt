<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:response>
        <xrl:variable name="tmp">
            <a>aaa</a>
            <b>bbb</b>
            <c>ccc</c>
            <d>ddd</d>
        </xrl:variable>

        <xrl:for-each select="$tmp/*">
            <xrl:value-of select="position()" />
            <xrl:text>. </xrl:text>
            <xrl:value-of select="local-name()" />
            <xrl:text>: </xrl:text>
            <xrl:value-of select="text()" />
            <xrl:text>, </xrl:text>
            <xrl:value-of select="position() = last()" />
            <xrl:text>
</xrl:text>
        </xrl:for-each>
    </xrl:response>

</xrl:requestsheet>
