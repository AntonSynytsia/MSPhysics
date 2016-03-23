module MSPhysics

  # @since 1.0.0
  class Joint

    DEFAULT_CONSTRAINT_TYPE = 0 # Standard - 0; Flexible - 1; Robust - 2.
    DEFAULT_STIFFNESS = 1.00
    DEFAULT_BODIES_COLLIDABLE = true
    DEFAULT_BREAKING_FORCE = 0.0

    class << self

      # Optimize joint name.
      # @param [Symbol, String] name Joint name.
      # @return [Symbol, nil] Proper name if successful.
      def optimize_joint_name(name)
        name = name.to_s.downcase.gsub(/\s|_/, '')
        MSPhysics::JOINT_TYPES.keys.each { |type|
          return type if type.to_s.gsub(/_/, '') == name
        }
        nil
      end

    end # class << self

    # Create joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    # @param [Fixnum] dof Maximum degrees of freedom.
    def initialize(world, parent, pin_tra, dof)
      MSPhysics::World.validate(world)
      MSPhysics::Body.validate(parent) if parent
      parent_address = parent ? parent.get_address : nil
      @address = MSPhysics::Newton::Joint.create(world.get_address, parent_address, pin_tra, dof)
      MSPhysics::Newton::Joint.set_user_data(@address, self)

      MSPhysics::Newton::Joint.set_constraint_type(@address, DEFAULT_CONSTRAINT_TYPE)
      MSPhysics::Newton::Joint.set_stiffness(@address, DEFAULT_STIFFNESS)
      MSPhysics::Newton::Joint.set_bodies_collidable(@address, DEFAULT_BODIES_COLLIDABLE)
      MSPhysics::Newton::Joint.set_breaking_force(@address, DEFAULT_BREAKING_FORCE)
      @name = ''
    end

    # Get joint address.
    # @return [Fixnum]
    def address
      @address
    end

    # Destroy joint.
    # @return [void]
    def destroy
      MSPhysics::Newton::Joint.destroy(@address)
    end

    # Determine whether joint is valid.
    # @return [Boolean]
    def valid?
      MSPhysics::Newton::Joint.is_valid?(@address)
    end

    # Connect joint to its desired child body.
    # @param [MSPhysics::Body] child
    # @return [Boolean] success
    def connect(child)
      MSPhysics::Body.validate(child)
      MSPhysics::Newton::Joint.connect(@address, child.get_address)
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
    # @return [Fixnum]
    def dof
      MSPhysics::Newton::Joint.get_dof(@address)
    end

    # Get joint type.
    # @return [Fixnum]
    # @see MSPhysics::JOINT_TYPES
    def type
      MSPhysics::Newton::Joint.get_type(@address)
    end

    # Get constraint type.
    # @return [Fixnum] A fixed value between 0 and 2.
    #   0. Standard: somewhat stiff and somewhat stable.
    #   1. Flexible: soft and stable.
    #   2. Robust: stiff and may be unstable.
    def constraint_type
      MSPhysics::Newton::Joint.get_constraint_type(@address)
    end

    # Set constraint type.
    # @param [Fixnum] ctype A fixed value between 0 and 2.
    #   0. Standard: somewhat stiff and somewhat stable.
    #   1. Flexible: soft and stable.
    #   2. Robust: stiff and may be unstable.
    def constraint_type=(ctype)
      MSPhysics::Newton::Joint.set_constraint_type(@address, ctype)
    end

    # Get joint parent body.
    # @return [MSPhysics::Body, nil]
    def parent
      address = MSPhysics::Newton::Joint.get_parent(@address)
      address ? MSPhysics::Body.get_body_by_address(address) : nil
    end

    # Get joint child body.
    # @return [MSPhysics::Body, nil]
    def child
      address = MSPhysics::Newton::Joint.get_child(@address)
      address ? MSPhysics::Body.get_body_by_address(address) : nil
    end

    # Get joint pin transformation in global space.
    # @return [Geom::Transformation]
    def get_pin_matrix
      MSPhysics::Newton::Joint.get_pin_matrix(@address)
    end

    # Set joint pin transformation in global space.
    # @param [Geom::Transformation, Array<Numeric>] matrix
    # @return [Geom::Transformation] New matrix.
    def set_pin_matrix(matrix)
      MSPhysics::Newton::Joint.set_pin_matrix(@address, matrix)
    end

    # Get joint pin direction in global space.
    # @return [Geom::Vector3d]
    def get_direction
      MSPhysics::Newton::Joint.get_direction(@address)
    end

    # Set joint pin direction in global space.
    # @param [Geom::Vector3d] dir
    # @return [void]
    def set_direction(dir)
      MSPhysics::Newton::Joint.set_direction(@address, dir)
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
