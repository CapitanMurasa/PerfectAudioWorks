# PerfectAudioWorks

## About this program

**PerfectAudioWorks** is an open-source media player inspired by the classic **Winamp design**.

Currently, the player:

* Uses **PortAudio** to initialize the audio output buffer.
* Uses **Qt6** for rendering the UI.
* Uses **Libtag** to pharse album art and metadata info
* Uses **nlohmann/json** to manipulate with json files
* Uses **Pyton** and **Pybind11** as core libs for plugin system

---

## Screenshots
<img width="603" height="683" alt="Perfect Audio Works on Windows" src="https://github.com/user-attachments/assets/3e1724c2-f818-445c-ab12-88f765c396ee">
> Perfect Audio Works on Windows (v0.1.2)

---

<img width="600" height="678" alt="Perfect Audio Works on Linux (Kubuntu)" src="https://github.com/user-attachments/assets/069ff9d5-5eee-4ea3-ab40-0b7832f778c9">
> Perfect Audio Works on GNU/Linux (Kubuntu, PAW v0.1.1)


## Supported File Formats

* `.wav` (uncompressed)
* `.flac` (Free Lossless Audio Codec)
* `.opus` (Opus codec in Ogg container)
* `.ogg` (Ogg Vorbis)
* `.mp3` (via mpg123)
* `.aac` (via FFmpeg)
* `.m4a` (via FFmpeg)

---

## Optional Dependencies & Codec Flags

| Codec      | Optional | CMake Flag       | Notes                                    |
| ---------- | -------- | ---------------- | ---------------------------------------- |
| libsndfile | ✅        | `-DENABLE_SNDFILE` | Handles WAV, FLAC, OGG, Opus             |
| mpg123     | ✅        | `-DENABLE_MPG123`  | Handles MP3 playback                     |
| ffmpeg     | ✅        | `-DENABLE_FFMPEG`  | Handles m4a aac playback                     |


## Installation & Compilation

### Dependencies

* `portaudio`
* `Qt6`
* `LibTag`
* `nlohmann/json`
* `pybind11`
* `Python (version 3.14)`
* Optional codecs:

  * `libsndfile` (WAV, FLAC, OGG, Opus)
  * `mpg123` (MP3)
  * `FFmpeg` (AAC M4A)


### Cloning repository

```
git clone --recursive https://github.com/CapitanMurasa/PerfectAudioWorks
```

### Ubuntu / Debian
```
sudo apt update
sudo apt install cmake build-essential qt6-base-dev \
                 portaudio19-dev libsndfile1-dev libmpg123-dev
```
### Arch Linux

```
sudo pacman -S cmake gcc make qt6-base \
               portaudio libsndfile mpg123
```

### Fedora
```
sudo dnf install cmake gcc-c++ make qt6-qtbase-devel \
                 portaudio-devel libsndfile-devel mpg123-devel
```

### Windows
Although windows release is out, it still less stable than linux release <br>
install qt framework from [qt's official site](https://www.qt.io/download-dev) <br>
install mingw compiler and then from mingw console install required components 
```
pacman -S mingw-w64-x86_64-gcc \
           mingw-w64-x86_64-cmake \
           mingw-w64-x86_64-portaudio \
           mingw-w64-x86_64-libsndfile \
           mingw-w64-x86_64-mpg123
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
## Plugins (EXPERIMENTAL SO FAR!!!)

Since version 0.1.2 you can embed plugins to extend functionality of program:

For example [discord rich presence plugin](https://github.com/CapitanMurasa/PAWDiscordPresencePlugin), which allows the program to indicate it's status inside your discord profile.

### How to write one?

Here are some basics using python

```python
# importing PAW module to python
import PAW_python as paw

#assigning class to variable
player = paw.PAW()

# checking if playback is active 
if player.IsPlaybackActive():
# display message box with Title Artist and Album info about track that is currently playing.
    player.SendMessageBox(f"Playing: {player.GetTitle()}\nBy: {player.GetArtist()}\n In Album: {player.GetAlbum()}")
```

as in result it displays us a message box with a basic information about song that is currently playing.

<img width="1337" height="777" alt="image" src="https://github.com/user-attachments/assets/9cb7d0af-d16a-406c-908c-7b2272360419" />

further information about usage will be in docs soon...

## Known Bugs

1. ~~**No sound output** if the program selects the default output, but your audio device is not set as system default.~~
   ✅ Fixed: you can now choose the device to play on, but playback still defaults to the system’s default device.

2. ~~**Crash on progress bar seek**: spamming left/right arrow keys after making the progress bar active may cause a crash.~~
   ✅ Fixed
3. ~~**Playing through mpg123 would result in white noise**~~
   ✅ Fixed: sample format conversion from int16 to float32 is now handled correctly.

### For more issues with this program look up the Issues tab in this repo.

---

## License

It uses GNU GPLv3 license (see LICENSE for further details)
