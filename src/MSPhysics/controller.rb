module MSPhysics

  # A Controller class is used by joint controllers.
  # @since 1.0.0
  class Controller < Common

    include Math

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

    # Get oscillated value of a sine curve.
    # @param [Numeric] rate
    # @return [Numeric] A value between -1.0 and 1.0.
    def oscillator(rate)
      inc = (2*Math::PI)/rate
      Math.sin(inc*frame)
    end

  end # class Controller
end # module MSPhysics
