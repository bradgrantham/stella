TODO
* RESP0, RESP1 without HMOVE and VDELPx+GRPxA (is playfield also delayed relative to WSYNC?)
  * might be correct now?
  * Setting COLUBK after 
  * Complicated sprite positioning routing at https://bumbershootsoft.wordpress.com/2018/08/30/an-arbitrary-sprite-positioning-routine-for-the-atari-2600/ with a magic number for RESP0
  * From https://problemkaputt.de/2k6specs.htm:
```
 Writing any value to these addresses sets the associated objects horizontal position equal to the current position of the cathode ray beam, if the write takes place anywhere within horizontal blanking then the position is set to the left edge of the screen (plus a few pixels towards right: 3 pixels for P0/P1, and only 2 pixels for M0/M1/BL).
Note: Because of opcode execution times, it is usually necessary to adjust the resulting position to the desired value by subsequently using the Horizontal Motion function.
```
  * From https://www.atarihq.com/danb/files/TIA_HW_Notes.txt, the following and a table!
```
Resetting the counter takes 4 CLK, decoding the 'start drawing' signal
takes 4 CLK, latching the 'start' takes a further 1 CLK giving a
total 9 CLK delay after a RESP0/1. Since the playfield takes 4 CLK
to start drawing the player is visibly delayed by exactly 5 CLK -
hence the magic '5' :)
```
  * All the emulators show that starting a 3-cycle write to RESP0 on the following column clocks results in  sprites starting on the given clocks:

| STA starts on CPU (pixel) clock within row | RESP0 wrote pixel clock within row | First Sprite Pixel |
| ------------------------------------------ | ---------------------------------- | ------------------ |
| 18 (54)                                    | 63                                 | 3 (71)             |
| 19 (57)                                    | 66                                 | 3 (71)             |
| 20 (60)                                    | 69                                 | 6 (74)             |
| 21 (63)                                    | 72                                 | 9 (77)             |

* VDELPx+GRPxA
* HMOVE
* At this point all sprites should look correct including high scores, bigsprite.a26
* Ball, missiles
* Implement paddle 0 using mouse position in window, see how that goes
* Shrink screen to only visible area
* Implement second joystick
* Factor audio into a header - use from main.cpp and trace_to_pcm.cpp
* Turn audio volume down to like 25%, be around the same volume as Youtube videos
* Add back in capability to capture register writes and reads (annotate with whether in HBLANK or not?), add a command line option
  * Cut to a frame with tracked register writes?


ISSUES
* sprites wrong place - everything and multisprite3.a26
* sprites look wrong - text in Yars Revenge and Kaboom

Audio capture tester

```
for i in kaboom pacman superbreakout yars ; do echo $i ; trace_to_pcm < $i.audio.txt  > $i.pcm_u8_2 ; sox -t u8 -c 2 -r 44100 -b 8 -e unsigned-integer $i.pcm_u8_2  $i.wav ; done
```

