
# **badpaint** 
*An experimental real-time image corruption editor*

[Image here]

*"It's like ms paint but bad" - Steve Jobs*

## Features 
- paint data directly onto a compressed image's data to make it glitch out
- multithreaded image encoding updates your corrupted image in real time as you work
- you can even break the image if you can figure out how (don't worry you have undo)
- different color/brush effects corrupt the data differently
- toggle between different encoding styles (at the moment only supports PNG post-filter corruption) 
- export your corrupted image to use however you want (greed, boasting, lust, etc.)

This tool was made as a part of the Handmade Network's [Wheel Reinvention Jam 2023](https://handmade.network/jam/2023). I'm releasing this app in it's primitive state after a week of development to get it in people's hands ASAP so others can try playing with it.

Warning: as this is was jam project, new features and experimentation were prioritized over optimization. The app only runs at a full framerate with smaller images. Larger images can still be edited but at the cost of performance.

The jam page for this project, including logs of the jam's development, can be found here: https://handmade.network/p/441/badpaint/.

----------------------
## Installation & basic usage
The jam version of the application (v0.0.1) is available for tinkering here. To use the application, uncompress it and open it. Then drag any image onto the window and paint the bottom image.

**Controls & keyboard shortcuts**
- Brush effect: **Erase** (e): Erase painted data
- Brush effect: <span style="background-color:lightgray;color:red"><b>Remove</b></span> (r): Replaces PNG data with 0 
- Brush effect: <span style="background-color:gray;color:yellow"><b>Max</b></span> (a): Replaces PNG data with the max color bit: 255
- Brush effect: <span style="background-color:lightgray;color:blue"><b>Shift</b></span> (s): Shifts PNG data forward by 36 bits (the 36 is arbitrary)
- Brush effect: <span style="background-color:lightgray;color:purple"><b>Random</b></span> (d): Adds a random value (between 0-255)
- Undo with **Ctrl-Z**
- Change the size of the brush with the brush size slider 
- Press **1-5** to toggle different PNG filter algorithms (Sub, Up, Average, Paeth, Optimal). Each distorts the image differently.

If the app crashes for any reason, a crash reporter will prompt you if you like to send a crash report via a discord webhook.

## What the heck is going on? 
The editor is surprisingly simple. The bottom image displays the raw data from a PNG in the middle of it's encoding process. Each byte is represented by a pixel ranging from black to white (0-255) after the PNG has been filter by the selected filtering algorithm and before it is compressed. The pixels you insert via painting adds unintended arbitrary numbers into the data stream and are then compressed incorrectly by the PNG encoding algorithm, resulting in the striking colorful glitches you see in the final top image. 

You can press 1-5 to toggle different PNG filter algorithms, the effects of which will be visible in both windows. Something I didn't expect from this project is that playing with the data gives you an intuitive view of how a PNG is encoded. 

An overview on PNG encoding and corruption behaviours that I used as reference while making this tool can be found here: https://ucnv.github.io/pnglitch/

## Implementation details 

This app was codded in C-styled C++. [raylib](https://www.raylib.com) is used for handling platform needs and rendering. The app uses my own custom string library, math library, and immediate-mode UI which were repurposed from my in-progress self-developed game engine. The crash report code, (which is also repurposed from my engine), is based on [Phillip Trudeau-Tavara's wonderful implementation.](https://lance.handmade.network/blog/p/8491-automated_crash_reporting_in_basically_one_400-line_function#26627) The multithreading API is implemented as inspired by [Casey Muratori's multithreaded job system as seen in his Handmade Hero series.](https://guide.handmadehero.org/code/day122) The [loadpng library](https://lodev.org/lodepng/) is used for PNG decoding and a modified version of [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h) is used for PNG encoding.

## TODO for the future whenever I have time
- More image encoding options. I definitely want to explore JPEG. 
- Layers, like in Photoshop, each layer with its own image encoding transforming the corrupted data from the previous layer
- Iteration on the editing process, brush effects, and painting styles. At the moment you only have a marker/paint tool. It would be nice to have all of the tools you see in old school MS Paint (bucket, rectangle, spray can, etc.)
- zoom & resizing 
- Optimization & performance improvements (SIMD, more GPU utilization, etc).

----------------------

If you would like to contact me, I can be reached on discord at @Ahmaykmewsik. Or if transmitting messages via the m-a-i-l is more your thing: notfungamesdeveloper at the Gee of the maleness of Doot Cumberbatch
