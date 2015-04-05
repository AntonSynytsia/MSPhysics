# Overview

## Scripting Documentation
Every body in MSPhysics has its own body context, see {MSPhysics::Body}.


## Fast Links
- See {MSPhysics::Simulation}
- See {MSPhysics::World}
- See {MSPhysics::Common}
- See {MSPhysics::Body}
- See {MSPhysics::Controller}


## Accessing Simulation Instance
Use the following to get reference to {MSPhysics::Simulation} instance:

* <tt>this.simulation</tt> or <tt>simulation</tt> - accessible within
  Body and Controller scopes only.
* <tt>MSPhysics::Simulation.instance</tt> - accessible from everywhere.


## Script Errors
All detected script errors will cause simulation to reset. A message box will
pop up displaying an error. Along with that, MSPhysics UI will attempt to locate
entity and line number associated with an error.


## Deleted Entities
Deleted entities/bodies in simulation are handled safely. Deleting an entity
that belongs to some Body instance will keep simulation running, however,
referenced body will remain alive until simulation is complete. To destroy the
referenced body, simply call <tt>Body.#destroy</tt>. You may also call
<tt>Body.#destroy(true)</tt> to destroy body and the referenced entity at once.

Calling any methods of a destroyed body, except for the <tt>Body.#is_valid?</tt>
method, will result in a TypeError causing simulation to reset. It is a good
practice to check if body is valid prior to using its functions.


## Resetting Simulation via Script
Unlike in SketchyPhysics, calling <tt>MSPhysics::Simulation.reset</tt> in
any code context will terminate simulation safely without crashes...


## Variables
* Use instance variables (<tt>@my_var</tt>) to implement data within the current
  body's scope.
* Use class variables (<tt>@@my_var</tt>) or +get_var+/+set_var+/+get_set_var+
  functions to implement data under all body scopes and controllers.
* Minimize the use of global variables (<tt>$my_var</tt>) as using plenty of
  them is considered a bad coding habit.


## Shapes
* **Box** - A rectangular prism collision shape that has its width, height, and
  depth determined from the bounding box of an entity.
* **Sphere** - A spherical collision shape with a diameter determined from the
  width, height, or depth of the bounding box of an entity, depending on which
  side is the longest.
* **Cone** - A conical collision shape with diameter and height determined from
  the bounding box of an entity.
* **Cylinder** - A cylindrical collision shape with diameter and height
  determined from the bounding box of an entity.
* **Chamfer Cylinder** - A cylindrical collision shape with rounded corners.
  Dimensions are determined from entity's bounding box.
* **Capsule** - A stretched/compressed spherical collision shape. Dimensions are
  determined from entity's bounding box.
* **Convex Hull** - A convex collision shape calculated from all the faces in a
  group/component.
* **Compound** - A compound collision with all sub-geometry and sub-groups
  considered as separate convex hulls.
* **Compound from CD** - A compound collision calculated from a convex
  decomposition algorithm.
* **Static Mesh** - A static tree collision derived from all faces within a
  group.
* **Null** - A dynamic collision shape with no collision.


## Shape Notes
* Entity's transformation x-axis is used as an up vector for collision shapes
  like cone and cylinder.
* Entity's bounding box, which is used to determine dimensions for many
  collision shapes, is not the actual group's/component's bounding box, but
  rather a custom one, calculated from all the faces within a group/component.
  This means that all edges, points, and construction geometry inside a group or
  a component are not taken into consideration in calculating the size a
  collision bounding box.
* Make sure to have all back faces inside and all front faces outside as Newton
  calculates collision intersections based on front faces only. This applies to
  static mesh collision shapes.
* A group/component will not be added to simulation if its calculated bounding
  box turns out flat or empty. This applies to shapes dependant on bounding box
  for collision.
* A group/component will not be added to simulation if its faces are all
  coplanar. This applies to convex hull collision shape.
* A group/component will not be added to simulation if its transformation axis
  are not perpendicular to each other. Non-uniform transformation is usually
  achieved through scaling entities the fancy way.
* Scaled groups/components are handled properly in terms of collision
  calculation.
* Flipped groups/components are handled properly as well.


## Tips & Security
* Avoid creating instance or class variables with an underscore in front. This
  will ensure that none of the body internal variables are modified.
* Avoid calling start/abort/commit operation or undo/redo while simulation is
  running; otherwise, simulation could run and reset improperly.
* Minimize the use of the <b>Compound From CD</b> shape as it takes time to
  generate collision and collision isn't always generated properly.
* Be sure to use realistic sized bodies as very small groups/components might
  not be handled properly.
* Avoid the use of high poly models as they could cause lag or result in a
  BugSplat while generating collision.
* Apply clean-up before uploading your model to 3DWarehouse as it could decrease
  file size and improve performance. [TT CleanUp](http://sketchucation.com/forums/viewtopic.php?f=323&t=22920)
  is recommended!
* Add <i>MSPhysics</i> to the tags list when uploading your model at
  3DWarehouse. This way people searching 3DWarehouse would easily find your
  model by writing "MSPhysics" in the search box, and sort models by uploaded
  date to find the newly uploaded models.


## Game Mode
When it comes to creating games, like FPS for instance, there are two things
developer might want to do:

**One** Disable pick-and-drag tool and intercept mouse operations, such as
right-clicking an entity and zooming in/out via the mouse wheel. This can be
done via <tt>MSPhysics::Simulation.#set_mode</tt> method:

    onStart {
      simulation.set_mode(1)
      # ...
    }

<i>Game mode</i> disables pick-and-drag tool and gives scripter full control
over user input. For instance, the mouse wheel could be used to switch weapons,
rather than having the zoom tool activating.

**Two** Set view full screen. Don't know about you, but playing FPS games full screen
is something. This can be done via the
<tt>MSPhysics::Simulation.#view_full_screen</tt> method:

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
  objects from passing or penetrating into each other at high speeds.


## Music & Sound
MSPhysics uses SDL2 and SDL2 Mixer for its music and sound API. Use MSPhysics UI
sound tab to embed sounds. When adding sounds, be cautious of file size as model
size increases significantly. The difference between music and sound is that
only one music can be mixed and played at a time, meanwhile up to 20 sounds can
be played simultaneously. In games, music is used as background music and sounds
are used as effects. Another difference between music and sound is that music
can be played of various formats, meanwhile sound is limited to a few formats.
Use <tt>play_music</tt> in body context to play music. Use <tt>play_sound</tt>
in body context to play sound. These commands will return Music/Sound instance
or nil if unsuccessful.

To play sound in 3d, use <tt>MSPhysics::Sound.#set_position</tt> method:

    onStart {
      sound = play_sound("sound_name")
      if sound
        pos = Geom::Point3d.new(0,0,100)
        sound.set_position(pos)
        sound.set_hearing_range(1000)
      end
    }
