# PID_crystallizer
Arduino-based programmable ramping PID controller 
<p>
Crystalheater Controller<br>
(c) N. Trapp @ Small Molecule Crystallography Center<br>
ETH Zürich, 2018<br>
</p>
<p>  
derived from:<br>
"Sous Vide Controller", (c) Bill Earl - for Adafruit Industries<br>
based on Arduino PID and PID AutoTune Libs, (c) Brett Beauregard<br>
</p>
<p>
 Used libraries: PID_v1.h, PID_AutoTune_v0.h, Wire.h, Adafruit_RGBLCDShield.h, SmoothThermistor.h, EEPROM.h
</p>
<p>
<ul>
  <li>designed around Arduino Uno with Robotdyn RGB 16x2 keypad shield, 100K NTC thermistor, standard DC/DC SSD and any 12V DC heating element</li>
  <li>minimized assembly time with ATX PSU & 3D printed case</li>
  <li>4 profiles for ramping (linear, oscillate, steps, sawtooth)</li>
  <li>uses PID autotuning or manually set PID parameters</li>
  <li>can heat or cool</li>
  <li>logging via USB serial</li>
  <li>external wire harness allows placing heater box in fridge for cooling below RT</li>
  <li>stores settings in EEPROM</li>
  <li>projected runtime is displayed when changing settings</li>
  <li>thermistor safety: switches off when sensor shorts, breaks or is unplugged</li>
  <li>make sure to adjust Tmax (default:60) & Tmin (default:-40) in the sketch to safe values for your heater setup before compiling!</li>
  <li>extensive testing has shown long-term stability (>2 weeks) and accuracy (+/-0.1°C)</li>
</ul>
</p>

Usage:<br>
<table style="">
  
  <tr>
   <td>right</td><td>start or next</td>
  </tr>
  <tr>   
   <td>left</td><td>stop or previous, during ramping: stop and hold current temp</td>
  </tr>
  <tr>
   <td>up/down</td><td>during ramping&heating: show info screens</td>
  </tr>
  <tr>
   <td>shift</td><td>hold for larger increments in settings</td>
  </tr>
  <tr>
   <td>shift & +</td><td>start autotune (temp must be within 1°C of setpoint)</td>
  </tr>
  <tr>
   <td>shift & -</td><td>PID parameter settings</td>
  </tr>
  <tr>
   <td>shift & right</td><td>start ramp</td>
  </tr>
</table>
