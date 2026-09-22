<Cabbage>[
{"type": "form", "caption": "JSON Opcode Test", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"}
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
    Sdoc = "{\"name\":\"test\",\"freq\":440,\"strnum\":\"42\",\"amp\":0.5,\"ok\":true,\"nothing\":null,\"tags\":[\"a\",\"b\",\"c\"],\"gains\":[1,2.5,3],\"nodes\":[{\"id\":\"n1\",\"params\":{\"rate\":2.5}}]}"

    ; ---- string queries (i-time) ----
    Swave cabbageJsonGet Sdoc, "name"
    cabbageSet "ps_wave", "label.text", Swave
    Smiss cabbageJsonGet Sdoc, "nodes.5.nope"
    cabbageSet "ps_miss", "label.text", Smiss
    Samp cabbageJsonGet Sdoc, "amp"
    cabbageSet "ps_amp", "label.text", Samp
    Stags[] cabbageJsonGet Sdoc, "tags"
    cabbageSet "ps_tag0", "label.text", Stags[0]
    cabbageSet "ps_tag2", "label.text", Stags[2]
    Stype_rate cabbageJsonType Sdoc, "nodes.0.params.rate"
    cabbageSet "ps_type_rate", "label.text", Stype_rate
    Stype_tags cabbageJsonType Sdoc, "tags"
    cabbageSet "ps_type_tags", "label.text", Stype_tags
    Stype_nothing cabbageJsonType Sdoc, "nothing"
    cabbageSet "ps_type_nothing", "label.text", Stype_nothing
    Stype_miss cabbageJsonType Sdoc, "nope"
    cabbageSet "ps_type_miss", "label.text", Stype_miss

    ; ---- builders round-tripping through getters ----
    Sset cabbageJsonSet Sdoc, "nodes.0.params.rate", 99
    Snewrate cabbageJsonGet Sset, "nodes.0.params.rate"
    cabbageSet "ps_newrate", "label.text", Snewrate
    Sset2 cabbageJsonSet Sdoc, "brand.new.path", "hi"
    Snewpath cabbageJsonGet Sset2, "brand.new.path"
    cabbageSet "ps_newpath", "label.text", Snewpath
    Ssetf cabbageJsonSet Sdoc, "freq", 880
    Snewfreq cabbageJsonGet Ssetf, "freq"
    cabbageSet "ps_newfreq", "label.text", Snewfreq

    ; ---- invalid input degrades gracefully ----
    Sbad cabbageJsonGet "{oops", "a"
    cabbageSet "ps_bad", "label.text", Sbad

    ; ---- numeric queries (i-time) ----
    irate cabbageJsonGet Sdoc, "nodes.0.params.rate"
    cabbageSetValue "pr_rate", irate
    istrnum cabbageJsonGet Sdoc, "strnum"
    cabbageSetValue "pr_strnum", istrnum
    ibool cabbageJsonGet Sdoc, "ok"
    cabbageSetValue "pr_bool", ibool
    imiss cabbageJsonGet Sdoc, "nope"
    cabbageSetValue "pr_miss", imiss
    ihas1 cabbageJsonHas Sdoc, "nodes.0.id"
    cabbageSetValue "pr_has1", ihas1
    ihasnull cabbageJsonHas Sdoc, "nothing"
    cabbageSetValue "pr_hasnull", ihasnull
    ihas0 cabbageJsonHas Sdoc, "nope"
    cabbageSetValue "pr_has0", ihas0
    ilen cabbageJsonLen Sdoc, "nodes"
    cabbageSetValue "pr_len", ilen
    ilenobj cabbageJsonLen Sdoc, "nodes.0.params"
    cabbageSetValue "pr_lenobj", ilenobj
    ilen0 cabbageJsonLen Sdoc, "nope"
    cabbageSetValue "pr_len0", ilen0
    iempty lenarray Stags
    cabbageSetValue "pr_taglen", iempty
    StagsE[] cabbageJsonGet Sdoc, "name"
    iemptyE lenarray StagsE
    cabbageSetValue "pr_emptyarr", iemptyE

    ; ---- k-rate queries ----
    kfreq cabbageJsonGet Sdoc, "freq"
    cabbageSetValue "pr_freq", kfreq
    kgains[] cabbageJsonGet Sdoc, "gains"
    cabbageSetValue "pr_gain1", kgains[1]
    kcount init 0
    kcount += 1
    SdocA = "{\"v\":0}"
    SdocB = "{\"v\":1}"
    Sdoc2 = kcount % 2 == 0 ? SdocA : SdocB
    Strig, kT cabbageJsonGet Sdoc2, "v"
    kfires init 0
    if kT == 1 then
        kfires += 1
    endif
    cabbageSetValue "pr_trigfires", kfires
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
