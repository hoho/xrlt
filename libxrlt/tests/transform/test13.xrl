<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:transformation name="testxsl" type="xslt" src="transform/test13.xsl">
        <xrl:param name="p1" />
    </xrl:transformation>


    <xrl:response>
        <xrl:transform name="testxsl">
            <test>yoyoyo</test>
            <test>yaya</test>
        </xrl:transform>
    </xrl:response>

</xrl:requestsheet>
