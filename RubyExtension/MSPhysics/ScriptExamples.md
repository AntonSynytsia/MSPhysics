# Example Scripts

## Emitter
Emitting body from its current position:

    onStart {
      # Set non-collidable to prevent collision with emitted clones.
      this.collidable = false
    }
    onTick {
      if frame % 5 == 0 && key('f') == 1
        dir = this.group.transformation.yaxis
        dir.length = this.mass * 10000
        lifetime = 4 # in seconds
        simulation.emit_body(this, dir, lifetime)
      end
    }

Emitting body at custom orientation:

    onStart {
      # Set non-collidable to prevent collision with emitted clones.
      this.collidable = false
    }
    onTick {
      if frame % 5 == 0 and key('f') == 1
        dir = this.group.transformation.yaxis
        dir.length = this.mass * 10000
        point = Geom::Point3d.new(0,0,100)
        tra = Geom::Transformation.new(point)
        lifetime = 4 # in seconds
        simulation.emit_body(this, tra, dir, lifetime)
      end
    }


## Thruster
To apply thrust to a body, utilize the <tt>add_force</tt> function:

    onUpdate {
      this.add_force(0, 0, 100)
    }

One way to make body hover in air is by adding a force opposite to its weight.

    onUpdate {
      # Get gravitational acceleration with magnitude in m/s/s.
      gravity = world.get_gravity
      # Get body mass in kilograms.
      mass = this.mass
      # Calculate gravitational force applied on this body.
      force = Geom::Vector3d.new(gravity.x * mass, gravity.y * mass, gravity.z * mass)
      # To oppose that force, simply apply an opposite force.
      this.add_force(force.reverse)
    }

Another way is to simply set applied force to zero.

    onUpdate {
      this.set_force(0, 0, 0)
    }

Making body "stick" to the ground.

    onUpdate {
      dir = this.group.transformation.zaxis.reverse
      dir.length = this.mass * 50
      this.add_force(dir)
    }


## Magnets
Make all bodies magnetic and have current body be a magnet.

    onStart {
      # Make all surrounding bodies magnetic
      world.bodies.each { |body|
        body.magnetic = true if body != this
      }
      # Make this body be a magnet
      this.magnet_force = 10000
      this.magnet_range = 1000
    }


## Explosion
Adding an impulse to all the surrounding bodies:

    onStart {
      center_point = this.get_position(1)
      blast_radius = 100
      blast_force = 1000 / simulation.update_timestep
      world.add_explosion(center_point, blast_radius, blast_force)
    }


## Touch Events

    onTouch { |toucher, position, normal, force, speed|
      simulation.log_line("onTouch #{frame}")
    }
    onTouching { |toucher|
      simulation.log_line("onTouching #{frame}")
    }
    onUntouch { |toucher|
      simulation.log_line("onUntouch #{frame}")
    }


## Conveyors
To create conveyors, utilizing the touch events isn't necessary. Simply assign
the desired velocity/omega vectors to the conveyors themselves and have them be
static. All the contacting bodies will move on them. Attach a similar code to
the conveyor:

    onStart {
      this.set_velocity(0, 10, 0)
      this.set_omega(0, 0, 0)
    }

You can, as well, have the conveyor alter its speed and direction dynamically:

    onUpdate {
      this.set_velocity(0, oscillator2(0.1) * 10, 0)
      this.set_omega(0, 0, 0)
    }
