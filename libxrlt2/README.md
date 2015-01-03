# XRLT2

## In progress

```json
RESPONSE
    CALL page title=('Hel' + 'lo')
        {}
            data1
                FETCH
                    HREF "/api/data"
                    METHOD "POST"
                    HEADER "Content-Type" "text/plain"
                    HEADER "Content-Length" (100 + 500)
                    PARAM "name" "value" type="GET"
                    BODY (JSON.stringify({a: 1, b: 2}))
                    SUCCESS $data
                        {}
                            ($data.key): ($data.value + $data.value2)
                            key2: ($data.value3)
                    ERROR
                        {}
                            error: true
            data2: "Some string"
            data3 []
                11
                22
                EACH $v ([444, 555])
                    ($v + $v)
                33
            data4 {}
                "key"
                    "value"
                EACH $k $v ({some: 'object', with: 'data'})
                    $k // Key.
                        $v // Value.
                    ($k + 2) // Key.
                        ($v + ' ' + Math.random()) // Value.


page $title
    CALL page2 ($title + $title)


EXTERNAL page2 $title
```

