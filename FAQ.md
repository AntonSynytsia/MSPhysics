# Frequently Asked Questions

## What is the aim of MSPhysics?
The major aim of MSPhysics is to improve gaming in SketchUp. One disturbing
thing that SketchyPhysics lacks is having control over all mouse and keyboard
operations. Microsoft Windows API has a feature called Hook Procedure. It gives
user access to all mouse and keyboard events, plus prevent these events from
reaching SketchUp window procedure. With the feature in hand we can get input
of all mouse and keyboard messages without SketchUp interfering. We can use the
mouse wheel without the camera zoom in/out tool activating, and use any keyboard
keys without SU keyboard shortcuts interfering. As well, MSPhysics is enchanted
with a stable scripting API and huge documentation.


## Does it work on Mac OS X?
No, MSPhysics does not work on Macs, as a lot of MSPhysics features depend on
the Microsoft Windows API. As far as I'm concerned, Mac API greatly differs from
the Windows API. A compatible version could be developed in the future, but it
will lack some features of the Windows version.


## How it works?
MSPhysics, as well as SketchyPhysics use NewtonDynamics physics engine by Juleo
Jerez. Newton is responsible for calculating collisions, and all the physics
operations. Newton is known for its accuracy and stability.


## Is MSPhysics compatible with SketchyPhysics?
MSPhysics is not compatible with SketchyPhysics. Both plugins may be installed
alongside, but each will act as a separate tool; all attributes differ. To have
one model working on both MSPhysics and SketchyPhysics simply assign similar
scripts and properties to both MSPhysics and SketchyPhysics UI.


## Is MSPhysics written to compete with SketchyPhysics?
No, MSPhysics is not written to compete with SketchyPhysics. MSPhysics is
written to bring SketchyPhysics community back to life.
