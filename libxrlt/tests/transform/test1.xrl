<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:include>
            <xrl:href>/test/href/1</xrl:href>
            <xrl:type>xml</xrl:type>
            <xrl:with-header name="Content-Type">Some type</xrl:with-header>
            <xrl:with-header name="Nope" test="false()" select="100500" />
            <xrl:with-header name="Content-Length" test="true()" select="100500" />
            <xrl:with-param name="par1">p&amp;1</xrl:with-param>
            <xrl:with-param name="par2" select="'p;2'" />
            <xrl:with-param name="par3" test="false()" select="'p3'" />
            <xrl:with-param>
                <xrl:test select="true()" />
                <xrl:name>par4</xrl:name>
                <xrl:value select="'p&amp;4'" />
            </xrl:with-param>
            <xrl:success select="concat(/hello, /hello)" />
        </xrl:include>

        <xrl:include>
            <xrl:href>/test/href/2</xrl:href>
            <xrl:type select="'TEXT'" />
            <xrl:with-body>ololololo</xrl:with-body>
            <xrl:success select="concat(substring-before(/, 'many'), 'no',
                                        substring-after(text(), 'many'))" />
        </xrl:include>

        <xrl:include>
            <xrl:href select="'/test/href/3'" />
            <xrl:method>POST</xrl:method>
            <xrl:type>json</xrl:type>
            <xrl:with-param body="yes" name="test">aa</xrl:with-param>
            <xrl:with-param body="yes" name="test2" select="'\bb/'" />
        </xrl:include>
    </xrl:response>
</xrl:requestsheet>
