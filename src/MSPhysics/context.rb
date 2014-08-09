module MSPhysics
  module ControllerContext

    # @!visibility private
    @@_variables = {}

    # @api private
    def self.clear_variables
      @@_variables.clear
    end

    # Get simulation frame.
    # @return [Fixnum]
    def frame
      simulation_tool.frame
    end

    # Get key state.
    # @see http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx Virtual Key Codes
    # @note You may either pass key value in Fixnum form, or key name in
    #   String/Symbol form. This function is not case sensitive.
    # @param [String, Symbol, Fixnum] vk Virtual key name or value.
    # @return [Fixnum] +1+ if down, +0+ if up.
    def key(vk)
      AMS::Keyboard.key(vk)
    end

    # LEFT & RIGHT
    # @return [Fixnum] -1, 0, or 1.
    def rightx
      key('right') - key('left')
    end

    # UP & DOWN
    # @return [Fixnum] -1, 0, or 1.
    def righty
      key('up') - key('down')
    end

    # A & D.
    # @return [Fixnum] -1, 0, or 1.
    def leftx
      key('d') - key('a')
    end

    # W & S
    # @return [Fixnum] -1, 0, or 1.
    def lefty
      key('w') - key('s')
    end

    # NUMPAD6 & NUMPAD4
    # @return [Fixnum] -1, 0, or 1.
    def numx
      key('numpad6') - key('numpad4')
    end

    # NUMPAD8 & NUMPAD5
    # @return [Fixnum] -1, 0, or 1.
    def numy
      key('numpad8') - key('numpad5')
    end

    # Get variable value.
    # @param [String] var Variable name.
    # @return [Object, NilClass] Variable value (if successful).
    def get_var(var)
      @@_variables[var.to_s]
    end

    # Set variable value.
    # @param [String] var Variable name.
    # @param [Object] val Variable value.
    # @return [Boolean] +true+ (if successful).
    def set_var(var, val)
      @@_variables[var.to_s] = val
      true
    end

    # Get original value and set the new value.
    # @param [String] var Variable name.
    # @param [Object] val New value.
    # @return [Object, NilClass] Original value (if successful).
    def get_set_var(var, val)
      v = @@_variables[var.to_s]
      @@_variables[var.to_s] = val
      v
    end

    # Remove a variable from hash.
    # @param [String] var Variable name.
    # @return [Boolean] +true+ (if successful).
    def delete_var(var)
      @@_variables.delete(var.to_s) != nil
    end

    # Get sine oscillated value.
    # @param [Numeric] rate
    # @return [Numeric] A value between -1 and 1.
    def oscillator(rate)
      inc = (2*Math::PI)/rate
      Math.sin(inc*frame)
    end

  end # module ControllerContext

  module CommonContext

    include ControllerContext

    # @return [Object] self
    def this
      self
    end

    # @return [MSPhysics::Simulation]
    def simulation
      MSPhysics::SimulationTool.instance.simulation
    end

    # @return [MSPhysics::SimulationTool]
    def simulation_tool
      MSPhysics::SimulationTool.instance
    end

  end # module CommonContext
end # module MSPhysics
