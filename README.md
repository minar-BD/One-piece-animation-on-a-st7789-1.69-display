# ESP32-S3 Smooth GIF/Animation Player

This repository demonstrates how to play smooth, full-color animations on an ST7789 display using an ESP32-S3. By bypassing standard `.h` memory limits and utilizing the ESP32-S3's LittleFS and PSRAM, this method allows for long, high-framerate animations without crashing the Arduino IDE or running out of flash memory during compilation.
https://github.com/user-attachments/assets/b5143dd6-3955-4502-b8cf-dd60941ef99e
## 🛠️ Workflow: From GIF to ESP32

To keep the ESP32-S3 fast and memory-efficient, we pre-process the GIF into a raw RGB565 binary file (`.bin`) before uploading it to the microcontroller's filesystem.

### Step 1: Split GIF to PNG Frames
1. Go to [EZGif - Split GIF](https://ezgif.com/split).
2. Upload your target `.gif` file.
3. Click **Split to frames** and output them in **PNG format**.
4. Download the frames as a `.zip` and extract them to a folder on your computer.

### Step 2: Styling and Scaling
1. Open the [LCD Assistant Tool](https://projedefteri.com/en/tools/lcd-assistant/).
2. Load your extracted PNG frames.
3. Use the tool to stretch, rotate, or scale the frames to match your exact display resolution (e.g., 240x240 or 240x280).
4. Save the processed/styled frames to a new folder on your computer (e.g., `processed_frames`).

### Step 3: Convert to Binary (`.bin`)
Instead of compiling a massive C++ array, we use a Python script to convert the styled frames into a single, continuous stream of RGB565 data. 

Run the h_to_bin.py to convert the .h file to .bin file

## 📁 Project Structure

Once you have your `gif.bin` file, structure your PlatformIO project exactly like this:

```text
your_project/
├── data/
│   └── gif.bin          ← Your generated binary animation file goes here
├── src/
│   └── main.cpp         ← Your ESP32-S3 playback code
├── partitions.csv       ← Custom partition table (allocating space for LittleFS)
└── platformio.ini       ← PlatformIO configuration file
```

## ⚡ Flashing Instructions

Because the animation data is separated from the code, you must flash the ESP32-S3 in two distinct steps using PlatformIO.

**Step 1: Flash the Animation Data**
* Click the PlatformIO icon (Alien) in the left sidebar.
* Expand your build environment folder (e.g., `esp32-s3-devkitc1-n16r8`).
* Expand **Platform**.
* Click **Upload Filesystem Image**. *(This writes `gif.bin` to LittleFS).*

**Step 2: Flash the Code**
* Once the filesystem upload is complete, click the standard **Upload** button (the right-pointing arrow at the bottom of VS Code).
* This compiles and flashes `main.cpp`.

Your ESP32-S3 will boot up, mount LittleFS, load the first frame into PSRAM, and push it directly to the ST7789 display for smooth playback!
