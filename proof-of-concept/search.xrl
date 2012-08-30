<?xml version="1.0" encoding="utf-8"?>
<xrl:requestsheet
    xmlns:xrl="http://xrlt.net/Transform"
    xmlns:json="http://xrlt.net/JSON">

    <xrl:param name="DENIED">file:denied.json</xrl:param>
    <xrl:param name="TWITTER">http://search.twitter.com/search.json</xrl:param>


    <xrl:form name="request-form">
        <xrl:field name="count" type="javascript"><![CDATA[
            if (!count || !count.value) {
                value(5);
                return;
            }
            var count_ = (count.value || '').replace(/(^[0 ]+| +$)/g, '');
            if (count_.match(/^\d+$/)) {
                count_ = parseInt(count_, 10);
                value(count_);
                if (count_ > 100) {
                    error('Too large');
                }
            } else {
                error('Numbers only');
            }
        ]]></xrl:field>

        <xrl:field name="text">
            <xrl:variable name="text_"><xrl:value-of select="normalize-space($text/value)" /></xrl:variable>
            <xrl:choose>
                <xrl:when test="$text_ = ''">
                    <xrl:error>Request is empty</xrl:error>
                </xrl:when>
                <xrl:otherwise>
                    <xrl:value><xrl:value-of select="$text_" /></xrl:value>
                    <xrl:include href="{$DENIED}" type="json">
                        <xrl:success>
                            <xrl:if test="/denied/values = $text_">
                                <xrl:error>Word is denied</xrl:error>
                            </xrl:if>
                        </xrl:success>
                        <xrl:failure>
                            <xrl:error>Failed to load stop words</xrl:error>
                        </xrl:failure>
                    </xrl:include>
                </xrl:otherwise>
            </xrl:choose>
        </xrl:field>
        <!-- <xrl:field name="text" type="javascript"><![CDATA[
            var text_ = ((text && text.value) || '').replace(/(^ +| +$)/g, '');
            if (!text_) {
                error('Request is empty');
                return;
            }
            value(text_);
            include(DENIED, {
                type: 'json',
                success: function(data) {
                    data = data.denied.values;
                    for (var i = 0; i < data.length; i++) {
                        if (data[i] == text_) {
                            error('Word is denied');
                            return;
                        } 
                    }
                },
                failure: function() {
                    error('Failed to load stop words');
                    return;
                }
            });
        ]]></xrl:field> -->
    </xrl:form>


    <xrl:slice>
        <xrl:variable name="form"><xrl:apply name="request-form" /></xrl:variable>
        <xrl:apply name="page">
            <xrl:with-param name="t" select="$form/text/value" />
            <xrl:with-param name="f" select="$form" />
            <xrl:with-param name="d">
                <xrl:if test="not($form/*/error)">
                    <xrl:include href="{$TWITTER}">
                        <xrl:with-param name="q"><xrl:value-of select="$form/text/value" /></xrl:with-param>
                        <xrl:with-param name="rpp" select="$form/count/value" />
                        <xrl:with-param name="include_entities">false</xrl:with-param>
                        <xrl:with-param name="result_type">recent</xrl:with-param>
                        <xrl:failure>
                            <failed />
                        </xrl:failure>
                    </xrl:include>
                </xrl:if>
            </xrl:with-param>
        </xrl:apply>
    </xrl:slice>


    <xrl:slice name="page">
        <xrl:param name="t" />
        <xrl:param name="f" />
        <xrl:param name="d" />
        <xrl:transform type="xslt" href="search.xsl">
            <page>
                <title>
                    <xrl:if test="$t">
                        <xrl:value-of select="$t" />
                        <xrl:text> – </xrl:text>
                    </xrl:if>
                    <xrl:text>Twitter Search</xrl:text>
                </title>
                <form>
                    <xrl:copy-of select="$f/text" />
                    <xrl:copy-of select="$f/count" />
                </form>
                
                <xrl:choose>
                    <xrl:when test="$d/*">
                        <data>
                            <xrl:copy-of select="$d/failed" />
                            <xrl:for-each select="$d/results">
                                <tweet>
                                    <date><xrl:value-of select="created_at" /></date>
                                    <user><xrl:value-of select="from_user" /></user>
                                    <text><xrl:value-of select="text" /></text>
                                </tweet>
                            </xrl:for-each>
                        </data>
                    </xrl:when>
                    <xrl:otherwise>
                        <help />
                    </xrl:otherwise>
                </xrl:choose>
            </page>
        </xrl:transform>
    </xrl:slice>

</xrl:requestsheet>
