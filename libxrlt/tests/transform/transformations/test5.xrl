<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="qss" type="querystring-stringify" />
    <xrl:transformation name="qsp" type="querystring-parse" />
    <xrl:transformation name="xmls" type="xml-stringify" />

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

        <xrl:text>|</xrl:text>

        <xrl:transform name="xmls">
            <xrl:transform name="qsp">
                <hello>world=awesome</hello>
                <xrl:text>&amp;</xrl:text>
                <hello>world=awesome2</hello>
                <xrl:text>&amp;</xrl:text>
                <xrl:text>this=is&amp;amazing=&amp;indeed</xrl:text>
            </xrl:transform>
        </xrl:transform>
    </xrl:response>

</xrl:requestsheet>
