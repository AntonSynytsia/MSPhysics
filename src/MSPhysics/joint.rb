module MSPhysics
  class Joint

    # @!visibility private
    @@joints = []
    # @!visibility private
    PIN_LENGTH = 1
    # @!visibility private
    TYPES = [
      :hinge,
      :motor,
      :servo,
      :slider,
      :piston,
      :up_vector,
      :spring,
      :corkscrew,
      :ball_and_socket,
      :universal,
      :fixed
    ]

    class << self

      # Optimize joint name.
      # @param [Symbol, String] name Joint name.
      # @return [Symbol, NilClass] Proper name if successful.
      def optimize_joint_name(name)
        name = name.to_s.downcase.gsub(/\s|_/, '')
        TYPES.each { |type|
          return type if type.to_s.gsub(/_/, '') == name
        }
        nil
      end

      # Destroy all joints.
      def destroy_all
        @@joints.each { |joint|
          joint.disconnect
        }
        @@joints.clear
      end

      # Get joints created within the body.
      # @param [Body] body
      # @return [Array<Joint>]
      def get_joints(body)
        list = []
        @@joints.each { |joint|
          list << joint if joint.parent == body
        }
        list
      end

      # Get joints the body is connected to.
      # @param [Body] body
      # @return [Array<Joint>]
      def get_connected_joints(body)
        list = []
        @@joints.each { |joint|
          list << joint if joint.child == body and joint.connected?
        }
        list
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
    # @param [Boolean] create Whether to create constraint.
    def initialize(pos, pin_dir, parent, child, dof = 0, create = true)
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
      @connect_proc = Proc.new {
        world_ptr = Newton.bodyGetWorld(@child._body_ptr)
        parent_ptr = @parent ? @parent._body_ptr : nil
        @joint_ptr = Newton.constraintCreateUserJoint(world_ptr, @dof, @submit_constraints, @get_info, @child._body_ptr, parent_ptr)
      }
      @destructor_callback = Proc.new { |joint_ptr|
        @joint_ptr = nil
        on_disconnect
      }
      @submit_constraints = Proc.new { |joint_ptr, timestep, thread_index|
        @child.set_sleep_state(false)
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
      @collidable = true
      @solver = 0
      @max_contact_joints = 100
      @stiffness = 0.9
      connect(child) if create
    end

    # @!attribute [r] joint_ptr
    #   @return [AMS::FFI::Pointer, NilClass]

    # @!attribute [r] solver
    #   @return [Fixnum] +0+ : exact, +1+ : interactive. Exact solver makes
    #     joint a lot stronger, but a little slower in performance.

    # @!attribute [r] max_contact_joints
    #   @return [Fixnum]

    # @!attribute [r] stiffness
    #   @return [Numeric]

    attr_reader :joint_ptr, :solver, :max_contact_joints, :stiffness

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

    def update_pos
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
      # Update position
      update_pos
      # Create constraint
      @connect_proc.call
      Newton.jointSetDestructor(@joint_ptr, @destructor_callback)
      @@joints << self unless @@joints.include?(self)
      self.bodies_collidable = @collidable
      self.solver = @solver
      self.max_contact_joints = @max_contact_joints
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
    # @param [Boolean] convert whether to convert units from meters to inches.
    # @return [Geom::Point3d]
    def get_position(convert = true)
      check_validity
      pos =  @parent ? @pos.transform(@parent.get_matrix(0)) : @pos
      return pos.clone unless convert
      Conversion.convert_point(pos, :m, :in)
    end

    # Set joint position in global space.
    # @param [Array<Numeric>, Geom::Point3d] pos
    def set_position(pos)
      check_validity
      pos = MSPhysics::Conversion.convert_point(pos, :in, :m)
      @pos = @parent ? pos.transform(@parent.get_matrix(0).inverse) : pos
      update_pos
    end

    # Get joint axis of rotation vector in global space.
    # @return [Geom::Vector3d]
    def get_direction
      check_validity
      if @parent
        @dir.transform(@parent.get_matrix(0)).normalize
      else
        @dir.clone
      end
    end

    # Set joint pin direction in global space.
    # @param [Array<Numeric>, Geom::Vector3d] dir
    def set_direction(dir)
      check_validity
      dir = Geom::Vector3d.new(dir.to_a)
      @dir = @parent ? dir.transform(@parent.get_matrix(0).inverse) : dir
      update_pos
    end

    # Modify parent and child body collision state.
    # @param [Boolean] state
    def bodies_collidable=(state)
      check_validity
      @collidable = state ? true : false
      Newton.jointSetCollisionState(@joint_ptr, @collidable ? 1 : 0) if connected?
    end

    # Determine whether the parent and child bodies are collidable with each
    # other.
    # @return [Boolean]
    def bodies_collidable?
      check_validity
      @collidable
    end

    # Set joint solver mode.
    # @note Exact solver makes joint a lot stronger, but a little slower in
    #   performance.
    # @param [Fixnum] mode +0+ : exact, +1+ : interactive.
    def solver=(mode)
      check_validity
      @solver = mode.zero? ? 0 : 1
      Newton.userJointSetSolver(@joint_ptr, @solver, @max_contact_joints) if connected?
    end

    # Set joint maximum number of contact joints.
    # @param [Fixnum] count
    def max_contact_joints=(count)
      check_validity
      @max_contact_joints = count.to_i.abs
      Newton.userJointSetSolver(@joint_ptr, @solver, @max_contact_joints) if connected?
    end

    # Set joint stiffness; set the maximum percentage of the constraint force
    # that will be applied to the constraint.
    # @note Ideally the value should be 1.0 (100% stiff), but dues to numerical
    #   integration error this could be the joint a little unstable, and lower
    #   values are preferred.
    # @param [Numeric] stiff A numeric value between 0.0 and 1.0.
    def stiffness=(stiff)
      @stiffness = MSPhysics.clamp(stiff, 0.0, 1.0)
    end

    # Disconnect and remove joint from the joints pointer.
    def destroy
      disconnect
      @@joints.delete(self)
    end

  end # class Joint
end # module MSPhysics

# Load joints
dir = File.dirname(__FILE__)
files = Dir.glob(File.join(dir, 'joints', '*.{rb, rbs}'))
files.each { |file| require file }
