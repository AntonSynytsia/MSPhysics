# Newton Issues

- Compound scaled sub-collisions cause improper collision intersections.
- Compounds don't collide into scaled tree collisions, while other convex bodies
  do.
~ NewtonMeshSimplify not working.


# Fix/Optimize/To Do

## Version 1.0.0
- Make sure animation tool is similar to SketchyPhysics, so that Twilight is
  compatible. En-clause simulation tool between start and commit operation.
- Modify Body.#look_at method, add accel, damp
- Fix the jumping of bodies and improper behaviours!
- Slider on interactive solver works improperly
- Add position= to slider
- When simulation starts
    - Create all bodies first
    - Use Sketchup.active_model.entities to check for top level joints.
    - Look in un-ignored group entities one level deep and see if they
      have joints as well.
    - Create joints from joint attributes.
- Create examples, such as onTouch, explosion, emitter, thruster, joints.
- Remove not needed methods from the body. Body.set_sleep_state not working...

- Choose particular bodies to be available in the solver:
    http://newtondynamics.com/forum/viewtopic.php?f=9&t=6923&hilit=frozen+bodies#p47966
    ~ simulation.set_simulation_bodies
- Joints go to sleep
    http://newtondynamics.com/forum/viewtopic.php?f=9&t=7710&p=52972&hilit=set+body+sleeping#p52972
- Spline Joint
    http://newtondynamics.com/forum/viewtopic.php?f=9&t=4716&p=33835#p33835
    http://newtondynamics.com/forum/viewtopic.php?f=9&t=8668

- Body.#add_buoyancy doesn't work properly.
- Update links when posting
- Add script version
- Check which functions to add from the LazyScript.
- Finish particle effects
- Joints dialog
- Joints: Hinge, Servo, Motor, Spring, Up, Slider, Piston, Fixed, Ball,
  Universal, CorkScrew.
- Test the access of get/set_var vs body class variables and methods under
  controllers.
- Add user mesh collision
- Bodies connected with joints disappear when excessive force is applied.
- Add body angular and linear damping properties.
- Draw all collisions in one time to improve performance. Use lines for such
  purpose.
- Crash occurs when another model is opened while simulation is running.


## Next Versions
- Mac, Linux.
- Custom bounding box is not properly calculated in some cases.
- Align custom bounding box to the normal of the biggest face when creating
  cylinders, cones, boxes, and more.
- Add Tapered Cylinder and Tapered Capsule shapes.
- Sounds.
- Convert SketchyPhysics to MSPhysics tool.
- Animations.
- Record video.
- Align collision tool.
- Serialized collisions.
- Double faced static mesh.
- Follow curves.
- Body.#split.
- Fractured compounds.
- Soft bodies.
- Cloth.
- Fix collision offset of scaled :static_mesh and :compound_from_mesh body.
- Create sphere, cone, cube with all size parameters of one. Then scale
  collision by the length of each side of the bounding box. That way sphere
  could be properly incorporated into the collision.
- Joints that automatically connect to the first touching bodies.
- Edit properties of multiple bodies and joints at once via the dialog.
- Classify collisions. That way each collision will have its own scale function
  that would scale bodies properly.
- Geared and Rack&Pinion joints.
- Differential joint if possible.
- Hinge2 - which has servo and motor in one.
~ Make material and body interconnected. Body.#material returns Material
  instance.
- Add functions like make compound, explode compound, add to compound, remove
  from compound.
- Each body inside a compound have its own class.
- Change get_function_name, set_fucntion_name to function_name, function_name=
- Run simulation of selected bodies.
- Add Body.#set_collidable_bodies - By default all bodies are collidable.
~ Recompile Newton to static library (.so). Make sure it's under the proper
  name-space and compatible with Ruby.
- Joystick input
- Custom compound from mesh groups.
- Reset simulation with saved body positions.
- Set editor theme, font, and some other editor settings.
- Make another version of MSPhysics with Bullet physics engine.
- Compound from mesh doesn't work properly in many cases. Create your own that
  would automatically divide any group into convex hulls.
- Fluid
- Add air density
- Add world destructor
- Add body destructor to the custom cloth.
- Compile Newton to double precision and change to double precision!
  pack('F') -> pack('D')
  pack('FFF') -> pack('DDD')
  pack('F*') -> pack('D*')
  Change all buffers size from 4 bytes per var to 8 bytes per var.
- Handle flipped bodies.
- Set body mass. Have temporary world that would calculate initial volume of the
  body.
- Have an easy way to create railroad pathfollow joint.


## Other
- Add "since" tag to first public release.
- Add "see also" tags to the documentation overview.
- Create an installer that would automatically install AMS Library, and MSPhysics
  into the desired plugins folder. Make the installer replace an outdated Ruby
  interpreter with a stable one as well.
