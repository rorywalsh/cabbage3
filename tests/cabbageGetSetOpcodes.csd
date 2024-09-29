<Cabbage>[
    {"type":"form","caption":"Simple Instrument","size":{"width":1000,"height":520},"pluginId":"RMSy"},
    {"type":"rotarySlider", "channel":"gain", "bounds":{"left":150, "top":10, "width":100, "height":100}, "range":{"min":0, "max":2, "defaultValue":1, "skew":1, "increment":0.1}, "text":"Gain"},
    {"type":"button", "channel":"button1", "bounds":{"left":0, "top":10, "width":100, "height":30}, "colour":{"on":[255, 0, 0], "off":"#0000ff"},"text":{"on":"I am on", "off":"I am off"}},
    {"type": "texteditor", "bounds": {"left": 17.0, "top": 169.0, "width": 341.0, "height": 40.0}, "channel": "infoText", "readOnly": 1.0, "wrap": 1.0, "scrollbars": 1.0, "text":"This instrument shows an example..."},
    {"type":"combobox", "channel":"combo1", "bounds":{"left":200, "top":200, "width":100, "height":30}, "items":["One", "Two", "Three"]},
    {"type":"csoundoutput", "channel":"output1", "bounds":{"left":10, "top":300, "width":800, "height":200}}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-dm0 -n -+rtmidi=NULL -M0 --midi-key=4 --midi-velocity=5
</CsOptions>
<CsInstruments>
; sr set by host
ksmps = 16
nchnls = 2
0dbfs = 1

/*
Test various string variants of cabbageSet opcodes. 
These opcodes provide an interface to widget properties. 
*/
instr TestStringSetOpcodes
    if p4 == 0 then
        cabbageSet "button1", "text.off", "p4 is 0"
    elseif p4 == 1 then
        cabbageSet "button1", "text.off", "p4 is 1"
    elseif p4 == 2 then
        cabbageSet "infoText", "text", "p4 is 2"
    elseif p4 == 3 then
        cabbageSet "infoText", sprintf({{"text":"%s"}}, "p4 is 3")
    elseif p4 == 4 then
        cabbageSet metro(1), "infoText", "text", "p4 is 4"
    elseif p4 == 5 then
        cabbageSet metro(1), "infoText", sprintf({{"text":"%s"}}, "p4 is 5")
    elseif p4 == 6 then
        cabbageSet "button1", "colour.off", "#00ff00"
    endif
endin

/*
Test various MYFLT variants of cabbageSet opcodes.
These opcodes provide an interface to widget properties. 
*/
instr TestMYFLTSetOpcodes
    kToggle = (oscili:k(1, 2) > 0 ? 1 : 0)
    if p4 == 0 then
        cabbageSet metro(10), "gain", "bounds.left", abs(randi:k(400, 10))
    elseif p4 == 1 then
        cabbageSet metro(10), "gain", "visible", kToggle
    elseif p4 == 2 then
        cabbageSet "gain", "bounds.left", 10
    elseif p4 == 3 then
        cabbageSet "gain", sprintf({{"visible":%d}}, 0)
    elseif p4 == 4 then
        cabbageSet "gain", sprintf({{"visible":%d}}, 1)
    endif
endin

instr TestMYFLTGetOpcodes
    if p4 == 0 then
        SText cabbageGet "infoText", "text"
        prints SText
    elseif p4 == 1 then
        SText cabbageGet "button1", "text.off"
        prints SText
    elseif p4 == 2 then
        iWidth cabbageGet "infoText", "bounds.width"
        print iWidth
    elseif p4 == 3 then
        kWidth cabbageGet "infoText", "bounds.width"
        printf "bounds.width: %d", metro(2), kWidth
    elseif p4 == 4 then
        kVisible cabbageGet "infoText", "visible"
        printf "visible: %d", metro(2), kVisible
    elseif p4 == 5 then
        //update text and check for changes
        event_i "i", "UpdateText", 0.1, 0
        SText, kTrigger cabbageGet "infoText", "text"
        printf "text: %s", kTrigger, SText
    elseif p4 == 6 then
        cabbageSet "gain", "colour", "#ff0000"
    elseif p4 == 7 then
        cabbageSet "combo1", "items", "1One", "2Two", "3Three", "4Four"
    endif
endin

instr TestGetValueOpcodes
    kVal cabbageGetValue "gain"
    kVal, kTrig cabbageGetValue "gain"
endin

instr TestSetValueOpcodes
    if p4 == 0 then
        prints "Testing cabbageSetValue i-rate"
        cabbageSetValue "gain", 2
    elseif p4 == 1 then
        prints "Testing cabbageSetValue k-rate"
        cabbageSetValue "gain", abs(randi:k(1, 10))
    endif
endin


instr UpdateText
    cabbageSet "infoText", "text", "This text has been updated" 
endin


</CsInstruments>  
<CsScore>
; i"TestStringSetOpcodes" 0 1 0
; i"TestStringSetOpcodes" + 1 1
; i"TestStringSetOpcodes" + 1 2
; i"TestStringSetOpcodes" + 1 3
; i"TestStringSetOpcodes" + 1 4
; i"TestStringSetOpcodes" + 1 5
; i"TestStringSetOpcodes" + 1 6
; i"TestStringSetOpcodes" + 1 7

; i"TestMYFLTSetOpcodes" 0 2 0
; i"TestMYFLTSetOpcodes" + 2 1
; i"TestMYFLTSetOpcodes" + 1 2
; i"TestMYFLTSetOpcodes" + 1 3
; i"TestMYFLTSetOpcodes" + 1 4

; i"TestMYFLTGetOpcodes" 1 1 0
; i"TestMYFLTGetOpcodes" + 1 1
; i"TestMYFLTGetOpcodes" + 1 2
; i"TestMYFLTGetOpcodes" + 1 3
; i"TestMYFLTGetOpcodes" + 1 4
; i"TestMYFLTGetOpcodes" + 1 5
; i"TestSetValueOpcodes" + 2 0
i"TestSetValueOpcodes" + 4 1
f0 z
</CsScore>
</CsoundSynthesizer>