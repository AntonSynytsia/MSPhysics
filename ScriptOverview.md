# Script Overview
This overview focus more on scripting rather than the general use. You may click
on the [Wiki](https://github.com/AntonSynytsia/MSPhysics/wiki) link for
information about the general use.


## Scripting Documentation
Scripting in MSPhysics is very similar to SketchyPhysics. The process is very
simple; you select a desired group, open MSPhysics UI, and activate the
scripting tab. Every group, being part of simulation, has its own body context,
an instance of {MSPhysics::Body} class. <tt>MSPhysis::Body</tt> class contains
a set of useful functions and events, allowing user to have control over body in
physics world. Refer to a simple tutorial below to get more familiar with
scripting.


## Controllable Sphere Tutorial
In this tutorial we will create a sphere and control it with keyboard.

1. To get you started, open SketchUp and create a floor group, settings its
   shape to "Static Mesh" ([Context Menu] MSPhysics -> Shape -> Static Mesh).
2. Now that you have a floor, create a sphere or even a cube over the floor,
   and set its shape to "Sphere".
3. Now select the sphere group, open MSPhysics UI, activate the scripting tab,
   and paste this code in it:

        # Triggered when simulation starts
        onStart {
          # Have the desired velocity as 5 m/s
          @des_vel = 5
        }

        # Triggered every time the world updates
        onUpdate {
          # Compute the force to apply
          timestep = simulation.update_timestep # (in seconds)
          force = this.mass / timestep # (in Newtons)
          # Get current velocity
          cur_vel = this.get_velocity
          # leftx() returns a value ranging from -1.0 to 1.0, depending on the state
          # of A and D keys or the horizontal position of the left joystick.
          fx = (leftx() * @des_vel - cur_vel.x) * force
          # lefty() returns a value ranging from -1.0 to 1.0, depending on the state
          # of W and S keys or the vertical position of the left joystick.
          fy = (lefty() * @des_vel - cur_vel.y) * force
          # Apply force on this body.
          this.add_force(fx, fy, 0)
        }

4. Start simulation and control the sphere with W,S,A,D keys or left joystick.
5. All you're left to do is create a simple maze in the staticmesh group, and
   there you have your first simple game.


## Scripting Reference
* See {MSPhysics::Simulation} for various functions in simulation. Access from
  the body context:

        onUpdate {
          simulation.some_simulation_method
          # Or (Accessible from everywhere)
          MSPhysics::Simulation.instance.some_simulation_method
        }

* See {MSPhysics::World} for various function in the current world. Access from
  the body context:

        onUpdate {
          world.some_world_method
          # OR
          simulation.world.some_world_method
          # OR (Accessible from everywhere)
          MSPhysics::Simulation.instance.world.some_world_method
        }

* See {MSPhysics::CommonContext} for various common functions accessible in the
  body context and controller context.
  Access from the controller context: <tt>some_common_method</tt>.
  Access from the body context:

        onUpdate {
          some_common_method
        }

* See {MSPhysics::BodyContext} for various events accessible in the body
  context. The <tt>MSPhysics::BodyContext</tt> encapsulates
  <tt>MSPhysics::Body</tt> instance. See {MSPhysics::Body} for various functions
  accessible from every body in simulation. Note that in order to call a body
  method, you must have a <tt>this</tt> key word in front, for example,
  <tt>this.mass = 20</tt>.
  Access from the body context:

        onUpdate { # An event belonging to MSPhysics::BodyContext
          this.some_body_method # A method belonging to MSPhysics::Body
        }

* See {MSPhysics::ControllerContext} for various functions accessible in the
  controllers. Access from the controller context:
  <tt>some_controller_method</tt>


## Accessing Simulation Instance
As described above, use the following code snippets to get reference to the
{MSPhysics::Simulation} instance:

* <tt>simulation</tt> - accessible within BodyContext and ControllerContext scopes only.
* <tt>MSPhysics::Simulation.instance</tt> - accessible from all scopes.

Displaying frame using <tt>log_line</tt> or <tt>display_note</tt> functions:

    onUpdate {
      simulation.log_line(frame)
      # OR
      simulation.display_note(frame)
    }


## Script Errors
All detected script errors will cause simulation to reset. A message box will
pop up displaying an error. Along with that, MSPhysics UI will attempt to locate
script and the line number associated with an error. Script errors in the
controller contexts are outputted in Ruby Console rather than resetting
simulation.


## Deleted Bodies/Entities
Deleted entities/bodies in simulation are handled safely. Deleting an entity
that belongs to some Body instance will keep simulation running, however,
referenced body will remain alive until simulation is complete. To destroy the
referenced body, call <tt>Body.#destroy</tt>. You may also call
<tt>Body.#destroy(true)</tt> to destroy body along with referenced entity at
once.

Calling any methods of a destroyed body, except for the <tt>Body.#valid?</tt>
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
  depth determined by the bounding box of a group.
* **Sphere** - A spherical collision shape with width, height, or depth
  determined by the bounding box of a group.
* **Cone** - A conical collision shape with diameter and height determined by a
  bounding box of a group.
* **Cylinder** - A cylindrical collision shape with diameter and height
  determined by the bounding box of a group.
* **Chamfer Cylinder** - A cylindrical collision shape with rounded corners.
  Dimensions are determined by the bounding box of a group.
* **Capsule** - A stretched/compressed spherical collision shape. Dimensions are
  determined by the bounding box of a group.
* **Convex Hull** - A convex collision shape calculated from all the faces in a
  group/component.
* **Compound** - A compound collision with all sub-geometry and sub-groups
  considered as separate convex hulls.
* **Compound from CD** - A compound collision with convex hulls calculated by a
  convex decomposition algorithm.
* **Static Mesh** - A static tree collision derived from all faces of a group.
* **Null** - A dynamic collision shape with no collision. Useful for linker
  bodies, such as the neck of a rag-doll.


## Shape Notes
* Entity's transformation X-axis is used as a default up indicator for collision
  shapes like cone and cylinder.
* A bounding box, which is used to determine dimensions for many collision
  shapes, is not the actual group's/component's bounding box, but rather a
  custom one, calculated from all the faces within a group/component. This means
  that all edges, points, and construction geometry inside a group or a
  component are not taken into consideration in calculating the size of a
  collision bounding box.
* Make sure to have all back faces inside and all front faces outside as Newton
  calculates collision intersections based on front faces only. This applies to
  static mesh and compound from mesh collision shapes.
* A group/component will not be added to simulation if its calculated bounding
  box turns out flat or empty. This applies to all collision shapes except
  "Null" and "Static Mesh", as "Null" collisions don't have any collisions and
  "Static Mesh" collisions may be flat.
* A group/component will not be added to simulation if its faces are all
  coplanar. This applies to convex hull collision shape, which also applies to
  compound shapes since their collision consists of convex sub-collisions.
* A group/component will not be added to simulation if its axis are not
  perpendicular to each other. Non-uniform transformation are not tolerated by
  Newton.
* The collisions for scaled and flipped groups/components are computed properly.


## Tips & Security
* Avoid calling start/abort/commit operation or undo/redo while simulation is
  running; otherwise, simulation could behave improperly.
* Minimize the use of the <b>Compound From CD</b> shape as it takes time to
  generate collision and collision isn't always generated properly.
* Increase world scale if dealing with tiny objects.
* Avoid the use of high poly models as they could cause lag or result in a
  BugSplat while generating collision (very unlikely but may happen).
* Apply clean-up to your model before uploading it to 3DWarehouse as cleanup
  could decrease file size and improve performance. [TT CleanUp](http://sketchucation.com/forums/viewtopic.php?f=323&t=22920)
  plugin is recommended!
* Add <i>MSPhysics</i> to the tags list when uploading your model to
  3DWarehouse. This way people searching 3DWarehouse would easily find your
  model by writing "MSPhysics" in the search box. Searchers could sort search
  results by uploaded date to find the most recent uploads.


## Game Mode
When it comes to creating games, like FPS, there are two commands to consider
adding to Body Script tab:

1. Enable game mode. <i>Game mode</i> disables pick-and-drag tool and gives
   scripter full control over user input. For instance, the mouse wheel could be
   used to switch weapons, rather than being a shortcut for a zoom tool. This
   can be done with the <tt>MSPhysics::Simulation.#mode=}</tt> command:

        onStart {
          simulation.mode = 1
          # ...
        }
        onMouseWheelRotate { |x, y, dir|
          # Do something...
        }

2. Set view full screen. This can be done with the
   <tt>MSPhysics::Simulation.view_full_screen</tt> command:

        onStart {
          # Set viewport full screen when simulation starts.
          simulation.view_full_screen(true)
        }
        onEnd {
          # Set viewport back to original placement when simulation resets.
          simulation.view_full_screen(false)
        }


## Optimization & Performance
* The slower the speed (smaller update timestep), the more accurate the
  simulation is.
* Use exact solver model when precision is more important than speed.
* Use iterative solver model when speed is more important than precision.
* To improve performance in general, go to <i>Preferences -> OpenGL</i> and
  reduce <i>Anti-Aliasing</i> as poor performance could be caused by slow
  graphics processing.
* Sometimes, performance lag could be caused by an anti-virus software,
  particularly Windows Defender on Windows 10. Disabling it should yield better
  performance.
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

To play sound in 3d, use <tt>simulation.position_sound</tt> command:

    onKeyDown { |k, v, c|
      channel = simulation.play_sound("ping") # Play sound and get channel
      pos = this.group.bounds.center # Sound position in global space
      range = 500 # Hearing range in meters
      if channel != nil
        simulation.position_sound(channel, pos, 500)
      end
    }


## Joystick
Joystick is accessible from the common context which is inherited by the body
and controller context. See {MSPhysics::CommonContext} for various functions and
their examples about joystick. Here is a simple example which outputs the state
of the left joystick onto screen:

    onTick {
      x = joystick(:leftx)
      y = joystick(:lefty)
      txt = "xaxis #{x}\nyaxis #{y}"
      simulation.display_note(txt)
    }

Note that xbox or any other joystick controller must be attached to PC before
SketchUp starts. Otherwise the joystick commands won't respond to it.


## Joints
Besides creating and connecting joints using the joint tool, it is possible to
create joints using the MSPhysics API. Assume you have to bodies in world; one
named 'body1' and the other named 'body2'. Here is a way to connect these two
bodies once they come in contact with each other:

    # Paste this script into 'body1'
    onStart {
      @joint = MSPhysics::Fixed.new(this.world, this, this.get_matrix, this.group)
      # Disconnect joint when the applied force is reaches or passes the desired
      # breaking force.
      #@joint.breaking_force = 50000
    }

    onTouch { |toucher, point, normal, force, speed|
      # Check if the touching body is 'body2' and connect only if the joint is
      # not already connected.
      if toucher.group.name == 'body2' && @joint.valid? && !@joint.connected?
        @joint.connect(toucher)
      end
      # Once the joint is connected it can be disconnected with @joint.disconnect command.
    }

MSPhysics has many joints and all of them can be created and controlled with the
MSPhysics API. Refer to the list below for documentation links to all the
joints:

* {MSPhysics::Joint} an abstract of common functions for all joints.
* {MSPhysics::Hinge}
* {MSPhysics::Motor}
* {MSPhysics::Servo}
* {MSPhysics::Slider}
* {MSPhysics::Piston}
* {MSPhysics::UpVector}
* {MSPhysics::Spring}
* {MSPhysics::Corkscrew}
* {MSPhysics::BallAndSocket}
* {MSPhysics::Universal}
* {MSPhysics::Fixed}
* {MSPhysics::CurvySlider}
* {MSPhysics::CurvyPiston}
* {MSPhysics::Plane}

To demonstrate, how to use joint API extensively, lets assume our model has two
groups, named 'A' and 'B'. We want these groups to connect to each other with a
hinge joint, whenever they contact. We also want them to disconnect whenever key
X is pressed. This must be a repetitive process.

There are various ways of coding it. One way is to create a Hinge object at the
beginning of simulation and connect/disconnect it when desired. Another way is to
create and connect a hinge joint whenever bodies A and B touch and destroy the
hinge joint when key X is pressed. The script below utilizes the first approach,
that is, the same Hinge object is used whenever connecting and disconnecting:

    onStart {
      # Find bodies A and B
      @bodyA = simulation.find_body_by_name('A')
      @bodyB = simulation.find_body_by_name('B')
      # To create a hinge joint, we must pass the arguments denoted below:
      # Hinge.#initialize(world, parent, pin_tra, group = nil)
      # We will use body A as hinge parent and body B as hinge child.
      # We will use body A's transformation as pin_tra, which indicates joint
      # origin and axes.
      # The pin_tra will stick to the parent body as if it were part of it;
      # so, moving the parent body, will automatically rearrange the pin_tra.
      # This is viable because, if the parent body moved, when connecting the hinge
      # joint, it won't be necessary to respecify the pin_tra.
      @joint = Hinge.new(simulation.world, @bodyA, @bodyA.get_matrix)
    }

    onTouch { |toucher, point, normal, force, speed|
      # Limit connection to body B only.
      if toucher == @bodyB # We can also do this instead: if toucher.group.name == 'B'
        # Connect body B to body A if not already connected.
        @joint.connect(@bodyB) if !@joint.connected?
      end
    }

    onTick {
      # Disconnect the joint whenever key x is pressed
      if key('x') == 1 && @joint.connected?
        @joint.disconnect
      end
    }


## Update vs Tick
MSPhysics BodyContext has two events: <tt>onUpdate</tt> and <tt>onTick</tt>.
As well, as their cousins, <tt>onPreUpdate</tt>, <tt>onPostUpdate</tt>,
<tt>onPreFrame</tt>, and <tt>onPostFrame</tt>. Meanwhile the tick/frame an the
update events have similar use, their purpose us not the same.

Simulation UI has an option, called <i>Update Rate</i>. <i>Update Rate</i>
indicates the number of times the physics world is to be updated per frame. If
the <i>Update Rate</i> were three, for example, the physics world would be
updating three times every frame, thus roughly increasing the speed by a factor
of three.

The update events are triggered [update_rate] times per frame. The tick/frame
events are triggered only one time per frame, regardless of the update rate.

These events have different purposes.

The tick/frame events must be used for moving, drawing, controlling, and
manipulating SketchUp geometry. Since the viewport is redrawn only once per
frame, it would be unnecessary and performance consuming to move geometry more
than once per frame, from the update events. So geometry manipulation functions
must be called from <tt>onPreFrame</tt>, <tt>onTick</tt>, or
<tt>onPostFrame</tt> events.

Physics calculations and force/torque/velocity/omega controlling must be
performed through the <tt>onPreUpdate</tt>, <tt>onUpdate</tt>, or
<tt>onPostUpdate</tt> events.

Let's assume, the world update rate were three, and you called the
<tt>add_force(force_vector)</tt> function from the <tt>onTick</tt> event.
Because the <tt>onTick</tt> event is triggered only once per frame, the force
would be applied for only one of the three times the physics world is updated:

    PhysicsWorldUpdate
    ApplyForce
    PhysicsWorldUpdate
    PhysicsWorldUpdate
    PhysicsWorldUpdate
    ApplyForce
    PhysicsWorldUpdate
    PhysicsWorldUpdate
    PhysicsWorldUpdate
    ApplyForce
    ...

This will result in a frequent jumping/bumping effect of the body the force is
applied to. If you were to call the <tt>add_force(force_vector)</tt> function
from the <tt>onUpdate</tt> event, the result would be a lot smoother, because
that way you're applying force every time the world is updated:

    PhysicsWorldUpdate
    ApplyForce
    PhysicsWorldUpdate
    ApplyForce
    PhysicsWorldUpdate
    ApplyForce
    PhysicsWorldUpdate
    ApplyForce
    ...

To summarize it up, use the update events for applying forces, torques,
velocities, and omegas. Use the tick/frame events for controlling SketchUp
entities.


## Kinematic Bodies
Unlike dynamic bodies, kinematic bodies are static bodies with mass, that are
suitable for emulating movable platforms. Let's assume, your model has a group
that you want to use as a movable platform, that would transition 10 meters,
along the x-axis, back and forth, every 25 seconds.

One way of doing it is by assigning the platform a dynamic type and connecting
it to a piston joint, with controller set to <tt>10 * oscillator2(1/25.0)</tt>.

Another way of doing it is by assigning the platform a kinematic type and adding
it the following script:

    onStart {
      @frequency = 1/25.0
      @amplitude = 10
    }

    onPreUpdate {
      this.set_velocity(@amplitude * oscillator2_slope(@frequency), 0, 0)
      this.set_omega(0, 0, 0)
      this.integrate_velocity(simulation.update_timestep)
    }

You may also assign a mass to a kinematic body to have its motion be altered by
the contacting bodies. See this link for more info:
http://newtondynamics.com/forum/viewtopic.php?f=9&t=9058
