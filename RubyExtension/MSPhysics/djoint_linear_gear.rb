module MSPhysics

  # @since 1.0.0
  class LinearGear < DoubleJoint

    # Create a LinearGear joint.
    # @param [MSPhysics::World] world
    # @param [Geom::Transformation, Array<Numeric>] pin1_tra Pin1 transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [MSPhysics::Body, nil] parent1
    # @param [Geom::Transformation, Array<Numeric>] pin2_tra Pin2 transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [MSPhysics::Body, nil] parent2
    def initialize(world, pin1_tra, parent1, pin2_tra, parent2)
      super(world, pin1_tra, parent1, pin2_tra, parent2)
      MSPhysics::Newton::LinearGear.create(@address)
    end

    # Get linear gear ratio.
    def ratio
      MSPhysics::Newton::LinearGear.get_ratio(@address)
    end

    # Set linear gear ratio.
    def ratio=(value)
      MSPhysics::Newton::LinearGear.set_ratio(@address, value)
    end

  end # class LinearGear < DoubleJoint
end # module MSPhysics
