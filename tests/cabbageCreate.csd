<Cabbage>
[
    {"type": "form", "caption": "SNow", "size": {"width": 600, "height": 300}, "pluginId": "DkPl"},
    {
        "type": "label",
        "text": "Stress test for cabbageCreate - Don't try this at home",
        "font":{
            "name": "Arial",
            "colour": "#ffffff",
            "size": 12
        },
        "fontSize": 24,
        "bounds": {"left": 10, "top": 10, "width": 600, "height": 30}
    }
]
</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -+rtmidi=NULL -M0 -dm0
</CsOptions>
<CsInstruments>
; sr set by host
ksmps = 64
nchnls = 2
0dbfs=1

snowballAmount@global:i = 50

instr    1
print chnget:i("WINDOW_HEIGHT")
    snowballCount:i = 0
    while (snowballCount < snowballAmount) do
        snowballCount += 1
        snowballSize:i = random(2, 15)
        cabbageCreate sprintf({{
        "channel": "image_%d",
        "type": "image",
        "colour": {"fill": "#ffffffff"},
        "corners": 5,
        "bounds": {"left": %d, "top": %d, "width": %d, "height": %d}
        }}, snowballCount, random:i(0, 600), random:i(0, 300), snowballSize, snowballSize)
    od
endin


instr 2
    count:k = 0
    if(metro(10) == 1) then
        while count < snowballAmount do
            count += 1
            channel:S = sprintfk("image_%d", count)
            yPos:k = cabbageGet(channel, "bounds.top");
            cabbageSet k(1), channel, "bounds.top", yPos
            if(yPos < 300) then
                yPos += 1
            else
                yPos = 0
            endif
            cabbageSet k(1), channel, "bounds.top", yPos
        od
    endif
endin

</CsInstruments>
<CsScore>
i1 2 1
i2 3 z
</CsScore>
</CsoundSynthesizer>
