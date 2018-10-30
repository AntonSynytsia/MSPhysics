# Controllers Overview

The controller fields within the MSPhysics UI allow dynamically controlling
specific properties of their associated objects. Whenever simulation is active,
all controller fields are evaluated, at least once per frame, altering the
behaviour of their associated objects. Controllers, in general, can control
emitter, thruster, and specific properties of joints. Each controller may
consist of a simple numeric value, function calls, equations, conditional
values, and even complex scripts - all with a purpose to return a single result,
which may be a numeric value, a vector, or nothing, depending on what the
associated objects expect.

Check out the {MSPhysics::CommonContext} for a reference of MSPhysics functions
dedicated to the controllers.


## Controlling Thrusters

Thruster sections allow applying a specific force to their associated bodies,
along a specific direction. The controller field of a thruster assumes a numeric
value, a Vector3d object, or an Array, repressing a force in Newtons to apply on
the body.

Passing a numeric value to the controller, like <tt>1000</tt>, will apply 1000
Newtons of force to the associated body, along the body's or world's Z-axis,
depending on the "Lock to Body Axes" state.

Passing a directional value to the controller, like <tt>[400, 30, 50]</tt> or
<tt>Geom::Vector3d.new(400, 30, 50)</tt> will apply ~1027 Newtons of force to
the body, along the direction of the vector. The direction will interpreted as
bounded to local or global coordinates, depending on the "Lock to Body Axes"
state.

So far we dealt with applying static forces that are unaltered throughout the
simulation. Now, we will look into applying dynamic forces or dynamic
controllers.

Say, we want to apply a force of 1000 Newtons whenever key 'space' is down. To
do that, we need to utilize the <tt>key(vk)</tt> function:

    1000 * key(' ')

The <tt>key(key_name)</tt> function returns two values: <tt>0</tt> if the key is
up or <tt>1</tt> if the key is down. Multiplying these values by <tt>1000</tt>,
we can expect two final results, either <tt>0</tt> when space bar is up or
<tt>1000</tt> when key space bar is down. That way we can control the force we
apply on the body.

We can also apply a dynamic directional force:

    [0, key(' ') * 100, 200]

This will apply a constant 200 Newtons of force along the Z-axis and also 100
Newtons of force along the Y-axis, whenever space bar is down.

We can also use multiple keys to control the thruster. Say we want to use W, S,
A, and D keys to control the motion along the X-axis and Y-axis and up/down
arrow keys for Z-axis. To do that, we rely on multiple key states:

    [(key('w') - key('s')) * 200, (key('d') - key('a')) * 200, (key('up') - key('down')) * 1000]

In the sample above, <tt>key('w') - key('s')</tt> is capable of returning three
values: -1, 0, or 1. Multiplying these values by 200, magnifies the three values
by 200. Same applies to other parts of the sample.

That same sample above can be simplified to existing key functions:

    [lefty * 200, leftx * 200, righty * 100]

We can also apply conditional forces:

    (frame > 150) ? 100 : 0

This will apply a force of 100 Newtons only if the frame is greater than 100.

We can also reference against the elapsed time, in seconds, rather than frame:

    (world.time > 10) ? 100 : 0

This will apply a force of 100 Newtons if the world time is greater than ten
seconds.

More complex conditional expressions can be used as well:

    if (key(' ') == 1) then oscillator(0.1) + 250; else lefty * 100 + 250; end

We can also have a controller utilizing a value of a variable that is controlled
by a script of some body. Say you want to have a variable, named, 'fly', that
would be set to 250 whenever key space is down. Here is a script to paste into
one of the body script tabs:

    onStart {
      set_var('fly', 0) # Declare a variable
    }

    onTick {
      # Update based on the state of 'space' key.
      if key('space') == 1
        set_var('fly', 250)
      else
        set_var('fly', 0)
      end
    }

And here is what should be pasted into any of the controller(s):

    get_var('fly')

There are countless of ways to control the controllers. They should work, as
long as they follow the Ruby syntax and rely on valid functions. All controllers
that evaluate to an error are outed into the Ruby Console.

Check out the {MSPhysics::CommonContext} for a reference of MSPhysics functions
dedicated to the controllers.

Check out the {MSPhysics::BodyContext} for a reference of MSPhysics functions
dedicated to the scripts.


## Controlling Emitters

Emitter sections allow emitting the associated body at a particular rate, for a
particular lifetime.

The controller section assumes the impulse in Newton-seconds to apply on the
emitted bodies. Emitters are controlled in the same manner as thrusters, but
they also control whether or not to emit the body. A non-zero, numeric value or
a vector with non-zero magnitude stimulates the emission of the body; whereas, a
numeric value of zero or a vector of zero magnitude inhibits the emission of the
body.

Referring to that concept, we can create a controller that emits a body whenever
a key is down:

    1000 * key('space')

This will emit the body with an impulse of 1000 Newton-seconds whenever key
space is down.

Many other controllers can be assigned to the emitter field. As long as they
follow the Ruby syntax and use valid functions, they should work. Refer to the
<b>Controlling Thrusters</b> section for more details.


## Controlling Joints

Joints are special because the controllers for joints depend on the type of
joint itself and its preset properties.

To go a little bit in depth, say we have a car made up of Servo and Motor
joints, that we want to control with arrow keys.

To do that, we can assign each servo the following controller:

    rightx * 30

When limited to keyboard, this is equivalent to:

    (key('right') - key('left')) * 30

What this does is tells the servo to rotate 30 degrees left or right whenever
one of the keys is down. The <tt>rightx</tt> command can also return the value
of a right joystick, for smoother steering.

For the motor joints we would assign the following controllers:

    righty * 5

This controls whether the wheel spins forward or backward and the speed it spins
at, whenever the right joystick or one of the vertical arrow keys is stimulated.

Many other joints can be controlled following the same concept. The description
of each joint within the MSPhysics UI dialog indicates what its controller
controls and what it expects for control.
