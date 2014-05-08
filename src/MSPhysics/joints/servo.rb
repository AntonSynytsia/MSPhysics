module MSPhysics
  class Servo < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    # @param [Numeric] min Min angle in degrees.
    # @param [Numeric] max Max angle in degrees.
    # @param [Numeric] rate Angular rate in degrees per second.
    # @param [Numeric] power Rotational force power in Newton meters.
    def initialize(pos, pin_dir, parent, child, min = 0, max = 0, rate = 50, power = 50000)
      super(pos, pin_dir, parent, child, 6)
      @min = min.degrees
      @max = max.degrees
      @angular_rate = rate.degrees.abs
      @power = power.abs
      @limits_enabled = true
      @target_angle = nil
      @angle = 0
      @omega = 0
    end

    private

    def calc_angle(new_cos_angle, new_sin_angle)
      sin_angle = Math.sin(@angle)
      cos_angle = Math.cos(@angle)
      sin_da = new_sin_angle * cos_angle - new_cos_angle * sin_angle
      cos_da = new_cos_angle * cos_angle + new_sin_angle * sin_angle
      @angle += Math.atan2(sin_da, cos_da) - Math::PI/2
    end

    def submit_constraints(timestep)
      # Calculate the position of the pivot point and the Jacobian direction
      # vectors in global space.
      matrix0 = @child.get_matrix(0)*@local_matrix0
      matrix1 = @parent ? @parent.get_matrix(0)*@local_matrix1 : @local_matrix1
      pos0 = matrix0.origin.to_a.pack('F*')
      pos1 = matrix1.origin.to_a.pack('F*')
      # Restrict the movement on the pivot point along all three orthonormal
      # directions.
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix1.xaxis.to_a.pack('F*'))
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix1.yaxis.to_a.pack('F*'))
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix1.zaxis.to_a.pack('F*'))
      # Get a point along the pin axis at some reasonable large distance from
      # the pivot.
      v1 = matrix0.zaxis
      v1.length = PIN_LENGTH
      q0 = (matrix0.origin + v1).to_a.pack('F*')
      v2 = matrix1.zaxis
      v2.length = PIN_LENGTH
      q1 = (matrix1.origin + v2).to_a.pack('F*')
      # Add two constraints row perpendicular to the pin vector.
      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix1.xaxis.to_a.pack('F*'))
      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix1.yaxis.to_a.pack('F*'))
      # Determine joint angle.
      sin_angle = (matrix0.yaxis * matrix1.yaxis) % matrix1.zaxis
      cos_angle = matrix0.yaxis % matrix1.yaxis
      calc_angle(sin_angle, cos_angle)
      # Determine joint omega.
      omega0 = @child.get_omega
      omega1 = @parent ? @parent.get_omega : Geom::Vector3d.new(0,0,0)
      @omega = (omega0 - omega1) % matrix1.zaxis
      # Apply forces
      apply_std = false
      if @limits_enabled
        if (@angle < @min)
          rel_angle = @min - @angle
          @angle = @min
          Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix1.zaxis.to_a.pack('F*'))
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
        elsif @angle > @max
          rel_angle = @angle - @max
          @angle = @max
          Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix1.zaxis.reverse.to_a.pack('F*'))
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
        else
          apply_std = true
          tar_angle = MSPhysics.clamp(@target_angle, @min, @max) if @target_angle
        end
      else
        apply_std = true
        tar_angle = @target_angle if @target_angle
      end
      if apply_std
        if @target_angle
          rel_angle = @angle - tar_angle
          step = @angular_rate * timestep
          if rel_angle.abs < step.abs
            Newton.userJointAddAngularRow(@joint_ptr, 0, matrix0.zaxis.to_a.pack('F*'))
          else
            Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix0.zaxis.to_a.pack('F*'))
            desired_speed = (rel_angle >= 0 ? 1 : -1) * -@angular_rate
            current_speed = @omega
            accel = (desired_speed - current_speed).to_f / timestep
            Newton.userJointSetRowAcceleration(@joint_ptr, accel)
          end
        else
          Newton.userJointAddAngularRow(@joint_ptr, 0, matrix0.zaxis.to_a.pack('F*'))
        end
        if @power != 0
          Newton.userJointSetRowMinimumFriction(@joint_ptr, -@power)
          Newton.userJointSetRowMaximumFriction(@joint_ptr, @power)
        end
      end
      Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
    end

    def on_disconnect
      @angle = 0
      @omega = 0
    end

    public

    # Get min angle in degrees.
    # @return [Numeric]
    def min
      @min.radians
    end

    # Set min angle in degrees.
    # @param [Numeric] value
    def min=(value)
      @min = value.degrees
    end

    # Get max angle in degrees.
    # @return [Numeric]
    def max
      @max.radians
    end

    # Set max angle in degrees.
    # @param [Numeric] value
    def max=(value)
      @max = value.degrees
    end

    # Set torque friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      @friction = value.abs
    end

    # Enable/Disable limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      @limits_enabled = state ? true : false
    end

    # Determine whether the limits are on.
    # @return [Boolean]
    def limits_enabled?
      @limits_enabled
    end

    # Get joint rotated angle in degrees.
    # @return [Numeric]
    def angle
      @angle.radians
    end

    # Get joint omega in degrees per second.
    # @return [Numeric]
    def omega
      @omega.radians
    end

    # Get target angle in degrees.
    # @return [Numeric, NilClass]
    def target_angle
      @target_angle.radians
    end

    # Set target angle in degrees.
    # @param [Numeric, NilClass] theta Pass +nil+ to have the servo off.
    def target_angle=(theta)
      @target_angle = theta.is_a?(Numeric) ? theta.degrees : nil
    end

    # Get angular rate in degrees per second.
    # @return [Numeric]
    def angular_rate
      @angular_rate.radians
    end

    # Set angular rate in degrees per second.
    # @param [Numeric] rate
    def angular_rate=(rate)
      @angular_rate = rate.degrees.abs
    end

    # Get the maximum force applied to the rotation of servo in Newton meters.
    # @return [Numeric]
    def power
      @power
    end

    # Set the maximum force applied to the rotation of servo in Newton meters.
    # @param [Numeric] force
    def power=(force)
      @power = force.abs
    end

  end # class Servo
end # module MSPhysics
