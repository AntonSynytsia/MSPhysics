[Homepage](http://sketchucation.com/forums/viewtopic.php?f=323&t=56852)

[Wiki](https://github.com/AntonSynytsia/MSPhysics/wiki)

[Documentation](http://www.rubydoc.info/github/AntonSynytsia/MSPhysics/index)

[GitHub](https://github.com/AntonSynytsia/MSPhysics)


##Overview

MSPhysics is a real-time physics simulation plugin for SketchUp, similar to
SketchyPhysics. Unlike SketchyPhysics, MSPhsyics is written completely from
scratch, integrating the latest NewtonDynamics physics SDK. The difference
between the two is that MSPhysics is significantly faster, has a more advanced
scripting API, and comes with a reliable Replay animation tool, which allows
recording simulation and exporting to SkIndigo, KerkyThea, and a sequence of
images.

Another difference between MSPhysics and SketchyPhysics is having advantage over
user input. In SketchyPhysics there was a struggle in creating keyboard and
mouse controlled games. Whenever simulation would run, there had to be an active
control panel window that would redirect user input, that is to prevent the
interference of SketchUp's keyboard and mouse shortcuts. In MSPhysics, however,
the control panel is not necessary. MSPhysics utilizes AMS Library, which on the
windows side, allows taking control over user input and switching SketchUp
fullscreen. Imagine playing FPS games in SketchUp, in fullscreen, without having
various SketchUp commands taking control over simulation. All that is possible
with MSPhysics.

At the moment, the project is under development. Various optimizations,
improvements, and bug fixes are made to enhance stability. Once MSPhysics gets
to a stable level, that is version 1.0.0, the focus will shift toward making
tutorials and enriching the Wiki branch.

Huge credit goes to Julio Jerez for writing the NewtonDynamics physics engine.
As well as, to those who contributed in making the project a reality.


## Access

* (Menu) Plugins → MSPhysics → [option]
* (Context Menu) MSPhysics → [Option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP or later / Mac OS X 10.6+
* SketchUp 6 or later. SU2016 64bit is recommended!
* [AMS_Library 3.4.0+](http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835)


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
    * For Windows
        - For SU8 and below the plugins folder is located in
            - <tt>C:\Program Files (x86)\Google\Google SketchUp #\</tt>
        - For SU2013 the plugins folder is located in
            - <tt>C:\Program Files (x86)\SketchUp\SketchUp 2013\</tt>
        - For SU2014 and later the plugins folder is located in
            - <tt>C:\Users\[User Name]\AppData\Roaming\SketchUp\SketchUp 20##\SketchUp\</tt>
            - %appdata%\SketchUp\SketchUp 20##\SketchUp\</tt>
            - <tt>C:\ProgramData\SketchUp\SketchUp 20##\SketchUp\Plugins</tt>
    * For Mac OS X
        - For SU8 and below the plugins folder is located in
            - <tt>[YOUR USER NAME]/Library/Application Support/Google SketchUp #/SketchUp/plugins</tt>
        - For SU2013 and later the plugins folder is located in
            - <tt>> Library > Application Support > SketchUp # > SketchUp > Plugins</tt>

2. Select <i>ams_Lib</i> folder, <i>ams_Lib.rb</i> file, <i>MSPhysics</i> folder, and <i>MSPhysics.rb</i> file and delete them.


## Version

* MSPhysics 0.9.9
* NewtonDynamics 3.14
* SDL 2.0.5
* SDL_Mixer 2.0.1


## Release Date

December 22, 2016


## Licence

[MIT](http://opensource.org/licenses/MIT) © 2014-2016, Anton Synytsia


## Credits

* **Julio Jerez** for the [NewtonDynamics](http://newtondynamics.com/forum/index.php) physics engine.
* **Chris Phillips** for ideas from [SketchyPhysics](https://code.google.com/p/sketchyphysics/).
* **István Nagy (PituPhysics)**, **Faust07**, and many others for testing.

## Author

Anton Synytsia
