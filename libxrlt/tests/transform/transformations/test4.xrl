<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="json-stringify" type="json-stringify" />
    <xrl:transformation name="xslt-stringify" type="xslt-stringify" src="test4.xsl" />

    <xrl:response>
        <xrl:include>
            <xrl:href>
                <xrl:text>/tmp/tmp</xrl:text>
            </xrl:href>
            <xrl:type>json</xrl:type>

            <xrl:with-param name="q">
                <xrl:transform name="json-stringify">
                    <tmp1>Blah blah</tmp1>
                    <tmp2>Mooo mooo</tmp2>
                </xrl:transform>
            </xrl:with-param>

            <xrl:success>
                <xrl:transform name="json-stringify">
                    <tmp3>
                        <xrl:transform name="xslt-stringify">
                            <tmp4>
                                <xrl:for-each select="/data[ololo]/yoyo">
                                    <item>
                                        <xrl:copy-of select="node()" />
                                    </item>
                                </xrl:for-each>
                            </tmp4>
                            <tmp5>
                                <xrl:for-each select="/data[alala]/yoyo">
                                    <item2>
                                        <xrl:copy-of select="*" />
                                    </item2>
                                </xrl:for-each>
                            </tmp5>
                        </xrl:transform>
                    </tmp3>
                </xrl:transform>
            </xrl:success>
        </xrl:include>
    </xrl:response>

</xrl:requestsheet>
