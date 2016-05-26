module MSPhysics

  # A Controller class is used by joint controllers.
  # @since 1.0.0
  class Controller < Common

    include Math

    # Output from LEFT and RIGHT arrow keys or x-axis position on the right
    # joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def rightx
      v = AMS::Keyboard.key('right') - AMS::Keyboard.key('left')
      return v.to_f if v != 0
      v2 = MSPhysics::Simulation.instance.joystick_data['rightx']
      v2 ? v2 : 0.0
    end

    # Output from UP and DOWN arrow keys or y-axis position on the right
    # joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def righty
      v = AMS::Keyboard.key('up') - AMS::Keyboard.key('down')
      return v.to_f if v != 0
      v2 = MSPhysics::Simulation.instance.joystick_data['righty']
      v2 ? v2 : 0.0
    end

    # Output from keys D and A or x-axis position on the left joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def leftx
      v = AMS::Keyboard.key('d') - AMS::Keyboard.key('a')
      return v.to_f if v != 0
      v2 = MSPhysics::Simulation.instance.joystick_data['leftx']
      v2 ? v2 : 0.0
    end

    # Output from keys W and S or y-axis position on the left joy-stick.
    # @return [Numeric] A value ranging from -1.0 to 1.0.
    def lefty
      v = AMS::Keyboard.key('w') - AMS::Keyboard.key('s')
      return v.to_f if v != 0
      v2 = MSPhysics::Simulation.instance.joystick_data['lefty']
      v2 ? v2 : 0.0
    end

    # Output from keys NUMPAD6 and NUMPAD4 or centered-x-axis position on the
    # joy-pad.
    # @return [Fixnum] -1, 0, or 1.
    def numx
      v = AMS::Keyboard.key('numpad6') - AMS::Keyboard.key('numpad4')
      return v if v != 0
      jpd = MSPhysics::Simulation.instance.joypad_data
      jpd == 2 ? 1 : (jpd == 8 ? -1 : 0)
    end

    # Output from keys NUMPAD8 and NUMPAD5 or centered-y-axis position on the
    # joy-pad.
    # @return [Fixnum] -1, 0, or 1.
    def numy
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

  end # class Controller
end # module MSPhysics
