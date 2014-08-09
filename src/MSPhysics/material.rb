module MSPhysics
  class Material

    # @param [String] name Material name. Passing the name of an existing
    #   material will simply overwrite it.
    # @param [Numeric] density Material density in kilograms per cubic meter.
    # @param [Numeric] static_friction Static friction coefficient. This value
    #   is clamped between +0.01+ and +2.00+.
    # @param [Numeric] dynamic_friction Dynamic friction coefficient. This value
    #   is clamped between +0.01+ and +2.00+.
    # @param [Numeric] elasticity The coefficient of restitution. This is the
    #   rebound ratio. A basketball has a rebound ratio of 0.83. This means the
    #   new height of a basketball is 83% of original height within each bounce.
    #   This value is clamped between +0.01+ and +2.00+.
    # @param [Numeric] softness The softness coefficient. Decreasing softness
    #   yields adaptable collision. Increasing softness yields resistible
    #   collision. This value is clamped between +0.01+ and +1.00+. Typical
    #   value is 0.15.
    def initialize(name, density, static_friction, dynamic_friction, elasticity, softness)
      @name = name.to_s
      @density = MSPhysics.clamp(density, 0.01, nil)
      @static_friction = MSPhysics.clamp(static_friction, 0.01, 2.00)
      @dynamic_friction = MSPhysics.clamp(dynamic_friction, 0.01, 2.00)
      @elasticity = MSPhysics.clamp(elasticity, 0.01, 2.00)
      @softness = MSPhysics.clamp(softness, 0.01, 1.00)
    end

    # @!attribute [r] name
    #   @return [String] Material name.

    # @!attribute [r] density
    #   @return [Numeric] Material density in <b>kg/m^3<b>.

    # @!attribute [r] static_friction
    #   @return [Numeric] Static friction coefficient.

    # @!attribute [r] dynamic_friction
    #   @return [Numeric] Dynamic friction coefficient.

    # @!attribute [r] elasticity
    #   @return [Numeric] Coefficient of restitution.

    # @!attribute [r] softness
    #   @return [Numeric] Softness coefficient.


    attr_reader :name, :density, :static_friction, :dynamic_friction, :elasticity, :softness

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
    def dynamic_friction=(coefficient)
      @dynamic_friction = MSPhysics.clamp(coefficient, 0.01, 2.0)
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

    # Determine whether current material has the same properties as another material.
    # @param [Material] other_material
    # @return [Boolean]
    def equals?(other_material)
      return false if other_material.density != @density
      return false if other_material.static_friction != @static_friction
      return false if other_material.dynamic_friction != @dynamic_friction
      return false if other_material.elasticity != @elasticity
      return false if other_material.softness != @softness
      true
    end

  end # class Material
end # module MSPhysics
