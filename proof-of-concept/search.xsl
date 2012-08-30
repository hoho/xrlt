<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    version="1.0">
    
    <xsl:output method="html" encoding="utf-8" />

    <xsl:template match="/page">
        <html>
            <head>
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                <title><xsl:value-of select="title" /></title>
                <link rel="stylesheet" href="/search.css" type="text/css" />
            </head>
            <body>
                <xsl:apply-templates select="form" />
                <xsl:apply-templates select="data | help" />
            </body>
        </html>
    </xsl:template>


    <xsl:template match="form">
        <form action="search.xrl" method="GET">
            <table>
                <tr>
                    <xsl:apply-templates />
                    <td>
                        <input type="submit" value="Search" />
                    </td>
                </tr>
                <xsl:if test="*/error and text/value">
                    <tr class="error">
                        <xsl:apply-templates mode="error" />
                        <td></td>
                    </tr>
                </xsl:if>
            </table>
        </form>
    </xsl:template>


    <xsl:template match="form/*">
        <td>
            <input class="form-{local-name()}" type="text" name="{local-name()}" value="{value/text()}" />
        </td>
    </xsl:template>


    <xsl:template match="form/*" mode="error">
        <td>
            <xsl:value-of select="error" />
        </td>
    </xsl:template>


    <xsl:template match="help">
        <div class="help data">
            <xsl:text>Type in your request and press «Search».</xsl:text>
        </div>
    </xsl:template>


    <xsl:template match="data">
        <div class="comment data">
            <xsl:text>No search results.</xsl:text>
        </div>
    </xsl:template>


    <xsl:template match="data[failed]">
        <div class="error data">
            <xsl:text>Search request is failed.</xsl:text>
        </div>
    </xsl:template>


    <xsl:template match="data[tweet]">
        <div class="tweets data">
            <xsl:apply-templates select="tweet" />
        </div>
    </xsl:template>


    <xsl:template match="tweet">
        <dl>
            <dt>
                <p>
                    <xsl:text>@</xsl:text>
                    <xsl:value-of select="user" />
                </p>
                <xsl:value-of select="date" />
            </dt>
            <dd>
                <p>
                    <span>
                        <xsl:value-of select="position()" />
                        <xsl:text>. </xsl:text>
                    </span>
                    <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
                </p>
                <xsl:value-of select="text" />
            </dd>
        </dl>
    </xsl:template>

</xsl:stylesheet>
