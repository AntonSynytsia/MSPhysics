- Create an installer that would automatically install AMS Library and MSPhysics
  into desired plugins folder.
- Update links when posting
- Make it work on Mac OS X
- Test under SU6, Windows XP
- Implement newton cloth and soft bodies.
- Implement user mesh collision.
- Implement newton vehicle.
- Add world compound interface
    * Convert group of bodies into compound.
    * Convert compound into group of bodies.
- Add path follow joint.
- Add geared joints.
- Add differential joint.
- Add hinge2 joint, a joint with motor and servo in one.
- Add spline joint
    * http://newtondynamics.com/forum/viewtopic.php?f=9&t=4716&p=33835#p33835
    * http://newtondynamics.com/forum/viewtopic.php?f=9&t=8668
- A dialog to set body mass and volume.
- Check which functions to add from LazyScript.
- Add body scaling feature.
- Handle flat and empty collisions for compound from mesh.
- Add MSPhysics functions to Ace text editor if possible.
- Add advanced explosion function.
- Add particle effects
- Add animation tool
- Add JoyStick to controller, serial port, and sliders dialog.
- Add joints
- Rename functions so they are easy to use.
- Update joint connection tool icon.
- Modify joint models and their icons.
- Add watermark text component.
- Update pick and drag tool.
- Allow settings gravity, force, velocity, omega, and other using 3 numeric
  parameters, rather than requiring an Array.
- Compound from convex decomposition doesn't generate right when scaled.
- Repack MSPhysics music and script to reduce size.
- Make sure invalidate world is enabled.
- Make threads compatible on Windows 10.
- Make a Mac OS X compatible project.
- Split documentation functions into groups.
~ Add JointCollection dialog
- Camera follow doesn't work when zooming out or orbiting camera using smooth zoom.
- Body.#get_point_velocity might not be working right.
- Add feature to create material, assign it to multiple bodies and edit it once.
- Add GitHub theme to MSPhysics script.
- Add angle/distance units.
- Add BodyContext, JointContext. Rename all functions for simpler usage.
- Improve the add_note function. Use custom watermark.
- Add collision up vector feature.
- Check if motor damp is supposed to be in degrees.
- Make sure camera follow maintains camera perspective.
- Make sure camera follow body can be deselected by user.

Newton Joint    MSP Joint
front           right
up              front
right           up

=Joints=

Joint ID should consist of five numbers: 10000 + rand(90000)
When joint is created, it should be assigned a unique joint ID.
If joint is copied it should remain with the same ID.
User should have an option to make joint ID unique from other joints.
A body should contain connected joint IDs.
As well, it should have an option to connect closest of the joints with particular IDs.

If user selects a body, the body tab should display a list of connected/potentially connected joints.
When user howers over a particular connected joint in the dialog, it should be highlighted in space.
When user selects a particular joint in a dialog, a joint should be added to selection and a joint tab
should be activated, however, the body should also remain in selection. When user goes back to the body
tab, and selects a different joint, the prior selected joint should be deselected and a new joint should
be selected.

The joints dialog should display joint properties, connected/potentially connected bodies, and joint ID.
Joint ID can be modified.
When joint ID is modified the connected bodies section should be updated.

Additional features:
- User can disconnect joints via dialog.
- User can connect joints via dialog.
- Named joints

When body is selected it should have these joint related commands:
- Connect Closest
- Disconnect All Joints

When joint is selected it should have these commands:
- Discnonnect All Bodies
- Make unique ID.
- Make same ID (if more than one joints are selected)
