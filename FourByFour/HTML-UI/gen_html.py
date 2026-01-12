def incrange(start, incl_end, step=1):
    return list(range(start, incl_end + step, step))


def mkchan(name, def_chan):
    out = f'<select name="{name}" id="{name}">\n'
    for i in incrange(1,16):
        sel = ' selected' if i == def_chan else ''
        out += f'<option value="{str(i)}"{sel}>Channel {str(i)}</option>\n'
    out+= '</select>'
    return out

def mktrig(name, def_val):
    vals = incrange(1,10) + incrange(15,120,5)
    out = f'<select name="{name}" id="{name}">\n'
    for v in vals:
        sel = ' selected' if v == def_val else ''
        out += f'<option value="{str(v)}"{sel}>{str(v)} ms</option>\n'
    out+= '</select>'
    return out

def mkgatemode(name, def_val):
    vals=[('Gate', 0), ('Trigger', 1), ('RUN gate', 2), ('RESET trigger', 3)]
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        sel = ' selected' if i == def_val else ''
        out += f'<option value="{str(i)}"{sel}>{v}</option>\n'
    out+= '</select>'
    return out

def mkgatesrc(name, def_val):
    vals=[('Note On', 0), ('Clock Pulses (24 ppqn)', 1)]
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        sel = ' selected' if i == def_val else ''
        out += f'<option value="{str(i)}"{sel}>{v}</option>\n'
    out+= '</select>'
    return out

def mknotes(name, def_val):
    out = f'<select name="{name}" id="{name}">\n'
    notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    for i in range(0, 128):
        octave = i // 12
        note = i % 12
        if note == 0:
            out += f'<optgroup label="Octave {str(octave)}">'
        v = f'{notes[note]}{str(octave)}'
        sel = ' selected' if i == def_val else ''
        out += f'<option value="{str(i)}"{sel}>{v} ({str(i)})</option>\n'
        if note == 11:
            out += '</optgroup>'
    out+= '</select>'
    return out

CCs = {
1 : 'ModWheel',
}
noCCs = set([120, 123])

def mkccs(name, def_val):
    out = f'<select name="{name}" id="{name}">\n'
    for i in range(0, 128):
        if i in noCCs: continue
        ccname = CCs.get(i)
        v = f'{i} ({ccname})' if ccname else f'{i}' 
        sel = ' selected' if i == def_val else ''
        out += f'<option value={i}{sel}>{v}</option>\n'
    out+= '</select>'
    return out
    
def mkgate_html(id, mode, src, note, pulses=0):
    return f'<tr><td></td><td>{id}</td><td>' +\
mkgatemode(f"gate{id}.mode", mode) + '</td><td>' +\
mkgatesrc(f"gate{id}.src", src) + '</td><td>' +\
mknotes(f"gate{id}.note", note) + '</td><td>' +\
f'<input type="number" min=1 max=127 name="gate{id}.pulses" id="gate{id}.pulses" value={pulses}>' +\
'</td></tr>\n'
    
def mkgate_js(id):
    return f'document.getElementById("gate{id}.mode").onchange=function(){{CheckGateMode({id});}}\n' +\
f'document.getElementById("gate{id}.src").onchange=function(){{CheckGateMode({id});}}\n' +\
f'CheckGateMode({id});\n\n'

def mkcvrange(name, def_val):
    vals=[('0-5V', 5), ('0-8V', 8)]
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        sel = ' selected' if i == def_val else ''
        out += f'<option value="{str(i)}"{sel}>{v}</option>\n'
    out+= '</select>'
    return out

def mkcvsrc(name, def_val):
    vals=[('Note Velocity', 0), ('Control Change', 1)]
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        sel = ' selected' if i == def_val else ''
        out += f'<option value="{str(i)}"{sel}>{v}</option>\n'
    out+= '</select>'
    return out

def mkcv_html(id, src, note, cc=0):
    return f'<tr><td></td><td>{id}</td><td>' +\
mkcvsrc(f"cv{id}.src", src) + '</td><td>' +\
mknotes(f"cv{id}.note", note) + '</td><td>' +\
mkccs(f"cv{id}.cc", cc) + '</td><td>' +\
'</td></tr>\n'
    
def mkcv_js(id):
    return f'document.getElementById("gate{id}.mode").onchange=function(){{CheckVoltageSrc({id});}}\n' +\
f'document.getElementById("gate{id}.src").onchange=function(){{CheckVoltageSrc({id});}}\n' +\
f'CheckVoltageSrc({id});\n\n'


    
# header
htm = '''
<!DOCTYPE html>
<html>
<body>

<p><b>LPZW Tram8 4x4 Firmware Version:</b> 0</p>
'''

# globals
htm += f'''

<hr>

<p><b>GLOBALS</b></p>
<table>
<tr>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the input MIDI channel to use for all mapings'">MIDI Channel</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the millisecond duration for trigger outputs">Trigger Length</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the maximum Control Voltage value">Control Voltage Range</td>
</tr>
<tr>
<td></td>
<td>{mkchan("global.channel", 10)}</td>
<td></td>
<td>{mktrig("global.triglen", 10)}</td>
<td></td>
<td>{mkcvrange("global.cvrange", 8)}</td>
</tr>
</table>
'''

# gates
htm += f'''
<hr>

<p>
<b>GATE/TRIGGER OUTPUTS</b><br>
<i>NOTE: Gate from a Clock source produces a flip-flop (a square LFO with the period twice the gate duration)</i>
</p>

<table>
<tr>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Gate output">Gate Output</td>
<td title="Select gate output mode">Mode</td>
<td title="Select the activation source">Source</td>
<td title="Note Number">Note</td>
<td title="Clock: Number of Pulses between activations)">Clock Pulses</td>
</tr>
'''

htm += mkgate_html(1, 1, 0, 36)
htm += mkgate_html(2, 1, 0, 38)
htm += mkgate_html(3, 1, 0, 42)
htm += mkgate_html(4, 1, 0, 46)
htm += mkgate_html(5, 1, 1, 0, 16)
htm += mkgate_html(6, 1, 1, 0, 24)
htm += mkgate_html(7, 2, 1, 0, 0)
htm += mkgate_html(8, 3, 1, 0, 0)

htm+='''
<script language="javascript">
var o_form = document.forms[1];
function CheckGateMode(id) {
	var d = (document.getElementById("gate" + id + ".mode").value > "1");
	document.getElementById("gate" + id + ".src").disabled = d;
	if (d) {
	   document.getElementById("gate" + id + ".note").disabled = d;
	   document.getElementById("gate" + id + ".pulses").disabled = d;
    } else {
	   var isnote = (document.getElementById("gate" + id + ".src").value == 0);
	   document.getElementById("gate" + id + ".note").disabled = !isnote;
	   document.getElementById("gate" + id + ".pulses").disabled = isnote;
    }
}
'''

for i in incrange(1, 8):
    htm += mkgate_js(str(i))

htm+='''
</script>
</table>
'''

# control voltages
htm += '''
<hr>

<p>
<b>CONTROL VOLTAGE OUTPUTS</b></p>

<table>
<tr>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="CV output">CV Output</td>
<td title="Select CV Source">CV Source</td>
<td title="Note Number">Note</td>
<td title="CC number">CC Number</td>
</tr>
'''

htm += mkcv_html(1, 0, 36)
htm += mkcv_html(2, 0, 38)
htm += mkcv_html(3, 0, 42)
htm += mkcv_html(4, 0, 46)
htm += mkcv_html(5, 1, 0, 24)
htm += mkcv_html(6, 1, 0, 29)
htm += mkcv_html(7, 1, 0, 63)
htm += mkcv_html(8, 1, 0, 82)

htm+='''
<script language="javascript">
var o_form = document.forms[1];
function CheckVoltageSrc(id) {
	var isnote = (document.getElementById("cv" + id + ".src").value == "0");
	document.getElementById("cv" + id + ".note").disabled = !isnote;
	document.getElementById("cv" + id + ".cc").disabled = isnote;
}
'''

for i in incrange(1, 8):
    htm += mkcv_js(str(i))

htm+='''
</script>
</table>
'''

# footer
htm+='''
</body>
</html>
'''

print(htm)