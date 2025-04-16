
# **badpaint**
*An experimental real-time image corruption editor*

https://github.com/Ahmaykmewsik/badpaint/assets/6686290/3d97b0c9-4557-444a-8a15-e29e99a30e05

*"It's like ms paint but bad" - Steve Jobs*

# Features 
- paint data transformations directly onto an image's data mid-encoding to make it glitch out
- multithreaded image processing updates your corrupted image as you paint with near-instant feedback
- different color/brush effects corrupt the data differently
- toggle between different PNG filters which change the corruption behavior
- export your corrupted image to use however you want (greed, boasting, lust, etc.)

This tool started as a part of the Handmade Network's week-long [Wheel Reinvention Jam 2023](https://handmade.network/jam/2023). Read initial logs for the jam [here.](https://handmade.network/p/441/badpaint/)

# Installation & basic usage
Download the latest release [here](https://github.com/Ahmaykmewsik/badpaint/releases). To open, uncompress the zip file and run the app. 

Paint colors on an image to corrupt it. Load your own image by dragging it into the window.

**Controls & keyboard shortcuts**
- Brush effect: ⚪ **Erase** (e): Erase paint
- Brush effect: 🔴 **Remove** (r): Removes filtered PNG pixels and sets it to zero.
- Brush effect: 🟡 **Max** (a): Replaces filtered PNG pixels with the max value.
- Brush effect: 🔵 **Shift** (s): Shifts filtered PNG pixels forward by 36 bits (the 36 is arbitrary)
- Brush effect: 🟣 **Random** (d): Adds a random value for that pixel.
- Undo with **Ctrl-Z**
- Change the size of the brush with the brush size slider 
- Press **0-5** to toggle PNG filter algorithms (None, Sub, Up, Average, Paeth, Optimal).
- Export your image to desktop when you're done via the button in the top right.

If the app crashes, you'll be prompted if you'd like to send an anonymous crash report via a discord webhook.

# What the heck is going on? 
Here's a breakdown of what happens when you paint onto the image:
1. Beforehand, the original image is run though PNG's first step in encoding a PNG image, which is to "filter" each row of pixels in the image via some method. This filter transforms the pixels into a format that makes the data easier to compress and make smaller. For example, a common filter is for each pixel to be rewritten to represent the difference between itself and the previous pixel, or in other words, the pixel to the left (the "Sub"(1) filter). PNG encoders typically will choose a filter for each scanline that results in the optimal compression rate (the "Optimal"(5) filter). A visualization of the pixels after they have been filtered is shown in the bottom image.
2. Paint represents a data transform that changes the modified pixel data in some unintended way, such as removing/deleting it, replacing it with something else, or moving it elsewhere.
3. The resulting corrupted data is then incorrectly converted back to the original image resulting in striking colorful distortions.

Here's an overview on PNG encoding and corruption that I used as reference when I first started making this tool: https://ucnv.github.io/pnglitch/

## Implementation details 
- Coded in C-styled C++. 
- [raylib](https://www.raylib.com) is used for handling platform needs and rendering.
- My own custom immediate-mode UI, string library, math library, and other base components.
- Crash report code is based on [Phillip Trudeau-Tavara's wonderful implementation.](https://lance.handmade.network/blog/p/8491-automated_crash_reporting_in_basically_one_400-line_function#26627)
- Multithreading API is implemented as inspired by [Casey Muratori's multithreaded job system as seen in his Handmade Hero series.](https://guide.handmadehero.org/code/day122)
- PNG filtering and decoding uses parts of [loadpng library](https://lodev.org/lodepng/) and [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h).

## TODO for the future whenever I have time
- A UI overhaul that makes the app more like ms paint but bad.
- zoom, resizing, and more flexibility in image arrangement
- More brush effects and painting styles. At the moment you only have a brush tool. It would be nice to have all of the tools you see in old school MS Paint (bucket, rectangle, line, spray can, etc.)
- More image distortion features. Custom PNG filters, interlacing. And explore JPEG! 
- Layers, like in Photoshop. Each layer with its own image encoding transforming the corrupted data from the previous layer.
- Save a project so you can edit it further later.
- More optimization & performance improvements (SIMD, more multithreading offloading, etc).

### CHANGELOG 
- **v0.0.3: the it runs fast now update** - April 14th 2025

    This update fixes issues in the jam release with a focus on stability, performance and building a strong foundation for future updates. The app still mostly resembles the jam release but is MUCH faster and less crash-prone.

    - Painting has been optimized to run well even on large images. There is no longer a limit on image size. Very large images will take a while to load and process considerably slower but should still function.
    - Key aspects of the image corruption process have been internally simplfied (such as removing PNG compression and decompression after realizing it was redundant!) resulting in a significant speedup, with percivabley instant corruption for most images.
    - Handles memory properly. Memory usage scales to the size of the currently loaded image (the jam release was particularly unstable due to committing WAY too much memory on startup) 
    - The bottom image has a differnet visualization of the filtered PNG data and also will update to the new switched PNG filters. Previously, each byte was mapped to a pixel directly, resulting in a black and white image that was 4 times as wide as the original image (1 pixel => 4 RGBA pixels). This new version switches to a visualization that matches the original resolution of the image where each colored pixel is displayed as its filtered colored pixel (1 RGBA pixel => 1 RGB pixel). This visualization additionally hides the filter type byte that is encoded at the start of each scanline (previously on the left side of the image) which was prone to breaking the image when modified.
    - You can paint on either image instead of just the bottom one, and it shows where you are painting on both images.
    - Enabled the "None" type filter which does no filtering. Useful for understanding what the paint transforms are doing to the data.
    - The image scales up to the window allowing for better editing of smaller images such as pixel art.

- **v0.0.2: Jam Release Day 1 patch** - October 2nd 2023
    - Removed webhook token from public repo (thank you for all the lovely crash reports whoever was spamming them every 15 seconds for 2 hours straight) and fixed a crash.

----------------------
Tag me on [bluesky](https://bsky.app/profile/ahmaykmewsik.bsky.social) if you make something cool!

Other ways to contact me: I can be reached on discord at @Ahmaykmewsik. Or if transmitting messages via the m-a-i-l is more your thing: notfungamesdeveloper at the g m-a-i-l. 
