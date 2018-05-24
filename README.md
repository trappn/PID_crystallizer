# PID_crystallizer
Arduino-based programmable ramping PID controller 

Crystalheater Controller<br>
(c) N. Trapp @ Small Molecule Crystallography Center<br>
ETH Zürich, 2018<br>

derived from:<br>
"Sous Vide Controller", (c) Bill Earl - for Adafruit Industries<br>
based on Arduino PID and PID AutoTune Libs, (c) Brett Beauregard<br>
<br>

<p>
<ul>
  <li>designed around Arduino Uno with  </li>
  <li>  </li>
  <li>  </li>
  <li>  </li>
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
