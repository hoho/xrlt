<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="json-stringify" type="json-stringify" />
    <xrl:transformation name="xslt-stringify" type="xslt-stringify" src="test4.xsl" />

    <xrl:response>
        <xrl:variable name="tmptmp" select="'-'" />

        <xrl:include>
            <xrl:href>
                <xrl:text>/tmp</xrl:text>
            </xrl:href>
            <xrl:type>json</xrl:type>

            <xrl:success>
                <xrl:transform name="json-stringify">
                    <test1>
                        <xrl:transform name="xslt-stringify">
                            <test2>
                                <xrl:for-each select="/data[name = 'test2']/item">
                                    <xrl:value-of select="$tmptmp" />
                                    <xrl:variable name="testi">
                                        <xrl:include>
                                            <xrl:href>/tmpi</xrl:href>
                                            <xrl:type>text</xrl:type>
                                        </xrl:include>
                                    </xrl:variable>

                                    <item>
                                        <xrl:copy-of select="concat($testi, text(), $testi)" />
                                    </item>
                                </xrl:for-each>
                            </test2>
                            <test3>
                                <xrl:for-each select="/data[name = 'test3']/item">
                                    <item2>
                                        <xrl:copy-of select="concat('f', text(), '-')" />
                                    </item2>
                                </xrl:for-each>
                            </test3>
                        </xrl:transform>
                    </test1>
                </xrl:transform>
            </xrl:success>
        </xrl:include>
    </xrl:response>

</xrl:requestsheet>
