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
    return f'document.getElementById("cv{id}.src").onchange=function(){{CheckVoltageSrc({id});}}\n' +\
           f'CheckVoltageSrc({id});\n\n'

# throw new Error("Not a valid SysEx file!");

# header
htm = '''
<!DOCTYPE html>
<html>
<body>

<p><b>LPZW Tram8 4x4 Firmware Version:</b> 0</p>

<hr>
<form name="sysex_in">
<p>Load an existing TRAM8 4x4 SysEx into this form
<input type="file" accept=".syx" onchange="load_sysex(this)">
</form>

<script>
const syx_head = Uint8Array.fromHex("F000297F5438465700");
const syx_tail = Uint8Array.fromHex("3A303030303030303146460D0AF7");

const cfg_head = Uint8Array.fromHex("900DF00D");
const cfg_tail = Uint8Array.fromHex("BAADF00D");

function isEqualArray(a, b) {
    if (a.length != b.length)
        return false;
    for (let i = 0; i < a.length; i++)
        if (a[i] != b[i])
            return false;
    return true;
}

function check_syx(sv) {
    const hdr = sv.subarray(0, syx_head.length)
    if (!isEqualArray(hdr, syx_head))
        return false;
    const tail = sv.subarray(sv.length - syx_tail.length);
    if (!isEqualArray(tail, syx_tail))
        return false;
    for (let i = 1; i < sv.length-1; ++i)
        if (sv[i] > 0x7F)
            return false;
    return true;
}

var SyxEx
var syx_line_offset = [];
var syx_line_length = [];
var syx_bytes;
var syx_cfg_offset;

function is_empty(ch) {
    return ch == 0x00 || ch == 0x0A;
}

function skip_empty(sv, off) {
    for (let i = off; i < sv.length; ++i)
        if (!is_empty(sv[i]))
            return i;
    return sv.length;
}

function report_bad(msg) {
    //window.alert(msg);
    throw new Error(msg);
}

function from_hex(ch) {
    if (ch >= 0x30 && ch <= 0x39)
        return ch - 0x30;
    else if (ch >= 0x41 && ch <= 0x46)
        return ch - 0x41 + 0x0A;
    else
        return report_bad('invalid hex');
}

function from_2hex(hi, lo) {
    return from_hex(hi) * 16 + from_hex(lo);
}

function decode_line(sv, off) {
    if (sv[off] != 0x3A) // ':'
        return report_bad("Bad line format!");
    const linelen = from_2hex(sv[off+1], sv[off+2]);
    var linepos = syx_bytes.byteLength;
    syx_bytes.resize(linepos + linelen);
    var store_bytes = new Uint8Array(syx_bytes);
    const lineend = off + 1 + (linelen + 4) * 2;
    var chksum = 0;
    for (let i = 1; i < 9; i += 2)
        chksum += from_2hex(sv[off+i], sv[off+i+1]);
    for (let i = off + 9; i < lineend; i += 2) {
        const b = from_2hex(sv[i], sv[i+1]);
        store_bytes[linepos++] = b;
        chksum += b;
    }
    if (linepos != syx_bytes.byteLength)
        return report_bad("Failed to decode the line at " + off + " linepos: " + linepos + " len: " + syx_bytes.byteLength);
    chksum += from_2hex(sv[lineend], sv[lineend + 1]);
    if (chksum & 0xFF)
        return report_bad("Invalid checksum of the line at " + off);
    return linelen;
}

function parse_lines(sv) {
    syx_line_offset.length = 0;
    syx_line_length.length = 0;
    const fw_fin = sv.length - syx_tail.length;
    var off = syx_head.length;
    var total_bytes = 0;
    while (off < fw_fin) {
        off = skip_empty(sv, off);
        if (off >= fw_fin)
            break;
        const linelen = decode_line(sv, off);
        syx_line_offset.push(off);
        syx_line_length.push(linelen);
        off += 1 + (linelen + 5)*2;
        total_bytes += linelen;
        if (!is_empty(sv[off]))
            return report_bad("Bad line len " + linelen + " " + off);
    }
    return total_bytes;
}

function find_cfg(v8) {
    var i = v8.indexOf(cfg_head[0]);
    while (i >= 0) {
        const head = v8.subarray(i, i + cfg_head.length);
        if (!isEqualArray(head, cfg_head)) {
            i = v8.indexOf(cfg_head[0], i + 1);
            continue;
        }
        const tail = v8.subarray(i + 32, i + 32 + cfg_tail.length);
        if (!isEqualArray(tail, cfg_tail)) {
            i = v8.indexOf(cfg_head[0], i + cfg_head.length);
            continue;
        }
        return i;
    }
    return report_bad("Cannot find configuration block!");
}

function parse_cfg(sv) {
    var syx8 = new Uint8Array(syx_bytes);
    syx_cfg_offset = find_cfg(syx8);
    const cfg_off = syx_cfg_offset
    window.alert("Found cfg block at " + cfg_off);
    const ver = syx8[cfg_off + 4];
    if (ver != 0)
        return report_bad("Wrong firmware version!");

    const cvRange = syx8[cfg_off + 5] >> 4;
    if (cvRange)
        document.getElementById("global.cvrange").value = 8;
    else
        document.getElementById("global.cvrange").value = 5;
    
    const midiChannel = (syx8[cfg_off + 5] & 0x0F) + 1;
    document.getElementById("global.channel").value = midiChannel;

    const triggerLength = syx8[cfg_off + 6];
    document.getElementById("global.triglen").value = triggerLength;
    
    // gates
    
    // voltages
    const cv_off = cfg_off + 24;
    for (let i = 0; i < 8; ++i) {
        const note_or_cc = syx8[cv_off + i];
        const isnote = (note_or_cc < 0x80);
        const id = i + 1;
        if (isnote) {
            document.getElementById("cv" + id + ".src").value = 0;
    	    document.getElementById("cv" + id + ".note").value = note_or_cc;
        } else { 
            document.getElementById("cv" + id + ".src").value = 1;
	        document.getElementById("cv" + id + ".cc").value = note_or_cc & 0x7F;
	    }
    	document.getElementById("cv" + id + ".note").disabled = !isnote;
	    document.getElementById("cv" + id + ".cc").disabled = isnote;
    }
}

function parse_sysex(sv) {
    try {
        if (!check_syx(sv))
            return report_bad("Bad food!");
        syx_bytes = new ArrayBuffer(0, { maxByteLength: 65536 });
        const total_bytes = parse_lines(sv);
        parse_cfg(sv);
        SysEx = sv;
        window.alert("Zehr gut! Lines:" + syx_line_length.length + " Bytes: " + total_bytes);
    } catch (err) {
        window.alert(err);
        syx_bytes = undefined;
        return;
    }
}

function load_sysex(inp) {
    var fr = new FileReader();
    fr.onload = function() { parse_sysex(new Uint8Array(fr.result)); }
    fr.readAsArrayBuffer(inp.files[0]);
}
</script>
'''

# globals
htm += f'''
<form name="sysex_out">

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
<td>{mkchan("global.channel", 1)}</td>
<td></td>
<td>{mktrig("global.triglen", 1)}</td>
<td></td>
<td>{mkcvrange("global.cvrange", 5)}</td>
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
function CheckGateMode(id) {
	const d = (document.getElementById("gate" + id + ".mode").value > 1);
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
function CheckVoltageSrc(id) {
	const isnote = (document.getElementById("cv" + id + ".src").value == 0);
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
<hr>
<input type="submit" value="Get the configured SysEx">
</form>

</body>
</html>
'''

print(htm)