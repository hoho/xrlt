<?xml version="1.0" encoding="utf-8"?>
<xrl:requestsheet
    xmlns:xrl="http://xrlt.net/Transform"
    xmlns:xrltype="http://xrlt.net/Type">

    <xrl:param name="DENIED">file:denied.json</xrl:param>
    <xrl:param name="TWITTER">http://search.twitter.com/search.json</xrl:param>

    <xrl:slice>
        <xrl:variable name="form"><xrl:apply name="request-form" /></xrl:variable>
        <xrl:apply name="page">
            <xrl:with-param name="t" select="$form/text/value" />
            <xrl:with-param name="f" select="$form" />
            <xrl:with-param name="d">
                <xrl:if test="not($form/*/error)">
                    <data>
                        <xrl:include href="{$TWITTER}">
                            <xrl:with-param name="q"><xrl:value-of select="$form/text/value" /></xrl:with-param>
                            <xrl:with-param name="rpp" select="$form/count/value" />
                            <xrl:with-param name="include_entities">false</xrl:with-param>
                            <xrl:with-param name="result_type">recent</xrl:with-param>
                            <xrl:failure>
                                <failed />
                            </xrl:failure>
                            <xrl:success>
                                <xrl:for-each select="//results">
                                    <tweet>
                                        <date>
                                            <xrl:apply name="format-date">
                                                <xrl:with-param name="date" select="created_at" />
                                            </xrl:apply>
                                        </date>
                                        <user><xrl:value-of select="from_user" /></user>
                                        <text><xrl:value-of select="text" /></text>
                                    </tweet>
                                </xrl:for-each>
                            </xrl:success>
                        </xrl:include>
                    </data>
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
                    <xrl:if test="$t != ''">
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
                    <xrl:when test="$d/data">
                        <xrl:copy-of select="$d/data" />
                    </xrl:when>
                    <xrl:otherwise>
                        <help />
                    </xrl:otherwise>
                </xrl:choose>
            </page>
        </xrl:transform>
    </xrl:slice>

    <xrl:slice name="format-date" type="javascript">
        <xrl:param name="date" async="yes" />
        <![CDATA[
            var ret = new Deferred();
            date.then(function(val) {
                var d = new Date(Date.parse(val));
                var now = new Date();
                if ((now.getDate() == d.getDate()) && (now.getMonth() == d.getMonth()) &&
                    (now.getFullYear() == d.getFullYear())) {
                    ret.resolve(d.toLocaleTimeString());
                } else {
                    ret.resolve(d.toLocaleDateString());
                }
            });
            return ret;
        ]]>
    </xrl:slice>

    <xrl:form name="request-form">
        <xrl:field name="count" type="javascript"><![CDATA[
            count.then(function(val) {
                if (!val || !val.value) {
                    push('value', 5, true);
                    return;
                }
                var count_ = (val.value || '').replace(/(^[0 ]+| +$)/g, '');
                if (count_.match(/^\d+$/)) {
                    count_ = parseInt(count_, 10);
                    push('value', count_, true);
                    if (count_ > 100) {
                        push('error', 'Too large', true);
                    }
                } else {
                    push('error', 'Numbers only', true);
                }
            });
        ]]></xrl:field>

        <xrl:field name="text">
            <xrl:variable name="text_"><xrl:value-of select="normalize-space($text/value)" /></xrl:variable>
            <xrl:choose>
                <xrl:when test="$text_ = ''">
                    <xrl:push name="error" replace="yes" stop="yes">Request is empty</xrl:push>
                </xrl:when>
                <xrl:otherwise>
                    <xrl:push name="value" replace="yes" select="$text_" />
                    <xrl:include href="{$DENIED}" type="json">
                        <xrl:success>
                            <xrl:if test="//values = $text_">
                                <xrl:push name="error" replace="yes">Word is denied</xrl:push>
                            </xrl:if>
                        </xrl:success>
                        <xrl:failure>
                            <xrl:push name="error" replace="yes">Failed to load stop words</xrl:push>
                        </xrl:failure>
                    </xrl:include>
                </xrl:otherwise>
            </xrl:choose>
        </xrl:field>
        <!-- <xrl:field name="text" type="javascript"><![CDATA[
            text.then(function(val) {
                var text_ = ((val && val.value) || '').replace(/(^ +| +$)/g, '');
                if (!text_) {
                    push('error', 'Request is empty', true);
                    return;
                }
                push('value', text_, true);
                include(DENIED, {type: 'json'}).then(
                    function(data) {
                        data = data.denied.values;
                        for (var i = 0; i < data.length; i++) {
                            if (data[i] == text_) {
                                push('error', 'Word is denied', true);
                                return;
                            }
                        }
                    },
                    function() {
                        push('error', 'Failed to load stop words', true);
                        return;
                    }
                );
            });
        ]]></xrl:field> -->
    </xrl:form>

</xrl:requestsheet>
