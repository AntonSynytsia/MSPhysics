module MSPhysics

  # @since 1.0.0
  class Hinge < Joint

    DEFAULT_MIN = -180.0
    DEFAULT_MAX = 180.0
    DEFAULT_STIFF = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_DAMPER_ENABLED = false
    DEFAULT_ROTATE_BACK_ENABLED = false
    DEFAULT_START_ANGLE = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a hinge joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::Hinge.create(@address)
      MSPhysics::Newton::Hinge.set_min(@address, DEFAULT_MIN.degrees)
      MSPhysics::Newton::Hinge.set_max(@address, DEFAULT_MAX.degrees)
      MSPhysics::Newton::Hinge.set_stiff(@address, DEFAULT_STIFF)
      MSPhysics::Newton::Hinge.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Hinge.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Hinge.enable_damper(@address, DEFAULT_DAMPER_ENABLED)
      MSPhysics::Newton::Hinge.enable_rotate_back(@address, DEFAULT_ROTATE_BACK_ENABLED)
      MSPhysics::Newton::Hinge.set_start_angle(@address, DEFAULT_START_ANGLE.degrees)
      MSPhysics::Newton::Hinge.set_controller(@address, DEFAULT_CONTROLLER)
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
      MSPhysics::Newton::Hinge.are_limits_enabled?(@address)
    end

    # Enable/Disable min & max angle limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Hinge.enable_limits(@address, state)
    end

    # Get rotational stiffness. Higher stiffness makes rotation stronger.
    # @return [Numeric]
    def stiff
      MSPhysics::Newton::Hinge.get_stiff(@address)
    end

    # Set rotational stiffness. Higher stiffness makes rotation stronger.
    # @param [Numeric] value A value greater than or equal to zero.
    def stiff=(value)
      MSPhysics::Newton::Hinge.set_stiff(@address, value)
    end

    # Get rotational damper. Higher damper makes rotation slower.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::Hinge.get_damp(@address)
    end

    # Set rotational damper. Higher damper makes rotation slower.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::Hinge.set_damp(@address, value)
    end

    # Determine whether rotational stiff & damp parameters are enabled.
    # @return [Boolean]
    def damper_enabled?
      MSPhysics::Newton::Hinge.is_damper_enabled?(@address)
    end

    # Enable/Disable rotational stiff & damp parameters.
    # @param [Boolean] state
    def damper_enabled=(state)
      MSPhysics::Newton::Hinge.enable_damper(@address, state)
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

    # Determine whether a feature to rotate back to a starting angle is enabled.
    # @return [Boolean]
    def rotate_back_enabled?
      MSPhysics::Newton::Hinge.is_rotate_back_enabled?(@address)
    end

    # Enable/Disable a feature to rotate back to a starting angle.
    # @param [Boolean] state
    def rotate_back_enabled=(state)
      MSPhysics::Newton::Hinge.enable_rotate_back(@address, state)
    end

    # Get starting angle in radians.
    # @note The actual starting angle is <tt>start_angle * controller</tt>.
    # @return [Numeric]
    def start_angle
      MSPhysics::Newton::Hinge.get_start_angle(@address)
    end

    # Set starting angle in radians.
    # @note The actual starting angle is <tt>start_angle * controller</tt>.
    # @param [Numeric] angle
    def start_angle=(angle)
      MSPhysics::Newton::Hinge.set_start_angle(@address, angle)
    end

    # Get hinge controller, magnitude and direction of the starting angle. By
    # default, controller value is 1.
    # @note The actual starting angle is <tt>start_angle * controller</tt>.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Hinge.get_controller(@address)
    end

    # Set hinge controller, magnitude and direction of the starting angle. By
    # default, controller value is 1.
    # @note The actual starting angle is <tt>start_angle * controller</tt>.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Hinge.set_controller(@address, value)
    end

  end # class Hinge < Joint
end # module MSPhysics
