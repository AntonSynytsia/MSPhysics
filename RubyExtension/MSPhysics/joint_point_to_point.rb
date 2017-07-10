module MSPhysics

  # @since 1.0.0
  class PointToPoint < Joint

    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 0.1
    DEFAULT_STRENGTH = 0.80
    DEFAULT_MODE = 0
    DEFAULT_START_DISTANCE = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a PointToPoint joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::PointToPoint.create(@address)
      MSPhysics::Newton::PointToPoint.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::PointToPoint.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::PointToPoint.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::PointToPoint.set_mode(@address, DEFAULT_MODE)
      MSPhysics::Newton::PointToPoint.set_start_distance(@address, DEFAULT_START_DISTANCE)
      MSPhysics::Newton::PointToPoint.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get spring oscillation acceleration factor.
    # @return [Numeric] A value greater than or equal to zero.
    def accel
      MSPhysics::Newton::PointToPoint.get_accel(@address)
    end

    # Set spring oscillation acceleration factor.
    # @param [Numeric] value Spring accel, a value greater than or equal to
    #   zero.
    def accel=(value)
      MSPhysics::Newton::PointToPoint.set_accel(@address, value)
    end

    # Get spring oscillation drag coefficient.
    # @return [Numeric] A value between 0.0 and 1.0.
    def damp
      MSPhysics::Newton::PointToPoint.get_damp(@address)
    end

    # Set spring oscillation drag coefficient.
    # @param [Numeric] value Spring damp, a value between 0.0 and 1.0.
    def damp=(value)
      MSPhysics::Newton::PointToPoint.set_damp(@address, value)
    end

    # Get spring strength coefficient.
    # @return [Numeric] A value between 0.0 and 1.0.
    def strength
      MSPhysics::Newton::PointToPoint.get_strength(@address)
    end

    # Set spring strength coefficient.
    # @param [Numeric] value Spring strength, a value between 0.0 and 1.0.
    def strength=(value)
      MSPhysics::Newton::PointToPoint.set_strength(@address, value)
    end

    # Get mode.
    # @return [Fixnum]
    #   * 0 - preserve distance.
    #   * 1 - preserve distance and direction.
    def mode
      MSPhysics::Newton::PointToPoint.get_mode(@address)
    end

    # Set mode.
    # @param [Fixnum] value
    #   * 0 - preserve distance.
    #   * 1 - preserve distance and direction.
    def mode=(value)
      MSPhysics::Newton::PointToPoint.set_mode(@address, value)
    end

    # Get starting distance in meters.
    # @note The actual, desired distance is,
    #   <tt>start_distance * controller</tt>.
    # @return [Numeric]
    def start_distance
      MSPhysics::Newton::PointToPoint.get_start_distance(@address)
    end

    # Set starting distance in meters.
    # @note The actual, desired distance is,
    #   <tt>start_distance * controller</tt>.
    # @param [Numeric] value
    def start_distance=(value)
      MSPhysics::Newton::PointToPoint.set_start_distance(@address, value)
    end

    # Get magnitude of the starting distance.
    # @note The actual, desired distance is,
    #   <tt>start_distance * controller</tt>.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::PointToPoint.get_controller(@address)
    end

    # Set magnitude of the starting distance.
    # @note The actual, desired distance is,
    #   <tt>start_distance * controller</tt>.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::PointToPoint.set_controller(@address, value)
    end

    # Get current distance in meters with respect to the starting position.
    # @return [Numeric]
    def cur_distance
      MSPhysics::Newton::PointToPoint.get_cur_distance(@address)
    end

  end # class PointToPoint < Joint
end # module MSPhysics
