# Frequently Asked Questions

## What is the aim of MSPhysics?
The major aim of MSPhysics is to improve gaming in SketchUp. One thing that
SketchyPhysics lacks is having control over all mouse and keyboard operations.
Microsoft Windows API has a feature called Hook Procedure. It gives user access
to all mouse and keyboard events, plus allows user to prevent these events from
reaching SketchUp window procedure. With such feature in hand we can get input
of all mouse and keyboard messages without SketchUp interfering. We can use the
mouse wheel without the camera zoom in/out tool activating and use any keyboard
keys without SU keyboard shortcuts interfering. This feature allows MSPhysics to
be independent from other shortcut keys. MSPhysics is also enchanted with a
stable scripting API and a huge scripting documentation.


## Does it work on Mac OS X?
Yes, since version 0.9.0, MSPhysics has been made compatible with Mac OS X 10.5
or later. Some of the features, such as receiving keyboard and mouse events, are
only possible with Windows, but most of the other general features are up and
running just fine.


## How it works?
MSPhysics, as well as SketchyPhysics use NewtonDynamics physics engine by Julio
Jerez. NewtonDynamics is responsible for calculating collisions and all physics
operations. NewtonDynamics is known for its accuracy and stability.


## Is MSPhysics compatible with SketchyPhysics?
MSPhysics is not compatible with SketchyPhysics. Both plugins may be installed
alongside, but each will act as a separate tool; all attributes differ. To have
one model working on both MSPhysics and SketchyPhysics simply assign similar
scripts and properties to both MSPhysics and SketchyPhysics UI. However, one
version may lack features of the other...


## Is MSPhysics written to compete with SketchyPhysics?
No, MSPhysics is not written to compete with SketchyPhysics. MSPhysics is
written to extend SketchyPhysics.
