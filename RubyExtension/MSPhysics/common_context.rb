# CommonContext contains methods that both {MSPhysics::BodyContext} and
# {MSPhysics::ControllerContext} objects have in common.
# @since 1.0.0
class MSPhysics::CommonContext < MSPhysics::Entity

  # @!visibility private
  @@_global_variables = {}
  # @!visibility private
  @@_variables = {}
  # @!visibility private
  @@_toggled = {}
  # @!visibility private
  @@_key_sliders = {}

  def initialize
    @_singular_repeater_flags = {}
  end

  # @!group Variables

  # Get variable value.
  # @note These variables are accessible in all body/controller scopes and
  #   exist only throughout the session of simulation. They are disposed of
  #   after simulation ends.
  # @param [String, Symbol] name
  # @return [Object] Variable value or +nil+ if variable with the specified
  #   name doesn't exist.
  # @example Joint controller based on a script
  #   # The following is pasted to one of the script tabs
  #   onStart {
  #     set_var('angle', 0.0) # This variable is accessible across all script and controller contexts.
  #     @step = 0.2 # This variable is accessible within this script context only.
  #   }
  #   onTick {
  #     cur_angle = get_var('angle') # Local variable accessible within a block.
  #     # Control angle with keys g and r, that is within the boundaries -30 and 12 degrees.
  #     if key('r') == 1 && cur_angle < 12.0
  #       cur_angle += @step
  #     elsif key('g') == 1 && cur_angle > -30.0
  #       cur_angle -= @step
  #     end
  #     # Clamp the angle value to ensure its within range
  #     cur_angle = AMS.clamp(cur_angle, -30.0, 12.0)
  #     # Assign new value to the 'angle' variable
  #     set_var('angle', cur_angle)
  #   }
  #
  #   # The following is pasted to one or more of the joint controllers
  #   get_var('angle')
  def get_var(name)
    @@_variables[name]
  end

  # Set variable value.
  # @note These variables are accessible in all body/controller scopes and
  #   exist only throughout the session of simulation. They are disposed of
  #   after simulation ends.
  # @param [String, Symbol] name
  # @param [Object] value
  # @return [Object] The newly assigned value.
  # @example (see #get_var)
  def set_var(name, value)
    @@_variables[name] = value
  end

  # Remove variable from hash.
  # @param [String, Symbol] name
  # @return [Boolean] success
  def delete_var(name)
    @@_variables.delete(name) != nil
  end

  # Get original variable value and assign a new value.
  # @note These variables are accessible in all body/controller scopes and
  #   exist only throughout the session of simulation. They are disposed of
  #   after simulation ends.
  # @param [String, Symbol] name
  # @param [Object] value
  # @return [Object] Original value or +nil+ if originally the variable didn't
  #   exist.
  def get_set_var(name, value)
    orig_value = @@_variables[name]
    @@_variables[name] = value
    orig_value
  end

  # Get global variable value.
  # @note These variables are accessible in all body/controller scopes and
  #   exist throughout the session of the SketchUp process. They are preserved
  #   after simulation ends.
  # @param [String, Symbol] name
  # @return [Object] Variable value or +nil+ if variable with the specified
  #   name doesn't exist.
  def get_global_var(name)
    @@_global_variables[name]
  end

  # Set global variable value.
  # @note These variables are accessible in all body/controller scopes and
  #   exist throughout the session of the SketchUp process. They are preserved
  #   after simulation ends.
  # @param [String, Symbol] name
  # @param [Object] value
  # @return [Object] The newly assigned value.
  def set_global_var(name, value)
    @@_global_variables[name] = value
  end

  # Get original global variable value and assign a new value.
  # @note These variables are accessible in all body/controller scopes and
  #   exist throughout the session of the SketchUp process. They are preserved
  #   after simulation ends.
  # @param [String, Symbol] name
  # @param [Object] value
  # @return [Object] Original value or +nil+ if originally the variable didn't
  #   exist.
  def get_set_global_var(name, value)
    orig_value = @@_global_variables[name]
    @@_global_variables[name] = value
    orig_value
  end

  # Remove global variable from hash.
  # @param [String, Symbol] name
  # @return [Boolean] success
  def delete_global_var(name)
    @@_global_variables.delete(name) != nil
  end

  # @!endgroup
  # @!group Instances

  # Get {Simulation} instance.
  # @return [Simulation]
  def simulation
    MSPhysics::Simulation.instance
  end

  # Get simulation {World} instance.
  # @return [World]
  # @example Using in controller
  #   (world.time > 4) ? 1 : 0
  def world
    MSPhysics::Simulation.instance.world
  end

  # @!endgroup
  # @!group Time

  # Get simulation frame.
  # @return [Integer]
  # @example Using in controller
  #   (frame > 100) ? 1 : 0
  def frame
    MSPhysics::Simulation.instance.frame
  end

  # @!endgroup
  # @!group User Input

  # Get state of a keyboard key.
  # @note The <tt>vk</tt> parameter is not case sensitive.
  # @param [String, Symbol, Integer] vk Virtual key code or name.
  # @return [Integer] +1+ if down, +0+ if up.
  # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
  # @see http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx Virtual Key Codes (Windows).
  # @example Using in controller
  #   key('space') * 10
  def key(vk)
    return 0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    AMS::Keyboard.key(vk)
  end

  # Get toggled state of a keyboard key.
  # @note The <tt>vk</tt> parameter is not case sensitive.
  # @param [String, Symbol, Integer] vk Virtual key code or name.
  # @return [Integer] +1+ if toggled down, +0+ if toggled up.
  # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
  # @see http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx Virtual Key Codes (Windows).
  # @example Using in controller
  #   toggle_key('space') * 10
  def toggle_key(vk)
    vkc = AMS::Keyboard.get_key_code(vk)
    vks = AMS::Keyboard.key_down?(vkc)
    vkt = @@_toggled[vkc] || 0
    if vkt == 0 && vks
      @@_toggled[vkc] = 1
      1
    elsif (vkt == 1 && vks) || (vkt == 2 && !vks)
      1
    elsif vkt == 1 && !vks
      @@_toggled[vkc] = 2
      1
    elsif vkt == 2 && vks
      @@_toggled[vkc] = 3
      0
    elsif vkt == 3 && !vks
      @@_toggled[vkc] = 0
      0
    else
      0
    end
  end

  # Create a new range slider or get slider value if slider with the specified
  # name already exists.
  # @param [String] name Slider name.
  # @param [Numeric] default_value Starting value.
  # @param [Numeric] min Minimum value.
  # @param [Numeric] max Maximum value.
  # @param [Numeric] step Snap step.
  # @return [Numeric] Slider value.
  # @example Using in controller
  #   slider('Rotate', 0, -10, 10, 0.01)
  def slider(name, default_value = 0.0, min = 0.0, max = 1.0, step = 1.0)
    unless MSPhysics::ControlPanel.slider_exists?(name)
      MSPhysics::ControlPanel.open
      MSPhysics::ControlPanel.show
      MSPhysics::ControlPanel.add_slider(name, default_value, min, max, step)
    end
    MSPhysics::ControlPanel.get_slider_value(name)
  end

  # Create a new range slider or get slider value if slider with the specified
  # name already exists.
  # @param [String] name Slider name.
  # @param [String] key1 The positive directing key.
  # @param [String] key2 The negative directing key.
  # @param [Numeric] default_value Starting value.
  # @param [Numeric] min Minimum value.
  # @param [Numeric] max Maximum value.
  # @param [Numeric] step Accumulation step in units per second.
  # @return [Numeric] Slider value.
  # @example Using in controller
  #   key_slider('Lift', 'up', 'down', 0, -4, 26, 5)
  # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
  def key_slider(name, key1, key2, default_value = 0.0, min = 0.0, max = 1.0, step = 1.0)
    data = @@_key_sliders[name]
    unless data
      data = {
        :key1 => key1,
        :key2 => key2,
        :value => default_value.to_f,
        :min => min.to_f,
        :max => max.to_f,
        :step => step.to_f
      }
      @@_key_sliders[name] = data
    end
    data[:value]
  end

  # Get joystick value.
  # @note Axis name parameter is not case sensitive.
  # @param [String, Symbol] axis Axis name. Valid names are:
  #   - <tt>'leftx'</tt> : X-XAXIS position on left thumbstick.
  #   - <tt>'lefty'</tt> : Y-XAXIS position on left thumbstick.
  #   - <tt>'leftz'</tt> : Left trigger position.
  #   - <tt>'rightx'</tt> : X-XAXIS position on right thumbstick.
  #   - <tt>'righty'</tt> : Y-XAXIS position on right thumbstick.
  #   - <tt>'rightz'</tt> : Right trigger position.
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
  #   - <tt>A</tt> : The A button.
  #   - <tt>B</tt> : The B button.
  #   - <tt>X</tt> : The X button.
  #   - <tt>Y</tt> : The Y button.
  #   - <tt>LB</tt> : The left shoulder button, above the trigger.
  #   - <tt>RB</tt> : The right shoulder button, above the trigger.
  #   - <tt>LT</tt> : The left trigger.
  #   - <tt>RT</tt> : The right trigger.
  #   - <tt>back</tt> : The back button.
  #   - <tt>start</tt> : The start button.
  #   - <tt>leftb</tt> : The left thumbstick button.
  #   - <tt>rightb</tt> : The right thumbstick button.
  # @return [Integer] +1+ if the button is down; +0+ if the button is up.
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
  # @return [Integer] Returns one of the following values:
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

  # Output from LEFT and RIGHT arrow keys or X-axis position on the right
  # joystick.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  def rightx
    return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('right') - AMS::Keyboard.key('left')
    return v.to_f if v != 0
    v = MSPhysics::Simulation.instance.joystick_data['rightx']
    v ? v : 0.0
  end

  # Output from UP and DOWN arrow keys or Y-axis position on the right
  # joystick.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  def righty
    return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('up') - AMS::Keyboard.key('down')
    return v.to_f if v != 0
    v = MSPhysics::Simulation.instance.joystick_data['righty']
    v ? v : 0.0
  end

  # Output from keys PageUp and PageDown or position of the joy controller's
  # right trigger.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  def rightz
    return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('PageUp') - AMS::Keyboard.key('PageDown')
    return v.to_f if v != 0
    v = MSPhysics::Simulation.instance.joystick_data['rightz']
    v ? v : 0.0
  end


  # Output from keys D and A or X-axis position on the left joystick.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  def leftx
    return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('d') - AMS::Keyboard.key('a')
    return v.to_f if v != 0
    v = MSPhysics::Simulation.instance.joystick_data['leftx']
    v ? v : 0.0
  end

  # Output from keys W and S or Y-axis position on the left joystick.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  def lefty
    return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('w') - AMS::Keyboard.key('s')
    return v.to_f if v != 0
    v = MSPhysics::Simulation.instance.joystick_data['lefty']
    v ? v : 0.0
  end

  # Output from keys E and Q or position of the joy controller's left trigger.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  def leftz
    return 0.0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('e') - AMS::Keyboard.key('q')
    return v.to_f if v != 0
    v = MSPhysics::Simulation.instance.joystick_data['leftz']
    v ? v : 0.0
  end

  # Output from keys NUMPAD6 and NUMPAD4 or centered-X-axis position on the
  # joy-pad.
  # @return [Integer] -1, 0, or 1.
  def numx
    return 0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('numpad6') - AMS::Keyboard.key('numpad4')
    return v if v != 0
    jpd = MSPhysics::Simulation.instance.joypad_data
    jpd == 2 ? 1 : (jpd == 8 ? -1 : 0)
  end

  # Output from keys NUMPAD8 and NUMPAD5 or centered-Y-axis position on the
  # joy-pad.
  # @return [Integer] -1, 0, or 1.
  def numy
    return 0 if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
    v = AMS::Keyboard.key('numpad8') - AMS::Keyboard.key('numpad5')
    return v if v != 0
    jpd = MSPhysics::Simulation.instance.joypad_data
    jpd == 1 ? 1 : (jpd == 4 ? -1 : 0)
  end

  # @!endgroup
  # @!group Functions

  # Calculate the value of a sine curve at a particular world time.
  # @param [Numeric] frequency Number of times to oscillate per second.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @return [Numeric] A value ranging from -1.0 to 1.0.
  # @example Using in controller
  #   oscillator(0.8) * 4
  def oscillator(frequency, delay = 0.0)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    return 0.0 if rtime < 0
    inc = frequency * 2 * Math::PI
    Math.sin(inc * rtime)
  end

  # Calculate the slope of a sine curve at a particular world time.
  # @param [Numeric] frequency Number of times to oscillate per second.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @return [Numeric] A value ranging from -2πf to 2πf, where f is the
  #   frequency.
  def oscillator_slope(frequency, delay = 0.0)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    return 0.0 if rtime < 0
    inc = frequency * 2 * Math::PI
    inc * Math.cos(inc * rtime)
  end

  # Compute the value of a shifted sine curve at a particular world time.
  # @param [Numeric] frequency Number of times to oscillate per second.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @return [Numeric] A value ranging from 0.0 to 1.0.
  def oscillator2(frequency, delay = 0.0)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    return 0.0 if rtime < 0
    Math.sin(2 * Math::PI * (frequency * rtime - 0.25)) * 0.5 + 0.5
  end

  # Calculate the slope of a shifted sine curve at a particular world time.
  # @param [Numeric] frequency Number of times to oscillate per second.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @return [Numeric] A value ranging from -πf to πf, where f is the
  #   frequency.
  def oscillator2_slope(frequency, delay = 0.0)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    return 0.0 if rtime < 0
    frequency * Math::PI * Math.cos(2 * Math::PI * (frequency * rtime - 0.25))
  end

  # Increment the accumulator by one at a specific rate and offset.
  # @param [Numeric] rate Accumulate every [rate] seconds.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @return [Integer] The accumulator value
  # @example Using in controller
  #   accumulator(0.25, 2)
  def accumulator(rate, delay = 0.0)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    (rtime > 0 && rate > MSPhysics::EPSILON) ? (rtime / rate).to_i : 0
  end

  # Compute a repeater value based on rate, hold, and delay.
  # @param [Numeric] rate Repeat every [rate] seconds.
  # @param [Numeric] hold The time, in seconds, to hold the repeater turned on
  #   whenever it is triggered.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @return [Integer] 1 or 0
  # @example Using in controller
  #   repeater(1, 0.4, 0)
  def repeater(rate, hold, delay = 0.0)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    (rtime > 0 && rate > MSPhysics::EPSILON && rtime % rate < hold) ? 1 : 0
  end

  # Compute a repeater value based on rate and delay that repeats only one
  # time whenever it is triggered.
  # @param [Numeric] rate Repeat every [rate] seconds.
  # @param [Numeric] delay The time to wait, in seconds, before starting.
  # @param [Object] id Unique repeat identifier.
  # @return [Integer] 1 or 0
  # @example Output frame every 0.25 seconds.
  #   onTick {
  #     if singular_repeater(0.25) == 1
  #       puts frame
  #     end
  #   }
  def singular_repeater(rate, delay = 0.0, id = nil)
    rtime = MSPhysics::Simulation.instance.world.time - delay
    if rtime > 0 && rate > MSPhysics::EPSILON
      res = (rtime / rate).to_i
      opts = [rate, delay, id]
      if @_singular_repeater_flags[opts] == res
        return 0
      else
        @_singular_repeater_flags[opts] = res
        return 1
      end
    else
      return 0
    end
  end

  # @!endgroup

  class << self

    # @!visibility private
    def reset_variables
      @@_variables.clear
      @@_toggled.clear
      @@_key_sliders.clear
    end

    # @!visibility private
    def update_key_sliders(timestep)
      return if AMS::IS_PLATFORM_WINDOWS && !AMS::Sketchup.is_main_window_active?
      @@_key_sliders.each { |name, data|
        data[:value] += (AMS::Keyboard.key(data[:key1]) - AMS::Keyboard.key(data[:key2])) * data[:step] * timestep
        if data[:value] < data[:min]
          data[:value] = data[:min]
        elsif data[:value] > data[:max]
          data[:value] = data[:max]
        end
      }
    end

  end # class << self
end # class MSPhysics::CommonContext
