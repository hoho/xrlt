<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">

    <xrl:response>
        <xrl:variable name="tmp">
            <xrl:include>
                <xrl:href>/tmp</xrl:href>
                <xrl:type>text</xrl:type>
            </xrl:include>
        </xrl:variable>

        <xrl:choose>
            <xrl:when test="false()">aa</xrl:when>
            <xrl:when test="false()">bb</xrl:when>
            <xrl:when test="true()">cc</xrl:when>
            <xrl:when test="true()">dd</xrl:when>
            <xrl:otherwise>ee</xrl:otherwise>
        </xrl:choose>

        <xrl:if test="false()">ff</xrl:if>

        <xrl:if test="true()">gg</xrl:if>

        <xrl:choose>
            <xrl:when test="false()">hh</xrl:when>
            <xrl:when test="false()">ii</xrl:when>
            <xrl:otherwise>jj</xrl:otherwise>
        </xrl:choose>

        <xrl:choose>
            <xrl:when test="false()">kk</xrl:when>
            <xrl:when test="$tmp = 'ololo'">ll</xrl:when>
            <xrl:when test="$tmp = 'alala'">mm</xrl:when>
        </xrl:choose>

        <xrl:if test="$tmp = 'ololo'">nn</xrl:if>

        <xrl:if test="$tmp = 'alala'">oo</xrl:if>
    </xrl:response>

</xrl:requestsheet>
