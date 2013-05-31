<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:querystring name="qs">
        <xrl:type>
            <xrl:include>
                <xrl:href>/qtype</xrl:href>
                <xrl:type>text</xrl:type>
            </xrl:include>
        </xrl:type>
        <xrl:success>
            <xrl:value-of select="concat(/, '|', /, '|', /)" />
        </xrl:success>
    </xrl:querystring>

    <xrl:response>
        <xrl:value-of select="$qs" />
    </xrl:response>

</xrl:requestsheet>
