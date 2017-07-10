module MSPhysics

  # @since 1.0.0
  class GearConnectionTool < Entity

    # @!visibility private
    @@instance = nil

    class << self

      # Activate joint connection tool.
      # @return [Boolean] success
      def activate
        return false if @@instance
        Sketchup.active_model.select_tool(self.new)
        true
      end

      # Deactivate joint connection tool.
      # @return [Boolean] success
      def deactivate
        return false unless @@instance
        Sketchup.active_model.select_tool(nil)
        true
      end

      # Determine whether joint connection tool is active.
      # @return [Boolean]
      def active?
        @@instance ? true : false
      end

    end # class << self

    # Gears can be formed between Hinge, Motor, Servo, Slider, Piston, Spring,
    # CurvySlider, and CurvyPiston joints.
    # To connect gears
    #   1. Select instance A.
    #   2. Hold CTRL/SHIFT or both and select instance B.
    # To disconnect gears
    #   1. Select a geared instance.
    #   2. Hold SHIFT or CTRl+SHIFT and select the other instance.
    # To change reference joint.
    #   1. Select a geared instance.
    #   2. And click on one of the connected joints.
    # Changing name and gear ratio
    #   1. Select a geared instance
    #   2. Open MSPhysics UI
    #   3. Navigate to gears section and edit properties.
    # Rules
    #   1. Both instances must not be ignored and must be top-level entities.
    #   2. Both instances must be connected to at least one joint.
    #   3. Both instances can be connected to the same joint if the joint is
    #      either a Hinge, a Slider, a Spring, or a CurvySlider.
    #   4. Both instances can be connected to multiple joints. By default,
    #      the first, connected joints are used for gears.
    # Copyable gears
    #   * Copyable gears work in the same concept as copyable joints.
    # Specs
    #   * Gear information is stored in one of the instances.
    #   * When two instances are connected, both are assigned unique
    #     identifiers.
    #   * Whenever simulation starts, all invalid gear attributes are removed.
    def initialize
    end

  end # class GearConnectionTool
end # module MSPhysics
