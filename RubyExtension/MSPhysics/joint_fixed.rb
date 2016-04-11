module MSPhysics

  # @since 1.0.0
  class Fixed < Joint

    # Create a fixed joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-axis should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::Fixed.create(@address)
    end

  end # class Fixed < Joint
end # module MSPhysics
