<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:variable name="hehehe">hahaha</xrl:variable>

    <xrl:function name="func1">
        <xrl:param name="p3" />
        <xrl:param name="p1" select="111" />
        <xrl:param name="p2">222</xrl:param>

        <xrl:variable name="test" select="concat('AAAA', $p2)"/>

        <a>|<xrl:value-of select="$p1" />|<xrl:value-of select="$p2" />|<xrl:value-of select="$p3" />|</a>
        <a>|<xrl:value-of select="$hehehe" />|<xrl:value-of select="$test" />|</a>
    </xrl:function>

    <xrl:function name="func2">
        <xrl:param name="par1" />
        <xrl:param name="par2" select="42" />

        <a>Third call:</a>
        <xrl:apply name="func1" />

        <a>Fourth call:</a>
        <xrl:apply name="func1">
            <xrl:with-param name="p1" select="$par1" />
            <xrl:with-param name="p2" select="$par2" />
        </xrl:apply>

        <a>Fifth call:</a>
        <xrl:apply name="func1">
            <xrl:with-param name="p1">fuckyeah</xrl:with-param>
            <xrl:with-param name="p3">yo</xrl:with-param>
        </xrl:apply>
    </xrl:function>

    <xrl:response>
        <xrl:variable name="ylyly">
            <a>f</a>
            <b>
                <xrl:include>
                    <xrl:href>hehe</xrl:href>
                    <xrl:type>text</xrl:type>
                </xrl:include>
            </b>
        </xrl:variable>

        <xrl:variable name="alala" select="concat(1, $ylyly, 2)" />

        <xrl:value-of select="$hehehe" />

        <xrl:value-of select="$alala" />

        <a>First call:</a>
        <xrl:apply name="func1">
            <xrl:with-param name="p2">22222</xrl:with-param>
        </xrl:apply>

        <a>Second call:</a>
        <xrl:apply name="func1">
            <xrl:with-param name="p4">alala</xrl:with-param>
            <xrl:with-param name="p3">|<xrl:value-of select="123 + 35" />|</xrl:with-param>
        </xrl:apply>

        <xrl:apply name="func2" />
    </xrl:response>

</xrl:requestsheet>
