<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:function name="func1">
        <xrl:param name="p2">val2</xrl:param>
        <xrl:param name="p1" select="111" />
        <xrl:param name="p3">val3</xrl:param>
        <xrl:param name="p4" />
        <xrl:param name="p5" select="123 + 234" />

        <love>peace</love>
        <a>|</a>
        <xrl:value-of select="$p1" />
        <a>|</a>
        <xrl:value-of select="$p2" />
        <a>|</a>
        <xrl:value-of select="$p3" />
        <a>|</a>
        <xrl:value-of select="$p4" />
        <a>|</a>
        <xrl:value-of select="$p5" />
    </xrl:function>

    <xrl:response>
        <xrl:apply name="func1">
            <xrl:with-param name="p4">alala</xrl:with-param>
            <xrl:with-param name="p1">olol|<xrl:value-of select="123 +35" />|</xrl:with-param>
        </xrl:apply>
    </xrl:response>
</xrl:requestsheet>
