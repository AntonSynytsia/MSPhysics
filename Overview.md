# Overview

## Documentation
Each Newton body has its own body context.
See {MSPhysics::BodyContext}


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
  to implement data under all body scopes and controllers.


## Security
* Avoid creating variables with an underscore in front. This will ensure that
  none of the body variables are modified.
* Avoid calling start/abort/commit operation or undo/redo while simulation is
  running; otherwise, simulation will reset improperly.


## Shapes
* Components in simulation = less collision calculation = faster start.
* Make sure to have all back faces inside and all front faces outside, as Newton
  calculates collision intersections for the front faces only.
* Use entity's x-axis as the up vector for shapes like cone, and cylinder.


## Rules
* Minimize the use of the <b>Compound From Mesh</b> shape, as it takes time to
  generate the collision, and as the collisions are generated improperly for the
  complex shapes.
* Avoid the use of high poly models as they cause lag, and are known to crash
  SketchUp while generating collisions.
* Apply clean-up before uploading your model to 3D Warehouse, as it decreases
  file size, and improves performance. [TT CleanUp](http://sketchucation.com/forums/viewtopic.php?f=323&t=22920)
  is recommended!
* Don't forget to add <i>MSPhysics</i> to the tags list when sharing your model.


## Game Mode
Switch simulation to the <i>game mode</i> when creating FPS type games:

    onStart {
      simulation_tool.set_mode(1)
      # ...
    }

<i>Game mode</i> disables the drag tool, and gives you full control over user
input. A mouse wheel could be used to switch weapons, rather than having the
zoom tool activating, for instance.


## Scaled Bodies
To scale body, simply call <tt>Body.#set_matrix(new_tra)</tt> with the scale
factors within the new transfomration. Not all bodies can be scaled though.
Scaling compound and staticmesh bodies was disabled because Newton is incomplete
here.


## Performance
* Use exact solver model when precision is more important than speed.
* Use interactive solver model when speed is more important than precision.
* To improve performance in general, go to <i>Preferences -> OpenGL</i> and
  reduce <i>Anti-Aliasing</i>.
