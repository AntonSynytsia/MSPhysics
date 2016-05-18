module MSPhysics

  # @since 1.0.0
  class Servo < Joint

    DEFAULT_MIN = -180.0.degrees
    DEFAULT_MAX = 180.0.degrees
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_STRENGTH = 0.0
    DEFAULT_REDUCTION_RATIO = 0.1
    DEFAULT_CONTROLLER = nil
    DEFAULT_SP_MODE_ENABLED = false

    # Create a servo joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, 6, group)
      MSPhysics::Newton::Servo.create(@address)
      MSPhysics::Newton::Servo.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Servo.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Servo.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Servo.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::Servo.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Servo.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::Servo.set_reduction_ratio(@address, DEFAULT_REDUCTION_RATIO)
      MSPhysics::Newton::Servo.enable_sp_mode(@address, DEFAULT_SP_MODE_ENABLED)
      MSPhysics::Newton::Servo.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get current angle in radians.
    # @return [Numeric]
    def cur_angle
      MSPhysics::Newton::Servo.get_cur_angle(@address)
    end

    # Get current omega in radians per second.
    # @return [Numeric]
    def cur_omega
      MSPhysics::Newton::Servo.get_cur_omega(@address)
    end

    # Get current acceleration in radians per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Servo.get_cur_acceleration(@address)
    end

    # Get minimum angle in radians.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Servo.get_min(@address)
    end

    # Set minimum angle in radians.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Servo.set_min(@address, value)
    end

    # Get maximum angle in radians.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Servo.get_max(@address)
    end

    # Set maximum angle in radians.
    # @param [Numeric] value
    def max=(value)
      MSPhysics::Newton::Servo.set_max(@address, value)
    end

    # Determine whether min & max angle limits are enabled.
    # @note This option has no effect if SP mode is enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Servo.limits_enabled?(@address)
    end

    # Enable/disable min & max angle limits.
    # @note This option has no effect if SP mode is enabled.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Servo.enable_limits(@address, state)
    end

    # Get angular acceleration in radians per second per second.
    # @note If SP mode is disabled, the maximum angular rate in radians per
    #   second is <tt>accel / damp</tt>.
    # @return [Numeric]
    def accel
      MSPhysics::Newton::Servo.get_accel(@address)
    end

    # Set angular acceleration in radians per second per second.
    # @note If SP mode is disabled, the maximum angular rate in radians per
    #   second is <tt>accel / damp</tt>.
    # @param [Numeric] value A value greater than or equal to zero.
    def accel=(value)
      MSPhysics::Newton::Servo.set_accel(@address, value)
    end

    # Get angular damper.
    # @note Higher damper makes rotation stronger.
    # @note If SP mode is disabled, the maximum angular rate in radians per
    #   second is <tt>accel / damp</tt>.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::Servo.get_damp(@address)
    end

    # Set angular damper.
    # @note Higher damper makes rotation stronger.
    # @note If SP mode is disabled, the maximum angular rate in radians per
    #   second is <tt>accel / damp</tt>.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::Servo.set_damp(@address, value)
    end

    # Get angular strength.
    # @return [Numeric]
    def strength
      MSPhysics::Newton::Servo.get_strength(@address)
    end

    # Set angular strength.
    # @note A strength value of 0.0 represents maximum strength.
    # @param [Numeric] value A value greater than or equal to zero.
    def strength=(value)
      MSPhysics::Newton::Servo.set_strength(@address, value)
    end

    # Get angular reduction ratio.
    # @note The reduction ratio is a feature that reduces angular rate when
    #   servo angle nears its desired angle. A value of 0.0 will disable the
    #   angular reduction. A value of greater than zero will enable angular
    #   reduction. Angular reduction starts acting upon the angular rate when
    #   the difference between the current angle and the desired angle is less
    #   than or equal to <tt>accel * reduction_ratio / damp</tt> radians.
    # @note This option has no effect if SP mode is enabled.
    # @return [Numeric] A value between 0.0 and 1.0.
    def reduction_ratio
      MSPhysics::Newton::Servo.get_reduction_ratio(@address)
    end

    # Set angular reduction ratio.
    # @note The reduction ratio is a feature that reduces angular rate when
    #   servo angle nears its desired angle. A value of 0.0 will disable the
    #   angular reduction. A value of greater than zero will enable angular
    #   reduction. Angular reduction starts acting upon the angular rate when
    #   the difference between the current angle and the desired angle is less
    #   than or equal to <tt>accel * reduction_ratio / damp</tt> radians.
    # @note This option has no effect if SP mode is enabled.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def reduction_ratio=(value)
      MSPhysics::Newton::Servo.set_reduction_ratio(@address, value)
    end

    # Determine whether SketchyPhysics mode is enabled.
    # @note In SketchyPhysics mode, the controller parameter is used to control
    #   the ratio between the min and max angle range, as well as, the servo is
    #   stronger and usually rotates faster to its desired destination angle.
    # @return [Boolean]
    def sp_mode_enabled?
      MSPhysics::Newton::Servo.sp_mode_enabled?(@address)
    end

    # Enable/disable SketchyPhysics mode.
    # @note In SketchyPhysics mode, the controller parameter is used to control
    #   the ratio between the min and max angle range, as well as, the servo is
    #   stronger and usually rotates faster to its desired destination angle.
    # @param [Boolean] state
    def sp_mode_enabled=(state)
      MSPhysics::Newton::Servo.enable_sp_mode(@address, state)
    end

    # Get servo controller, a desired angle in radians or ratio between the min
    # and max angle range, depending on the mode. Nil is returned if the servo
    # is turned off.
    # @return [Numeric, nil]
    def controller
      MSPhysics::Newton::Servo.get_controller(@address)
    end

    # Set servo controller, a desired angle in radians or ratio between the min
    # and max angle range, depending on the mode. Pass nil to turn off the
    # servo.
    # @param [Numeric, nil] value
    def controller=(value)
      MSPhysics::Newton::Servo.set_controller(@address, value)
    end

  end # class Servo < Joint
end # module MSPhysics
