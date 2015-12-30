module MSPhysics

  # @since 1.0.0
  class UpVector < Joint

    DEFAULT_STIFF = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_DAMPER_ENABLED = false
    DEFAULT_PIN_DIR = Z_AXIS

    # Create an UpVector joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::UpVector.create(@address)
      MSPhysics::Newton::UpVector.set_stiff(@address, DEFAULT_STIFF)
      MSPhysics::Newton::UpVector.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::UpVector.enable_damper(@address, DEFAULT_DAMPER_ENABLED)
      MSPhysics::Newton::UpVector.set_pin_dir(@address, DEFAULT_PIN_DIR)
    end

    # Get rotation stiffness. Higher stiffness makes rotation stronger.
    # @return [Numeric]
    def stiff
      MSPhysics::Newton::UpVector.get_stiff(@address)
    end

    # Set rotation stiffness. Higher stiffness makes rotation stronger.
    # @param [Numeric] value A value greater than or equal to zero.
    def stiff=(value)
      MSPhysics::Newton::UpVector.set_stiff(@address, value)
    end

    # Get rotation damper. Higher damper makes rotation slower.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::UpVector.get_damp(@address)
    end

    # Set rotation damper. Higher damper makes rotation slower.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::UpVector.set_damp(@address, value)
    end

    # Determine whether rotation stiff & damp parameters are enabled.
    # @return [Boolean]
    def damper_enabled?
      MSPhysics::Newton::UpVector.is_damper_enabled?(@address)
    end

    # Enable/Disable rotation stiff & damp parameters.
    # @param [Boolean] state
    def damper_enabled=(state)
      MSPhysics::Newton::UpVector.enable_damper(@address, state)
    end

    # Get desired pin direction with respect to joint orientation.
    # @return [Geom::Vector3d]
    def get_pin_dir
      MSPhysics::Newton::UpVector.get_pin_dir(@address)
    end

    # Set desired pin direction with respect to joint orientation.
    # @param [Geom::Vector3d] pin_dir
    # @return [Geom::Vector3d] New pin direction
    def set_pin_dir(pin_dir)
      MSPhysics::Newton::UpVector.set_pin_dir(@address, pin_dir)
    end

  end # class UpVector < Joint
end # module MSPhysics
