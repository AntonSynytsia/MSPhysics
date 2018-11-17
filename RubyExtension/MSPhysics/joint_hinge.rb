module MSPhysics

  # @since 1.0.0
  class Hinge < Joint

    DEFAULT_MIN = -180.0.degrees
    DEFAULT_MAX = 180.0.degrees
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_MODE = 0
    DEFAULT_FRICTION = 0.0
    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 0.1
    DEFAULT_STRENGTH = 0.8
    DEFAULT_SPRING_CONSTANT = 40.0
    DEFAULT_SPRING_DRAG = 1.0
    DEFAULT_START_ANGLE = 0.0.degrees
    DEFAULT_CONTROLLER = 1.0

    # Create a hinge joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::Hinge.create(@address)
      MSPhysics::Newton::Hinge.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Hinge.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Hinge.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Hinge.set_mode(@address, DEFAULT_MODE)
      MSPhysics::Newton::Hinge.set_friction(@address, DEFAULT_FRICTION)
      MSPhysics::Newton::Hinge.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::Hinge.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Hinge.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::Hinge.set_spring_constant(@address, DEFAULT_SPRING_CONSTANT)
      MSPhysics::Newton::Hinge.set_spring_drag(@address, DEFAULT_SPRING_DRAG)
      MSPhysics::Newton::Hinge.set_start_angle(@address, DEFAULT_START_ANGLE)
      MSPhysics::Newton::Hinge.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get current angle in radians with respect to the starting angle.
    # @return [Numeric]
    def cur_angle
      MSPhysics::Newton::Hinge.get_cur_angle(@address)
    end

    # Get current angular velocity in radians per second.
    # @return [Numeric]
    def cur_omega
      MSPhysics::Newton::Hinge.get_cur_omega(@address)
    end

    # Get current angular acceleration in radians per second per second.
    # @return [Numeric]
    def cur_alpha
      MSPhysics::Newton::Hinge.get_cur_alpha(@address)
    end

    # Get minimum angle in radians with respect to the starting angle.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Hinge.get_min(@address)
    end

    # Set minimum angle in radians with respect to the starting angle.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Hinge.set_min(@address, value)
    end

    # Get maximum angle in radians with respect to the starting angle.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Hinge.get_max(@address)
    end

    # Set maximum angle in radians with respect to the starting angle.
    # @param [Numeric] value
    def max=(value)
      MSPhysics::Newton::Hinge.set_max(@address, value)
    end

    # Determine whether min & max angle limits are enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Hinge.limits_enabled?(@address)
    end

    # Enable/disable min & max angle limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Hinge.enable_limits(@address, state)
    end

    # Get mode.
    # @return [Integer]
    #   * 0 - if using friction option
    #   * 1 - if using spring accel, damp, and strength options.
    #   * 2 - if using Hooke's spring constant and drag coefficient.
    def mode
      MSPhysics::Newton::Hinge.get_mode(@address)
    end

    # Set mode.
    # @param [Integer] value
    #   * 0 - use friction option
    #   * 1 - use spring accel, damp, and strength options.
    #   * 2 - use Hooke's spring constant and drag coefficient options.
    def mode=(value)
      MSPhysics::Newton::Hinge.set_mode(@address, value)
    end

    # Get rotational friction.
    # @note This option associates with friction mode.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @return [Numeric] A value greater than or equal to zero.
    def friction
      MSPhysics::Newton::Hinge.get_friction(@address)
    end

    # Set rotational friction.
    # @note This option associates with friction mode.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      MSPhysics::Newton::Hinge.set_friction(@address, value)
    end

    # Get spring oscillation acceleration factor.
    # @note This option associates with the normal spring mode.
    # @return [Numeric] An acceleration factor, a value greater than or equal to
    #   zero.
    def accel
      MSPhysics::Newton::Hinge.get_accel(@address)
    end

    # Set spring oscillation acceleration factor.
    # @note This option associates with the normal spring mode.
    # @param [Numeric] value An acceleration factor, a value greater than or
    #   equal to zero.
    def accel=(value)
      MSPhysics::Newton::Hinge.set_accel(@address, value)
    end

    # Get spring oscillation damping coefficient.
    # @note This option associates with the normal spring mode.
    # @return [Numeric] A value between 0.0 and 1.0.
    def damp
      MSPhysics::Newton::Hinge.get_damp(@address)
    end

    # Set spring oscillation damping coefficient.
    # @note This option associates with the normal spring mode.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def damp=(value)
      MSPhysics::Newton::Hinge.set_damp(@address, value)
    end

    # Get spring oscillation strength coefficient.
    # @note This option associates with the normal spring mode.
    # @return [Numeric] A value between 0.0 and 1.0.
    def strength
      MSPhysics::Newton::Hinge.get_strength(@address)
    end

    # Set spring oscillation strength coefficient.
    # @note This option associates with the normal spring mode.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def strength=(value)
      MSPhysics::Newton::Hinge.set_strength(@address, value)
    end

    # Get Hooke's spring constant.
    # @note This option associates with the Hooke's spring mode.
    # @return [Numeric] A spring constant in kg/s², a value greater than or
    #   equal to zero.
    def spring_constant
      MSPhysics::Newton::Hinge.get_spring_constant(@address)
    end

    # Set Hooke's spring constant.
    # @note This option associates with the Hooke's spring mode.
    # @param [Numeric] value A spring constant in kg/s², a value greater than or
    #   equal to zero.
    def spring_constant=(value)
      MSPhysics::Newton::Hinge.set_spring_constant(@address, value)
    end

    # Get Hooke's spring drag.
    # @note This option associates with the Hooke's spring mode.
    # @return [Numeric] A spring drag coefficient in kg/s, a value greater than
    #   or equal to zero.
    def spring_drag
      MSPhysics::Newton::Hinge.get_spring_drag(@address)
    end

    # Set Hooke's spring drag.
    # @note This option associates with the Hooke's spring mode.
    # @param [Numeric] value A spring drag coefficient in kg/s, a value greater
    #   than or equal to zero.
    def spring_drag=(value)
      MSPhysics::Newton::Hinge.set_spring_drag(@address, value)
    end

    # Get starting angle in radians.
    # @note This associates with the spring modes only.
    # @note The actual starting angle is, <tt>start_angle * controller</tt>.
    # @return [Numeric]
    def start_angle
      MSPhysics::Newton::Hinge.get_start_angle(@address)
    end

    # Set starting angle in radians.
    # @note This associates with the spring modes only.
    # @note The actual starting angle is, <tt>start_angle * controller</tt>.
    # @param [Numeric] angle
    def start_angle=(angle)
      MSPhysics::Newton::Hinge.set_start_angle(@address, angle)
    end

    # Get hinge controller, the magnitude of the rotational friction or the
    # magnitude and direction of the starting angle, depending on the mode.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Hinge.get_controller(@address)
    end

    # Set hinge controller, the magnitude of the rotational friction or the
    # magnitude and direction of the starting angle, depending on the mode.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Hinge.set_controller(@address, value)
    end

  end # class Hinge < Joint
end # module MSPhysics
