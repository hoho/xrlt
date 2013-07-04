<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="json" type="json-stringify" />

    <xrl:response>
        <xrl:transform name="json">
            <test>yoyoyo</test>
            <test>yaya</test>
            <test xrl:type="number">123</test>
            <test>
                <a>aaa</a>
                <b>"b\b'b</b>
                <c>ccc</c>
            </test>
            <test>piu<xrl:value-of select="'-'" />piu</test>
        </xrl:transform>
    </xrl:response>

</xrl:requestsheet>
