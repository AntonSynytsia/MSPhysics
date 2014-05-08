module MSPhysics
  class Joint

    PIN_LENGTH = 50
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

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point global space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
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
        on_disconnect
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
      connect(child)
    end

    private

    def submit_constraints(timestep)
    end

    def get_info(info_ptr)
    end

    def on_connect
    end

    def on_disconnect
    end

    def check_validity
      @parent = nil if @parent and @parent.invalid?
      @child = nil if @child and @child.invalid?
    end

    public

    # @return [Body, NilClass]
    def parent
      check_validity
      @parent
    end

    # @return [Body, NilClass]
    def child
      check_validity
      @child
    end

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
      check_validity
      body = args.size.zero? ? @child : args[0]
      return false if body.nil?
      return false if connected? and body == @child
      unless body.is_a?(Body)
        raise ArgumentError, "Expected Body, but got #{body.class}."
      end
      raise 'The body is invalid!' if body.invalid?
      disconnect
      @child = body
      world_ptr = Newton.bodyGetWorld(@child._body_ptr)
      parent_ptr = @parent ? @parent._body_ptr : nil
      # Update position
      pos = @pos.clone
      dir = @dir.clone
      if @parent
        tra = @parent.get_matrix(0)
        pos.transform!(tra)
        dir.transform!(tra)
      end
      jnt_matrix = Geom::Transformation.new(pos, dir)
      @local_matrix0 = @child.get_matrix(0).inverse*jnt_matrix
      @local_matrix1 = @parent ? @parent.get_matrix(0).inverse*jnt_matrix : jnt_matrix
      # Create constraint
      @joint_ptr = Newton.constraintCreateUserJoint(world_ptr, @dof, @submit_constraints, @get_info, @child._body_ptr, parent_ptr)
      Newton.jointSetDestructor(@joint_ptr, @destructor_callback)
      on_connect
      true
    end

    # Disconnect connected body from the joint.
    # @return [Boolean] +true+ if successful.
    def disconnect
      check_validity
      return false unless connected?
      world_ptr = Newton.bodyGetWorld(@child._body_ptr)
      Newton.destroyJoint(world_ptr, @joint_ptr)
      @joint_ptr = nil
      true
    end

    # Determine whether a child body is connected to the joint.
    # @return [Boolean]
    def connected?
      check_validity
      @joint_ptr ? true : false
    end

    # Get joint position in global space.
    # @return [Geom::Point3d]
    def position
      check_validity
      pos =  @parent ? @pos.transform(@parent.get_matrix(0)) : @pos
      Conversion.convert_point(pos, :m, :in)
    end

    # Get joint axis of rotation vector in global space.
    # @return [Geom::Vector3d]
    def direction
      check_validity
      if @parent
        @dir.transform(@parent.get_matrix(0)).normalize
      else
        @dir.clone
      end
    end

  end # class Joint
end # module MSPhysics

# Load joints
dir = File.dirname(__FILE__)
files = Dir.glob(File.join(dir, 'joints', '*.{rb, rbs}'))
files.each{ |file| require file }
