# Example Scripts

## Emitter

Emit body from its current position.

    onStart {
      # Set non-collidable to prevent collision with emitted clones.
      this.set_collidable(false)
    }
    onUpdate {
      if frame % 5 == 0 && key('f') == 1
        dir = this.get_group.transformation.yaxis
        dir.length = this.get_mass * 10000
        lifetime = 100
        simulation.emit_body(this, dir, lifetime)
      end
    }

Emit body at custom orientation.

    onStart {
      # Set non-collidable to prevent collision with emitted clones.
      this.set_collidable(false)
    }
    onUpdate {
      if frame % 5 == 0 and key('f') == 1
        dir = this.get_group.transformation.yaxis
        dir.length = this.get_mass * 10000
        point = Geom::Point3d.new(0,0,100)
        tra = Geom::Transformation.new(point)
        lifetime = 100
        simulation.emit_body(this, tra, dir, lifetime)
      end
    }

## Thruster

One way to make body hover in air is by adding a force opposite to gravitational
force.

    onUpdate {
      # Get gravitational acceleration with magnitude in m/s/s.
      gravity = world.get_gravity
      # Get body mass in kilograms.
      mass = this.get_mass
      # Calculate gravitational force applied on this body.
      force = Geom::Vector3d.new(gravity.x * mass, gravity.y * mass, gravity.z * mass)
      # To oppose that force, simply apply an opposite force.
      this.add_force(force.reverse)
    }

Another way is to simply set applied force to zero.

    onUpdate {
      this.set_force([0,0,0])
    }

Make body "stick" to the ground.

    onUpdate {
      dir = this.get_group.transformation.zaxis.reverse
      dir.length = this.get_mass * 50
      this.add_force(dir)
    }

## Magnets

Make all bodies magnetic and have current body be a magnet.

    onStart {
      # Make all surrounding bodies magnetic
      world.get_bodies.each { |body|
        body.set_magnetic(true) if body != this
      }
      # Make this body be a magnet
      this.set_magnet_force(10000)
      this.set_magnet_range(1000)
    }

## Explosion

Add an impulsive explosion.

    onStart {
      center_point = this.get_position(1)
      blast_radius = 100
      blast_force = 1000 / simulation.get_update_timestep
      world.add_explosion(center_point, blast_radius, blast_force)
    }

## Touch Events

    onTouch { |toucher, position, normal, force, speed|
      simulation.log_line("onTouch #{frame}")
    }
    onTouching { |toucher, position, normal|
      simulation.log_line("onTouching #{frame}")
    }
    onUntouch { |toucher|
      simulation.log_line("onUntouch #{frame}")
    }
