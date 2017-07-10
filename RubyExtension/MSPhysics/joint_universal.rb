module MSPhysics

  # @since 1.0.0
  class Universal < Joint

    DEFAULT_MIN = -180.0.degrees
    DEFAULT_MAX = 180.0.degrees
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_FRICTION = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a universal joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::Universal.create(@address)
      MSPhysics::Newton::Universal.set_min1(@address, DEFAULT_MIN)
      MSPhysics::Newton::Universal.set_max1(@address, DEFAULT_MAX)
      MSPhysics::Newton::Universal.enable_limits1(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Universal.set_min2(@address, DEFAULT_MIN)
      MSPhysics::Newton::Universal.set_max2(@address, DEFAULT_MAX)
      MSPhysics::Newton::Universal.enable_limits2(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Universal.set_friction(@address, DEFAULT_FRICTION)
      MSPhysics::Newton::Universal.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get current angle in radians along joint Z-axis.
    # @return [Numeric]
    def cur_angle1
      MSPhysics::Newton::Universal.get_cur_angle1(@address)
    end

    # Get current angular velocity in radians per second along joint Z-axis.
    # @return [Numeric]
    def cur_omega1
      MSPhysics::Newton::Universal.get_cur_omega1(@address)
    end

    # Get current angular acceleration in radians per second per second along
    # joint Z-axis.
    # @return [Numeric]
    def cur_alpha1
      MSPhysics::Newton::Universal.get_cur_alpha1(@address)
    end

    # Get minimum angle in radians along joint Z-axis.
    # @return [Numeric]
    def min1
      MSPhysics::Newton::Universal.get_min1(@address)
    end

    # Set minimum angle in radians along joint Z-axis.
    # @param [Numeric] value
    def min1=(value)
      MSPhysics::Newton::Universal.set_min1(@address, value)
    end

    # Get maximum angle in radians along joint Z-axis.
    # @return [Numeric]
    def max1
      MSPhysics::Newton::Universal.get_max1(@address)
    end

    # Set maximum angle in radians along joint Z-axis.
    # @param [Numeric] value
    def max1=(value)
      MSPhysics::Newton::Universal.set_max1(@address, value)
    end

    # Determine whether min & max angle limits along joint Z-axis are enabled.
    # @return [Boolean]
    def limits1_enabled?
      MSPhysics::Newton::Universal.limits1_enabled?(@address)
    end

    # Enable/disable min & max angle limits along joint Z-axis.
    # @param [Boolean] state
    def limits1_enabled=(state)
      MSPhysics::Newton::Universal.enable_limits1(@address, state)
    end

    # Get current angle in radians along joint X-axis.
    # @return [Numeric]
    def cur_angle2
      MSPhysics::Newton::Universal.get_cur_angle2(@address)
    end

    # Get current angular velocity in radians per second along joint X-axis.
    # @return [Numeric]
    def cur_omega2
      MSPhysics::Newton::Universal.get_cur_omega2(@address)
    end

    # Get current angular acceleration in radians per second per second along
    # joint X-axis.
    # @return [Numeric]
    def cur_alpha2
      MSPhysics::Newton::Universal.get_cur_alpha2(@address)
    end

    # Get minimum angle in radians along joint X-axis.
    # @return [Numeric]
    def min2
      MSPhysics::Newton::Universal.get_min2(@address)
    end

    # Set minimum angle in radians along joint X-axis.
    # @param [Numeric] value
    def min2=(value)
      MSPhysics::Newton::Universal.set_min2(@address, value)
    end

    # Get maximum angle in radians along joint X-axis.
    # @return [Numeric]
    def max2
      MSPhysics::Newton::Universal.get_max2(@address)
    end

    # Set maximum angle in radians along joint X-axis.
    # @param [Numeric] value
    def max2=(value)
      MSPhysics::Newton::Universal.set_max2(@address, value)
    end

    # Determine whether min & max angle limits along joint X-axis are enabled.
    # @return [Boolean]
    def limits2_enabled?
      MSPhysics::Newton::Universal.limits2_enabled?(@address)
    end

    # Enable/disable min & max angle limits along joint X-axis.
    # @param [Boolean] state
    def limits2_enabled=(state)
      MSPhysics::Newton::Universal.enable_limits2(@address, state)
    end

    # Get angular friction.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @return [Numeric] A value greater than or equal to zero.
    def friction
      MSPhysics::Newton::Universal.get_friction(@address)
    end

    # Set angular friction.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      MSPhysics::Newton::Universal.set_friction(@address, value)
    end

    # Get magnitude of the angular friction.
    # @note Default controller value is 1.0.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Universal.get_controller(@address)
    end

    # Set magnitude of the angular friction.
    # @note Default controller value is 1.0.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Universal.set_controller(@address, value)
    end

  end # class Universal < Joint
end # module MSPhysics
