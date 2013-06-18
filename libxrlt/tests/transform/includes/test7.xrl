<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:include>
            <xrl:href>/tmp</xrl:href>
            <xrl:type>text</xrl:type>
            <xrl:success test="/text() != 'hello'">hooray</xrl:success>
            <xrl:failure>oops</xrl:failure>
        </xrl:include>

        <xrl:variable name="ololo">
            <xrl:include>
                <xrl:href>/tmp2</xrl:href>
                <xrl:type>text</xrl:type>
                <xrl:success test="/text() = 'hello'">hooray</xrl:success>
                <xrl:failure>oops</xrl:failure>
            </xrl:include>
        </xrl:variable>

        <xrl:include>
            <xrl:href>/tmp3</xrl:href>
            <xrl:type>text</xrl:type>
            <xrl:success test="$ololo != 'oops'">hooray</xrl:success>
            <xrl:failure>oops</xrl:failure>
        </xrl:include>
    </xrl:response>
</xrl:requestsheet>
