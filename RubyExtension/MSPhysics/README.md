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
* (Context Menu) MSPhysics → [Option]
* MSPhysics Toolbars


## Compatibility and Requirements

* Microsoft Windows XP or later / Mac OS X 10.5+
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

* MSPhysics 0.9.7
* NewtonDynamics 3.14
* SDL 2.0.5
* SDL_Mixer 2.0.1


## Release Date

November 10, 2016


## Licence

[MIT](http://opensource.org/licenses/MIT) © 2015-2016, Anton Synytsia


## Credits

* **Julio Jerez** for the [NewtonDynamics](http://newtondynamics.com/forum/index.php) physics SDK.
* **Chris Phillips** for ideas from [SketchyPhysics](https://code.google.com/p/sketchyphysics/).
* **István Nagy (PituPhysics)** and **Faust07** for testing.
