module MSPhysics
  class Joint

    TYPES = [
      :hinge,
      :servo,
      :motor,
      :spring,
      :slider,
      :piston,
      :up,
      :fixed,
      :ball,
      :universal,
      :corkscrew
    ]

    class << self

      # Optimize joint name.
      # @param [Symbol, String] name Joint name.
      # @return [Symbol, NilClass] Proper name if successful.
      def optimize_joint_name(name)
        name = name.to_s.downcase.gsub(/\s|_/, '')
        JOINT_TYPES.each { |type|
          return type if type.to_s.gsub(/_/, '') == name
        }
        nil
      end

    end # proxy class

    # @param [Array<Numeric>, Geom::Point3d] pos Origin of hinge in global
    #   space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent
    # @param [Body, NilClass] child
    # @param [Numeric] dof Degrees of freedom
    def initialize(pos, pin_dir, parent, child, dof = 0)
      if parent
        unless parent.is_a?(Body)
          raise ArgumentError, "Expected Body, but got #{parent.class}."
        end
        raise 'The parent body is invalid!' unless parent.valid?
        @parent = parent
      else
        @parent = nil
      end
      @child = nil
      @dof = dof.abs # degrees of freedom
      @joint_ptr = nil
      @destructor_callback = Proc.new { |joint_ptr|
        @joint_ptr = nil
      }
      @submit_constraints = Proc.new { |joint_ptr, timestep, thread_index|
        submit_constraints(timestep)
      }
      @get_info = Proc.new { |joint_ptr, info_ptr|
        get_info(info_ptr)
      }
      @pos = Conversion.convert_point(pos, :in, :m)
      @dir = Geom::Vector3d.new(pin_dir).normalize
      if parent
        tra = @parent.get_matrix(0).inverse
        @pos.transform!(tra)
        @dir.transform!(tra)
      end
      @local_matrix0 = nil
      @local_matrix1 = nil
      @angle = 0
      connect(child)
    end

    # @!attribute [r] parent
    #   @return [Body, NilClass]

    # @!attribute [r] child
    #   @return [Body, NilClass]


    attr_reader :parent, :child

    private

    def submit_constraints(timestep)
    end

    def get_info(info_ptr)
    end

    def calc_angle(new_cos_angle, new_sin_angle)
      sin_angle = Math.sin(@angle)
      cos_angle = Math.cos(@angle)
      sin_da = new_sin_angle * cos_angle - new_cos_angle * sin_angle
      cos_da = new_cos_angle * cos_angle + new_sin_angle * sin_angle
      @angle += Math.atan2(sin_da, cos_da)-Math::PI/2
    end

    def update_pos
      pos = @pos.clone
      dir = @dir.clone
      if @parent
        tra = @parent.get_matrix(0)
        pos.transform!(tra)
        dir.transform!(tra)
      end
      jnt_matrix = Geom::Transformation.new(pos, dir)
      @local_matrix0 = jnt_matrix*@child.get_matrix(0).inverse
      @local_matrix1 = @parent ? jnt_matrix*@parent.get_matrix(0).inverse : jnt_matrix
    end

    public

    # Connect body to the joint.
    # @note Each joint can have one child body only.
    # @note When this method is called the originally connected body is
    #   disconnected first, and then the new body is connected.
    # @overload connect
    #   Connect joint to the last connected body.
    # @overload connect(body)
    #   Connect joint to a new body.
    #   @param [Body] body
    # @return [Boolean] +true+ if successful.
    def connect(*args)
      body = args.size.zero? ? @child : args[0]
      return false unless body
      unless body.is_a?(Body)
        raise ArgumentError, "Expected Body, but got #{body.class}."
      end
      raise 'The body is invalid!' unless body.valid?
      disconnect
      @child = body
      world_ptr = Newton.bodyGetWorld(@child._body_ptr)
      parent_ptr = @parent ? @parent._body_ptr : nil
      update_pos
      @joint_ptr = Newton.constraintCreateUserJoint(world_ptr, @dof, @submit_constraints, @get_info, @child._body_ptr, parent_ptr)
      Newton.jointSetDestructor(@joint_ptr, @destructor_callback)
      true
    end

    # Disconnect connected body from the joint.
    # @return [Boolean] +true+ if successful.
    def disconnect
      return false unless connected?
      world_ptr = Newton.bodyGetWorld(@child._body_ptr)
      Newton.destroyJoint(world_ptr, @joint_ptr)
      @joint_ptr = nil
      true
    end

    # Determine whether a child body is connected to the joint.
    # @return [Boolean]
    def connected?
      @joint_ptr ? true : false
    end

    # Get joint position in global space.
    # @return [Geom::Vector3d]
    def get_pos
      pos =  @parent ? @pos.transform(@parent.get_matrix(0)) : @pos
      Conversion.convert_point(pos, :m, :in)
    end

    # Get joint axis of rotation vector in global space.
    # @return [Geom::Vector3d]
    def get_dir
      if @parent
        @dir.transform(@parent.get_matrix(0)).normalize
      else
        @dir.clone
      end
    end

    # Set joint position in global space.
    # @param [Geom::Vector3d, Array<Numeric>] pos
    def set_pos(pos)
      @pos = Conversion.convert_point(pos, :in, :m)
      @pos.transform!(@parent.get_matrix(0).inverse) if @parent
      update_pos
    end

    # Set joint axis of rotation vector in global space.
    # @param [Geom::Vector3d, Array<Numeric>] dir
    def set_dir(dir)
      @dir = Geom::Vector3d.new(dir)
      @dir.transform!(@parent.get_matrix(0).inverse) if @parent
      update_pos
    end

  end # class Joint
end # module MSPhysics
