# Overview

## Scripting Documentation
Each Newton body has its own body context, see {MSPhysics::BodyContext}.


## Utility
- See {MSPhysics::Body}
- See {MSPhysics::CommonContext}
- See {MSPhysics::Simulation}


## Accessing Simulation Instances
To get reference to {MSPhysics::SimulationTool} instance you may use the
following:

* <tt>this.simulation_tool</tt> if you code inside the Body scope only.
* <tt>MSPhysics::SimulationTool.instance</tt>
* <tt>$msp_simulation_tool</tt>

To get reference to {MSPhysics::Simulation} instance you may use the following:

* <tt>this.simulation</tt> if you code inside the Body scope only.
* <tt>MSPhysics::SimulationTool.instance.simulation</tt>
* <tt>$msp_simulation</tt>


## Script Errors
All detected script errors will cause simulation to reset. A message box will
pop up displaying the error.


## Deleted Entities
A deleted entity/body in simulation is automatically detected and removed from
further processing. Don't be afraid to use <code>Body.#destroy</code> or
<code>Body.#entity.erase!</code> functions.


## Variables
* Use instance variables (<tt>@my_var</tt>) to implement data under the current
  body's scope.
* Use class variables (<tt>@@my_var</tt>) or +get_var+/+set_var+/+get_set_var+
  functions to implement data under all body scopes and controllers.


## Security
* Avoid creating instance or class variables with an underscore in front. This
  will ensure that none of the body variables are modified.
* Avoid calling start/abort/commit operation or undo/redo while simulation is
  running; otherwise, simulation will reset improperly.


## Shapes
* Components in simulation = less collision calculation = faster start.
* Make sure to have all back faces inside and all front faces outside, as Newton
  calculates collision intersections for the front faces only.
* Use entity's x-axis as the up vector for shapes like cone, and cylinder.
* Use null collision for bodies that don't need collision.
* For some reason body stacks made from box collision collide improperly, use
  convex collision instead.


## Rules
* Minimize the use of the <b>Compound From Mesh</b> shape, as it takes time to
  generate collision, and as the collisions are generated improperly for many
  shapes.
* Avoid the use of high poly models as they cause lag, and are known to crash
  SketchUp while generating collisions.
* Apply clean-up before uploading your model to 3D Warehouse, as it decreases
  file size, and improves performance. [TT CleanUp](http://sketchucation.com/forums/viewtopic.php?f=323&t=22920)
  is recommended!
* Don't forget to add <i>MSPhysics</i> to the tags list when sharing your model.


## Game Mode
Switch simulation to the <i>game mode</i> when creating FPS type games:

    onStart {
      simulation_tool.mode = 1
      # ...
    }

<i>Game mode</i> disables the drag tool, and gives you full control over user
input. Mouse wheel could be used to switch weapons, rather than having the zoom
tool activating, for instance.


## Scaled Bodies
To scale body, simply call <tt>Body.#set_matrix(new_tra)</tt> with the scale
factors inscribed within the new transformation matrix. Not all bodies can be
scaled though. You cannot scale compound and staticmesh bodies as Newton doesn't
support such feature, yet.


## Flipped Entities
Flipped entities will force the body to unflip when simulation starts. This is
the bug that could be fixed in the next version.


## Small Bodies
Small bodies are not recommended, as the centre of mass is calculated improperly
for them. Make sure all physics bodies are in realistic scale.


## Collision at High Speeds / Box Stacks
To prevent objects from passing or penetrating through each other at high
speeds, you can either enable continuous collision or reduce simulation speed.
It's recommended to reduce simulation speed rather than enabling c.c. mode when
performing box stacks as c.c. check is known to affect performance.


## Speed Rule
The smaller the speed (update step), the more accurate the simulation is.


## Performance
* Use exact solver model when precision is more important than speed.
* Use interactive solver model when speed is more important than precision.
* To improve performance in general, go to <i>Preferences -> OpenGL</i> and
  reduce <i>Anti-Aliasing</i>.
