--------------------------------------------------------------------------------

# Newton Issues

- Scene/Tree collisions may cause bodies to penetrate into them if the solver
  model is set to exact. This may be seen as a dynamic object bouncing on a
  static mesh.
- Compound scaled sub-collisions cause improper collision intersections.
- Compounds don't collide into scaled tree collisions, while other convex bodies
  do.
- Convex bodies jump on scaled tree collisions, and sometimes fall through them.
~ NewtonMeshSimplify not working.


# Fix/Optimize/To Do

## Version 1.0.0
- Complete remove all physics attributes feature.
- Default density and other settings.
- Test the access of get/set_var vs body class variables and methods under
  controllers.
- Magnetic bodies
- Finish materials
- Joints: Hinge, Servo, Motor, Spring, Up, Slider, Piston, Fixed, Ball, Universal,
  CorkScrew.
- Dialog
  - Update all options on mouse enter
  - If one body has an error, open the dialog and navigate to the error.
  - Check syntax errors
  Tabs:
    - Simulation UI
    - Joint UI
    - Body Script
    - Body Properties
        - shape
            - convex hull
        - material
            - density
            - elasticity
            - softness
            - friction
            - mass
- Remove many commands, and add them to the dialog - save space.
- Update joint description that says 'Two bodies is the capacity of the joint.'
- Differential joints
- Automatically disconnect joint from a destroyed body.
- Automatically destroy joint if the parent is a compound body, which is
  destroyed.
~ Make sure user cant create joints inside a body, but compound.
- Joint destructor callback

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
- Control flipped bodies.
- Fix collision offset of scaled :static_mesh and :compound_from_mesh body.
- Create sphere, cone, cube with all size parameters of one. Then scale
  collision by the length of each side of the bounding box. That way sphere
  could be properly incorporated into the collision.
- Joints that automatically connect to the first touching bodies.
- Edit properties of multiple bodies and joints at once via the dialog.
- Classify collisions. That way each collision will have its own scale function
  that would scale bodies properly.
- Geared joints
- Differential joint


## Other

- Add "since" tag to first public release.
- Add "see also" tags to the documentation overview.

- Create an installer that would automatically install AMS Library, and MSPhysics
  into the desired plugins folder. Make the installer replace an outdated Ruby
  interpreter with a stable one as well.

--------------------------------------------------------------------------------
