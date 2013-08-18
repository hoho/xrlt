<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="qss" type="querystring-stringify" />

    <xrl:response>
        <xrl:transform name="qss">
            <test>hello</test>
            <test />
            <test></test>
            <test>world</test>
            <xrl:text>this</xrl:text>
            <xrl:text>is</xrl:text>
            <boo>something <very>awesome</very> indeed</boo>
        </xrl:transform>
    </xrl:response>

</xrl:requestsheet>
