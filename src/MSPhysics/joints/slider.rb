module MSPhysics
  class Slider < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    # @param [Numeric] min Min distance in inches.
    # @param [Numeric] max Max distance in inches.
    # @param [Numeric] friction Moving friction in Newtons.
    def initialize(pos, pin_dir, parent, child, min = 0, max = 0, friction = 0)
      super(pos, pin_dir, parent, child, 6)
      @min = min.to_m
      @max = max.to_m
      @friction = friction.abs
      @limits_enabled = false
      @position = 0
      @speed = 0
      @controller = 0
    end

    # @!attribute [r] friction Get moving friction in Newtons.
    #   @return [Numeric]


    attr_reader :friction

    private

    def submit_constraints(timestep)
      matrix0 = @child.get_matrix(0)*@local_matrix0
      matrix1 = @parent ? @parent.get_matrix(0)*@local_matrix1 : @local_matrix1

      p0 = matrix0.origin
      p1 = (matrix1.origin + MSPhysics.scale_vector(matrix1.zaxis, (p0 - matrix1.origin) % matrix1.zaxis))

      p0p = p0.to_a.pack('F*')
      p1p = p1.to_a.pack('F*')
      Newton.userJointAddLinearRow(@joint_ptr, p0p, p1p, matrix0.yaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      Newton.userJointAddLinearRow(@joint_ptr, p0p, p1p, matrix0.xaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)

      q0 = (p0 + MSPhysics.scale_vector(matrix0.zaxis, PIN_LENGTH)).to_a.pack('F*')
      q1 = (p1 + MSPhysics.scale_vector(matrix1.zaxis, PIN_LENGTH)).to_a.pack('F*')

      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix0.yaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix0.xaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)

      r0 = (p0 + MSPhysics.scale_vector(matrix0.yaxis, PIN_LENGTH)).to_a.pack('F*')
      r1 = (p1 + MSPhysics.scale_vector(matrix1.yaxis, PIN_LENGTH)).to_a.pack('F*')

      Newton.userJointAddLinearRow(@joint_ptr, r0, r1, matrix1.xaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)

      v0 = @child.get_velocity
      v1 = @parent ? @parent.get_velocity : Geom::Vector3d.new(0,0,0)

      @position = (matrix0.origin - matrix1.origin) % matrix1.zaxis
      @speed = (v0 - v1) % matrix1.zaxis

      if @limits_enabled
        if @position < @min
          p0 = matrix0.origin.to_a.pack('F*')
          p1 = (matrix0.origin + MSPhysics.scale_vector(matrix0.zaxis, @min - @position)).to_a.pack('F*')
          Newton.userJointAddLinearRow(@joint_ptr, p0, p1, matrix0.zaxis.to_a.pack('F*'))
          Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
          return
        elsif @position > @max
          p0 = matrix0.origin.to_a.pack('F*')
          p1 = (matrix0.origin + MSPhysics.scale_vector(matrix0.zaxis, @max - @position)).to_a.pack('F*')
          Newton.userJointAddLinearRow(@joint_ptr, p0, p1, matrix0.zaxis.reverse.to_a.pack('F*'))
          Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
          Newton.userJointSetRowMinimumFriction(@joint_ptr, 0.0)
          return
        end
      end

      return if @friction.zero?

      p0 = matrix0.origin
      omega1 = @parent ? @parent.get_omega : Geom::Vector3d.new(0,0,0)
      v1 += omega1 * (matrix1.origin - p0)
      rel_accel = ((v1 - v0) % matrix0.zaxis) / timestep

      p0p = p0.to_a.pack('F*')
      Newton.userJointAddLinearRow(@joint_ptr, p0p, p0p, matrix0.zaxis.to_a.pack('F*'))
      Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      Newton.userJointSetRowAcceleration(@joint_ptr, rel_accel)
      Newton.userJointSetRowMinimumFriction(@joint_ptr, -@friction)
      Newton.userJointSetRowMaximumFriction(@joint_ptr, @friction)
    end

    def on_disconnect
      @position = 0
      @speed = 0
    end

    public

    # Get min distance in inches.
    # @return [Numeric]
    def min
      @min.m
    end

    # Set min distance in inches.
    # @param [Numeric] value
    def min=(value)
      @min = value.to_m
    end

    # Get max distance in inches.
    # @return [Numeric]
    def max
      @max.m
    end

    # Set max distance in inches.
    # @param [Numeric] value
    def max=(value)
      @max = value.to_m
    end

    # Set moving friction in Newtons.
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

    # Get the distance moved along the pin axis in inches.
    # @return [Numeric]
    def position
      @position.m
    end

    # Get moving speed along the pin axis in inches per second.
    # @return [Numeric]
    def speed
      @speed.m
    end

    # Get joint target position along the pin axis.
    # @return [Numeric, NilClass] Joint target position along the pin axis or
    #   +nil+ if there is no target position.
    def controller
      @controller
    end

    # Set joint target position along the pin axis.
    # @param [Numeric, NilClass] value Pass +nil+ to enable free motion along
    #   the pin axis.
    def controller=(value)
      @controller = value.is_a(Numeric) ? value.to_f : nil
    end

  end # class Slider
end # module MSPhysics
