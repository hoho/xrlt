<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:include>
            <xrl:href>/test/href/1</xrl:href>
            <xrl:type>json</xrl:type>
            <xrl:success select="/*" />
            <xrl:failure>
                <a>aaaa</a>
            </xrl:failure>
        </xrl:include>

        <xrl:include>
            <xrl:href>/test/href/2</xrl:href>
            <xrl:type>text</xrl:type>
            <xrl:success select="substring-before(/, '123')" />
            <xrl:failure>
                <b>bbbb</b>
            </xrl:failure>
        </xrl:include>

        <xrl:include>
            <xrl:href select="'/test/href/3'" />
            <xrl:type>xml</xrl:type>
            <xrl:failure>
                <c>cccc</c>
            </xrl:failure>
        </xrl:include>
    </xrl:response>
</xrl:requestsheet>
