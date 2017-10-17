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

Huge credit goes to Julio Jerez for writing the Newton Dynamics physics engine;
as well as, to those who contributed in making this project a reality.


## Access

* (Menu) Plugins → MSPhysics → [option]
* (Context Menu) MSPhysics → [Option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP or later / Mac OS X 10.6+
* SketchUp 6 or later. SU2017 64bit is recommended!
* [AMS_Library 3.5.0+](http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835)


## Installation Instructions

1. Navigate to [MSPhysics Homepage](http://sketchucation.com/forums/viewtopic.php?f=323&t=56852).
2. Download <i>ams_Lib_x.y.z.rbz</i> and <i>MSPhysics_x.y.z.rbz</i>.
3. Open SketchUp.
4. Select <i>(Menu) Window → Preferences</i>.
5. Navigate to <i>Extensions</i> section within the <i>System Preferences</i> dialog.
6. Click <i>Install Extension...</i> button, select <i>ams_Lib_x.y.z.rbz</i>, and click <i>Open</i>.
7. Repeat the same step for <i>MSPhysics_x.y.z.rbz</i>
8. Ensure that both <i>AMS Library</i> and <i>MSPhysics</i> extensions are checked.


## Uninstallation Instructions

Follow these steps in case you intend to uninstall the plugin.

1. Navigate to the desired Plugins folder:
    * On Windows
        * For SU8 and below the plugins folder is located in
            * <tt>C:\Program Files (x86)\Google\Google SketchUp #\</tt>
        * For SU2013 the plugins folder is located in
            * <tt>C:\Program Files (x86)\SketchUp\SketchUp 2013\</tt>
        * For SU2014 and later the plugins folder is located in
            * <tt>C:\Users\[User Name]\AppData\Roaming\SketchUp\SketchUp 20##\SketchUp\</tt>
            * <tt>%appdata%\SketchUp\SketchUp 20##\SketchUp\</tt>
            * <tt>C:\ProgramData\SketchUp\SketchUp 20##\SketchUp\Plugins</tt>
    * On Mac OS X
        * For SU8 and below the plugins folder is located in
            * <tt>[YOUR USER NAME]/Library/Application Support/Google SketchUp #/SketchUp/plugins</tt>
        * For SU2013 and later the plugins folder is located in
            * <tt>> Library > Application Support > SketchUp # > SketchUp > Plugins</tt>
2. Select <i>ams_Lib</i> folder, <i>ams_Lib.rb</i> file, <i>MSPhysics</i> folder, and <i>MSPhysics.rb</i> file and delete them.


## Version

1.0.3


## Release Date

October 16, 2017


## ThirdParty Libraries

* NewtonDynamics 3.14
* SDL 2.0.5
* SDL_Mixer 2.0.1
* jQuery 1.12.4
* Ace 1.2.6
* Chosen 1.7.0
* dhtmlxSlider 4.6


## Licence

[MIT](http://opensource.org/licenses/MIT) © 2014-2017, Anton Synytsia


## Credits

* **Julio Jerez** for [NewtonDynamics](http://newtondynamics.com/forum/index.php) physics engine.
* **Chris Phillips** for some ideas from [SketchyPhysics](https://code.google.com/p/sketchyphysics/).
* **István Nagy (PituPhysics)**, **Faust07**, and many others for testing.


## Copyright

© 2014-2017 Anton Synytsia.
All Rights Reserved.


## Author

Anton Synytsia {mailto:anton.synytsia@gmail.com}
