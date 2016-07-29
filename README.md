[Homepage](http://sketchucation.com/forums/viewtopic.php?f=323&t=56852)

[Wiki](https://github.com/AntonSynytsia/MSPhysics/wiki)

[Documentation](http://www.rubydoc.info/github/AntonSynytsia/MSPhysics/index)

[GitHub](https://github.com/AntonSynytsia/MSPhysics)


##Overview

MSPhysics is a real-time physics simulation plugin for SketchUp, similar to
SketchyPhysics. Unlike SketchyPhysics, MSPhsyics is a written completely from
scratch, integrating the latest NewtonDynamics physics SDK. The differences
between the two is that MSPhysics is a lot faster, has a far more advanced
scripting API, and comes with a reliable Replay animation tool. Furthermore
MSPhysics uses AMS Library, which on the Windows side, allows taking control
over user input, such as mouse and keyboard, and switching SketchUp fullscreen.
With such advantage, creating FPS type games in MSPhysics, without interference
of various keyboard/mouse shortcuts, is now a possibility. At the moment, the
project is under development, particularly within the Wiki branch. All in all
huge credit goes to Julio Jerez for writing the NewtonDynamics physics engine.
As well as to those who contributed in making the project a reality.


## Access

* (Menu) Plugins → MSPhysics → [option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP or later / Mac OS X 10.5+
* SketchUp 6 or later. SU2016 64bit is recommended!
* [AMS_Library 3.3.0+](http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835)


## Manual Installation Instructions
If installing manually, place <i>MSPhysics</i> folder and <i>MSPhysics.rb</i>
into SketchUp's <b>Plugins</b> folder. Ensure to download and install
AMS Library too!

* For SU8 and prior the path to the plugins folder is:
    - <b>C:/Program Files (x86)/Google/Google SketchUp X/Plugins/</b>
* For SU2013 the path to the plugins folder is:
    - <b>C:/Program Files (x86)/SketchUp/SketchUp 2013/Plugins/</b>
* For SU2014 and later the path to the plugins folder is:
    - <b>C:/Users/[User Name]/AppData/Roaming/SketchUp/SketchUp 20XY/SketchUp/Plugins</b>
    - <b>%appdata%/SketchUp/SketchUp 20XY/Sketchup/Plugins/</b>

When extracted and moved into the Plugins folder, the content should be
resembled in the following way:

<i>../Plugins/</i>
* <i>MSPhysics</i> folder
* <i>MSPhysics.rb</i> file
* <i>ams_Lib</i> folder
* <i>ams_lib.rb</i> file


## Version

* MSPhysics 0.9.2
* NewtonDynamics 3.14
* SDL 2.0.4
* SDL_Mixer 2.0.1


## Release Date

July 28, 2016


## Licence

[MIT](http://opensource.org/licenses/MIT) © 2015-2016, Anton Synytsia


## Credits

* **Julio Jerez** for the [NewtonDynamics](http://newtondynamics.com/forum/index.php) physics SDK.
* **Chris Phillips** for ideas from [SketchyPhysics](https://code.google.com/p/sketchyphysics/).
* **István Nagy (PituPhysics)** and **Faust07** for testing.
