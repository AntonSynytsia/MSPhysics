module MSPhysics

  # @since 1.0.0
  class UpVector < Joint

    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_STRENGTH = 0.98
    DEFAULT_PIN_DIR = Z_AXIS

    # Create an UpVector joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::UpVector.create(@address)
      MSPhysics::Newton::UpVector.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::UpVector.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::UpVector.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::UpVector.set_pin_dir(@address, DEFAULT_PIN_DIR)
    end

    # Get angular acceleration.
    # @note Higher acceleration makes rotation faster.
    # @return [Numeric]
    def accel
      MSPhysics::Newton::UpVector.get_accel(@address)
    end

    # Set angular acceleration.
    # @note Higher acceleration makes rotation faster.
    # @param [Numeric] value A value greater than or equal to zero.
    def accel=(value)
      MSPhysics::Newton::UpVector.set_accel(@address, value)
    end

    # Get angular damper.
    # @note Higher damper makes rotation stronger.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::UpVector.get_damp(@address)
    end

    # Set angular damper.
    # @note Higher damper makes rotation stronger.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::UpVector.set_damp(@address, value)
    end

    # Get strength of the acceleration and damper.
    # @return [Numeric] A value between 0.0 and 1.0.
    def strength
      MSPhysics::Newton::UpVector.get_strength(@address)
    end

    # Set strength of the acceleration and damper.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def strength=(value)
      MSPhysics::Newton::UpVector.set_strength(@address, value)
    end

    # Get desired pin direction with respect to joint orientation.
    # @return [Geom::Vector3d]
    def get_pin_dir
      MSPhysics::Newton::UpVector.get_pin_dir(@address)
    end

    # Set desired pin direction with respect to joint orientation.
    # @param [Geom::Vector3d] pin_dir
    # @return [nil]
    def set_pin_dir(pin_dir)
      MSPhysics::Newton::UpVector.set_pin_dir(@address, pin_dir)
    end

  end # class UpVector < Joint
end # module MSPhysics
