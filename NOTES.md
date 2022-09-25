TODO
* RESP0, RESP1 without HMOVE and VDELPx+GRPxA
  * What does VCS do?  Stellarator? - do experiments at boundaries of timing

* VDELPx+GRPxA

* HMOVE
* At this point all sprites should look correct including high scores, bigsprite.a26
* Ball, missiles
* Pac-Man death sounds wrong - what's going on there?
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

