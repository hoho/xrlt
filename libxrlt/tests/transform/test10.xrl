<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:body name="b">
        <xrl:type>
            <xrl:include>
                <xrl:href>/btype</xrl:href>
                <xrl:type>json</xrl:type>
                <xrl:success>
                    <xrl:header name="OO" />
                    <xrl:header name="RRR" select="'O'" />
                    <xrl:value-of select="/some/value/text()" />
                </xrl:success>
            </xrl:include>
        </xrl:type>
        <xrl:success>
            <xrl:value-of select="concat(/, /)" />
        </xrl:success>
    </xrl:body>

    <xrl:response>
        <xrl:header name="EEE" />|<xrl:cookie name="CCC" />
        <xrl:text>|</xrl:text>
        <xrl:value-of select="$b" />
    </xrl:response>

</xrl:requestsheet>
