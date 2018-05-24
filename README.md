# PID_crystallizer
Arduino-based programmable ramping PID controller 

 Crystalheater Controller
 (c) N. Trapp @ Small Molecule Crystallography Center
 ETH Zürich, 2018

 derived from:
 "Sous Vide Controller", (c) Bill Earl - for Adafruit Industries
 based on Arduino PID and PID AutoTune Libs, (c) Brett Beauregard
 Usage:
        right         : start or next
        left          : stop or previous,
                        during ramping: stop and hold current temp
        up/down       : during ramping&heating: show info screens
        shift         : hold for larger increments in settings
        shift & +     : start autotune (temp must be within 1°C
                        of setpoint)
        shift & -     : PID parameter settings
        shift & right : start ramp
