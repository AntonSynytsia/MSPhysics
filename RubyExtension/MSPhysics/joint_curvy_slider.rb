module MSPhysics

  # @since 1.0.0
  class CurvySlider < Joint

    DEFAULT_LINEAR_FRICTION = 0.0
    DEFAULT_ANGULAR_FRICTION = 0.0
    DEFAULT_ALIGNMENT_POWER = 0.0
    DEFAULT_CONTROLLER = 1.0
    DEFAULT_LOOP_ENABLED = false
    DEFAULT_ALIGNMENT_ENABLED = true
    DEFAULT_ROTATION_ENABLED = true

    # Create a CurvySlider joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::CurvySlider.create(@address)
      MSPhysics::Newton::CurvySlider.set_linear_friction(@address, DEFAULT_LINEAR_FRICTION)
      MSPhysics::Newton::CurvySlider.set_angular_friction(@address, DEFAULT_ANGULAR_FRICTION)
      MSPhysics::Newton::CurvySlider.set_alignment_power(@address, DEFAULT_ALIGNMENT_POWER)
      MSPhysics::Newton::CurvySlider.set_controller(@address, DEFAULT_CONTROLLER)
      MSPhysics::Newton::CurvySlider.enable_loop(@address, DEFAULT_LOOP_ENABLED)
      MSPhysics::Newton::CurvySlider.enable_alignment(@address, DEFAULT_ALIGNMENT_ENABLED)
      MSPhysics::Newton::CurvySlider.enable_rotation(@address, DEFAULT_ROTATION_ENABLED)
    end

    # Append point to the curve.
    # @param [Geom::Point3d] position
    # @return [Fixnum] point index.
    def add_point(position)
      MSPhysics::Newton::CurvySlider.add_point(@address, position)
    end

    # Remove point from curve at index.
    # @param [Fixnum] index
    # @return [Boolean] success
    def remove_point(index)
      MSPhysics::Newton::CurvySlider.remove_point(@address, index)
    end

    # Get all points the curve is associated of.
    # @return [Array<Geom::Point3d>]
    def points
      MSPhysics::Newton::CurvySlider.get_points(@address)
    end

    # Get the number of points that make up the curve.
    # @return [Fixnum]
    def points_size
      MSPhysics::Newton::CurvySlider.get_points_size(@address)
    end

    # Remove all points that make up the curve.
    # @return [Fixnum] The number of points removed.
    def clear
      MSPhysics::Newton::CurvySlider.clear_points(@address)
    end

    # Get curve length in meters.
    # @return [Numeric]
    def length
      MSPhysics::Newton::CurvySlider.get_length(@address)
    end

    # Get point position by index.
    # @param [Fixnum] index
    # @return [Geom::Point3d, nil] Point position in global space if the index
    #   references an existing point; +nil+ otherwise.
    def get_point_position(index)
      MSPhysics::Newton::CurvySlider.get_point_position(@address, index)
    end

    # Set point position by index.
    # @param [Fixnum] index
    # @param [Geom::Point3d] position Point position in global space.
    # @return [Boolean] success
    def set_point_position(index, position)
      MSPhysics::Newton::CurvySlider.set_point_position(@address, index, position)
    end

    # Get current position along the curve in meters.
    # @return [Numeric]
    def cur_position
      MSPhysics::Newton::CurvySlider.get_cur_position(@address)
    end

    # Get current velocity along the curve in meters per second.
    # @return [Numeric]
    def cur_velocity
      MSPhysics::Newton::CurvySlider.get_cur_velocity(@address)
    end

    # Get current acceleration along the curve in meters per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::CurvySlider.get_cur_acceleration(@address)
    end

    # Get current point on curve.
    # @return [Geom::Point3d, nil] A point in global space or +nil+ if curve is
    #   empty or joint is disconnected.
    def cur_point
      MSPhysics::Newton::CurvySlider.get_cur_point(@address)
    end

    # Get current vector on curve.
    # @return [Geom::Point3d, nil] A vector in global space or +nil+ if curve is
    #   empty or joint is disconnected.
    def cur_vector
      MSPhysics::Newton::CurvySlider.get_cur_vector(@address)
    end

    # Get current tangent vector on curve.
    # @return [Geom::Point3d, nil] A vector in global space or +nil+ if curve is
    #   empty or joint is disconnected.
    def cur_tangent
      MSPhysics::Newton::CurvySlider.get_cur_tangent(@address)
    end

    # Get linear friction.
    # @return [Numeric]
    def linear_friction
      MSPhysics::Newton::CurvySlider.get_linear_friction(@address)
    end

    # Set linear friction.
    # @param [Numeric] friction A value greater than or equal to zero.
    def linear_friction=(friction)
      MSPhysics::Newton::CurvySlider.set_linear_friction(@address, friction)
    end

    # Get angular friction.
    # @return [Numeric]
    def angular_friction
      MSPhysics::Newton::CurvySlider.get_angular_friction(@address)
    end

    # Set angular friction.
    # @param [Numeric] friction A numeric value greater than or equal to zero.
    def angular_friction=(friction)
      MSPhysics::Newton::CurvySlider.set_angular_friction(@address, friction)
    end

    # Get magnitude of the linear and angular friction.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::CurvySlider.get_controller(@address)
    end

    # Set magnitude of the linear and angular friction.
    # @param [Numeric] value A numeric value greater than or equal to zero.
    def controller=(value)
      MSPhysics::Newton::CurvySlider.set_controller(@address, value)
    end

    # Determine whether curve looping is enabled.
    # @return [Boolean]
    def loop_enabled?
      MSPhysics::Newton::CurvySlider.loop_enabled?(@address)
    end

    # Enable/disable curve looping.
    # @param [Boolean] state
    def loop_enabled=(state)
      MSPhysics::Newton::CurvySlider.enable_loop(@address, state)
    end

    # Determine whether the connected body is supposed to align with the
    # direction of curve.
    # @return [Boolean]
    def alignment_enabled?
      MSPhysics::Newton::CurvySlider.alignment_enabled?(@address)
    end

    # Enable/disable alignment to curve.
    # @param [Boolean] state
    def alignment_enabled=(state)
      MSPhysics::Newton::CurvySlider.enable_alignment(@address, state)
    end

    # Get alignment power.
    # @note Has an effect only if alignment is enabled.
    # @return [Numeric]
    def alignment_power
      MSPhysics::Newton::CurvySlider.get_alignment_power(@address)
    end

    # Set alignment power.
    # @note Has an effect only if alignment is enabled.
    # @param [Numeric] value A value greater than or equal to zero. Pass zero to
    #   use maximum power.
    def alignment_power=(value)
      MSPhysics::Newton::CurvySlider.set_alignment_power(@address, value)
    end

    # Determine whether the rotation along the point on curve is enabled.
    # @return [Boolean]
    def rotation_enabled?
      MSPhysics::Newton::CurvySlider.rotation_enabled?(@address)
    end

    # Enable/disable rotation along the point on curve.
    # @param [Boolean] state
    def rotation_enabled=(state)
      MSPhysics::Newton::CurvySlider.enable_rotation(@address, state)
    end

    # Get point and direction on curve by linear position on curve.
    # @param [Numeric] pos Linear position on curve in meters.
    # @return [Array<(Geom::Point3d, Geom::Vector3d)>, nil] A array containing
    #   point and direction on curve or +nil+ if curve is empty.
    def info_by_pos(pos)
      MSPhysics::Newton::CurvySlider.get_info_by_pos(@address, pos)
    end

  end # class CurvySlider < Joint
end # module MSPhysics
