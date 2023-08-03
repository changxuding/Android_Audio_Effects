<!--
 * @Descripttion: 
 * @version: 
 * @Author: ASR_AUDIO
 * @Date: 2023-08-03 09:15:08
-->
# Android Audio Effects Module
Extract from Audio Effects Module from Android 10

## Usage
```
make clean
make
```

Usage see ```./test/lvmtest.c```, only support stereo input/output

## Example
```
 ./bin/test_lvm -i:"music.pcm" -o:"music_out_bass14.pcm" fs:"16000" -bE -basslvl:"14"
```
