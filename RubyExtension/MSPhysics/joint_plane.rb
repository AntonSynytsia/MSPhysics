module MSPhysics

  # @since 1.0.0
  class Plane < Joint

    DEFAULT_LINEAR_FRICTION = 0.0
    DEFAULT_ANGULAR_FRICTION = 0.0
    DEFAULT_ROTATION_ENABLED = false

    # Create a plane joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::Plane.create(@address)
      MSPhysics::Newton::Plane.set_linear_friction(@address, DEFAULT_LINEAR_FRICTION)
      MSPhysics::Newton::Plane.set_angular_friction(@address, DEFAULT_ANGULAR_FRICTION)
      MSPhysics::Newton::Plane.enable_rotation(@address, DEFAULT_ROTATION_ENABLED)
    end

    # Get movement friction.
    # @return [Numeric] A value greater than or equal to zero.
    def linear_friction
      MSPhysics::Newton::Plane.get_linear_friction(@address)
    end

    # Set movement friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def linear_friction=(value)
      MSPhysics::Newton::Plane.set_linear_friction(@address, value)
    end

    # Get rotation friction.
    # @return [Numeric] A value greater than or equal to zero.
    def angular_friction
      MSPhysics::Newton::Plane.get_angular_friction(@address)
    end

    # Set rotation friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def angular_friction=(value)
      MSPhysics::Newton::Plane.set_angular_friction(@address, value)
    end

    # Determine whether the rotation along the connected body's center of mass
    # and the plane's z-axis is enabled.
    # @return [Boolean]
    def rotation_enabled?
      MSPhysics::Newton::Plane.rotation_enabled?(@address)
    end

    # Enable/disable the rotation along the connected body's center of mass and
    # the plane's z-axis.
    # @param [Boolean] state
    def rotation_enabled=(state)
      MSPhysics::Newton::Plane.enable_rotation(@address, state)
    end

  end # class Plane < Joint
end # module MSPhysics
