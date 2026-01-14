# How it was done.

## The firmware.

### Why the fuss.

When I initially bought Tram8, it did exactly what I needed back then.
However, having an open source firware was one of the selling points.

I also have a Hexinverter's' Mutant Brain module (now produced by Erica Synths) which has a very nice configuration UI website where you can upload a firmware SysEx, tweak the settings and get the modified SysEx back.

In the eurorack land there's some variation in control voltage range standards between module manufacturers. There are 0..5V -5..+5V 0..8V 0..10V and -10..+10V. What a zoo! I happen to have and mostly use a system by Make Noise with 0..8V CV range.

When I started digging into Tram8 firmware(s), I found out that the hardware is capable to output both 0..5V and 0..8V voltage ranges on its CV outputs (the manual states 0..10V but that was changes in the production revisions to 0..8V for cost reasons, according to the creator). I also found that there're several firmware versions which do different things, but on the source level lool almost identical.


### A flashback from the past.

The last two of my school years I spent lots of time programming in x86 assembly. While it's a pain to do manual register allocation and similar micromanagement, exactly because writing code is tedious, it forces you do to as much as possible with minimal amounts of code. One of my serious projects (and my first commercial one) was a driver for some enterprise Lexmark printers - they had a very strange codepoints for cyrillic glyphs (didn't match any encoding I knew) making it impossible to print texts in Russian without special conversion. So my task was to write a resident program which intercepts stream of data to the printer, parses printer commands, and converts only text data, passing graphic commands and others through.

Because it was assembly which punishes for overly long and complex code, and I am not a graphomaniac, I spent a lot of time creating a table-driven algorithm which can parse, selectively convert or pass through any data stream. When I started coding I already a clear understanding of what to code, and had lots of hand-written sheets with data structures and the control flow.


### I've seen this before.

Studying Tram8 firmware sources immediately made me realise that this is a perfect case for a data-driven design. And this also means that I can create a configuration editor - which will patch the data section of the firmware.

Having learnt what hardware can do, knowing what I need and what is possible, I started designing the data structures and the control flow in my head, when walking, getting grocceries etc, making notes on my phone. Like the old times - designing algorithms and data structures in my head without any coding.

Then I started coding, having a very clear picture of how the code will work. It took a couple of iterations, first it was a hardcoded configuration table. After I made it work, I restructured the data for easier patching by a configuration tool - with compact representation in a byte array with unique markers for start and end of data - because I don't what to manually find and hardcode the offsets in the editor, and also at this point I had no idea how it will look in the SysEx, but hoped it would be something recognizable.

The firmware worked. Now it was time to create the editor.



## The editor.


### The inspiration.

Thinking of the configuration editor I was heavily inspired by Mutant Brain Surgery - the configuration website for the Mutant Brain module (http://mutantbrainsurgery.hexinverter.net). It is slick, minimal and very easy to use. The main downside I saw - it is a website, meaning you need to be online each time you. want to reconfigure the module. I might be in my shed in the woods - how will I change that output mode?


### The ugly truth.

I haven't touched HTML and JS for 20 years. My first realization was that ComboBoxes cannot use a shared dataset for their choice list. It was a shame. It meant that for each output I have to copy-paste the ComboBox descriptions, and I hate stupid work. So, what do we do then? This is a poster case for code generation. The initial HTML attempt was scrapped and reused as a template for a Python script which would do the repetitive stuff for me.


### The fork in the road.

Generating the HTML UI was pretty straightforward. Now comes the fun part - reading and writing SysEx.

Trying to find the configuration block markers in the firmware SysEx gave nothing. I haven't dealt with writing for embedded hardware before. All I knew that the toolchain produces a .hex file which I then convert using a hex_2_syx.py tool provided with the firmware sources. I started digging.

hex_2_syx.py turned out to be very simple and straightforward: it just dumps the .hex file interspersed with some zeroes, adds a head and tail markers and that's it. Getting the original .hex from .syx was pretty easy - just check the markers, skip the zeroes and stick together what's left.

.hex files seem to be a common format for firmware encapsulation and is very simple
https://www.tasking.com/documentation/smartcode/ctc/reference/objfmt_hex.html

One way of getting and setting the configuration block was to get the firmware image from IntelHex in the SysEx, edit the data, and then recreate the .hex and .syx back. While looking straightforward, it would require an elaborate .hex parser, taking into the account all record types and their meaning. I didn't want to spend my time on this - it would be long, not fun, with complex and error prone code. An overkill for a humble configuration editor. And I didn't want to become a JavaScript professional.

### The patching trick.

What if we parse the SysEx, getting the raw byte represenation of the firmware, while remembering where these bytes are located in the SysEx? This was we can use the raw firmware bytes to locate and read the configuration block, and use the remembered positions of these bytes to patch them directly in the SysEx image.

This is exactly how I did it. I just parse the SysEx, finding IntelHex records inside and remember the position of each record in the SysEx Uint8Aarray. At the same time I decode records and append their content to a separate Uint8Array which I will refer to as a raw firmware bytes. Strictly speaking, there are several record types in an IntelHex file, but I just clump the data of all the records together in the hope that my configuration block is in a continious set of data records, not interleaved with anything else. And practice showed this is exactly the case.

Using the arrays of offsets and lengths of every IntelHex record I can locate every byte of the raw representation back in the original SysEx. Because I am patching the data in-place, not changing its length, I do not need to recreate the SysEx from scratch - I can directly modify every byte I need. The only additional burden is to update IntelHex checksums of each record I touched.

### It loads!

The SysEx loading part of the editor:
 - uses an HTML form trick to avoid reloading the page: the `Browse` button `onchange` JS handler loads the file without a need to submit the form
 - Reads SysEx from the file and feeds it into the parser
 - Validates SysEx header and footer
 - finds IntelHex records, skipping zeros and LF characters
 - parses each records, adding its offset and content length to respective arrays
 - decodes content of each record and appends it to the `syx_bytes` Uint8Array
 - Finds the configuration block in the `syx_bytes` array using the markers, and stores its offset
 - Parses the configuration and sets GUI controls to match the settings.
 
### It saves!
 
The SysEx saving part does the following:
 - Uses an HTML form `onsubmit` JS trick to avoid reloading the page and calls the JS writer
 - Makes a copy of `syx_bytes` array and packs current GUI settings into configuration block. (Trying to patch the original `syx_bytes` directly costed me about 30 min of debugging and not understading why the firmware gets corrupted. Still don't know why it got corrupted, but writing to a firmware copy resolved the issue.)
 - Now comes the fun part: for each byte of the new configuration block I locate its position in the SysEx, patch it. And for each touched IntelHex records I update its checksum.
 - After all is said and done the editor forces a download of the generated SysEx using another trick from StackOverflow.

 
## Closing thoughts.

Using vanilla HTML and JS allowed to make this editor a single self-containted HTML page which works completely offline. A single-page WebApp if you like.

Initially it required to load a firmware first, but I quickly realised that I can embed the corresponing firmware directly into the page and parse it in the `onpageload` JS handler. Rev.A does exactly this.

I decided to use numbers for firmware version/configuration block format id, because it is stored inside as uint8_t,  and use Rev [A-Z] for pure GUI changes. 

I think this editor might serve as a base for similar tools because the Tram8 specifics occupy only a small part of code. Most of the stuff is pretty universal for any IntelHex/SysEx patching tasks.
 
The editor was created in one long 16-hour coding session. It was fun.


### experienced and documented by Dmitry S. Baikov