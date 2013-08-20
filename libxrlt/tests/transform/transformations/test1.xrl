<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="xsl" type="xslt" src="test1.xsl">
        <xrl:param name="papapa1" />
        <xrl:param name="papapa2" />
        <xrl:param name="papapa3" select="'pep&amp;epe'"/>
        <xrl:param name="papapa4" />
    </xrl:transformation>
    <xrl:transformation name="xsl2" type="xslt-stringify" src="test1.xsl" />

    <xrl:response>
        <xrl:transform name="xsl">
            <xrl:with-param name="papapa1" select="9876" />
            <xrl:with-param name="papapa2">'alala'</xrl:with-param>
            <test>yoyoyo</test>
            <test>yaya</test>
        </xrl:transform>
        <xrl:text>|||</xrl:text>
        <xrl:transform name="xsl2">
            <quack>opop</quack>
            <xrl:include>
                <xrl:href>/heck</xrl:href>
                <xrl:type>text</xrl:type>
                <xrl:success>
                    <heck>
                        <xrl:value-of select="/" />
                    </heck>
                </xrl:success>
            </xrl:include>
            <quack>apap</quack>
        </xrl:transform>
    </xrl:response>

</xrl:requestsheet>
