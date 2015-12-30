module MSPhysics

  # @since 1.0.0
  class ScriptException < Exception

    # @param [String] message
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @param [Fixnum, nil] line
    def initialize(message, entity, line)
      AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
      super(message)
      @entity = entity
      @line = line ? line.to_i : nil
    end

    # @!attribute [r] entity
    # @return [Sketchup::Group, Sketchup::ComponentInstance]

    # @!attribute [r] line
    # @return [Fixnum, nil]


    attr_reader :entity, :line

  end # class ScriptException
end # module MSPhysics
