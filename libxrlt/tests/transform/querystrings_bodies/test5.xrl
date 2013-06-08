<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:querystring name="qs">
        <xrl:type>json</xrl:type>
        <xrl:success select="concat(/hello/text(), '-', /hello/text())" />
    </xrl:querystring>

    <xrl:response>
        <xrl:text>|</xrl:text>
        <xrl:value-of select="$qs" />
        <xrl:text>|</xrl:text>
    </xrl:response>

</xrl:requestsheet>
