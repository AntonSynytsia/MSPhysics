module MSPhysics

  # @since 1.0.0
  class BallAndSocket < Joint

    DEFAULT_STIFF = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_DAMPER_ENABLED = false
    DEFAULT_MAX_CONE_ANGLE = 30.0
    DEFAULT_CONE_LIMITS_ENABLED = false
    DEFAULT_MIN_TWIST_ANGLE = -180.0
    DEFAULT_MAX_TWIST_ANGLE = 180.0
    DEFAULT_TWIST_LIMITS_ENABLED = false

    # Create a BallAndSocket joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::BallAndSocket.create(@address)
      MSPhysics::Newton::BallAndSocket.set_stiff(@address, DEFAULT_STIFF)
      MSPhysics::Newton::BallAndSocket.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::BallAndSocket.enable_damper(@address, DEFAULT_DAMPER_ENABLED)
      MSPhysics::Newton::BallAndSocket.set_max_cone_angle(@address, DEFAULT_MAX_CONE_ANGLE.degrees)
      MSPhysics::Newton::BallAndSocket.enable_cone_limits(@address, DEFAULT_CONE_LIMITS_ENABLED)
      MSPhysics::Newton::BallAndSocket.set_min_twist_angle(@address, DEFAULT_MIN_TWIST_ANGLE.degrees)
      MSPhysics::Newton::BallAndSocket.set_max_twist_angle(@address, DEFAULT_MAX_TWIST_ANGLE.degrees)
      MSPhysics::Newton::BallAndSocket.enable_twist_limits(@address, DEFAULT_TWIST_LIMITS_ENABLED)
    end

    # Get rotation stiffness. Higher stiffness makes rotation stronger.
    # @return [Numeric]
    def stiff
      MSPhysics::Newton::BallAndSocket.get_stiff(@address)
    end

    # Set rotation stiffness. Higher stiffness makes rotation stronger.
    # @param [Numeric] value A value greater than or equal to zero.
    def stiff=(value)
      MSPhysics::Newton::BallAndSocket.set_stiff(@address, value)
    end

    # Get rotation damper. Higher damper makes rotation slower.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::BallAndSocket.get_damp(@address)
    end

    # Set rotation damper. Higher damper makes rotation slower.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::BallAndSocket.set_damp(@address, value)
    end

    # Determine whether rotation stiff & damp parameters are enabled.
    # @return [Boolean]
    def damper_enabled?
      MSPhysics::Newton::BallAndSocket.is_damper_enabled?(@address)
    end

    # Enable/Disable rotation stiff & damp parameters.
    # @param [Boolean] state
    def damper_enabled=(state)
      MSPhysics::Newton::BallAndSocket.enable_damper(@address, state)
    end

    # Get maximum cone angle in radians.
    # @return [Numeric]
    def max_cone_angle
      MSPhysics::Newton::BallAndSocket.get_max_cone_angle(@address)
    end

    # Set maximum cone angle in radians.
    # @param [Numeric] angle
    def max_cone_angle=(angle)
      MSPhysics::Newton::BallAndSocket.set_max_cone_angle(@address, angle)
    end

    # Determine whether cone angle limits are enabled.
    # @return [Boolean]
    def cone_limits_enabled?
      MSPhysics::Newton::BallAndSocket.are_cone_limits_enabled?(@address)
    end

    # Enable/Disable cone angle limits.
    # @param [Boolean] state
    def cone_limits_enabled=(state)
      MSPhysics::Newton::BallAndSocket.enable_cone_limits(@address, state)
    end

    # Get minimum twist angle in radians.
    # @return [Numeric]
    def min_twist_angle
      MSPhysics::Newton::BallAndSocket.get_min_twist_angle(@address)
    end

    # Set minimum twist angle in radians.
    # @param [Numeric] angle
    def min_twist_angle=(angle)
      MSPhysics::Newton::BallAndSocket.set_min_twist_angle(@address, angle)
    end

    # Get maximum twist angle in radians.
    # @return [Numeric]
    def max_twist_angle
      MSPhysics::Newton::BallAndSocket.get_max_twist_angle(@address)
    end

    # Set maximum twist angle in radians.
    # @param [Numeric] angle
    def max_twist_angle=(angle)
      MSPhysics::Newton::BallAndSocket.set_max_twist_angle(@address, angle)
    end

    # Determine whether twist angle limits are enabled.
    # @return [Boolean]
    def twist_limits_enabled?
      MSPhysics::Newton::BallAndSocket.are_twist_limits_enabled?(@address)
    end

    # Enable/Disable twist angle limits.
    # @param [Boolean] state
    def twist_limits_enabled=(state)
      MSPhysics::Newton::BallAndSocket.enable_twist_limits(@address, state)
    end

    # Get current cone angle in radians.
    # @return [Numeric]
    def cur_cone_angle
      MSPhysics::Newton::BallAndSocket.get_cur_cone_angle(@address)
    end

    # Get current twist angle in radians.
    # @return [Numeric]
    def cur_twist_angle
      MSPhysics::Newton::BallAndSocket.get_cur_twist_angle(@address)
    end

  end # class BallAndSocket < Joint
end # module MSPhysics
