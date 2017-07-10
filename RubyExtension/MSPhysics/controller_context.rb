module MSPhysics

  # All controllers are evaluated under the ControllerContext scope.
  # @since 1.0.0
  class ControllerContext < CommonContext

    include Math

    def initialize
      super()
    end

    # @param [String] script
    # @param [String] script_name
    # @param [Fixnum] line
    def eval_script(script, script_name, line)
      eval(script, binding, script_name, line)
    end

  end # class ControllerContext
end # module MSPhysics
