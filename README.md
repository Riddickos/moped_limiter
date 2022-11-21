# Aprilia limiter
<p> Limiter is designed for the 2 stroke Aprilia moped RS4 50 model
<p> It uses hall sensor to read RPM from the gearbox
<p> Arduino Nano is used as main controller <br>
MLT-BT05 is used to get commands from the phone remotely
<p> To block the engine it has solid state relay which turns off kill switch on the moped
<p> Supported commands: <br>
	- TOGGLE_LIMIT Enables/Disables limiter <br>
	- TOGGLE_DEBUG Activates debug output to connected bluetooth device <br>
	- RPM_[value] sets required max RPM after which controller suppose to start blocking the engine <br>
	- DELAY_[value] sets required delay for how long to block the engine after each rotation over the limit
