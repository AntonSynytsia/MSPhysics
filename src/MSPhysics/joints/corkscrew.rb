module MSPhysics
  class Corkscrew < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    def initialize(pos, pin_dir, parent, child)
      super(pos, pin_dir, parent, child, 6)
      @angular_limits_enabled = false
      @linear_limits_enabled = false
      @angular_limits = [0,0]
      @linear_limits = [0,0]
      @angle = 0
      @omega = 0
      @dist = 0
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
      matrix0 = @child.get_matrix(0)*@local_matrix0
      matrix1 = @parent ? @parent.get_matrix(0)*@local_matrix1 : @local_matrix1

      p0 = matrix0.origin
      front = matrix1.zaxis
      front.length = (p0 - matrix1.origin) % matrix1.zaxis
      p1 = (matrix1.origin + front)

      Newton.userJointAddLinearRow(@joint_ptr, p0.to_a.pack('F*'), p1.to_a.pack('F*'), matrix0.xaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      Newton.userJointAddLinearRow(@joint_ptr, p0.to_a.pack('F*'), p1.to_a.pack('F*'), matrix0.yaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)

      v0 = matrix0.zaxis
      v0.length = PIN_LENGTH
      q0 = (p0 + v0).to_a.pack('F*')
      v1 = matrix1.zaxis
      v1.length = PIN_LENGTH
      q1 = (p1 + v1).to_a.pack('F*')

      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix0.xaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix0.yaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)

      # Determine joint angle.
      sin_angle = (matrix0.yaxis * matrix1.yaxis) % matrix1.zaxis
      cos_angle = matrix0.yaxis % matrix1.yaxis
      calc_angle(sin_angle, cos_angle)
       # Determine joint omega.
      omega0 = @child.get_omega
      omega1 = @parent ? @parent.get_omega : Geom::Vector3d.new(0,0,0)
      @omega = (omega0 - omega1) % matrix1.zaxis

      if @angular_limits_enabled
        if @angle < @angular_limits[0]
          rel_angle = @angular_limits[0] - @angle
          @angle = @angular_limits[0]
          Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix0.zaxis.to_a.pack('F*'))
          Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
        elsif @angle > @angular_limits[1]
          rel_angle = @angle - @angular_limits[1]
          @angle = @angular_limits[1]
          Newton.userJointAddAngularRow(@joint_ptr, rel_angle, matrix0.zaxis.reverse.to_a.pack('F*'))
          Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
        end
      end

      if @linear_limits_enabled
        @dist = (matrix0.origin - matrix1.origin) % matrix0.zaxis
        if @dist < @linear_limits[0]
          @dist = @linear_limits[0]
          pos = matrix0.origin.to_a.pack('F*')
          Newton.userJointAddLinearRow(@joint_ptr, pos, pos, matrix0.zaxis.to_a.pack('F*'))
          Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
        elsif @dist > @linear_limits[1]
          @dist = @linear_limits[1]
          pos = matrix0.origin.to_a.pack('F*')
          Newton.userJointAddLinearRow(@joint_ptr, pos, pos, matrix0.zaxis.reverse.to_a.pack('F*'))
          Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
        end
      end
    end

    def on_disconnect
      @angle = 0
      @omega = 0
      @dist = 0
    end

    public

    # Determine whether angular limits are enabled.
    # @return [Boolean]
    def angular_limits_enabled?
      @angular_limits_enabled
    end

    # Enable/Disable angular limits.
    # @param [Boolean] state
    def angular_limits_enabled=(state)
      @angular_limits_enabled = state ? true : false
    end

    # Determine whether linear limits are enabled.
    # @return [Boolean]
    def linear_limits_enabled?
      @linear_limits_enabled
    end

    # Enable/Disable linear limits.
    # @param [Boolean] state
    def linear_limits_enabled=(state)
      @linear_limits_enabled = state ? true : false
    end

    # Get angular limits in degrees.
    # @return [Array<Numeric>] +[min, max]+
    def get_angular_limits
      [@angular_limits[0].radians, @angular_limits[1].radians]
    end

    # Set angular limits in degrees.
    # @param [Numeric] min
    # @param [Numeric] max
    def set_angular_limits(min, max)
      @angular_limits = [min.degrees, max.degrees]
    end

    # Get angular limits in inches.
    # @return [Array<Numeric>] +[min, max]+
    def get_liniear_limits
      [@linear_limits[0].m, @linear_limits[1].m]
    end

    # Set liniear limits in inches.
    # @param [Numeric] min
    # @param [Numeric] max
    def set_linear_limits(min, max)
      @linear_limits = [min.to_m, max.to_m]
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

    # Get the distance moved along the pin axis in inches.
    # @return [Numeric]
    def distance
      @dist.m
    end

  end # class Corkscrew
end # module MSPhysics
