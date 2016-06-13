## 0.8.0 - Jun 12, 2016
- Added CurvySlider and CurvyPiston joints.
- Many bug fixes and improvements.

## 0.7.4 - May 26, 2016
- Fixed encoding issues.

## 0.7.3 - May 25, 2016
- Implemented latest Newton version which fixes static mesh collision bug.
- Fixed bug in replay after stopping animation. Thanks to faust07 for report.
- Added a feature to save replay into model.
- Added joystick support. See {MSPhysics::Common} for joystick usage.

## 0.7.2 - May 22, 2016
- Fixed typos.

## 0.7.1 - May 17, 2016
- Various optimizations and minor bug fixes.
- Added a feature to load MSPhysics without SDL in case SDL is not supported on your system.

## 0.7.0 - Apr 18, 2016
- Added particle interface.
- Control panel no longer steals keyboard focus.
- Fixed look_at function. Thanks to PituPhysics for report.
- Added a feature to automatically generate slider controller for particular joints.
- Added an option to control body mass via the properties tab.
- Added curve functions similar to SketchyPhysics.
- Added a feature to follow/track from scene data, just like in SketchyPhysics.
- Added more options to the script tab of the dialog.
- Body properties tab now displays internal and connected joints.

## 0.6.0 - Apr 11, 2016
- Added strong mode to linear and angular springs.
- Added a feature to allow customizing angle and distance units for joints.
- Added more options to the MSPhysics context menu.
- Revamped the connect closest joints feature.
- Added cursor visibility control functions.
- Added sliders control panel.
- Completed ball & socket and universal joints.
- Updated joint icons. Thanks to PituPhysics for helping.

## 0.5.0 - Mar 22, 2016
- Added corkscrew joint.
- Removed dependency on external VC libraries.
- Fixed crash that could occur on particular platforms. Thanks to pcberdwin for testing.
- Optimized the dialog and fixed some bugs.

## 0.4.0 - Feb 28, 2016
- Upgraded to custom recompiled SDL2 and SDL2 Mixer.
- Added an option to export replay animation to Skindigo.
- Added a feature to the animation tool to record/replay shadows and rendering options.
- Added a feature to allow scene transitioning while simulation is running.
- More work with joints.
- More work with scripting API.
- Added a feature to control dialog font, theme, and other settings. Thanks to PituPhysics for MSPhysics Light and Dark themes.
- Reworked the update-rate feature, which now updates simulation n times per frame rather than updating simulation once every n frames. Now simulation speed is similar to SketchyPhysics.
- Updated buoyancy function.

## 0.3.0 - Dec 30, 2015
- Reverted to SDL1 and SDL_Mixer1, which fixed the crash that occurred after using SketchUp for a few minutes.
- Optimized the C++ extension.
- Upgraded to Newton 3.14.
- Added joints; still more work to go.
- Added replay animation tool.

## 0.2.1 - Apr 16, 2015
- Fixed a bug which prevented MSPhysics from working in 32bit SU versions.

## 0.2.0 - Apr 06, 2015
- Alpha release 2.

## 0.1.0 - Apr 26, 2014
- Alpha release 1.
