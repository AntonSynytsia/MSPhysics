module MSPhysics

  # An abstract for all joints.
  # @since 1.0.0
  class Joint < Entity

    DEFAULT_SOLVER_MODEL = 2
    DEFAULT_STIFFNESS = 1.00
    DEFAULT_BODIES_COLLIDABLE = false
    DEFAULT_BREAKING_FORCE = 0.0

    class << self

      # Verify that joint is valid.
      # @api private
      # @param [Joint] joint
      # @param [World, nil] world A world the joint ought to belong to or +nil+.
      # @raise [TypeError] if joint is invalid or destroyed.
      # @return [void]
      def validate(joint, world = nil)
        AMS.validate_type(joint, MSPhysics::Joint)
        unless joint.valid?
          raise(TypeError, "Joint #{joint} is invalid/destroyed!", caller)
        end
        if world != nil
          AMS.validate_type(world, MSPhysics::World)
          if joint.world.address != world.address
            raise(TypeError, "Joint #{joint} belongs to a different world!", caller)
          end
        end
      end

      # Get joint by address.
      # @param [Integer] address
      # @return [Joint, nil] A Joint object if successful.
      # @raise [TypeError] if the address is invalid.
      def joint_by_address(address)
        data = MSPhysics::Newton::Joint.get_user_data(address.to_i)
        data.is_a?(MSPhysics::Joint) ? data : nil
      end

      # Get all joints.
      # @note Joints that do not have a {Joint} instance are not included in the
      #   array.
      # @return [Array<Joint>]
      def all_joints
        MSPhysics::Newton.get_all_joints() { |ptr, data| data.is_a?(MSPhysics::Joint) ? data : nil }
      end

    end # class << self

    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group_inst
    def initialize(world, parent, pin_tra, group_inst = nil)
      if self.class == MSPhysics::Joint
        raise(TypeError, "Creating an instance of the Joint abstract class is not allowed!", caller)
      end
      MSPhysics::World.validate(world)
      MSPhysics::Body.validate(parent, world) if parent
      parent_address = parent ? parent.address : nil
      @address = MSPhysics::Newton::Joint.create(world.address, parent_address, pin_tra, group_inst)
      MSPhysics::Newton::Joint.set_user_data(@address, self)
      MSPhysics::Newton::Joint.set_stiffness(@address, DEFAULT_STIFFNESS)
      MSPhysics::Newton::Joint.set_bodies_collidable(@address, DEFAULT_BODIES_COLLIDABLE)
      MSPhysics::Newton::Joint.set_breaking_force(@address, DEFAULT_BREAKING_FORCE)
      MSPhysics::Newton::Joint.set_solver_model(@address, DEFAULT_SOLVER_MODEL)
      @name = ''
    end

    # Determine whether joint is valid.
    # @return [Boolean]
    def valid?
      MSPhysics::Newton::Joint.is_valid?(@address)
    end

    # Get pointer the joint.
    # @return [Integer]
    def address
      @address
    end

    # Get group/component associated with the joint.
    # @return [Sketchup::Group, Sketchup::ComponentInstance, nil]
    def group
      MSPhysics::Newton::Joint.get_group(@address)
    end

    # Get the world the joint is associated to.
    # @return [MSPhysics::World]
    def world
      world_address = MSPhysics::Newton::Joint.get_world(@address)
      MSPhysics::Newton::World.get_user_data(world_address)
    end

    # Destroy joint.
    # @return [void]
    def destroy
      MSPhysics::Newton::Joint.destroy(@address)
    end

    # Connect joint to its desired child body.
    # @param [MSPhysics::Body] child
    # @return [Boolean] success
    def connect(child)
      MSPhysics::Body.validate(child, self.world)
      MSPhysics::Newton::Joint.connect(@address, child.address)
    end

    # Disconnect joint from its child body.
    # @return [Boolean] success
    def disconnect
      MSPhysics::Newton::Joint.disconnect(@address)
    end

    # Determine whether joint is connected to its child body.
    # @return [Boolean]
    def connected?
      MSPhysics::Newton::Joint.is_connected?(@address)
    end

    # Get joint maximum degrees of freedom.
    # @return [Integer]
    def dof
      MSPhysics::Newton::Joint.get_dof(@address)
    end

    # Get joint type.
    # @return [Integer]
    # @see MSPhysics::JOINT_ID_TO_NAME
    def type
      MSPhysics::Newton::Joint.get_type(@address)
    end

    # Get joint parent body.
    # @return [MSPhysics::Body, nil]
    def parent
      address = MSPhysics::Newton::Joint.get_parent(@address)
      address ? MSPhysics::Body.body_by_address(address) : nil
    end

    # Get joint child body.
    # @return [MSPhysics::Body, nil]
    def child
      address = MSPhysics::Newton::Joint.get_child(@address)
      address ? MSPhysics::Body.body_by_address(address) : nil
    end

    # Get joint pin transformation in global space.
    # @return [Geom::Transformation]
    def get_pin_matrix
      MSPhysics::Newton::Joint.get_pin_matrix(@address)
    end

    # Set joint pin transformation in global space.
    # @param [Geom::Transformation, Array<Numeric>] matrix
    # @return [nil]
    def set_pin_matrix(matrix)
      MSPhysics::Newton::Joint.set_pin_matrix(@address, matrix)
    end

    # Get joint pin transformation in global space.
    # @param [Integer] mode
    #  * Pass 0 to obtain aligned pin matrix in global space with respect to the
    #    child body.
    #  * Pass 1 to obtain aligned pin matrix in global space with respect to the
    #    parent body.
    #  * Pass 2 to obtain non-aligned pin matrix in global space with respect to
    #    the parent body.
    # @return [Geom::Transformation, nil]
    def get_pin_matrix2(mode)
      MSPhysics::Newton::Joint.get_pin_matrix2(@address, mode)
    end

    # Determine whether parent body is collidable with its child body.
    # @return [Boolean]
    def bodies_collidable?
      MSPhysics::Newton::Joint.bodies_collidable?(@address)
    end

    # Set parent body collidable/noncollidable with its child body.
    # @param [Boolean] state
    def bodies_collidable=(state)
      MSPhysics::Newton::Joint.set_bodies_collidable(@address, state)
    end

    # Get joint stiffness.
    # @return [Numeric] A value between 0.0 (soft) and 1.0 (stiff).
    def stiffness
      MSPhysics::Newton::Joint.get_stiffness(@address)
    end

    # Set joint stiffness
    # @param [Numeric] value A value between 0.0 (soft) and 1.0 (stiff).
    def stiffness=(value)
      MSPhysics::Newton::Joint.set_stiffness(@address, value)
    end

    # Get joint breaking force in Newtons.
    # @return [Numeric]
    def breaking_force
      MSPhysics::Newton::Joint.get_breaking_force(@address)
    end

    # Set joint breaking force in Newtons.
    # @param [Numeric] force
    def breaking_force=(force)
      MSPhysics::Newton::Joint.set_breaking_force(@address, force)
    end

    # Get solver model for calculating the constraint forces.
    # @return [Integer] Model:
    #   * 0 - Use best algorithm.
    #   * 1 - Signal the engine that two joints form a kinematic loop.
    #   * 2 - Use less accurate algorithm.
    def solver_model
      MSPhysics::Newton::Joint.get_solver_model(@address)
    end

    # Set solver model for calculating the constraint forces.
    # @param [Integer] model Solver model:
    #   * 0 - Use best algorithm.
    #   * 1 - Signal the engine that two joints form a kinematic loop.
    #   * 2 - Use less accurate algorithm.
    def solver_model=(model)
      MSPhysics::Newton::Joint.set_solver_model(@address, model)
    end

    # Get primary tension force on a joint, which is usually the linear
    # tension, in Newtons, in global space.
    # @return [Geom::Vecotor3d]
    # @since 1.0.3
    def get_tension1
      MSPhysics::Newton::Joint.get_tension1(@address)
    end

    # Get secondary tension force on a joint, which is usually the angular
    # tension, in Newton-meters, in global space.
    # @return [Geom::Vecotor3d]
    # @since 1.0.3
    def get_tension2
      MSPhysics::Newton::Joint.get_tension2(@address)
    end

    # Get joint name.
    # @return [String]
    def name
      @name.dup
    end

    # Set joint name
    # @param [String] value
    def name=(value)
      @name = value.to_s.dup
    end

  end # class Joint
end # module MSPhysics
