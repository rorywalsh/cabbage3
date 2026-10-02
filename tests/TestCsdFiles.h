#pragma once
#include <string>

namespace TestCsdFiles {

const std::string basicOscillator = R"(
<Cabbage>[
{"type": "form", "caption": "Dummy Example", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
    aout oscili 0.1, 440
    outs aout, aout
endin
</CsInstruments>
<CsScore>
i 1 0 10
</CsScore>
</CsoundSynthesizer>
)";

const std::string rotarySliders = R"(
<Cabbage>[
{"type": "form", "caption": "Slider Example", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"},
{"type": "rotarySlider", "bounds": {"left": 20.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic1", "range": {"min": 0.0, "max": 1.0,  "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 100.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic2", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 180.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic3", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 260.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic4", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 20.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic5", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 100.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic6", "range": {"min": 0.0, "max": 1.0,"defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 180.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic7", "range": {"min": 0.0, "max": 1.0,"defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]},
{"type": "rotarySlider", "bounds": {"left": 260.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channels": [{"event": "valueChanged", "id": "harmonic8", "range": {"min": 0.0, "max": 1.0,"defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}]}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
; Initialize the global variables. 
ksmps = 1
nchnls = 2
0dbfs = 1

; Rory Walsh 2021 
;
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 

instr 1   
    printf("Harmonic1 value: %f\n", cabbageGetValue:k("harmonic1"), cabbageGetValue:k("harmonic1"))
    printf("Harmonic2 value: %f\n", cabbageGetValue:k("harmonic2"), cabbageGetValue:k("harmonic2"))
    printf("Harmonic3 value: %f\n", cabbageGetValue:k("harmonic3"), cabbageGetValue:k("harmonic3"))
    printf("Harmonic4 value: %f\n", cabbageGetValue:k("harmonic4"), cabbageGetValue:k("harmonic4"))
    printf("Harmonic5 value: %f\n", cabbageGetValue:k("harmonic5"), cabbageGetValue:k("harmonic5"))
    printf("Harmonic6 value: %f\n", cabbageGetValue:k("harmonic6"), cabbageGetValue:k("harmonic6"))
    printf("Harmonic7 value: %f\n", cabbageGetValue:k("harmonic7"), cabbageGetValue:k("harmonic7"))
    printf("Harmonic8 value: %f\n", cabbageGetValue:k("harmonic8"), cabbageGetValue:k("harmonic8"))
endin       

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 z
</CsScore>
</CsoundSynthesizer>

)";

const std::string cabbageSet = R"(
<Cabbage>[
{"type": "form", "caption": "Slider Example", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
; Initialize the global variables. 
ksmps = 1
nchnls = 2
0dbfs = 1

; Rory Walsh 2021 
;
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 

instr 1   
    cabbageSet "dummy", "colour.fill", "222222"
    cabbageSet "dummy", "text", "Hello from Csound!"
    cabbageSetValue "dummy", oscili:k(1, 10)
endin           

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
;starts instrument 1 and runs it for a week
i1 0 .1
</CsScore>
</CsoundSynthesizer>

)";
const std::string cabbageJson = R"(
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

    Sset cabbageJsonSet Sdoc, "nodes.0.params.rate", 99
    Snewrate cabbageJsonGet Sset, "nodes.0.params.rate"
    cabbageSet "ps_newrate", "label.text", Snewrate
    Sset2 cabbageJsonSet Sdoc, "brand.new.path", "hi"
    Snewpath cabbageJsonGet Sset2, "brand.new.path"
    cabbageSet "ps_newpath", "label.text", Snewpath
    Ssetf cabbageJsonSet Sdoc, "freq", 880
    Snewfreq cabbageJsonGet Ssetf, "freq"
    cabbageSet "ps_newfreq", "label.text", Snewfreq

    Sbad cabbageJsonGet "{oops", "a"
    cabbageSet "ps_bad", "label.text", Sbad

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

)";
const std::string cabbageJsonFunc = R"(
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

)";

// Side-by-side legacy (kVal, kTrig) vs struct (res:CabbageNumTrig /
// res:CabbageStrTrig) outputs for cabbageGetValue. Both forms are read in the
// same instrument from the same channels, so any semantic drift shows up as a
// non-zero error count published via cabbageSetValue.
const std::string trigStructs = R"(
<Cabbage>[
{"type": "form", "caption": "Trig Structs", "size": {"width": 360.0, "height": 200.0}, "guiMode": "queue", "pluginId": "def1"},
{"type": "label", "channels": [{"event": "valueChanged", "id": "tslbl"}], "bounds": {"left": 10.0, "top": 10.0, "width": 300.0, "height": 30.0}, "text": "struct trigger comparison"}
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
    kCnt init 0
    kCnt = kCnt + 1

    ; numeric channel steps 0 -> 1 -> 2 every 4 k-cycles
    kMod = kCnt % 12
    kPhas = 0
    if kMod >= 4 && kMod < 8 then
        kPhas = 1
    elseif kMod >= 8 then
        kPhas = 2
    endif
    chnset kPhas, "tsIn"

    ; string channel alternates every 4 k-cycles (S assignment inside a k-branch
    ; is lowered to init-time only; a k-conditional S expression re-evaluates)
    Stmp = kCnt % 8 < 4 ? "hello" : "world"
    chnset Stmp, "tsStr"

    ; --- cabbageGetValue num: legacy vs struct
    kLegVal, kLegTrig cabbageGetValue "tsIn"
    numRes:CabbageNumTrig cabbageGetValue "tsIn"
    kNumErr init 0
    if kLegVal != numRes.val || kLegTrig != numRes.trig then
        kNumErr = kNumErr + 1
    endif

    ; --- cabbageGetValue str: legacy vs struct
    SLegVal, kStrLegTrig cabbageGetValue "tsStr"
    strRes:CabbageStrTrig cabbageGetValue "tsStr"
    kStrErr init 0
    if kStrLegTrig != strRes.trig || strcmpk(SLegVal, strRes.val) != 0 then
        kStrErr = kStrErr + 1
    endif

    ; every form must actually fire (drives the > 0 assertions)
    kNumTrigLegacy init 0
    kNumTrigLegacy = kNumTrigLegacy + kLegTrig
    kNumTrigStruct init 0
    kNumTrigStruct = kNumTrigStruct + numRes.trig
    kStrTrigLegacy init 0
    kStrTrigLegacy = kStrTrigLegacy + kStrLegTrig
    kStrTrigStruct init 0
    kStrTrigStruct = kStrTrigStruct + strRes.trig

    kCycles init 0
    kCycles = kCycles + 1

    cabbageSetValue "tsNumErr", kNumErr
    cabbageSetValue "tsStrErr", kStrErr
    cabbageSetValue "tsNumTrigLegacy", kNumTrigLegacy
    cabbageSetValue "tsNumTrigStruct", kNumTrigStruct
    cabbageSetValue "tsStrTrigLegacy", kStrTrigLegacy
    cabbageSetValue "tsStrTrigStruct", kStrTrigStruct
    cabbageSetValue "tsCycles", kCycles
endin

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>

)";

// Struct-only coverage for the widget-property and JSON trigger forms
// (cabbageGet / cabbageJsonGet). These two share the process-wide
// TriggerMemo with their legacy siblings, so a side-by-side pairing in one
// run would let only the first reader of each key observe the edge; this CSD
// therefore exercises the struct forms alone against known expected values.
const std::string trigStructsSolo = R"(
<Cabbage>[
{"type": "form", "caption": "Trig Struct Solo", "size": {"width": 360.0, "height": 220.0}, "guiMode": "queue", "pluginId": "def2"},
{"type": "textEditor", "channels": [{"event": "valueChanged", "id": "twg1"}], "bounds": {"left": 10.0, "top": 10.0, "width": 200.0, "height": 40.0}, "text": "alpha", "readOnly": 1.0, "wrap": 1.0, "scrollbars": 1.0}
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
    ; widget property form: fires on first sight and when instr 2 changes the text
    propRes:CabbageStrTrig cabbageGet "twg1", "text"
    kPropFires init 0
    if propRes.trig == 1 then
        kPropFires = kPropFires + 1
    endif
    kPropIsBeta init 0
    if strcmpk(propRes.val, "beta") == 0 then
        kPropIsBeta = 1
    endif

    ; JSON form: document alternates every 4 k-cycles, value must track it
    kJCnt init 0
    kJCnt = kJCnt + 1
    Sja = "{\"v\":\"zero\"}"
    Sjb = "{\"v\":\"one\"}"
    Sjd = kJCnt % 8 < 4 ? Sja : Sjb
    jsonRes:CabbageStrTrig cabbageJsonGet Sjd, "v"
    kJsonFires init 0
    if jsonRes.trig == 1 then
        kJsonFires = kJsonFires + 1
    endif
    kJsonErr init 0
    if kJCnt % 8 < 4 then
        if strcmpk(jsonRes.val, "zero") != 0 then
            kJsonErr = kJsonErr + 1
        endif
    else
        if strcmpk(jsonRes.val, "one") != 0 then
            kJsonErr = kJsonErr + 1
        endif
    endif

    kCycles init 0
    kCycles = kCycles + 1

    cabbageSetValue "twgPropFires", kPropFires
    cabbageSetValue "twgPropIsBeta", kPropIsBeta
    cabbageSetValue "twgJsonFires", kJsonFires
    cabbageSetValue "twgJsonErr", kJsonErr
    cabbageSetValue "twgCycles", kCycles
endin

instr 2
    cabbageSet "twg1", "text", "beta"
endin

</CsInstruments>
<CsScore>
i1 0 10
i2 1 1
</CsScore>
</CsoundSynthesizer>

)";
} // namespace TestCsdFiles