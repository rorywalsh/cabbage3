<Cabbage>[
{"type": "form", "caption": "Button Example", "size": {"width": 380.0, "height": 300.0}, "guiMode": "queue", "colour": [2.0, 145.0, 209.0], "pluginId": "def1"},
{"type": "button", "bounds": {"left": 16.0, "top": 12.0, "width": 117.0, "height": 30.0}, "channel": "trigger", "text": {"off":"Start Synth", "on":"Stop Synth"}, "corners": 2.0},
{"type": "button", "bounds": {"left": 146.0, "top": 12.0, "width": 80.0, "height": 30.0}, "channel": "mute", "text": {"off": "Unmute", "on": "Mute"}, "corners": 2.0},
{"type": "button", "bounds": {"left": 240.0, "top": 12.0, "width": 121.0, "height": 30.0}, "channel": "toggleFreq", "text": "Toggle Freq"},
{"type": "texteditor", "bounds": {"left": 17.0, "top": 69.0, "width": 341.0, "height": 208.0}, "channel": "infoText", "readOnly": 1.0, "wrap": 1.0, "scrollbars": 1.0}
]
</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d -+rtmidi=NULL -M0 -m0d 
</CsOptions>
<CsInstruments>
; Initialize the global variables. 
ksmps = 32
nchnls = 2
0dbfs = 1

; Rory Walsh 2021 
;
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 

instr 1

    SText = {{This instrument shows an example of how buttons can be used in Cabbage. Button will send a 0 or a 1 when they are pressed. Typically you simply test if they have been pressed and do somthing accordingly. In this example, each time the Start Synth button is pressed Csound will either enable or disable the Synth instrument. The other two button show how the instrument can be controlled in real time.}} 
    cabbageSet "infoText", "text", SText

    kVal, kTrig cabbageGetValue "trigger"

    if kTrig == 1 then
        if kVal == 1 then
            event "i", "Synth", 0, 3600
        else
            turnoff2 nstrnum("Synth"), 0, 0
        endif
    endif

endin

instr Synth
    prints "Starting Synth"
    kMute cabbageGetValue "mute"
    a1 oscili .5*kMute, 300*(cabbageGetValue:k("toggleFreq")+1)
    outs a1, a1  
endin

                

</CsInstruments>
<CsScore>
;starts instrument 1 and runs it for a week
i1 0 z
</CsScore>
</CsoundSynthesizer>
