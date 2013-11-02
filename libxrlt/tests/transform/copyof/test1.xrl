<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:function name="somefunc">
        <xrl:param name="papapa" />
        <fff>111</fff>
        <xrl:copy-of select="$papapa" />
        <uuu>222</uuu>
    </xrl:function>

    <xrl:response>
        <xrl:variable name="tmp">
            <xrl:include>
                <xrl:href>/tmp</xrl:href>
                <xrl:type>xml</xrl:type>
            </xrl:include>
        </xrl:variable>

        <xrl:variable name="tmp2">
            <a>aaa</a>
            <b>bbb</b>
            <c>ccc</c>
            <d>ddd</d>
        </xrl:variable>

        <xrl:copy-of select="$tmp/r/*[local-name() = 'a' or local-name() = 'e']" />
        <xrl:copy-of select="$tmp2/*[local-name() = 'a' or local-name() = 'd']" />

        <xrl:include>
            <xrl:href>eee</xrl:href>
            <xrl:type>json</xrl:type>
            <xrl:success>
                <xrl:copy-of select="/" />

                <xrl:variable name="tmp3">
                    <xrl:apply name="somefunc">
                        <xrl:with-param name="papapa" select="/" />
                    </xrl:apply>
                </xrl:variable>

                <xrl:copy-of select="$tmp3" />
                <xrl:copy-of select="/" />
            </xrl:success>
        </xrl:include>
    </xrl:response>

</xrl:requestsheet>
