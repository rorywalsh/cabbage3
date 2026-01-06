<Cabbage>
[
    {
        "type": "form",
        "caption": "Checkbox Default Test",
        "size": {"width": 400, "height": 200},
        "guiMode": "queue",
        "pluginId": "chktest"
    },
    {
        "type": "checkBox",
        "bounds": {"left": 20, "top": 40, "width": 150, "height": 30},
        "channels": [
            {
                "id": "testCheckbox",
                "event": "valueChanged",
                "range": {"min": 0, "max": 1, "defaultValue": 1, "skew": 1, "increment": 1}
            }
        ],
        "text": "Should be checked",
        "corners": 2
    },
    {
        "type": "label",
        "bounds": {"left": 20, "top": 80, "width": 360, "height": 30},
        "text": "This checkbox should appear checked by default",
        "fontSize": 14
    }
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

instr 1
    kCheckbox cabbageGetValue "testCheckbox"
    printk2 kCheckbox
endin

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 [60*60*24*7]
</CsScore>
</CsoundSynthesizer>