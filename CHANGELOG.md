## 1.1.0 -
- Updated to latest revision of NewtonDynamics 3.14.
- Removed world scale option.
- Made dialog and control panel non-modal on Mac OS X.
- Use UI::HtmlDialog for SU2017 and later.
- Added Ruby 2.5/2.7 binaries.

## 1.0.3 - October 16, 2017
- Improved joint connection tool. Now closest joints are determined by closest
  distance to object's bounding box rather than distance to object's center.
- Added an option to connect/disconnect to from all alike instances. This makes
  the joint connection tool more user friendly when dealing with chain like
  connections.
- Improved angular stiffness of a Fixed joint.
- Added <tt>MSPhysics::Joint.#get_tension1</tt>,
  <tt>MSPhysics::Joint.#get_tension2</tt>,
  <tt>MSPhysics::Body.#net_joint_force</tt>, and
  <tt>MSPhysics::Body.#net_joint_torque</tt>.
- Added context menu option in simulation to toggle visibility of a selected
  entity.
- Made UI dialog compatible with IE9.

## 1.0.2 - July 19, 2017
- Fixed control panel error on Mac OS X.

## 1.0.1 - July 11, 2017
- Fixed a crash that occurs when continuous collision check and non-collidable
  options are enabled together.

## 1.0.0 - July 9, 2017
- Fixed the bug with negative magnet forces. Thanks to Ali Karakus for the
  report.
- Made the magnet force to dissipate quadratically over the magnet range, rather
  than linearly, as previously was. Now the following equation is used for
  computing the actual force: <tt>f * (d - r)^2 / r^2</tt>; where f is the
  maximum magnet force (in Newtons), d is the distance between magnet and the
  magnetic body (in meters), and r is the maximum magnet range (in meters).
- Added an option to control magnets with a slightly different equation:
  <tt>actual_force = magnet_strength / distance^2</tt>.
- Fixed the onTouching continuous response bug. Thanks to Wim van den Dungen for
  the report.
- Fixed the issue with MSPhysics UI text input values not saving on Mac OS X.
  Thanks to TheSniper for the report.
- Added an option to control Piston by speed.
- Added 'rotation along Z-axis' option to Spring joint.
- Fixed joystick input and added <tt>leftz</tt>/<tt>rightz</tt> commands. Thanks
  to SynSuka3D for the report and request.
- Made joystick input be read based on the type of connected joystick, which
  again fixes the improper input mapping. Thanks to Istvan Nagy for the report.
- Added better oscillator functions.
- Fixed the gravity bug in type 1 particle effects.
- Made type 2 and 3 particles be exportable with replay.
- Windows only: Added an option to abort the exporting of replay animation.
- Simulation stop command no longer erases copied/emitted bodies and particles.
- Added <tt>MSPhysics::Body.#inertia</tt> for acquiring the rotational
  equivalent of mass.
- Fixed the improper behavior of static bodies. Thanks to Istvan Nagy for the
  report.
- Fixed the improper behaviour of touch events with non-collidable bodies.
- Added an option to create kinematic bodies. See script overview on using them.
- Added additional context menu options for controlling body selectable options,
  such as magnet mode.
- Added <tt>toggle_key(vk)</tt> command for getting the toggled state of a
  virtual key.
- Added <tt>key_slider(...)</tt> command.
- Added <tt>accumulator(rate, delay)</tt>, <tt>repeater(rate, hold, delay)</tt>,
  and <tt>singular_repeater(rate, delay, id)</tt> commands.
- Upgraded to the latest version of NewtonDynamics. Note that models made with
  previous MSPhysics versions may need to be updated for compatibility with the
  new version.
- Fixed a crash that could occur when generating convex/compound collisions from
  tiny meshes.
- Fixed the bug with assigning huge values for mass and other body properties.
  Thanks to SynSuka3D for the report.
- Fixed an unhandled exception of touch events and emitters, which caused jerky
  movements when exporting to replay. Thanks to Faust07 for reporting the issue.
- All functions involving rates and lifetimes now assume time in seconds. This,
  too, applies to emitters and particles, ensuring that simulation results
  remain persistent whenever changing update timestep and rate.
- Added an option to control the up direction of specific shapes.
- Removed 'Compound from CD' shape due to its instability.
- Improved simulation scene transitioning.
- Added a feature to animate scenes.
- Added a feature to do keyboard pan/orbit.
- Added an option to ignore hidden instances.
- Added recoil and delay options to the emitter.
- Improved functionality of many joints.
- Improved joint connection tool.
- Removed 'Connect Closest Joints' option and made copyable joints a default
  feature.
- Reverted to running simulation via the animation API, rather than timer, for
  the proper rendering of shader and shadows.

## 0.9.9 - December 22, 2016
- Reworked all joints. This includes bug fixes to the limits of the
  BallAndSocket and Universal joints, improved behaviour of the CurvySlider and
  CurvyPiston joints, removal of robust and flexible constraints, and various
  improvements to the behaviours of all other joints.
- Added a Plane joint.
- Added a feature to control Spring and Hinge joint oscillation with parameters
  basing on Hooke's law.
- Reworked the UI dialog. This addresses various syntax errors within the layout
  code, the undesired behavior of the fullscreen command, the bug with
  simulation tab not showing preset settings, the bug with editor not preserving
  size, and the issue with the dialog not considering DPI scale factor in
  SU2017.
- Added a feature to adjust the scale of the MSPhysics UI. This feature only
  works on Windows.
- Added a feature to switch Ace Editor fullscreen (use F11).
- Fixed the issue where newly defined script functions were only accessible to
  the body instance defining them rather than to all the body instances.
- Added onTick, onPreFrame, and onPostFrame events. Also changed the nature of
  onUpdate, onPreUpdate, and onPostUpdate events. See scripting documentation
  for details. This includes the removal of some Body functions that aren't
  necessary anymore.
- Fixed the issue with the saved replay animation not loading properly if the
  model wasn't previously saved.
- Removed support for running without SDL, as adding it in the first place has
  been unnecessary.
- Improved the behaviour of pick and drag tool.
- Improved buoyancy.

## 0.9.8 - November 13, 2016
- Fixed camera follow/track commands in scenes.
- Fixed issue with the orbit tool not activating while simulation is running.
  Thanks to cristyan, faust07, and PituPhysics for report.
- Fixed loading errors on Mac OS X. Thanks to Oxer for report.
- Added help-box to the UI dialog.

## 0.9.7 - November 10, 2016
- Fixed the incompatibility with Windows XP.
- Fixed encoding with non-ASCII user names.
- Fixed loading errors on Windows.

## 0.9.6 - November 4, 2016
- Fixed encoding with non-ASCII user names.

## 0.9.5 - October 31, 2016
- Made keyboard input and MIDI compatible with Mac OS X. On Mac OS X, the
  control panel must be active to prevent the triggering of SketchUp commands.
  This version requires updating AMS Library to versions 3.4.0 or later.
- Fixed dialog issues with the updated jQuery.
- Added a feature that displays MSPhysics version, whenever simulation starts.
- Added another play command which starts simulation from selection. All hidden
  groups/components are ignored, all non-selected groups/components act static,
  and all selected groups/components are dynamic.
- Added a stop command which ends simulation without resetting positions.
- Added 'Control by Speed' mode to CurvyPiston joint.
- Fixed improper behavior with curvy joints where connected bodies flip at
  particular points if rotation is disabled and alignment enabled.
- Fixed improper behavior with curvy joints where looping is disabled, yet the
  curve is complete.
- Updated toolbar icons.
- Many minor fixes and upgrades.

## 0.9.4 - October 08, 2016
- Fixed friction bug. Thanks to Ulrich. W. (bambutec) for report.
- Improved performance with closest joints when simulation starts.

## 0.9.3 - August 02, 2016
- Fixed crash with loading large replay data files. Thanks to faust07 for
  report.
- Fixed loading error that occurs if account name contains Latin characters.
  Thanks to Micceo for report.

## 0.9.2 - July 28, 2016
- Fixed the bug in fullscreen checkbox option. Thanks to faust07 for report.
- Added statusbar notifications when replay is being loaded/saved.
- For size, performance, and stability purposes replay information is now saved
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
- Fixed bug in replay after stopping animation. Thanks to faust07 for report.
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
