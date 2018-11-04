[Homepage](http://sketchucation.com/forums/viewtopic.php?f=323&t=56852)

[Wiki](https://github.com/AntonSynytsia/MSPhysics/wiki)

[Documentation](http://www.rubydoc.info/github/AntonSynytsia/MSPhysics/index)

[GitHub](https://github.com/AntonSynytsia/MSPhysics)


## Overview

MSPhysics is a real-time physics simulation extension for SketchUp.

MSPhysics allows doing physics simulation of groups and component instances,
where each object can be assigned a specific shape, specific states, density,
contact properties, magnet properties, script, and more. These features allow
complex interactions between objects and the physics world. The parameters of
the physics world, such as gravity, update timestep, and solver model, can too
be adjusted.

MSPhysics also allows interconnecting objects with joints (constraints) for
establishing mechanical interactions between objects. MSPhysics has 14 joints:
Hinge, Motor, Servo, Slider, Piston, UpVector, Spring, Corkscrew, BallAndSocket,
Universal, Fixed, CurvySlider, CurvyPiston, and Plane. Each joint can be
assigned its specific properties, such as minimum and maximum position/angle
limits, as well as controllers for controlling position, angle, speed, linear or
angular friction, and other. The controllers themselves can be stimulated with a
use of a slider controller, keyboard key(s), joystick, and/or a desired script.
This flexibility allows creating and inventing many things, such as vehicles,
robots, and instruments.

MSPhysics also comes with a reliable Replay animation tool, which allows
recording simulation and exporting to SkIndigo, KerkyThea, or a sequence of
images.

In many ways, MSPhysics resembles its predecessor SketchyPhysics. First of all,
MSPhysics is not a new version of SketchyPhysics. MSPhsyics is written entirely
from scratch, integrating the latest Newton Dynamics Physics SDK and heavily
basing on a C++ extension. Both are capable of achieving same things, in one way
or the other; however, MSPhysics is significantly faster and goes further,
especially with a lot of the features described above. MSPhysics has a by far
more advanced and a well documented scripting API, allowing users to write more
proficient scripts for their models. Another difference is having advantage over
user input. In SketchyPhysics there was a struggle in creating keyboard and
mouse controlled games. Whenever simulation would run, there had to be an active
control panel window to redirect user input, that is to prevent the interference
of SketchUp's keyboard shortcuts. In MSPhysics, however, the control panel is
not necessary. MSPhysics utilizes AMS Library, which on the Windows side, allows
taking control over user input and switching SketchUp fullscreen. Imagine
playing FPS games in SketchUp, in fullscreen mode, without having various
keyboard commands taking control over the simulation. All that is possible with
MSPhysics.


## Access

* (Menu) Plugins → MSPhysics → [option]
* (Context Menu) MSPhysics → [Option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP or later / Mac OS X 10.6+
* SketchUp 6 or later. SU2017 64bit is recommended!
* [AMS_Library 3.5.0+](http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835)


## ThirdParty Libraries

* NewtonDynamics 3.14
* SDL 2.0.5
* SDL_Mixer 2.0.1
* jQuery 1.12.4
* Ace 1.2.6
* Chosen 1.7.0
* dhtmlxSlider 4.6


## License

[MIT](http://opensource.org/licenses/MIT) (C) 2014 - 2018, Anton Synytsia
