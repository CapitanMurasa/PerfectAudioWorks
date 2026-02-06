# PerfectAudioWorks

## About this program

**PerfectAudioWorks** is an open-source media player inspired by the classic **Winamp design**.
It is built with performance and simplicity in mind, while remaining modular for future expansion.

Currently, the player:

* Uses **PortAudio** to initialize the audio output buffer.
* Uses **Qt6** for rendering the UI.
* Uses **Libtag** to pharse album art and metadata info

---

## Supported File Formats

* `.wav` (uncompressed)
* `.flac` (Free Lossless Audio Codec)
* `.opus` (Opus codec in Ogg container)
* `.ogg` (Ogg Vorbis)
* `.mp3` (via mpg123)

### Planned via **separate codecs**

* `.aac` (via FFmpeg)
* Other compressed/streaming formats (future plugin system)

---

## Codecs Table

| Format | Handled by | Status     |
| ------ | ---------- | ---------- |
| WAV    | libsndfile | ✅ Works    |
| FLAC   | libsndfile | ✅ Works    |
| OGG    | libsndfile | ✅ Works    |
| Opus   | libsndfile | ✅ Works    |
| MP3    | mpg123     | ✅ Works    |
| AAC    | FFmpeg     | 🔜 Planned |

---

## Optional Dependencies & Codec Flags

| Codec      | Optional | CMake Flag       | Notes                                    |
| ---------- | -------- | ---------------- | ---------------------------------------- |
| libsndfile | ✅        | `-DENABLE_SNDFILE` | Handles WAV, FLAC, OGG, Opus             |
| mpg123     | ✅        | `-DENABLE_MPG123`  | Handles MP3 playback                     |


## Installation & Compilation

### Dependencies

* `portaudio`
* `Qt6`
* Optional codecs:

  * `libsndfile` (WAV, FLAC, OGG, Opus)
  * `mpg123` (MP3)


### Cloning repository

```
git clone --recursive https://github.com/CapitanMurasa/PerfectAudioWorks
```
### Ubuntu / Debian

```
sudo apt update
sudo apt install portaudio19-dev qt6-base-dev cmake build-essential \
                 libsndfile1-dev mpg123-dev 
```

### Arch Linux

```
sudo pacman -S portaudio qt6-base cmake make gcc \
               libsndfile mpg123
```

### Windows
Although windows release is out, it still less stable than linux release <br>
install qt framework from [qt's official site](https://www.qt.io/download-dev) <br>
install mingw compiler and then from mingw console install required components 
```
pacman -S mingw-w64-x86_64-portaudio mingw-w64-x86_64-libsndfile mingw-w64-x86_64-mpg123
```

### Build Instructions

By default, all codecs are **enabled**. You can disable a codec by passing `-D<FLAG>=OFF` to CMake:

```
git clone https://github.com/loh1naalt/PerfectAudioWorks.git
cd PerfectAudioWorks
mkdir build && cd build
cmake .. -DENABLE_SNDFILE=ON -DENABLE_MPG123=OFF 
make
```

* The example above **disables MP3 support via mpg123**, while keeping libsndfile enabled.
* CMake automatically skips building source files for any disabled codecs.

---
## Usage

Run the player:

```
./PAW 
```

 Or pass an audio file as argument:
 
```
./PAW  <audio_file_path>
```

---

## Known Bugs

1. ~~**No sound output** if the program selects the default output, but your audio device is not set as system default.~~
   ✅ Fixed: you can now choose the device to play on, but playback still defaults to the system’s default device.

2. ~~**Crash on progress bar seek**: spamming left/right arrow keys after making the progress bar active may cause a crash.~~
   ✅ Fixed
3. ~~**Playing through mpg123 would result in white noise**~~
   ✅ Fixed: sample format conversion from int16 to float32 is now handled correctly.

---

## License

It uses GNU GPLv3 license (see LICENSE for further details)
