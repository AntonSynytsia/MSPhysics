# Frequently Asked Questions

## What is the aim of MSPhysics?
The major aim of MSPhysics is to improve gaming in SketchUp. One disturbing
thing that SketchyPhysics lacks is having control over all mouse and keyboard
operations. Microsoft Windows API has a feature called Hook Procedure. It gives
user access to all mouse and keyboard events. It is not only capable of
monitoring user input messages, but also intercept and prevent them from
reaching the target window procedure. With the feature in hand we can get input
of all mouse and keyboard keys without SketchUp interfering. We can use the
mouse wheel without the camera zoom in/out tool activating, and use any keyboard
keys without SU keyboard shortcuts messing.


## Will it work on Macs?
No, not for long, as the Mac API greatly differs from the Windows API.


## How it works?
MSPhysics, as well as SketchyPhysics use Newton Dynamics physics engine by
Juleo Jerez. Newton is responsible for calculating collisions and all the
physics operations.


## Is MSPhysics compatible with SketchyPhysics?
MSPhysics is compatible and may run alongside SketchyPhysics, but all attributes
differ from SketchyPhysics. If you want to have a model compatible with
SketchyPhysics, simply assign similar properties and scripts to both, the
MSPhysics UI and the SketchyPhysics UI, but note that one plugin may not have
all the features of the other.


## Is MSPhysics written to compete with SketchyPhysics?
No! MSPhysics is the unofficial SketchyPhysics.


## Why the name MSPhysics?
This project is written from scratch. It has similar features to
SketchyPhysics, but the API is quite different. Including, the first version
lacked SketchyPhysics features, such as sounds, and compatibility with Macs.
Naming it SketchyPhysics4 was quite unacceptable.
