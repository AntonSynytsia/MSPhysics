# @since 1.0.0
class MSPhysics::ScriptException < Exception

  # @param [String] message
  # @param [Array<String>] backtrace
  # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
  # @param [Integer, nil] line
  def initialize(message, backtrace, entity, line)
    AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
    super(message)
    set_backtrace(backtrace)
    @entity = entity
    @line = line ? line.to_i : nil
  end

  # @!attribute [r] entity
  # @return [Sketchup::Group, Sketchup::ComponentInstance]

  # @!attribute [r] line
  # @return [Integer, nil]


  attr_reader :entity, :line

end # class MSPhysics::ScriptException
