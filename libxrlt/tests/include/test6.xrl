<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <a>11abcsdafkjhl</a>
        <xrl:include>
            <xrl:href>/sub2</xrl:href>
            <xrl:type>text</xrl:type>
            <xrl:failure>ooooooo</xrl:failure>
            <xrl:success>a<xrl:include>
                    <xrl:href>/sub</xrl:href>
                    <xrl:with-param name="test">hello</xrl:with-param>
                    <xrl:with-param name="test" select="123" />
                    <xrl:type>text</xrl:type>
                </xrl:include>bbbb</xrl:success>
        </xrl:include>
    </xrl:response>
</xrl:requestsheet>
