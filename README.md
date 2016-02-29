[Homepage](http://sketchucation.com/forums/viewtopic.php?f=323&t=56852)

[GitHub](https://github.com/AntonSynytsia/MSPhysics)

[Documentation](http://www.rubydoc.info/github/AntonSynytsia/MSPhysics/index)


##Overview

MSPhysics is a physics simulation tool similar to SketchyPhysics. Unlike
SketchyPhysics, MSPhysics has a far more advanced scripting API. When it came
to creating FPS type games in SP, scripters were limited to having control over
user input. Scripters had to add extra work-around code in order to gain control
over mouse input, which was very time consuming and not as reliable as wanted.
MSPhysics, however, gives scripters full control over all mouse and keyboard
events, including the mouse wheel. Instead of having keyboard keys serve as
shortcuts, they can be intercepted and used as game controls. Instead of
having mouse wheel serve as a shortcut for the native zoom in/out operation, it
can be intercepted and serve as a command to switch weapons for instance. All
such operations might seem a fantasy, but thanks to Microsoft Windows API, which
is heavily implemented in AMS Library, gaining control over user input is
possible. Along the lines, MSPhysics uses NewtonDynamics 3.14 by Juleo Jerez in
order to produce fast and realistic physics effects. Compared to Newton 1.53,
which was used by SketchyPhysics, Newton 3.14 is faster and more advanced. In
many ways the goal of this project is to bring SketchyPhysics back to life.


## Access

* (Menu) Plugins → MSPhysics → [option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP, Vista, 7, 8, or 10.
  This plugin will not work on Mac OS X as many of the techniques and features
  are achieved through Windows API.
* SketchUp 6 or later. SU2016 64bit is recommended!
* [AMS_Library 3.2.0 +](http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835)


## Installation

Place <i>MSPhysics</i> folder and <i>MSPhysics.rb</i> into SketchUp's _Plugins_
folder. Make sure to download and install AMS Library!

* For SU8 and prior the plugins folder is located in
    - <i>C:/Program Files (x86)/Google/Google SketchUp [n]/</i>
* For SU2013 the plugins folder is located in
    - <i>C:/Program Files (x86)/SketchUp/SketchUp 2013/</i>
* For SU2014 and later the plugins folder is located in
    - <i>C:/Users/[User Name]/AppData/Roaming/SketchUp/SketchUp 20XY/SketchUp/</i>


## Version

* MSPhysics 0.4.0
* NewtonDynamics 3.14
* SDL 2.0.4
* SDL_Mixer 2.0.1


## Release Date

February 28, 2016


## Licence

[MIT](http://opensource.org/licenses/MIT) © 2015 - 2016, Anton Synytsia


## Credits

* **Juleo Jerez** for the [NewtonDynamics](http://newtondynamics.com/forum/index.php) physics engine.
* **Chris Phillips** for ideas from [SketchyPhysics](https://code.google.com/p/sketchyphysics/).
