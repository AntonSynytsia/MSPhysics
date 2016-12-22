# Frequently Asked Questions

## What is the aim of MSPhysics?
The major aim of MSPhysics is to improve gaming in SketchUp. One thing that
SketchyPhysics lacks is having control over all mouse and keyboard operations.
Microsoft Windows API has a feature called Hook Procedure. Hook Procedure allows
monitoring and making decisions to the messages reaching a window procedure.
This advantage allows processing of all mouse and keyboard messages, without
SketchUp shortcuts interfering. For example, a mouse wheel that is, by default,
a shortcut for a zoom in/out command could be instead used as a controller to
switch weapons in an FPS game. All mouse clicks and keyboard buttons could be
processed without having to create a focus redirecting control panel. All in
all, this feature allows MSPhysics to be independent from shortcut commands,
which is very suitable for developers who seek to create FPS games with more
control over user input.

## Is MSPhysics a new version of SketchyPhysics?
MSPhysics is not the new version of SketchyPhysics. MSPhysics is written
completely from scratch, integrating the latest NewtonDynamics Physics SDK. Yes,
it does have a similar functionality to SketchyPhysics, but that was done so that
SketchyPhysics users wouldn't have hard time transitioning to MSPhysics.

## How is MSPhysics different from SketchyPhysics?
MSPhysics implements the latest NewtonDynamics physics SDK, making it faster
than SketchyPhysics by a significant factor.

MSPhysics has a very powerful scripting API and a well documented scripting
documentation.

MSPhysics has more and better joints than SketchyPhysics.


## Does MSPhysics work on Mac OS X?
MSPhysics is compatible with Mac OS X 10.6 or later. Some of the features, such
as receiving keyboard and mouse events, are limited, but most of the other
general features are up and running just fine.


## How MSPhysics Works?
MSPhysics uses NewtonDynamics physics engine made by Julio Jerez. NewtonDynamics
is responsible for calculating all the collisions and physics operations.
NewtonDynamics is known for its accuracy and stability.


## Is MSPhysics compatible with SketchyPhysics?
MSPhysics is not compatible with SketchyPhysics. Both plugins may be installed
alongside, but each will act as a separate tool; all attributes differ. To have
a particular model working on both, MSPhysics and SketchyPhysics, assign similar
scripts and properties to the model, through MSPhysics and SketchyPhysics UIs.
On the down side, SketchyPhysics may lack the features of MSPhysics and vise
versa.
