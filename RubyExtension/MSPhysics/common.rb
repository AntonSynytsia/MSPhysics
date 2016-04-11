module MSPhysics

  # A Common class contains methods that both {Body} and {Controller} objects
  # have in common.
  # @since 1.0.0
  class Common

    # @!visibility private
    @@_variables = {}

    class << self

      # Remove all variables created by {#set_var} or {#get_set_var} functions.
      # @api private
      # @return [void]
      def clear_variables
        @@_variables.clear
      end

    end # class << self

    # Get variable value.
    # @param [String, Symbol] name
    # @return [Object] Variable value or +0+ if variable with the specified name
    #   doesn't exist.
    def get_var(name)
      v = @@_variables[name.to_s]
      v.nil? ? 0 : v
    end

    # Set variable value.
    # @param [String, Symbol] name
    # @param [Object] value
    # @return [Object] The newly set value.
    def set_var(name, value)
      @@_variables[name.to_s] = value
    end

    # Get original variable value and set the new value.
    # @param [String, Symbol] name
    # @param [Object] value
    # @return [Object] Original value or +0+ if originally the variable didn't
    #   exist.
    def get_set_var(name, value)
      orig_value = @@_variables[name.to_s]
      @@_variables[name.to_s] = value
      orig_value.nil? ? 0 : orig_value
    end

    # Remove variable from hash.
    # @param [String, Symbol] name
    # @return [Boolean] success
    def delete_var(name)
      @@_variables.delete(name.to_s) != nil
    end

    # Get {Simulation} instance.
    # @return [Simulation]
    def simulation
      MSPhysics::Simulation.instance
    end

    # Get simulation {World} instance.
    # @return [World]
    def world
      MSPhysics::Simulation.instance.world
    end

    # @return [self]
    def this
      self
    end

    # Get simulation frame.
    # @return [Fixnum]
    def frame
      MSPhysics::Simulation.instance.frame
    end

    # Get key state.
    # @note You may either pass key value in Fixnum form, or key name in
    #   String/Symbol form. This function is not case sensitive.
    # @param [String, Symbol, Fixnum] vk Virtual key name or value.
    # @return [Fixnum] +1+ if down, +0+ if up.
    # @see http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx Virtual Key Codes
    def key(vk)
      AMS::Keyboard.key(vk)
    end

    # Create a new range slider or get slider value if slider with the specified
    # name already exists.
    # @param [String] name Slider name.
    # @param [Numeric] default_value Starting value.
    # @param [Numeric] min Minimum value.
    # @param [Numeric] max Maximum value.
    # @param [Numeric] step Snap step.
    # @return [Numeric] Slider value.
    def slider(name, default_value = 0, min = 0, max = 1, step = 0.01)
      unless MSPhysics::ControlPanel.slider_exists?(name)
        MSPhysics::ControlPanel.show(true)
        MSPhysics::ControlPanel.add_slider(name, default_value, min, max, step)
      end
      MSPhysics::ControlPanel.get_slider_value(name)
    end

  end # class Common
end # module MSPhysics
