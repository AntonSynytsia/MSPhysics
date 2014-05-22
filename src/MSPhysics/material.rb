module MSPhysics
  class Material

    # @param [String] name Material name. Passing the name of an existing
    #   material will simply overwrite it.
    # @param [Numeric] density Material density in kilograms per cubic meter.
    # @param [Numeric] static_friction Static friction coefficient. This value
    #   is clamped between +0.01+ and +2.00+.
    # @param [Numeric] kinetic_friction Dynamic friction coefficient. This value
    #   is clamped between +0.01+ and +2.00+.
    # @param [Numeric] elasticity The coefficient of restitution. This is the
    #   rebound ratio. A basketball has a rebound ratio of 0.83. This means the
    #   new height of a basketball is 83% of original height within each bounce.
    #   This value is clamped between +0.01+ and +2.00+.
    # @param [Numeric] softness The softness coefficient. Typical value is 0.15.
    #   This value is clamped between +0.01+ and +1.00+.
    # @param [Numeric] thickness Collision thickness in meters.
    #   Typical value is 0.00. This value is clamped between +0.00+ and +0.125+.
    def initialize(name, density, static_friction, kinetic_friction, elasticity, softness, thickness)
      @name = name.to_s.downcase
      @density = MSPhysics.clamp(density, 0.01, nil)
      @static_friction = MSPhysics.clamp(static_friction, 0.01, 2.00)
      @kinetic_friction = MSPhysics.clamp(kinetic_friction, 0.01, 2.00)
      @elasticity = MSPhysics.clamp(elasticity, 0.01, 2.00)
      @softness = MSPhysics.clamp(softness, 0.01, 1.00)
      @thickness = MSPhysics.clamp(thickness, 0.00, 0.125)
    end

    # @!attribute [r] name
    #   @return [String] Material name.

    # @!attribute [r] density
    #   @return [Numeric] Material density in <b>kg/m^3<b>.

    # @!attribute [r] static_friction
    #   @return [Numeric] Static friction coefficient.

    # @!attribute [r] kinetic_friction
    #   @return [Numeric] Dynamic friction coefficient.

    # @!attribute [r] elasticity
    #   @return [Numeric] Coefficient of restitution.

    # @!attribute [r] softness
    #   @return [Numeric] Softness coefficient.

    # @!attribute [r] thickness
    #   @return [Numeric] Material thickness in meters.


    attr_reader :name, :density, :static_friction, :kinetic_friction, :elasticity, :softness, :thickness

    # Modify body density.
    # @param [Numeric] value Material density in kilograms per cubic meter.
    def density=(value)
      @density = MSPhysics.clamp(value, 0.01, nil)
    end

    # Modify static friction coefficient.
    # @param [Numeric] coefficient
    def static_friction=(coefficient)
      @static_friction = MSPhysics.clamp(coefficient, 0.01, 2.0)
    end

    # Modify kinetic friction coefficient.
    # @param [Numeric] coefficient
    def kinetic_friction=(coefficient)
      @kinetic_friction = MSPhysics.clamp(coefficient, 0.01, 2.0)
    end

    # Modify coefficient of restitution.
    # @param [Numeric] coefficient
    def elasticity=(coefficient)
      @elasticity = MSPhysics.clamp(coefficient, 0.01, 2.0)
    end

    # Modify coefficient of softness.
    # @param [Numeric] coefficient
    def softness=(coefficient)
      @softness = MSPhysics.clamp(coefficient, 0.01, 1.0)
    end

    # Modify material thickness.
    # @param [Numeric] value
    def thickness=(value)
      @thickness = MSPhysics.clamp(value, 0.00, 0.125)
    end

  end # class Material
end # module MSPhysics
