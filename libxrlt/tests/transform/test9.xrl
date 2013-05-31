<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:querystring name="qs" type="text" />
    <xrl:body name="b" type="json" />

    <xrl:response>
        <xrl:header name="EEE" />|<xrl:cookie name="CCC" />|<xrl:status />
        <xrl:text>|</xrl:text>
        <xrl:value-of select="$qs" />|<xrl:value-of select="$b" />
    </xrl:response>

</xrl:requestsheet>
