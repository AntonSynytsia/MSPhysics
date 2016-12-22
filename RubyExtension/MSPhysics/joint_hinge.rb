module MSPhysics

  # @since 1.0.0
  class Hinge < Joint

    DEFAULT_MIN = -180.0.degrees
    DEFAULT_MAX = 180.0.degrees
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_FRICTION = 0.0
    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_STRENGTH = 0.98
    DEFAULT_HOOKES_ENABLED = false
    DEFAULT_ROTATE_BACK_ENABLED = false
    DEFAULT_START_ANGLE = 0.0.degrees
    DEFAULT_CONTROLLER = 1.0

    # Create a hinge joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::Hinge.create(@address)
      MSPhysics::Newton::Hinge.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Hinge.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Hinge.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Hinge.set_friction(@address, DEFAULT_FRICTION)
      MSPhysics::Newton::Hinge.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::Hinge.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Hinge.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::Hinge.set_mode(@address, DEFAULT_HOOKES_ENABLED ? 1 : 0)
      MSPhysics::Newton::Hinge.enable_rotate_back(@address, DEFAULT_ROTATE_BACK_ENABLED)
      MSPhysics::Newton::Hinge.set_start_angle(@address, DEFAULT_START_ANGLE)
      MSPhysics::Newton::Hinge.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get current angle in radians with respect to the starting angle.
    # @return [Numeric]
    def cur_angle
      MSPhysics::Newton::Hinge.get_cur_angle(@address)
    end

    # Get current omega in radians per second.
    # @return [Numeric]
    def cur_omega
      MSPhysics::Newton::Hinge.get_cur_omega(@address)
    end

    # Get current acceleration in radians per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Hinge.get_cur_acceleration(@address)
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

    # Get rotational friction.
    # @note This option has an effect only if "rotate-back" is disabled.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @return [Numeric] A value greater than or equal to zero.
    def friction
      MSPhysics::Newton::Hinge.get_friction(@address)
    end

    # Set rotational friction.
    # @note This option has an effect only if "rotate-back" is disabled.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      MSPhysics::Newton::Hinge.set_friction(@address, value)
    end

    # Get rotational spring oscillation acceleration.
    # @note This option has an effect only if "rotate back" is enabled.
    # @return [Numeric] A spring constant in kg/s² or spring accel in 1/s²,
    #   depending on the mode; a value greater than or equal to zero.
    def accel
      MSPhysics::Newton::Hinge.get_accel(@address)
    end

    # Set rotational spring oscillation acceleration.
    # @note This option has an effect only if "rotate back" is enabled.
    # @param [Numeric] value A spring constant in kg/s² or spring accel in 1/s²,
    #   depending on the mode; a value greater than or equal to zero.
    def accel=(value)
      MSPhysics::Newton::Hinge.set_accel(@address, value)
    end

    # Get rotational spring oscillation drag.
    # @note This option has an effect only if "rotate back" is enabled.
    # @return [Numeric] A spring drag coefficient in kg/s or spring damp in 1/s,
    #   depending on the mode; a value greater than or equal to zero.
    def damp
      MSPhysics::Newton::Hinge.get_damp(@address)
    end

    # Set rotational spring oscillation drag.
    # @note This option has an effect only if "rotate back" is enabled.
    # @param [Numeric] value A spring drag coefficient in kg/s or spring damp
    #    in 1/s, depending on the mode; a value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::Hinge.set_damp(@address, value)
    end

    # Get rotational spring strength coefficient.
    # @note This option has an effect only if "rotate back" is enabled and mode
    #   is set to zero.
    # @return [Numeric] A value between 0.0 and 1.0.
    def strength
      MSPhysics::Newton::Hinge.get_strength(@address)
    end

    # Set rotational spring strength coefficient.
    # @note This option has an effect only if "rotate back" is enabled and mode
    #   is set to zero.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def strength=(value)
      MSPhysics::Newton::Hinge.set_strength(@address, value)
    end

    # Get rotational spring mode.
    # @note This option has an effect only if "rotate back" is enabled.
    # @return [Fixnum]
    #   * 0 - if using standard accel/damp/strength.
    #   * 1 - if using Hooke's spring constant and spring damping coefficient.
    def mode
      MSPhysics::Newton::Hinge.get_mode(@address)
    end

    # Set rotational spring mode.
    # @note This option has an effect only if "rotate back" is enabled.
    # @param [Fixnum] value
    #   * 0 - use standard accel/damp/strength.
    #   * 1 - use Hooke's spring constant and spring damping coefficient.
    def mode=(value)
      MSPhysics::Newton::Hinge.set_mode(@address, value)
    end

    # Determine whether the option to rotate back to a starting angle is
    # enabled.
    # @return [Boolean]
    def rotate_back_enabled?
      MSPhysics::Newton::Hinge.rotate_back_enabled?(@address)
    end

    # Enable/disable a feature to rotate back to a starting angle.
    # @param [Boolean] state
    def rotate_back_enabled=(state)
      MSPhysics::Newton::Hinge.enable_rotate_back(@address, state)
    end

    # Get starting angle in radians.
    # @note This feature has an effect only if "rotate back" is enabled.
    # @note The actual starting angle is <tt>start_angle * controller</tt>.
    # @return [Numeric]
    def start_angle
      MSPhysics::Newton::Hinge.get_start_angle(@address)
    end

    # Set starting angle in radians.
    # @note This feature has an effect only if "rotate back" is enabled.
    # @note The actual starting angle is <tt>start_angle * controller</tt>.
    # @param [Numeric] angle
    def start_angle=(angle)
      MSPhysics::Newton::Hinge.set_start_angle(@address, angle)
    end

    # Get hinge controller, magnitude of the rotational friction or magnitude
    # and direction of the starting angle.
    # @note By default, the controller value is 1.0.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Hinge.get_controller(@address)
    end

    # Set hinge controller, magnitude of the rotational friction or magnitude
    # and direction of the starting angle.
    # @note By default, the controller value is 1.0.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Hinge.set_controller(@address, value)
    end

  end # class Hinge < Joint
end # module MSPhysics
