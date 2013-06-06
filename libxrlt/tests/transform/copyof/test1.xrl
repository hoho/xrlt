<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

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
    </xrl:response>

</xrl:requestsheet>
