<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="xml-stringify" type="xml-stringify" />

    <xrl:function name="test" type="javascript">
        <xrl:param name="a"/>
        <![CDATA[

        return {
            "hi": 123,
            "yoo": null,
            "zozoz": true,
            "bye": "ololo",
            "piu": [
                123,
                false,
                true,
                "ggg",
                [
                    1,
                    2,
                    3,
                    {"h": true},
                    [6, 8],
                    [],
                    {"test": [null, [null], []]}
                ],
                {"love": 34, "hate": false}
            ],
            "mmm": {}
        }

        ]]>
    </xrl:function>

    <xrl:response>
        <xrl:transform name="xml-stringify">
            <root>
                <xrl:apply name="test" />
            </root>
        </xrl:transform>
    </xrl:response>

</xrl:requestsheet>
