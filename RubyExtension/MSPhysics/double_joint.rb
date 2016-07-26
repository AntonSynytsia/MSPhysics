module MSPhysics

  # An abstract for all double joints.
  # @since 1.0.0
  class DoubleJoint

    class << self

      # Get double joint by address.
      # @param [Fixnum] address
      # @return [DoubleJoint, nil] A DoubleJoint object if successful.
      # @raise [TypeError] if the address is invalid.
      def double_joint_by_address(address)
        data = MSPhysics::Newton::DoubleJoint.get_user_data(address.to_i)
        data.is_a?(MSPhysics::DoubleJoint) ? data : nil
      end

      # Get all double joints.
      # @note Double Joints that do not have a {DoubleJoint} instance are not
      #   included in the array.
      # @return [Array<DoubleJoint>]
      def all_double_joints
        MSPhysics::Newton.get_all_double_joints() { |ptr, data| data.is_a?(MSPhysics::DoubleJoint) ? data : nil }
      end

    end # class << self

    # @param [MSPhysics::World] world
    # @param [Geom::Transformation, Array<Numeric>] pin1_tra Pin1 transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [MSPhysics::Body, nil] parent1
    # @param [Geom::Transformation, Array<Numeric>] pin2_tra Pin2 transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [MSPhysics::Body, nil] parent2
    def initialize(world, pin1_tra, parent1, pin2_tra, parent2)
      if self.class == MSPhysics::DoubleJoint
        raise(TypeError, "Creating an instance of the DoubleJoint abstract class is not allowed!", caller)
      end
      MSPhysics::World.validate(world)
      MSPhysics::Body.validate(parent1, world) if parent1
      MSPhysics::Body.validate(parent2, world) if parent2
      parent1_address = parent1 ? parent1.address : nil
      parent2_address = parent2 ? parent2.address : nil
      @address = MSPhysics::Newton::DoubleJoint.create(world.address, pin1_tra, parent1_address, pin2_tra, parent2_address)
      MSPhysics::Newton::DoubleJoint.set_user_data(@address, self)
      MSPhysics::Newton::DoubleJoint.set_stiffness(@address, Joint::DEFAULT_STIFFNESS)
      MSPhysics::Newton::DoubleJoint.set_bodies_collidable(@address, Joint::DEFAULT_BODIES_COLLIDABLE)
    end

    # Determine whether joint is valid.
    # @return [Boolean]
    def valid?
      MSPhysics::Newton::DoubleJoint.is_valid?(@address)
    end

    # Get joint address.
    # @return [Fixnum]
    def address
      @address
    end

    # Get the world the joint is associated to.
    # @return [MSPhysics::World]
    def world
      world_address = MSPhysics::Newton::DoubleJoint.get_world(@address)
      MSPhysics::Newton::World.get_user_data(world_address)
    end

    # Destroy joint.
    # @return [void]
    def destroy
      MSPhysics::Newton::DoubleJoint.destroy(@address)
    end

    # Connect joint to its desired child body.
    # @param [MSPhysics::Body] child1
    # @param [MSPhysics::Body] child2
    # @return [Boolean] success
    def connect(child1, child2)
      MSPhysics::Body.validate(child1, self.world)
      MSPhysics::Body.validate(child2, self.world)
      MSPhysics::Newton::DoubleJoint.connect(@address, child1.address, child2.address)
    end

    # Disconnect joint from its child body.
    # @return [Boolean] success
    def disconnect
      MSPhysics::Newton::DoubleJoint.disconnect(@address)
    end

    # Determine whether joint is connected to its child bodies.
    # @return [Boolean]
    def connected?
      MSPhysics::Newton::DoubleJoint.is_connected?(@address)
    end

    # Get joint maximum degrees of freedom.
    # @return [Fixnum]
    def dof
      MSPhysics::Newton::DoubleJoint.get_dof(@address)
    end

    # Get joint type.
    # @return [Fixnum]
    # @see MSPhysics::DOUBLE_JOINT_TYPES
    def type
      MSPhysics::Newton::DoubleJoint.get_type(@address)
    end

    # Get joint parent1 body.
    # @return [MSPhysics::Body, nil]
    def parent1
      address = MSPhysics::Newton::DoubleJoint.get_parent1(@address)
      address ? MSPhysics::Body.body_by_address(address) : nil
    end

    # Get joint parent2 body.
    # @return [MSPhysics::Body, nil]
    def parent2
      address = MSPhysics::Newton::DoubleJoint.get_parent2(@address)
      address ? MSPhysics::Body.body_by_address(address) : nil
    end

    # Get joint child1 body.
    # @return [MSPhysics::Body, nil]
    def child1
      address = MSPhysics::Newton::DoubleJoint.get_child1(@address)
      address ? MSPhysics::Body.body_by_address(address) : nil
    end

    # Get joint child2 body.
    # @return [MSPhysics::Body, nil]
    def child2
      address = MSPhysics::Newton::DoubleJoint.get_child2(@address)
      address ? MSPhysics::Body.body_by_address(address) : nil
    end

    # Get joint pin1 transformation in global space.
    # @return [Geom::Transformation]
    def get_pin1_matrix
      MSPhysics::Newton::DoubleJoint.get_pin1_matrix(@address)
    end

    # Set joint pin1 transformation in global space.
    # @param [Geom::Transformation, Array<Numeric>] matrix
    # @return [Geom::Transformation] New matrix.
    def set_pin1_matrix(matrix)
      MSPhysics::Newton::DoubleJoint.set_pin1_matrix(@address, matrix)
    end

    # Get joint pin2 transformation in global space.
    # @return [Geom::Transformation]
    def get_pin2_matrix
      MSPhysics::Newton::DoubleJoint.get_pin2_matrix(@address)
    end

    # Set joint pin2 transformation in global space.
    # @param [Geom::Transformation, Array<Numeric>] matrix
    # @return [Geom::Transformation] New matrix.
    def set_pin2_matrix(matrix)
      MSPhysics::Newton::DoubleJoint.set_pin2_matrix(@address, matrix)
    end

    # Get the collision state between the two connected child bodies.
    # @return [Boolean]
    def bodies_collidable?
      MSPhysics::Newton::DoubleJoint.bodies_collidable?(@address)
    end

    # Set the collision state between the two connected child bodies.
    # @param [Boolean] state
    def bodies_collidable=(state)
      MSPhysics::Newton::DoubleJoint.set_bodies_collidable(@address, state)
    end

    # Get joint stiffness.
    # @return [Numeric] A value between 0.0 (soft) and 1.0 (stiff).
    def stiffness
      MSPhysics::Newton::DoubleJoint.get_stiffness(@address)
    end

    # Set joint stiffness
    # @param [Numeric] value A value between 0.0 (soft) and 1.0 (stiff).
    def stiffness=(value)
      MSPhysics::Newton::DoubleJoint.set_stiffness(@address, value)
    end

  end # class DoubleJoint
end # module MSPhysics
