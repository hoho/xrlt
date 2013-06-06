<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:response-header name="Test-Header">With-Value</xrl:response-header>
        <xrl:response-header test="false()" name="Test-Header2">With-Value2</xrl:response-header>
        <xrl:response-header test="true()" name="Test-Header3">With-Value3</xrl:response-header>

        <xrl:response-header name="Test-Header4" select="12 + 34" />
        <xrl:response-header test="false()" name="Test-Header5" select="12 + 35" />
        <xrl:response-header test="true()" name="Test-Header6" select="12 + 36" />

        <xrl:response-header>
            <xrl:name select="concat('This', '-', 'Is')" />
            <xrl:value select="concat('Some', '-', 'Header')" />
        </xrl:response-header>

        <xrl:response-header>
            <xrl:name>It-Is</xrl:name>
            <xrl:value>Another-Header</xrl:value>
        </xrl:response-header>

        <xrl:response-header>
            <xrl:test>ololo</xrl:test>
            <xrl:name select="'He1'" />
            <xrl:value>
                <xrl:include>
                    <xrl:href>/some-name</xrl:href>
                    <xrl:type>text</xrl:type>
                </xrl:include>
            </xrl:value>
        </xrl:response-header>

        <xrl:response-header>
            <xrl:test select="false()" />
            <xrl:name select="'He2'" />
            <xrl:value select="'Va2'" />
        </xrl:response-header>

        <xrl:response-header>
            <xrl:test select="true()" />
            <xrl:name select="'He3'" />
            <xrl:value select="'Va3'" />
        </xrl:response-header>


        <xrl:variable name="te1">
            <xrl:include>
                <xrl:href>/yeah1</xrl:href>
                <xrl:type>text</xrl:type>
            </xrl:include>
        </xrl:variable>

        <xrl:variable name="na1">
            <xrl:include>
                <xrl:href>/yeah2</xrl:href>
                <xrl:type>text</xrl:type>
            </xrl:include>
        </xrl:variable>

        <xrl:variable name="va1">
            <xrl:include>
                <xrl:href>/yeah3</xrl:href>
                <xrl:type>text</xrl:type>
            </xrl:include>
        </xrl:variable>

        <xrl:response-header test="$te1" select="$va1">
            <xrl:name select="$na1" />
        </xrl:response-header>
    </xrl:response>
</xrl:requestsheet>
