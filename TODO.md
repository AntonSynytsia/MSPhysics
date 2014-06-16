# Newton Issues

- Scene/Tree collisions may cause bodies to penetrate into them if the solver
  model is set to exact. This may be seen as a dynamic object bouncing on a
  static mesh.
- Compound scaled sub-collisions cause improper collision intersections.
- Compounds don't collide into scaled tree collisions, while other convex bodies
  do.
- Convex bodies jump on scaled tree collisions, and sometimes fall through them.
~ NewtonMeshSimplify not working.
- Compound from mesh doesn't work properly in all cases.


# Fix/Optimize/To Do

## Version 1.0.0
- Complete remove all physics attributes feature.
- Test the access of get/set_var vs body class variables and methods under
  controllers.
- Magnetic bodies
- Joints: Hinge, Servo, Motor, Spring, Up, Slider, Piston, Fixed, Ball,
  Universal, CorkScrew.
  Ensure that auto_sleep is off when force is added to a joint.
- Dialog
  Tabs:
    - Simulation UI
    - Joint UI
    - Body Properties
        - shape
            - convex hull
        - material
            - density
            - elasticity
            - softness
            - friction
- Remove many commands, and add them to the dialog - save space.
- Control flipped bodies.
- Add dialog to the animation tool.
- Work on body materials.
- Make sure animation tool is similar to SketchyPhysics, so that Twilight is compatible.
- Add options to the editor, control font, theme, and line wrap-up.
- Add air density
- Modify Body.#look_at method, add accel, damp
- Fix the jumping of bodies and improper behaviours!
- Slider on interactive solver works improperly
- Add position= to slider
- Fix script error that generates after executing this script
  this.entity.erase!
- When simulation starts
    - Create all bodies first
    - Use Sketchup.active_model.entities to check for top level joints.
    - Look in un-ignored group entities one level deep and see if they
      have joints as well.
    - Create joints from joint attributes. Don't forget to find joint parent.
- Default Settings


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
- Make material and body interconnected. Body.#material returns Material instance
- Add functions like make compound, explode compound, add to compound, remove from compound.
- Each body inside a compound have its own class.
- Change get_function_name, set_fucntion_name to function_name, function_name=
- Run simulation of selected bodies.
- Add Body.#set_collidable_bodies - By default all bodies are collidable.
~ Recompile Newton to static library (.so). Make sure it's under the proper namespace and
  compatible with Ruby.
- Joystick input
- Custom compound from mesh groups.
- Reset simulation with saved body positions.


## Other

- Add "since" tag to first public release.
- Add "see also" tags to the documentation overview.
- Create an installer that would automatically install AMS Library, and MSPhysics
  into the desired plugins folder. Make the installer replace an outdated Ruby
  interpreter with a stable one as well.
