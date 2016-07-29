## 0.9.2 - July 28, 2016
- Fixed the bug in fullscreen checkbox option. Thanks to Faust07 for report.
- Added statusbar notifications when Replay is being loaded/saved.
- For size, performance, and stability purposes Replay information is now saved
  into a separate file, named <b>[model_name].mspreplay</b>.

## 0.9.1 - July 26, 2016
- Reworked the replay animation tool. Now replay is faster than before. Also,
  replay data is compressed with Zlib, which reduces model/file size by a
  significant factor.
- Added fullscreen mode, game mode, undo on end, and hide joint layer options to
  the UI dialog.
- Minor bug fixes and improvements.

## 0.9.0 - July 10, 2016
- Compatibility with Mac OS X 10.5+; still limited when it comes to managing
  keyboard, mouse, and MIDI, but the most important stuff is working just fine.
- Fixed a bug where MSPhysics failed to load previous editor settings. Thanks to
  PituPhysics for report.
- Joystick should work even if connected after SketchUp starts.
- Added alignment power option to CurvySlider and CurvyPiston joints.
- Added LinearGear, AngularGear, and RackAndPinion constraints.
- Reworked the scripting API. Various functions were renamed and rearranged.
    * Renamed various (<tt>get_some_method</tt>/<tt>set_some_method</tt>)
      functions, that acquired one parameter, to
      <tt>some_method</tt>/<tt>some_method=</tt>.
    * Added <tt>BodyContext</tt> class. All body events, such as
      <tt>onUpdate</tt>, were moved to the <tt>BodyContext</tt>.
    * To reference a <tt>Body</tt> associated with the <tt>BodyContext</tt>,
      call <tt>BodyContext.#this</tt>.
    * To reference a <tt>BodyContext</tt> associated with the <tt>Body</tt>,
      call <tt>Body.#context</tt>.
    * Renamed <tt>Common</tt> and <tt>Controller</tt> to <tt>CommonContext</tt>
      and <tt>ControllerContext</tt>.
    * Scripting scope was changed to <tt>BodyContext</tt>. To call body methods
      from the scripting scope, it is now essential to place the keyword "this"
      in front.
    * To call <tt>BodyContext</tt> methods, which are mostly events, from the
      scripting scope, you don't need to place any keywords in front.
    * Renamed <tt>get_body_by_group</tt>, <tt>get_group_by_name</tt>, etc.. to
      <tt>find_body_by_group</tt>, <tt>find_by_by_name</tt>, etc...
    * Renamed all <tt>is_some_method?</tt> functions to <tt>some_method?</tt>.
    * Many methods that focused on general stuff were moved to AMS Library. For
      instance, the whole <tt>Group</tt> and <tt>Geometry</tt> classes were
      transferred to AMS Library.
- The scripting changes were made for coding convenience and are very likely to
  remain unchanged in the upcoming releases.
- Enchanted control panel and the dialog.
- Various other bug fixes and improvements.

## 0.8.0 - June 12, 2016
- Added CurvySlider and CurvyPiston joints.
- Many bug fixes and improvements.

## 0.7.4 - May 26, 2016
- Fixed encoding issues.

## 0.7.3 - May 25, 2016
- Implemented latest Newton version which fixes static mesh collision bug.
- Fixed bug in replay after stopping animation. Thanks to Faust07 for report.
- Added a feature to save replay into model.
- Added joystick support.

## 0.7.2 - May 22, 2016
- Fixed typos.

## 0.7.1 - May 17, 2016
- Various optimizations and minor bug fixes.
- Added a feature to load MSPhysics without SDL in case SDL is not supported on
  your system.

## 0.7.0 - April 18, 2016
- Added particle interface.
- Control panel no longer steals keyboard focus.
- Fixed look_at function. Thanks to PituPhysics for report.
- Added a feature to automatically generate slider controller for particular
  joints.
- Added an option to control body mass via the properties tab.
- Added curve functions similar to SketchyPhysics.
- Added a feature to follow/track from scene data, just like in SketchyPhysics.
- Added more options to the script tab of the dialog.
- Body properties tab now displays internal and connected joints.

## 0.6.0 - April 11, 2016
- Added strong mode to linear and angular springs.
- Added a feature to allow customizing angle and distance units for joints.
- Added more options to the MSPhysics context menu.
- Revamped the connect closest joints feature.
- Added cursor visibility control functions.
- Added sliders control panel.
- Completed ball & socket and universal joints.
- Updated joint icons. Thanks to PituPhysics for helping.

## 0.5.0 - March 22, 2016
- Added corkscrew joint.
- Removed dependency on external VC libraries.
- Fixed crash that could occur on particular Windows platforms. Thanks to
  pcberdwin for testing.
- Optimized the dialog and fixed some bugs.

## 0.4.0 - February 28, 2016
- Upgraded to custom recompiled SDL2 and SDL2 Mixer.
- Added an option to export replay animation to Skindigo.
- Added a feature to the animation tool to record/replay shadows and rendering
  options.
- Added a feature to allow scene transitioning while simulation is running.
- More work with joints.
- More work with scripting API.
- Added a feature to control dialog font, theme, and other settings. Thanks to
  PituPhysics for MSPhysics Light and Dark themes.
- Reworked the update-rate feature, which now updates simulation n times per
  frame rather than updating simulation once every n frames. Now default
  simulation speed is similar to SketchyPhysics.
- Updated buoyancy function.

## 0.3.0 - December 30, 2015
- Reverted to SDL1 and SDL_Mixer1, which fixed the crash that occurred after
  using SketchUp for a few minutes.
- Optimized the C++ extension.
- Upgraded to Newton 3.14.
- Added joints; still more work to go.
- Added replay animation tool.

## 0.2.1 - April 16, 2015
- Fixed a bug which prevented MSPhysics from working in 32bit SU versions.

## 0.2.0 - April 06, 2015
- Alpha release 2.

## 0.1.0 - April 26, 2014
- Alpha release 1.
