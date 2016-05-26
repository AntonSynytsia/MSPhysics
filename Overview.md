# Overview

## Scripting Documentation
Scripting in MSPhysics is very similar to SketchyPhysics. The process is very
simple; you select a desired group, open MSPhysics UI, and activate the
scripting tab. Every group, being part of simulation, has its own body context,
an instance of {MSPhysics::Body} class. <tt>MSPhysis::Body</tt> class contains
a set of useful functions and events, allowing user to have control over body in
physics world.

To get more familiar with scripting lets refer to a simple tutorial below.

# Controllable Sphere Tutorial
In this tutorial we will create a sphere and control it with keyboard.

1. To get you started, open SketchUp and create a floor group, settings its
   shape to "Static Mesh".
2. Now that you have a floor, create a sphere or even a cube over the floor,
   and set its shape to "Sphere".
3. Now select the sphere group, open MSPhysics UI, activate the scripting tab,
   and paste this code in it:
    onUpdate {
      mag = this.get_mass() * 5 # Force strength and magnitude.
      v = key('w') - key('s') # This will return 1, 0, or -1.
      h = key('d') - key('a') # This will return 1, 0, or -1.
      this.add_force2(h * mag, v * mag, 0) # Apply force on this object.
    }
4. Start simulation and control the sphere with W,S,A,D keys.
5. All you're left to do is create a simple maze in the staticmesh group, and
   there you have your first simple game.

## Scripting Links
- See {MSPhysics::Simulation}
- See {MSPhysics::World}
- See {MSPhysics::Common}
- See {MSPhysics::Body}
- See {MSPhysics::Controller}


## Accessing Simulation Instance
Use the following to get reference to {MSPhysics::Simulation} instance:

* <tt>this.simulation</tt> or <tt>simulation</tt> - accessible within
  Body and Controller scopes only.
* <tt>MSPhysics::Simulation.instance</tt> - accessible from all scopes.

Displaying frame using simulation log_line or display_note functions:
    onUpdate {
      simulation.log_line(frame)
      # OR
      simulation.display_note(frame)
    }

## Script Errors
All detected script errors will cause simulation to reset. A message box will
pop up displaying an error. Along with that, MSPhysics UI will attempt to locate
script and the line number associated with an error.


## Deleted Bodies/Entities
Deleted entities/bodies in simulation are handled safely. Deleting an entity
that belongs to some Body instance will keep simulation running, however,
referenced body will remain alive until simulation is complete. To destroy the
referenced body, call <tt>Body.#destroy</tt>. One may also call
<tt>Body.#destroy(true)</tt> to destroy body and the referenced entity at once.

Calling any methods of a destroyed body, except for the <tt>Body.#is_valid?</tt>
method, will result in a TypeError causing simulation to reset. It is a good
practice to check if body is valid prior to using its functions.


## Resetting Simulation via Script
Use <tt>MSPhysics::Simulation.reset</tt> in any scope.


## Variables
* Use instance variables (<tt>@my_var</tt>) to implement data within the current
  body's scope.
* Use class variables (<tt>@@my_var</tt>) or +get_var+/+set_var+/+get_set_var+
  functions to implement data under all body scopes and controllers.
* Minimize the use of global variables (<tt>$my_var</tt>) as using plenty of
  them is considered a bad coding habit.


## Shapes
* **Box** - A rectangular prism collision shape that has its width, height, and
  depth determined by a bounding box of group.
* **Sphere** - A spherical collision shape with a diameter determined by the
  width, height, or depth of a bounding box of a group, depending on which side
  is the longest.
* **Cone** - A conical collision shape with diameter and height determined by a
  bounding box of a group.
* **Cylinder** - A cylindrical collision shape with diameter and height
  determined by a bounding box of a group.
* **Chamfer Cylinder** - A cylindrical collision shape with rounded corners.
  Dimensions are determined by bounding box of a group.
* **Capsule** - A stretched/compressed spherical collision shape. Dimensions are
  determined by a bounding box of a group.
* **Convex Hull** - A convex collision shape calculated from all the faces in a
  group/component.
* **Compound** - A compound collision with all sub-geometry and sub-groups
  considered as separate convex hulls.
* **Compound from CD** - A compound collision calculated from a convex
  decomposition algorithm.
* **Static Mesh** - A static tree collision derived from all faces of a group.
* **Null** - A dynamic collision shape with no collision.


## Shape Notes
* Entity's transformation X-axis is used as an up vector for collision shapes
  like cone and cylinder.
* A bounding box, which is used to determine dimensions for many collision
  shapes, is not the actual group's/component's bounding box, but rather a
  custom one, calculated from all the faces within a group/component. This means
  that all edges, points, and construction geometry inside a group or a
  component are not taken into consideration in calculating the size a collision
  bounding box.
* Make sure to have all back faces inside and all front faces outside as Newton
  calculates collision intersections based on front faces only. This applies to
  static mesh and compound from mesh collision shapes.
* A group/component will not be added to simulation if its calculated bounding
  box turns out flat or empty. This applies to shapes dependant on bounding box
  for collision.
* A group/component will not be added to simulation if its faces are all
  coplanar. This applies to convex hull collision shape.
* A group/component will not be added to simulation if its transformation axis
  are not perpendicular to each other. Non-uniform transformation are not
  tolerated by Newton.
* The collisions for scaled and flipped groups/components are computed properly.


## Tips & Security
* Avoid calling start/abort/commit operation or undo/redo while simulation is
  running; otherwise, simulation could behave improperly.
* Minimize the use of the <b>Compound From CD</b> shape as it takes time to
  generate collision and collision isn't always generated properly.
* When dealing with tiny objects, increase world scale.
* Avoid the use of high poly models as they could cause lag or result in a
  BugSplat while generating collision.
* Apply clean-up to your model before uploading it to 3DWarehouse as cleanup
  could decrease file size and improve performance. [TT CleanUp](http://sketchucation.com/forums/viewtopic.php?f=323&t=22920)
  plugin is recommended!
* Add <i>MSPhysics</i> to the tags list when uploading your model to
  3DWarehouse. This way people searching 3DWarehouse would easily find your
  model by writing "MSPhysics" in the search box. Searchers could sort search
  results by uploaded date to find the most rescent uploads.


## Game Mode
When it comes to creating games, like FPS, there are two commands to consider
adding to Body Script tab:

**One** Disable pick-and-drag tool and intercept mouse operations, such as
right-clicking an entity and zooming in/out via the mouse wheel. This can be
done via <tt>MSPhysics::Simulation.#set_mode</tt> command:

    onStart {
      simulation.set_mode(1)
      # ...
    }

<i>Game mode</i> disables pick-and-drag tool and gives scripter full control
over user input. For instance, the mouse wheel could be used to switch weapons,
rather than being a shortcut for a zoom tool.

**Two** Set view full screen. This can be done via the
<tt>MSPhysics::Simulation.#view_full_screen</tt> command:

    onStart {
      # Set viewport full screen when simulation starts.
      simulation.view_full_screen(true)
    }
    onEnd {
      # Set viewport back to original placement when simulation resets.
      simulation.view_full_screen(false)
    }


## Optimization & Performance
* The slower the speed (smaller update timestep), the more realistic the
  simulation is.
* Use exact solver model when precision is more important than speed.
* Use iterative solver model when speed is more important than precision.
* To improve performance in general, go to <i>Preferences -> OpenGL</i> and
  reduce <i>Anti-Aliasing</i> as poor performance could be a result of graphics
  processing.
* Use iterative solver when simulating stacked geometry, like walls, as exact
  solver might result in lag.
* Enable continuous collision check or reduce simulation speed to prevent
  objects from passing or penetrating through each other at high speeds.


## Music & Sound
MSPhysics uses SDL and SDL Mixer for its music and sound API. Use MSPhysics UI
sound tab to embed sounds. When adding sounds, be cautious of file size as model
size increases significantly. The difference between music and sound is that
only one music can be mixed and played at a time, meanwhile up to 20 sounds can
be played simultaneously. In games, music is used as background music and sounds
are used as effects. Another difference between music and sound is that music
can be played of various formats, meanwhile sound is limited to a few formats.
Use <tt>simulation.play_music</tt> in body context to play music. Use
<tt>simulation.play_sound</tt> in body context to play sound.

To play sound in 3d, use <tt>simulation.set_sound_position</tt> command:

    onKeyDown { |k, v, c|
      channel = simulation.play_sound("ping") # Play sound and get channel
      pos = this.get_group.bounds.center # Sound position in global space
      range = 500 # Hearing range in meters
      if channel != nil
        simulation.set_sound_position(channel, pos, 500)
      end
    }


## Joystick
Joystick is accessible from the common context and from the controller context.
See {MSPhysics::Common} and {MSPhysics::Controller} for details.
