<Cabbage>[
{"type": "form", "caption": "JSON Functional Syntax Probe", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
    doc:S = "{\"name\":\"test\",\"freq\":440,\"tags\":[\"a\",\"b\"]}"
    wave:S = cabbageJsonGet(doc, "name")
    cabbageSet "ps_fwave", "label.text", wave
    len:i = cabbageJsonLen(doc, "tags")
    cabbageSetValue "pr_flen", len
    type:S = cabbageJsonType(doc, "freq")
    cabbageSet "ps_ftype", "label.text", type
    has:i = cabbageJsonHas(doc, "name")
    cabbageSetValue "pr_fhas", has
    freq:k = cabbageJsonGet(doc, "freq")
    cabbageSetValue "pr_ffreq", freq
    doc2:S = cabbageJsonSet(doc, "freq", 880)
    newfreq:i = cabbageJsonGet(doc2, "freq")
    cabbageSetValue "pr_fnewfreq", newfreq
    id:S, trig:k = cabbageJsonGet(doc, "name")
    cabbageSet trig, "ps_fid", "label.text", id
    fires:k init 0
    if trig == 1 then
        fires = 1
    endif
    cabbageSetValue "pr_ftrigseen", fires
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
