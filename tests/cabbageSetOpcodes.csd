<Cabbage>[
    {"type":"form","caption":"Simple Instrument","size":[500,300],"pluginId":"RMSy"},
    {"type":"rotarySlider", "channel":"gain", "bounds":{"left":150, "top":10, "width":100, "height":100}, "range":{"min":0, "max":2, "value":1, "skew":1, "increment":0.1}, "text":"Gain"},
    {"type":"button", "channel":"button1", "bounds":{"left":0, "top":10, "width":100, "height":30}, "colour":{"on":[255, 0, 0], "off":"#0000ff"},"text":{"on":"I am on", "off":"I am off"}},
    {"type": "texteditor", "bounds": {"left": 17.0, "top": 169.0, "width": 341.0, "height": 40.0}, "channel": "infoText", "readOnly": 1.0, "wrap": 1.0, "scrollbars": 1.0},
    {"type":"combobox", "channel":"combo1", "bounds":{"left":200, "top":200, "width":100, "height":30}, "items":["One", "Two", "Three"]}
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


instr TestStringOpcodes
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

instr TestMYFLTOpcodes
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
    elseif p4 == 5 then
        cabbageSet "gain", "colour", "#ff0000"
    elseif p4 == 6 then
        cabbageSet "combo1", "items", "1One", "2Two", "3Three", "4Four"
    endif
endin


</CsInstruments>  
<CsScore>
; i"TestStringOpcodes" 0 1 0
; i"TestStringOpcodes" + 1 1
; i"TestStringOpcodes" + 1 2
; i"TestStringOpcodes" + 1 3
; i"TestStringOpcodes" + 1 4
; i"TestStringOpcodes" + 1 5
; i"TestStringOpcodes" + 1 6
; i"TestMYFLTOpcodes" 0 2 0
; i"TestMYFLTOpcodes" + 2 1
; i"TestMYFLTOpcodes" + 1 2
; i"TestMYFLTOpcodes" + 1 3
i"TestMYFLTOpcodes" + 1 4
i"TestMYFLTOpcodes" + 1 5
i"TestMYFLTOpcodes" + 1 6
</CsScore>
</CsoundSynthesizer>