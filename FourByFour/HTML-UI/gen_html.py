# LPZW TRAM8 4x4 firmware
# Offline HTML-based SysEx configurator generator.
# CC BY-NC-ND 4.0 Dmitry S. Baikov
# aka c0ff
# Initially written in early January 2026
#
# https://github.com/c0ff/TRAM8-4x4
#
import argparse
import binascii

CONFIGURATION_FORMAT = 7
CONFIGURATION_FORMAT_MIN = 7
CONFIGURATION_LENGTH = 56

CONFIGURATION_FN_PREFIX = f'tram8_4x4_fw{CONFIGURATION_FORMAT}'

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

def mkclockmode(name):
    vals=[('Trigger', 0), ('FlipFlop', 1), ('RESET trigger', 0x80), ('RUN gate', 0x81)]
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        out += f'<option value="{str(i)}">{v}</option>\n'
    out+= '</select>'
    return out

def mknotemode(name):
    vals=[('Trigger', 0), ('Gate', 1), ('Velocity', 2), ('Dynamic Gate', 3)] # TODO , ('Pitch', 4)
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        out += f'<option value="{str(i)}">{v}</option>\n'
    out+= '</select>'
    return out

def mksrc(name):
    vals=[('Note On', 0x90), ('After Touch', 0xA0), ('Control Change', 0xB0), ('Channel Pressure', 0xD0), ('Clock Divider', 0xF0)] # TODO: Pitch Wheel 0xE0
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        out += f'<option value="{str(i)}">{v}</option>\n'
    out+= '</select>'
    return out

def mknotes(name):
    out = f'<select name="{name}" id="{name}">\n'
    notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    for i in range(0, 128):
        octave = i // 12
        note = i % 12
        if note == 0:
            out += f'<optgroup label="Octave {str(octave)}">'
        v = f'{notes[note]}{str(octave)}'
        out += f'<option value="{str(i)}">{v} ({str(i)})</option>\n'
        if note == 11:
            out += '</optgroup>'
    out+= '</select>'
    return out

# TODO: add well-known CC names
CCs = {
1 : 'ModWheel',
2 : 'Breath Controller (coarse)',
34: 'Breath Controller (fine)',
4 : 'Foot Pedal (coarse)',
36: 'Foot Pedal (fine)',
7 : 'Volume (coarse)',
39: 'Volume (fine)',
11: 'Expression (coarse)',
43: 'Expression (fine)',
12: 'Effect Control 1 (coarse)',
44: 'Effect Control 1 (fine)',
13: 'Effect Control 2 (coarse)',
45: 'Effect Control 2 (fine)',
}
noCCs = set([0, 32, 98, 99, 100, 101, 120, 121, 123])

def mkccs(name):
    out = f'<select name="{name}" id="{name}">\n'
    for i in range(0, 128):
        if i in noCCs: continue
        ccname = CCs.get(i)
        v = f'{i} ({ccname})' if ccname else f'{i}' 
        out += f'<option value={i}>{v}</option>\n'
    out+= '</select>'
    return out
    
def mkout_html(outtype, id):
    return f'<tr><td></td><td>{id}</td>' +\
'<td>' + mksrc(f"{outtype}{id}.src") + '</td>' +\
'<td>' + mkclockmode(f"{outtype}{id}.clkmode") + '</td>' +\
f'<td><input type="number" min=1 max=127 name="{outtype}{id}.pulses" id="{outtype}{id}.pulses" /></td>'+\
'<td>' + mknotes(f"{outtype}{id}.note") + '</td>' +\
'<td>' + mknotemode(f"{outtype}{id}.notemode") + '</td>' +\
'<td>' + mkccs(f"{outtype}{id}.cc") + '</td>' +\
f'<td><input type="checkbox" name="{outtype}{id}.usechan" id="{outtype}{id}.usechan" /></td>'+\
'<td>' + mkchan(f"{outtype}{id}.channel", 0) + '</td>'
'</tr>\n'
    
def mkout_js(oid):
    return f'document.getElementById("{oid}.src").onchange=function(){{CheckMode("{oid}");}};\n' +\
           f'document.getElementById("{oid}.clkmode").onchange=function(){{CheckMode("{oid}");}};\n' +\
           f'document.getElementById("{oid}.usechan").onchange=function(){{CheckMode("{oid}");}};\n'+\
           f'//CheckMode("{oid}");\n\n'

def mkcvrange(name, def_val):
    vals=[('0-5V', 5), ('0-8V', 8)]
    out = f'<select name="{name}" id="{name}">\n'
    for v,i in vals:
        sel = ' selected' if i == def_val else ''
        out += f'<option value="{str(i)}"{sel}>{v}</option>\n'
    out+= '</select>'
    return out

def mkglobalbool(name):
    return f'<input type="checkbox" name="{name}" id="{name}" />'

def mkclkscale(name, def_val):
    out = f'<select name="{name}" id="{name}">\n'
    modes = [
        (1, 0, "1/64 triplet (24 ppqn)"),
        (1, 1, "1/32 triplet"),
        (3, 0, "1/32 note (8 ppqn))"),
        (1, 2, "1/16 triplet"),
        (3, 1, "1/16 note (4 ppqn))"),
        (1, 3, "1/8 triplet"),
        (3, 2, "1/8 note (2 ppqn)"),
        (1, 4, "1/4 triplet"),
        (3, 3, "1/4 note (1 ppqn)"),
        (1, 5, "1/2 triplet"),
        (3, 4, "1/2 note"),
        (1, 6, "Whole triplet"),
        (3, 5, "Whole note (1 bar in 4/4)"),
        (1, 7, "2 Whole triplets"),
        (3, 6, "2 Whole notes (2 bars in 4/4)"),
        (3, 7, "4 Whole notes (4 bars in 4/4)"),
    ]
    for i in modes:
        scale = i[0] << i[1]
        sel = ' selected' if scale == def_val else ''
        val = str(scale)
        out += f'<option value={val}{sel}>{val}  =  {i[2]}</option>\n'
    out+= '</select>'
    return out

# header
htm = '''
<!DOCTYPE html>
<!-- offline HTML-based SysEx editor for LPZW TRAM8 4x4 Firmware -->
<!-- Generated by gen_html.py -->
<!-- CC BY-NC-ND 4.0 Dmitry S. Baikov -->
<html>
<body>

<p>
'''

htm += f'''
<b>LPZW Tram8 4x4 Firmware.</b> Configuration Format: {CONFIGURATION_FORMAT}<br />
<i>{CONFIGURATION_FN_PREFIX}.syx loaded by default.</i><br />
'''

htm += '''
<br />
For updates visit: 
<a href="https://github.com/c0ff/TRAM8-4x4">https://github.com/c0ff/TRAM8-4x4</a>
</p>

<hr>
<form name="sysex_in">
<p>Load an existing TRAM8 4x4 SysEx into this form
<input type="file" accept=".syx" onchange="load_sysex(this)">
</form>

<script>
'''

htm += f'''
const CONFIGURATION_FORMAT = {CONFIGURATION_FORMAT};
const CONFIGURATION_FORMAT_MIN = {CONFIGURATION_FORMAT_MIN};
const CONFIGURATION_LENGTH = {CONFIGURATION_LENGTH};
const CONFIGURATION_FILENAME = "{CONFIGURATION_FN_PREFIX}_edit.syx";
'''

htm += '''
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

var SyxEx;
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
    console.error(msg);
    throw new Error(msg);
}

function debug_log(msg) {
    console.log(msg);
    //window.alert(msg);
}

function hexstr(i) {
   return (i+0x100).toString(16).substr(-2).toUpperCase();
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
        const tail_pos = i + CONFIGURATION_LENGTH - cfg_tail.length;
        const tail = v8.subarray(tail_pos, tail_pos + cfg_tail.length);
        if (!isEqualArray(tail, cfg_tail)) {
            i = v8.indexOf(cfg_head[0], i + cfg_head.length);
            continue;
        }
        return i;
    }
    return report_bad("Cannot find configuration block!");
}

function parse_channel_map(syx8, map_off, ctl_type, def_chan) {
    var usemask = syx8[map_off];
    for (let i = 0; i < 8; ++i) {
        const id = i + 1;
        const usechan = (usemask >> i) & 1
        document.getElementById(ctl_type + id + ".usechan").checked = usechan;
        const chan = ((syx8[map_off + 1 + (i >> 1)]) >> ((i & 1) ? 4 : 0)) & 0x0F;
        document.getElementById(ctl_type + id + ".channel").value = usechan ? (chan + 1) : def_chan;
    }
}

function parse_8outputs(syx8, cfg_off, outtype) {
    for (let i = 0; i < 8; ++i) {
        const src_mode  = syx8[cfg_off + i*2 + 0];
        const param = syx8[cfg_off + i*2 + 1];
        const src = src_mode & 0xF0;
        const mode = src_mode & 0x0F;

        const oid = outtype + (i + 1).toString();
        debug_log("read out: " + oid + " src_mode: " + hexstr(src_mode) + ", param: " + hexstr(param));

        document.getElementById(oid + ".src").value = src;
        if (src == 0xF0) { // OutputSource_Clock
        	if (param == 0)
        		document.getElementById(oid + ".clkmode").value = mode | 0x80;
        	else {
        		document.getElementById(oid + ".clkmode").value = mode;
        	    document.getElementById(oid + ".pulses").value = param;
        	}
        } else if (src == 0x90) { // OutputSource_NoteOn
        		document.getElementById(oid + ".notemode").value = mode;
        		document.getElementById(oid + ".note").value = param;
        } else if (src == 0xA0) { // OutputSource_AfterTouch
        		document.getElementById(oid + ".note").value = param;
        } else if (src == 0xB0) { // OutputSource_ControlChange
        		document.getElementById(oid + ".cc").value = param;
        }
        CheckMode(oid);
    }
}

function parse_cfg(syx8) {
    syx_cfg_offset = find_cfg(syx8);
    debug_log("Found cfg block at " + syx_cfg_offset);
    
    const cfg_off = syx_cfg_offset + cfg_head.length;
    const ver = syx8[cfg_off];
    if (ver > CONFIGURATION_FORMAT || ver < CONFIGURATION_FORMAT_MIN)
        return report_bad("Unsupported configuration format: " + ver);

    const cvRange = syx8[cfg_off + 1] & 0x10;
    if (cvRange)
        document.getElementById("global.cvrange").value = 8;
    else
        document.getElementById("global.cvrange").value = 5;
    
    const midiChannel = (syx8[cfg_off + 1] & 0x0F) + 1;
    document.getElementById("global.channel").value = midiChannel;
    
    const resetOnContinue = syx8[cfg_off + 1] & 0x20;
    document.getElementById("global.resetoncontinue").checked = (resetOnContinue != 0);

    const triggerLength = syx8[cfg_off + 2];
    document.getElementById("global.triglen").value = triggerLength;
    
    const clockmode = syx8[cfg_off + 3] & 0x0F;
    debug_log("read clock mode: " + clockmode);
    const clockscale = (clockmode & 1 ? 3 : 1) << (clockmode >> 1);
    debug_log("read clock scale = " + clockscale);
    document.getElementById("global.clockscale").value = clockscale;

    // TODO: if (cvRange): syx8[cfg_off + 4] is CV outputs 5V downscaling mask.
    
    parse_channel_map(syx8, cfg_off + 6, "gate", midiChannel);
    parse_channel_map(syx8, cfg_off + 11, "cv", midiChannel);
    
    parse_8outputs(syx8, cfg_off + 16, "gate");
    parse_8outputs(syx8, cfg_off + 32, "cv");
}

function do_parse(sv) {
    if (!check_syx(sv))
        return report_bad("Bad food!");
    syx_bytes = new ArrayBuffer(0, { maxByteLength: 65536 });
    const total_bytes = parse_lines(sv);
    parse_cfg(new Uint8Array(syx_bytes));
    SysEx = sv;
    debug_log("Zehr gut! Lines:" + syx_line_length.length + " Bytes: " + total_bytes);
}

function test_identity() {
    var syx8 = new Uint8Array(syx_bytes.slice());
    write_cfg_bytes(syx8);
    do_parse(patch_sysex(syx8));
}

function parse_sysex(sv) {
    try {
        do_parse(sv);
        //test_identity();
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

// writing config
function write_channel_map(syx8, map_off, ctl_type) {
    var usemask = 0;
    var chans = new Uint8Array(8);
    for (let i = 0; i < 8; ++i) {
        const id = i + 1;
        const usechan = document.getElementById(ctl_type + id + ".usechan").checked;
        if (usechan)
        {
            usemask |= 1 << i;
            chans[i] = document.getElementById(ctl_type + id + ".channel").value - 1;
        }
        else
            chans[i] = 0;
    }

    syx8[map_off] = usemask;
    for (let i = 0; i < 4; ++i)
        syx8[map_off + 1 + i] = ((chans[i*2 + 1] & 0x0F) << 4) | (chans[i*2 + 0] & 0x0F);
}

function ispowerof2(val) {
    return val ? (val & (val - 1)) == 0 : false;
}

function getpowerof2(val) {
    var p = 0;
    while (!(val & 1))
        val >>= 1, ++p;
    return p;
}

function pack_clock_mode(val) {
    var mode = 0;
    var d = Math.floor(val/3);
    debug_log("d = " + d);
    if (d * 3 == val) // divides by 3
        mode = 1;
    else
        d = val;
    debug_log("mode = " + mode);
    //assert(ispowerof2(d));
    const p2 = getpowerof2(d);
    debug_log("p2 = " + p2);
    //assert(p2 < 8);
    mode |= (p2 & 0x07) << 1;
    debug_log("clock mode = " + mode);
    return mode;
}

function write_8outputs(syx8, cfg_off, outtype) {
    for (let i = 0; i < 8; ++i) {
        const oid = outtype + (i + 1).toString();
        const src = document.getElementById(oid + ".src").value;

        var src_mode = src & 0xF0;        
        var param = 0;
        if (src == 0xF0) { // OutputSource_Clock
            const mode = document.getElementById(oid + ".clkmode").value;
        	src_mode |= (mode & 0x0F);
        	if (mode < 0x80)
        	    param = document.getElementById(oid + ".pulses").value;
        } else if (src == 0x90) { // OutputSource_NoteOn
        	const mode = document.getElementById(oid + ".notemode").value;
        	src_mode |= mode;
        	param = document.getElementById(oid + ".note").value;
        } else if (src == 0xA0) { // OutputSource_AfterTouch
        	param = document.getElementById(oid + ".note").value;
        } else if (src == 0xB0) { // OutputSource_ControlChange
        	param = document.getElementById(oid + ".cc").value;
        }
        debug_log("write out: " + oid + " src_mode: " + hexstr(src_mode) + ", param: " + hexstr(param));
        syx8[cfg_off + i*2 + 0] = src_mode;
        syx8[cfg_off + i*2 + 1] = param;
    }  
}

function write_cfg_bytes(syx8) {
    const cfg_off = syx_cfg_offset + cfg_head.length;
    
    var cvRange = 0;
    if (document.getElementById("global.cvrange").value > 5)
        cvRange = 0x10;
    const midiChannel = document.getElementById("global.channel").value - 1;
    var resetOnContinue = 0;
    if (document.getElementById("global.resetoncontinue").checked)
        resetOnContinue = 0x20;
    syx8[cfg_off + 1] = resetOnContinue | cvRange | (midiChannel & 0x0F);

    const triggerLength = document.getElementById("global.triglen").value;
    syx8[cfg_off + 2] = triggerLength;
    
    const clockScale = document.getElementById("global.clockscale").value;
    debug_log("write clock scale = " + clockScale);
    syx8[cfg_off + 3] = pack_clock_mode(clockScale);

    write_channel_map(syx8, cfg_off + 6, "gate");
    write_channel_map(syx8, cfg_off + 11, "cv");

    write_8outputs(syx8, cfg_off + 16, "gate");
    write_8outputs(syx8, cfg_off + 32, "cv");
}

function tohex(nib) {
    if (nib >= 0 && nib <= 9)
        return nib + 0x30;
    else if (nib >= 0x0A && nib <= 0x0F)
        return nib - 0x0A + 0x41;
    else
        return report_bad("Invalid nib for tohex " + nib);
}

function fix_checksum(syx, cfg_line) {
    const off = syx_line_offset[cfg_line];
    const lineend = off + 1 + (syx_line_length[cfg_line] + 4) * 2;
    var chksum = 0;
    for (let i = 1; i < 9; i += 2)
        chksum += from_2hex(syx[off+i], syx[off+i+1]);
    for (let i = off + 9; i < lineend; i += 2) {
        const b = from_2hex(syx[i], syx[i+1]);
        chksum += b;
    }
    const chkb = (-chksum) & 0xFF;
    syx[lineend] = tohex(chkb >> 4);
    syx[lineend + 1] = tohex(chkb & 0x0F);
 
}

function patch_sysex(syx8) {
    var syx = SysEx.slice();
    var cfg_line = 0;
    var cfg_pos = 0;
    var cfg_off = 0;
    while (cfg_off < syx_cfg_offset) {
        const diff = syx_cfg_offset - cfg_off;
        if (syx_line_length[cfg_line] <= diff) {
            cfg_off += syx_line_length[cfg_line];
            ++cfg_line;
        } else {
            cfg_pos = diff;
            cfg_off += diff;
            break;
        }
    }
    //debug_log("cfg_off: " + cfg_off + " cfg_line: " + cfg_line + " cfg_pos: " + cfg_pos);
    var patched_bytes = 0;
    // found the config block start, now patch it
    for (let i = 0; i < CONFIGURATION_LENGTH; ) {
        const avail = syx_line_length[cfg_line] - cfg_pos;
        const todo = Math.min(avail, CONFIGURATION_LENGTH - i);
        //debug_log("todo: " + todo);
        for (let k = 0; k < todo; ++k) {
            const b = syx8[syx_cfg_offset + i + k];
            const hi = tohex(b >> 4);
            const lo = tohex(b & 0x0F);
            const syxpos = syx_line_offset[cfg_line] + 9 + cfg_pos * 2;
            if (syx[syxpos] != hi)
                ++patched_bytes;
            syx[syxpos] = hi;
            if (syx[syxpos+1] != lo)
                ++patched_bytes;
            syx[syxpos + 1] = lo
            ++cfg_pos;
        }
        i += todo;
        if (cfg_pos == syx_line_length[cfg_line] || i == CONFIGURATION_LENGTH) {
            //debug_log("fix_checksum line: " + cfg_line);
            fix_checksum(syx, cfg_line);
            ++cfg_line;
            cfg_pos = 0;
        }
    }
    debug_log("Patched bytes: " + patched_bytes);
    return syx;
}

// https://stackoverflow.com/questions/3665115/how-to-create-a-file-in-memory-for-user-to-download-but-not-through-server/18197341
function save(filename, data) {
    const blob = new Blob([data], {type: 'application/octet-stream'});
    if(window.navigator.msSaveOrOpenBlob) {
        window.navigator.msSaveBlob(blob, filename);
    }
    else{
        const elem = window.document.createElement('a');
        elem.href = window.URL.createObjectURL(blob);
        elem.download = filename;        
        document.body.appendChild(elem);
        elem.click();        
        document.body.removeChild(elem);
        window.URL.revokeObjectURL(elem.href);
    }
}

function test_checksum_fix() {
    var out_syx = SysEx.slice();
    for (let i = 0; i < syx_line_length.length; ++i)
        fix_checksum(out_syx, i);
    return out_syx;
}

function download_sysex() {
    //const out_syx = test_checksum_fix();
    var syx8 = new Uint8Array(syx_bytes.slice());
    write_cfg_bytes(syx8);
    const out_syx = patch_sysex(syx8);
    save(CONFIGURATION_FILENAME, out_syx);
}

function CheckMode(oid) {
	const src = document.getElementById(oid + ".src").value;
	if (src == 0xF0) { // OutputSource_Clock
		document.getElementById(oid + ".clkmode").disabled = false;
    	const mode = document.getElementById(oid + ".clkmode").value;
    	document.getElementById(oid + ".pulses").disabled = (mode >= 0x80);
		document.getElementById(oid + ".note").disabled = true;
		document.getElementById(oid + ".notemode").disabled = true;
		document.getElementById(oid + ".cc").disabled = true;
		document.getElementById(oid + ".usechan").disabled = true;
		document.getElementById(oid + ".channel").disabled = true;
	} else {
		document.getElementById(oid + ".clkmode").disabled = true;
		document.getElementById(oid + ".pulses").disabled = true;
		document.getElementById(oid + ".usechan").disabled = false;
		document.getElementById(oid + ".channel").disabled = !document.getElementById(oid + ".usechan").checked;
		if (src == 0x90) { // OutputSource_NoteOn
			document.getElementById(oid + ".note").disabled = false;
			document.getElementById(oid + ".notemode").disabled = false;
			document.getElementById(oid + ".cc").disabled = true;
    	} else if (src == 0xA0) { // OutputSource_AfterTouch
			document.getElementById(oid + ".note").disabled = false;
			document.getElementById(oid + ".notemode").disabled = true;
			document.getElementById(oid + ".cc").disabled = true;
    	} else if (src == 0xB0) { // OutputSource_ControlChange
			document.getElementById(oid + ".note").disabled = true;
			document.getElementById(oid + ".notemode").disabled = true;
			document.getElementById(oid + ".cc").disabled = false;
    	} else if (src == 0xD0) { // OutputSource_ChannelPressure
			document.getElementById(oid + ".note").disabled = true;
			document.getElementById(oid + ".notemode").disabled = true;
			document.getElementById(oid + ".cc").disabled = true;
    	}
    }
}
</script>
'''

# globals
htm += f'''
<hr>

<p><b>GLOBALS</b></p>
<table>
<tr>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the input MIDI channel to use for all mapings'">Default MIDI Channel</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the millisecond duration for trigger outputs">Trigger Length</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the maximum Control Voltage value">Control Voltage Range</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Restart on MIDI CONTINUE">Restart Dividers on MIDI Continue</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Clock Divider Scale">Clock Divider Scale</td>
</tr>
<tr>
<td></td>
<td>{mkchan("global.channel", 0)}</td>
<td></td>
<td>{mktrig("global.triglen", 0)}</td>
<td></td>
<td>{mkcvrange("global.cvrange", 0)}</td>
<td></td>
<td>{mkglobalbool("global.resetoncontinue")}</td>
<td></td>
<td>{mkclkscale("global.clockscale", 1)}</td>
</tr>
</table>
'''

# outputs
def make_outhdr(name):
    return f'''
<tr>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="{name} Output">{name} Output</td>
<td title="Source">Source</td>
<td title="Clock Mode">Clock Mode</td>
<td title="Clock: Number of ticks between activations">Clock Divisor</td>
<td title="Note Number">Note</td>
<td title="Note Mode">Note Mode</td>
<td title="CC number">CC Number</td>
<td title="Note: listen to custom channel number">Use Channel</td>
<td title="Note: custom MIDI channel number">Channel</td>
</tr>
'''

# gates/triggers
htm += '''
<hr>

<p>
<b>GATE/TRIGGER OUTPUTS</b><br>
<i>NOTE: Flip-Flop is a square LFO with full period twice the clock divider duration)</i>
<i>NOTE: Continuous sources produce high output for values >=64.</i>
</p>
<table>
'''
htm += make_outhdr('Gate')
for i in incrange(1, 8):
    htm += mkout_html('gate', i)
htm += '''
<script>
'''
for i in incrange(1, 8):
    htm += mkout_js('gate' + str(i))
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
'''
htm += make_outhdr('CV')
for i in incrange(1, 8):
    htm += mkout_html('cv', i)
htm += '''
<script>
'''
for i in incrange(1, 8):
    htm += mkout_js('cv' + str(i))
htm+='''
</script>
</table>
'''

# footer
def add_footer(htm):
    return htm + '''
<hr>
<form name="sysex_out" onsubmit="download_sysex(); event.preventDefault()">
<input type="submit" value="Get the configured SysEx" onchange="download_sysex()">
</form>
<hr>
<p>LPZW TRAM8 4x4 firmware<br />
CC BY-NC-ND 4.0 Dmitry S. Baikov<br />
<a href="https://github.com/c0ff/TRAM8-4x4">https://github.com/c0ff/TRAM8-4x4</a></p>
</body>
</html>
'''

def add_firmware(htm, fw_file):
    fw = open(fw_file, 'rb').read()
    fw_hex = binascii.hexlify(fw).decode('utf-8')
    ah = f'''
<script>
// default 4x4 Firmware SysEx
const firmware_syx = Uint8Array.fromHex("{fw_hex}");
''' + '''
window.onload = function() { parse_sysex(firmware_syx); };
</script>
'''
    return htm + ah

def main():
    global htm
    ap = argparse.ArgumentParser()
    ap.add_argument('-o', '--output')
    ap.add_argument('--firmware-sysex')
    args = ap.parse_args()
    
    if args.firmware_sysex:
        htm = add_firmware(htm, args.firmware_sysex)
    
    htm = add_footer(htm)
    
    if args.output:
        open(args.output, 'w', encoding='utf8').write(htm)
    else:
        print(htm)
    
if __name__=="__main__":
    main()