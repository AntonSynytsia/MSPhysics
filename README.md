[Homepage](http://sketchucation.com/forums/viewtopic.php?f=323&t=56852)

##Overview

MSPhysics is a physics simulation tool similar to SketchyPhysics. Unlike,
SketchyPhysics, MSPhysics has a far more advanced scripting API. When it came
to creating FPS type games in SP, scripters were limited in having control over
user input. Scripters had to add extra work-around code in order to gain control
over mouse input, which was very time consuming and not as reliable as wanted.
MSPhysics, however, gives scripters full control over all mouse and keyboard
events, including the mouse wheel. Instead of having keyboard keys serve as
shortcuts, they could be intercepted and used as game controls. Instead of
having the mouse wheel serve as a native zoom in/out operation, it could be
intercepted and serve as a command to switch weapons. All such operations might
seem a fantasy, but thanks to Microsoft Windows API, which is heavily
implemented in AMS Library, gaining control over user input is possible. Along
the lines, MSPhysics uses NewtonDynamics 3.13 by Juleo Jerez in order to produce
fast and realistic physics effects. Compared to Newton 1.53, which was used by
SketchyPhysics, Newton 3.13 is said to be ten times faster and has far more
advanced physics SDK. In many ways the goal of this project is to bring
SketchyPhysics back to life.


## Access

* (Menu) Plugins → MSPhysics → [option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP, Vista, 7, or 8.
  This plugin will not work on Mac OS X as many of the techniques and features
  are achieved via the mighty Windows API. Other platforms might be incompatible
  with Windows.
* SketchUp 6 +. Latest SU version is recommended!
  To get it fully working with SU2013 and below, your SU ruby core must be
  upgraded to 1.8.6 or 1.8.7. See plugin homepage for ruby upgrade instructions.
* [AMS_Library 2.2.0 +](http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835)


## Installation

Place the files within the _src_ folder into SketchUp's _Plugins_ folder. Make
sure to download AMS Library as well!

* For SU8 and below the plugins folder is located in
    - <i>C:/Program Files (x86)/Google/Google SketchUp [n]/</i>
* For SU2013 the plugins folder is located in
    - <i>C:/Program Files (x86)/SketchUp/SketchUp 2013/</i>
* For SU2014 and above the plugins folder is located in
    - <i>C:/Users/[User Name]/AppData/Roaming/SketchUp/SketchUp 20XY/SketchUp/</i>


## Version

* MSPhysics 0.2.1
* NewtonDynamics 3.13
* SDL 2.0.3
* SDL_Mixer 2.0.0


## Release Date

April 16, 2015


## Change Log

* **April 16, 2015**: 0.2.1
    - Fixed a bug which prevented MSPhysics from working in 32bit SU versions.
* **April 05, 2015**: 0.2.0
    - Alpha release 2.
* **April 26, 2014**: 0.1.0
    - Alpha release 1.


## Licence

[MIT](http://opensource.org/licenses/MIT) © 2015, Anton Synytsia


## Credits

* **Juleo Jerez** for the [NewtonDynamics](http://newtondynamics.com/forum/index.php) physics engine.
* **Chris Phillips** for ideas from [SketchyPhysics](https://code.google.com/p/sketchyphysics/).
