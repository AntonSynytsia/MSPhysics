module MSPhysics

  # @since 1.0.0
  class Spring < Joint

    DEFAULT_MIN = -10.0
    DEFAULT_MAX = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_ROTATION_ENABLED = false
    DEFAULT_MODE = 0
    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 0.1
    DEFAULT_STRENGTH = 0.8
    DEFAULT_SPRING_CONSTANT = 40.0
    DEFAULT_SPRING_DRAG = 1.0
    DEFAULT_START_POSITION = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a spring joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::Spring.create(@address)
      MSPhysics::Newton::Spring.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Spring.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Spring.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Spring.enable_rotation(@address, DEFAULT_ROTATION_ENABLED)
      MSPhysics::Newton::Spring.set_mode(@address, DEFAULT_MODE)
      MSPhysics::Newton::Spring.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::Spring.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Spring.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::Spring.set_spring_constant(@address, DEFAULT_SPRING_CONSTANT)
      MSPhysics::Newton::Spring.set_spring_drag(@address, DEFAULT_SPRING_DRAG)
      MSPhysics::Newton::Spring.set_start_position(@address, DEFAULT_START_POSITION)
      MSPhysics::Newton::Spring.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get minimum position in meters with respect to the starting position.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Spring.get_min(@address)
    end

    # Set minimum position in meters with respect to the starting position.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Spring.set_min(@address, value)
    end

    # Get maximum position in meters with respect to the starting position.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Spring.get_max(@address)
    end

    # Set maximum position in meters with respect to the starting position.
    def max=(value)
      MSPhysics::Newton::Spring.set_max(@address, value)
    end

    # Determine whether min and max position limits are enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Spring.limits_enabled?(@address)
    end

    # Enable/disable min and max position limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Spring.enable_limits(@address, state)
    end

    # Determine whether rotation along Z-axis of joint is enabled.
    # @return [Boolean]
    def rotation_enabled?
      MSPhysics::Newton::Spring.rotation_enabled?(@address)
    end

    # Enable/disable rotation along Z-axis of joint.
    # @param [Boolean] state
    def rotation_enabled=(state)
      MSPhysics::Newton::Spring.enable_rotation(@address, state)
    end

    # Get mode.
    # @return [Integer]
    #   * 0 - if using spring accel, damp, and strength options.
    #   * 1 - if using Hooke's spring constant and drag coefficient.
    def mode
      MSPhysics::Newton::Spring.get_mode(@address)
    end

    # Set mode.
    # @param [Integer] value
    #   * 0 - use spring accel, damp, and strength options.
    #   * 1 - use Hooke's spring constant and drag coefficient options.
    def mode=(value)
      MSPhysics::Newton::Spring.set_mode(@address, value)
    end

    # Get spring oscillation acceleration factor.
    # @note This option associates with the normal spring mode.
    # @return [Numeric] An acceleration factor, a value greater than or equal to
    #   zero.
    def accel
      MSPhysics::Newton::Spring.get_accel(@address)
    end

    # Set spring oscillation acceleration factor.
    # @note This option associates with the normal spring mode.
    # @param [Numeric] value An acceleration factor, a value greater than or
    #   equal to zero.
    def accel=(value)
      MSPhysics::Newton::Spring.set_accel(@address, value)
    end

    # Get spring oscillation damping coefficient.
    # @note This option associates with the normal spring mode.
    # @return [Numeric] A value between 0.0 and 1.0.
    def damp
      MSPhysics::Newton::Spring.get_damp(@address)
    end

    # Set spring oscillation damping coefficient.
    # @note This option associates with the normal spring mode.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def damp=(value)
      MSPhysics::Newton::Spring.set_damp(@address, value)
    end

    # Get spring oscillation strength coefficient.
    # @note This option associates with the normal spring mode.
    # @return [Numeric] A value between 0.0 and 1.0.
    def strength
      MSPhysics::Newton::Spring.get_strength(@address)
    end

    # Set spring oscillation strength coefficient.
    # @note This option associates with the normal spring mode.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def strength=(value)
      MSPhysics::Newton::Spring.set_strength(@address, value)
    end

    # Get Hooke's spring constant.
    # @note This option associates with the Hooke's spring mode.
    # @return [Numeric] A spring constant in kg/s², a value greater than or
    #   equal to zero.
    def spring_constant
      MSPhysics::Newton::Spring.get_spring_constant(@address)
    end

    # Set Hooke's spring constant.
    # @note This option associates with the Hooke's spring mode.
    # @param [Numeric] value A spring constant in kg/s², a value greater than or
    #   equal to zero.
    def spring_constant=(value)
      MSPhysics::Newton::Spring.set_spring_constant(@address, value)
    end

    # Get Hooke's spring drag.
    # @note This option associates with the Hooke's spring mode.
    # @return [Numeric] A spring drag coefficient in kg/s, a value greater than
    #   or equal to zero.
    def spring_drag
      MSPhysics::Newton::Spring.get_spring_drag(@address)
    end

    # Set Hooke's spring drag.
    # @note This option associates with the Hooke's spring mode.
    # @param [Numeric] value A spring drag coefficient in kg/s, a value greater
    #   than or equal to zero.
    def spring_drag=(value)
      MSPhysics::Newton::Spring.set_spring_drag(@address, value)
    end

    # Get starting position along joint Z-axis in meters.
    # @note The actual, desired starting position is,
    #   <tt>start_position * controller</tt>.
    # @return [Numeric]
    def start_position
      MSPhysics::Newton::Spring.get_start_position(@address)
    end

    # Set starting position along joint Z-axis in meters.
    # @note The actual, desired starting position is,
    #   <tt>start_position * controller</tt>.
    # @param [Numeric] position
    def start_position=(position)
      MSPhysics::Newton::Spring.set_start_position(@address, position)
    end

    # Get magnitude and direction of the starting position.
    # @note The actual, desired starting position is,
    #   <tt>start_position * controller</tt>.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Spring.get_controller(@address)
    end

    # Set magnitude and direction of the starting position.
    # @note The actual, desired starting position is,
    #   <tt>start_position * controller</tt>.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Spring.set_controller(@address, value)
    end

    # Get current position in meters with respect to the starting position.
    # @return [Numeric]
    def cur_position
      MSPhysics::Newton::Spring.get_cur_position(@address)
    end

    # Get current velocity in meters per second.
    # @return [Numeric]
    def cur_velocity
      MSPhysics::Newton::Spring.get_cur_velocity(@address)
    end

    # Get current acceleration in meters per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Spring.get_cur_acceleration(@address)
    end

  end # class Spring < Joint
end # module MSPhysics
