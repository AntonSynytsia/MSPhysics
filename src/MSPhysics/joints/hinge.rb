module MSPhysics
  class Hinge < Joint

    PIN_LENGTH = 50

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    # @param [Numeric] min Min angle in degrees.
    # @param [Numeric] max Max angle in degrees.
    # @param [Numeric] friction The coefficient of friction.
    def initialize(pos, pin_dir, parent, child, min = 0, max = 0, friction = 0)
      super(pos, pin_dir, parent, child, 6)
      @min = min
      @max = max
      @friction = friction
      @limits_enabled = true
      @angle = 0
      @omega = 0
    end

    # @!attribute [r] min Get min angle in degrees.
    #   @return [Numeric]

    # @!attribute [r] max Get max angle in degrees.
    #   @return [Numeric]

    # @!attribute [r] friction Get torque friction.
    #   @return [Numeric]


    attr_reader :min, :max, :friction

    private

    def calc_angle(new_cos_angle, new_sin_angle)
      sin_angle = Math.sin(@angle)
      cos_angle = Math.cos(@angle)
      sin_da = new_sin_angle * cos_angle - new_cos_angle * sin_angle
      cos_da = new_cos_angle * cos_angle + new_sin_angle * sin_angle
      @angle += Math.atan2(sin_da, cos_da)-Math::PI/2
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
      # Four possibilities.
      if @friction != 0
        if @limits_enabled
          # Friction and limits at the same time.
          if (@angle < @min.degrees)
            rel_angle = @min.degrees - @angle
            # Clip the angle and save new clip limit
            @angle = @min.degrees
            # Tell joint error will minimize the exceeded angle error
            Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix1.zaxis.to_a.pack('F*'))
            # Allow the joint to move back freely
            Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
            # Need high stiffness here
            Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
          elsif @angle > @max.degrees
            rel_angle = @angle - @max.degrees
            # Clip the angle and save new clip limit
            @angle = @max.degrees
            # Tell joint error will minimize the exceeded angle error
            Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix1.zaxis.reverse.to_a.pack('F*'))
            # Allow the joint to move back freely
            Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
            # Need high stiffness here
            Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
          else
            # Friction but no limits.
            alpha = @omega / timestep.to_f
            Newton.userJointAddAngularRow(@joint_ptr, 0, matrix1.zaxis.to_a.pack('F*'))
            Newton.userJointSetRowAcceleration(@joint_ptr, -alpha)
            Newton.userJointSetRowMinimumFriction(@joint_ptr, -@friction)
            Newton.userJointSetRowMaximumFriction(@joint_ptr, @friction)
            Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
          end
        else
          # Friction but no limits.
          alpha = @omega / timestep.to_f
          Newton.userJointAddAngularRow(@joint_ptr, 0, matrix1.zaxis.to_a.pack('F*'))
          Newton.userJointSetRowAcceleration(@joint_ptr, -alpha)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, -@friction)
          Newton.userJointSetRowMaximumFriction(@joint_ptr, @friction)
          Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
        end
      elsif @limits_enabled
        # Limits but no friction
        if @angle < @min.degrees
          rel_angle = @min.degrees - @angle
          @angle = @min.degrees
          Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix1.zaxis.to_a.pack('F*'))
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
          Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
        elsif @angle > @max.degrees
          rel_angle = @angle - @max.degrees
          @angle = @max.degrees
          Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix1.zaxis.reverse.to_a.pack('F*'))
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
          Newton.userJointSetRowStiffness(@joint_ptr, 1.0)
        end
      end
    end

    def on_disconnect
      @angle = 0
      @omega = 0
    end

    public

    # Set min angle in degrees.
    # @param [Numeric] value
    def min=(value)
      @min = value.to_f
    end

    # Set max angle in degrees.
    # @param [Numeric] value
    def max=(value)
      @max = value.to_f
    end

    # Set torque friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      @friction = value.to_f.abs
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

  end # class Hinge
end # module MSPhysics
