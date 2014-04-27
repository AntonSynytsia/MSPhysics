module MSPhysics
  class Joint

    TYPES = [
      :hinge,
      :servo,
      :motor,
      :spring,
      :slider,
      :piston,
      :up,
      :fixed,
      :ball,
      :universal,
      :corkscrew
    ]

    class << self

      # Optimize joint name.
      # @param [Symbol, String] name Joint name.
      # @return [Symbol, NilClass] Proper name if successful.
      def optimize_joint_name(name)
        name = name.to_s.downcase.gsub(/\s|_/, '')
        JOINT_TYPES.each { |type|
          return type if type.to_s.gsub(/_/, '') == name
        }
        nil
      end

    end # proxy class

	# @param [Array<Numeric>, Geom::Point3d] pos Origin of hinge in global
    #   space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
	#	space.
    # @param [Body, NilClass] parent
    # @param [Body, NilClass] child
    # @param [Numeric] dof Degrees of freedom
    def initialize(pos, pin_dir, parent, child, dof = 0)
      if parent
        unless parent.is_a?(Body)
          raise ArgumentError, "Expected Body, but got #{parent.class}."
        end
        raise 'The parent body is invalid!' unless parent.valid?
        @parent = parent
      else
        @parent = nil
      end
      @child = nil
      @dof = dof.abs # degrees of freedom
      @joint_ptr = nil
      @destructor_callback = Proc.new { |joint_ptr|
        @joint_ptr = nil
      }
      @submit_constraints = Proc.new { |joint_ptr, timestep, thread_index|
        submit_constraints(timestep)
      }
      @get_info = Proc.new { |joint_ptr, info_ptr|
        get_info(info_ptr)
      }
	  @pos = Geom::Point3d.new(pos)
      @dir = Geom::Vector3d.new(pin_dir).normalize
	  if @parent
        tra = @parent.matrix.inverse
        @pos.transform!(tra)
        @dir.transform!(tra)
      end
      connect(child)
    end

    # @!attribute [r] parent
    #   @return [Body, NilClass]

    # @!attribute [r] child
    #   @return [Body, NilClass]


    attr_reader :parent, :child

    private

    def submit_constraints(timestep)
    end

    def get_info(info_ptr)
    end

    public

    # Connect body to the joint.
    # @note Each joint can have one child body only.
    # @note When this method is called the originally connected body is
    #   disconnected first, and then the new body is connected.
    # @overload connect
    #   Connect joint to the last connected body.
    # @overload connect(body)
    #   Connect joint to a new body.
    #   @param [Body] body
    # @return [Boolean] +true+ if successful.
    def connect(*args)
      body = args.size.zero? ? @child : args[0]
      return false unless body
      unless body.is_a?(Body)
        raise ArgumentError, "Expected Body, but got #{body.class}."
      end
      raise 'The body is invalid!' unless body.valid?
      disconnect
      @child = body
	  @local_matrix
      body_ptr = body._body_ptr
      world_ptr = Newton.bodyGetWorld(body_ptr)
      @join_ptr = Newton.constraintCreateUserJoint(world_ptr, @dof, @submit_constraints, @get_info, body_ptr, @parent.body_ptr)
      Newton.jointSetDestructor(@joint_ptr, @destructor_callback)
      true
    end

    # Disconnect connected body from the joint.
    # @return [Boolean] +true+ if successful.
    def disconnect
      return false unless connected?
      world_ptr = Newton.bodyGetWorld(@child)
      Newton.destroyJoint(world_ptr, @joint_ptr)
      @joint_ptr = nil
      true
    end

    # Determine whether a child body is connected to the joint.
    # @return [Boolean]
    def connected?
      @joint_ptr ? true : false
    end

  end # class Joint

  class Hinge < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Origin of hinge in global
    #   space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
	#	space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    # @param [Numeric] min Min angle in degrees.
    # @param [Numeric] max Max angle in degrees.
    # @param [Numeric] friction The coefficient of friction.
    def initialize(pos, pin_dir, parent, child, min = 0, max = 0, friction = 0)
      super(parent, child, 6)
      @min = min
      @max = max
      @friction = friction
      @enable_limits = true
      @pos = Geom::Point3d.new(pos)
      @dir = Geom::Vector3d.new(pin_dir).normalize
      if @parent
        tra = @parent.matrix.inverse
        @pos.transform!(tra)
        @dir.transform!(tra)
      end
    end

    # @!attribute [r] min Get min angle in degrees.
    #   @return [Numeric]

    # @!attribute [r] max Get max angle in degrees.
    #   @return [Numeric]

    # @!attribute [r] friction Get torque friction.
    #   @return [Numeric]


    attr_reader :min, :max, :friction

    private

    def submit_constraints(timestep)
	  return unless connected?
	  # Calculate position of pivot point in global space
	  pos = @pos.clone
	  pos.transform!(@parent.transformation) if @parent
	  matrix1 = @child.matrix
	  pos0 = pos.to_a.pack('F*')
	  pos1 = matrix1.origin.to_a.pack('F*')
	  # Restrict the movement of the pivot point along all three orthonormal
	  # directions.
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix1.xaxis.to_a.pack('F*'))
	  Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix1.yaxis.to_a.pack('F*'))
	  Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix1.zaxis.to_a.pack('F*'))
	  # Get a point along the pin axis at some reasonable large distance from
	  # the pivot.
	  q0 = pos + 
    end

    def get_info(info_ptr)
	  return unless connected?
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
    def enable_limits(state = true)
      @enable_limits = state ? true : false
    end

    # Determine whether the limits are on.
    # @return [Boolean]
    def limits_enabled?
      @enable_limits
    end

  end # class Hinge
end # module MSPhysics
