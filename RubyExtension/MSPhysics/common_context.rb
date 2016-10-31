module MSPhysics

  # CommonContext contains methods that both {BodyContext} and
  # {ControllerContext} objects have in common.
  # @since 1.0.0
  class CommonContext

    # @!visibility private
    @@__vars = {}

    class << self

      # Remove all variables created by {#set_var} or {#get_set_var} functions.
      # @api private
      # @return [void]
      def clear_vars
        @@__vars.clear
      end

    end # class << self

    # Get variable value.
    # @param [String, Symbol] name
    # @return [Object] Variable value or +0+ if variable with the specified name
    #   doesn't exist.
    def get_var(name)
      v = @@__vars[name.to_s]
      v.nil? ? 0 : v
    end

    # Set variable value.
    # @param [String, Symbol] name
    # @param [Object] value
    # @return [Object] The newly set value.
    def set_var(name, value)
      @@__vars[name.to_s] = value
    end

    # Get original variable value and set the new value.
    # @param [String, Symbol] name
    # @param [Object] value
    # @return [Object] Original value or +0+ if originally the variable didn't
    #   exist.
    def get_set_var(name, value)
      orig_value = @@__vars[name.to_s]
      @@__vars[name.to_s] = value
      orig_value.nil? ? 0 : orig_value
    end

    # Remove variable from hash.
    # @param [String, Symbol] name
    # @return [Boolean] success
    def delete_var(name)
      @@__vars.delete(name.to_s) != nil
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

    # Get simulation frame.
    # @return [Fixnum]
    def frame
      MSPhysics::Simulation.instance.frame
    end

    # Get state of a keyboard key.
    # @note You may either pass key code in Fixnum form, or key name in
    #   String/Symbol form. This function is not case sensitive.
    # @note Some virtual key names on Mac are different on Mac OS X
    # @param [String, Symbol, Fixnum] vk Virtual key name or value.
    # @return [Fixnum] +1+ if down, +0+ if up.
    # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
    # @see http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx Virtual Key Codes (Windows).
    def key(vk)
      return 0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
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
        MSPhysics::ControlPanel.visible = true
        MSPhysics::ControlPanel.add_slider(name, default_value, min, max, step)
      end
      MSPhysics::ControlPanel.get_slider_value(name)
    end

    # Get joy-stick value.
    # @note Axis name parameter is not case sensitive.
    # @param [String, Symbol] axis Axis name. Valid names are:
    #   - <tt>'leftx'</tt> : X-XAXIS position on left stick.
    #   - <tt>'lefty'</tt> : Y-XAXIS position on left stick.
    #   - <tt>'rightx'</tt> : X-XAXIS position on right stick.
    #   - <tt>'righty'</tt> : Y-XAXIS position on right stick.
    # @return [Numeric] Stick position on the axis, a ranging value from -1.0 to 1.0.
    # @example
    #   onUpdate {
    #     value = joystick(:leftx)
    #     simulation.display_note value.to_s
    #   }
    def joystick(axis)
      v = MSPhysics::Simulation.instance.joystick_data[axis.to_s.downcase]
      v ? v : 0.0
    end

    # Get joy-button value.
    # @note Button name parameter is not case sensitive.
    # @param [String, Symbol] button Button name. Valid names are:
    #   - <tt>X</tt> : The X button.
    #   - <tt>A</tt> : The A button.
    #   - <tt>B</tt> : The B button.
    #   - <tt>Y</tt> : The Y button.
    #   - <tt>LT</tt> : The top-left button.
    #   - <tt>RT</tt> : The top-right button.
    #   - <tt>LB</tt> : The bottom-left button.
    #   - <tt>RB</tt> : The bottom-right button.
    #   - <tt>back</tt> : The back button.
    #   - <tt>start</tt> : The start button.
    #   - <tt>leftb</tt> : The left joystick button.
    #   - <tt>rightb</tt> : The right joystick button.
    # @return [Fixnum] +1+ if the button is down; +0+ if the button is up.
    # @example
    #   onUpdate {
    #     value = joybutton(:lt)
    #     simulation.display_note value.to_s
    #   }
    def joybutton(button)
      v = MSPhysics::Simulation.instance.joybutton_data[button.to_s.downcase]
      v ? v : 0
    end

    # Get joy-pad value.
    # @return [Fixnum] Returns one of the following values:
    # - +0+ if hat is centered
    # - +1+ if hat is up
    # - +2+ if hat is right
    # - +4+ if hat is down
    # - +8+ if hat is left
    # - +12+ if hat is left-down
    # - +9+ if hat is left-up
    # - +6+ if hat is right-down
    # - +3+ if hat is right-up
    # @example
    #   onUpdate {
    #     value = joypad()
    #     simulation.display_note value.to_s
    #   }
    def joypad
      MSPhysics::Simulation.instance.joypad_data
    end

    # Output from LEFT and RIGHT arrow keys or x-axis position on the right
    # joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def rightx
      return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      v = AMS::Keyboard.key('right') - AMS::Keyboard.key('left')
      return v.to_f if v != 0
      v = MSPhysics::Simulation.instance.joystick_data['rightx']
      v ? v : 0.0
    end

    # Output from UP and DOWN arrow keys or y-axis position on the right
    # joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def righty
      return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      v = AMS::Keyboard.key('up') - AMS::Keyboard.key('down')
      return v.to_f if v != 0
      v = MSPhysics::Simulation.instance.joystick_data['righty']
      v ? v : 0.0
    end

    # Output from keys D and A or x-axis position on the left joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def leftx
      return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      v = AMS::Keyboard.key('d') - AMS::Keyboard.key('a')
      return v.to_f if v != 0
      v = MSPhysics::Simulation.instance.joystick_data['leftx']
      v ? v : 0.0
    end

    # Output from keys W and S or y-axis position on the left joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def lefty
      return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      v = AMS::Keyboard.key('w') - AMS::Keyboard.key('s')
      return v.to_f if v != 0
      v = MSPhysics::Simulation.instance.joystick_data['lefty']
      v ? v : 0.0
    end

    # Output from keys NUMPAD6 and NUMPAD4 or centered-x-axis position on the
    # joy-pad.
    # @return [Fixnum] -1, 0, or 1.
    def numx
      return 0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      v = AMS::Keyboard.key('numpad6') - AMS::Keyboard.key('numpad4')
      return v if v != 0
      jpd = MSPhysics::Simulation.instance.joypad_data
      jpd == 2 ? 1 : (jpd == 8 ? -1 : 0)
    end

    # Output from keys NUMPAD8 and NUMPAD5 or centered-y-axis position on the
    # joy-pad.
    # @return [Fixnum] -1, 0, or 1.
    def numy
      return 0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      v = AMS::Keyboard.key('numpad8') - AMS::Keyboard.key('numpad5')
      return v if v != 0
      jpd = MSPhysics::Simulation.instance.joypad_data
      jpd == 1 ? 1 : (jpd == 4 ? -1 : 0)
    end

    # Get oscillated value of a sine curve.
    # @param [Numeric] rate
    # @return [Numeric] A value ranging between -1.0 and 1.0.
    def oscillator(rate)
      return 0.0 if rate.zero?
      inc = (2 * Math::PI) / rate
      Math.sin(inc * MSPhysics::Simulation.instance.frame)
    end

  end # class CommonContext
end # module MSPhysics
