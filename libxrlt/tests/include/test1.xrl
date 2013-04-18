<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:include>
            <xrl:href>/test/href/1</xrl:href>
        </xrl:include>

        <xrl:include>
            <xrl:href>/test/href/2</xrl:href>
            <xrl:type>JSON</xrl:type>
            <xrl:success select="/*" />
        </xrl:include>

        <xrl:include>
            <xrl:href>/test/href/3</xrl:href>
            <xrl:type>TEXT</xrl:type>
        </xrl:include>

        <!--<xrl:include>
            <xrl:href select="'/test/href/2'" />
            <xrl:method>POST</xrl:method>
            <xrl:type>json</xrl:type>
            <xrl:with-param type="body" name="test">aa</xrl:with-param>
            <xrl:with-param type="body" name="test2" select="'\bb/'" />
        </xrl:include>

        <xrl:include>
            <xrl:href select="'/test/href/3'" />
            <xrl:method select="'HEAD'" />
            <xrl:type select="'xml'" />
            <xrl:with-body>bobobo</xrl:with-body>
        </xrl:include>

        <xrl:include>
            <xrl:href select="'/test/href/4'" />
            <xrl:method>put</xrl:method>
            <xrl:type select="'text'" />
            <xrl:with-body test="false()">bobobo</xrl:with-body>
        </xrl:include>

        <xrl:include>
            <xrl:href select="'/test/href/5'" />
            <xrl:method select="'HEAD'" />
            <xrl:with-body>
                <xrl:test select="true()" />
                <xrl:value>bababa</xrl:value>
            </xrl:with-body>
        </xrl:include>

        <xrl:include>
            <xrl:href select="'/test/href/6'" />

            <xrl:with-header name="he1" select="'va1'" />

            <xrl:with-header name="he2" test="false()" select="'va2'" />

            <xrl:with-header>
                <xrl:name>he3</xrl:name>
                <xrl:value>va3</xrl:value>
            </xrl:with-header>

            <xrl:with-header>
                <xrl:test select="false()" />
                <xrl:name>he4</xrl:name>
                <xrl:value>va4</xrl:value>
            </xrl:with-header>

            <xrl:with-param name="pa1" select="'pava1'" />

            <xrl:with-param name="pa2" test="false()" select="'pava2'" />

            <xrl:with-param>
                <xrl:name>pa3</xrl:name>
                <xrl:value>p,a;v&amp;a=3</xrl:value>
            </xrl:with-param>

            <xrl:with-param>
                <xrl:test></xrl:test>
                <xrl:name>pa4</xrl:name>
                <xrl:value>pava4</xrl:value>
            </xrl:with-param>

            <xrl:with-param>
                <xrl:test>true</xrl:test>
                <xrl:name>pa5</xrl:name>
                <xrl:value>pava5</xrl:value>
            </xrl:with-param>

            <xrl:with-body>bobobo</xrl:with-body>
        </xrl:include>-->

    </xrl:response>
</xrl:requestsheet>
