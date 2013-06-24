<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:include>
            <xrl:href>/tmp</xrl:href>
            <xrl:type>text</xrl:type>
            <xrl:success>hooray</xrl:success>
            <xrl:failure>oops: <xrl:value-of select="/" /><xrl:status /></xrl:failure>
        </xrl:include>

        <xrl:include>
            <xrl:href>/tmp2</xrl:href>
            <xrl:type>text</xrl:type>
            <xrl:success test="status() = 500">hooray: <xrl:value-of select="concat(status(), /)" /></xrl:success>
            <xrl:failure>oops</xrl:failure>
        </xrl:include>
    </xrl:response>
</xrl:requestsheet>
