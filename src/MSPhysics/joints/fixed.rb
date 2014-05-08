module MSPhysics
  class Fixed < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    # @param [Numeric] breaking_force The force required for the body to
    #   disconnect. Given force must be in Newtons. Pass +0+ to remain the body
    #   connected regardless of force.
    def initialize(pos, parent, child, breaking_force = 0)
      super(pos, [0,0,1], parent, child, 6)
      @breaking_force = breaking_force.to_f.abs
    end

    # @!attribute [r] breaking_force Get required disconnection force in
    #   Newtons.
    #   @return [Numeric]


    attr_reader :breaking_force

    private

    def submit_constraints(timestep)
      if @breaking_force != 0 and @child.get_force.length >= @breaking_force
        disconnect
        return
      end
      # Calculate the position of the pivot point and the Jacobian direction
      # vectors in global space.
      matrix0 = @child.get_matrix(0)*@local_matrix0
      matrix1 = @parent ? @parent.get_matrix(0)*@local_matrix1 : @local_matrix1
      pos0 = matrix0.origin.to_a.pack('F*')
      pos1 = matrix1.origin.to_a.pack('F*')
      # Restrict the movement on the pivot point along all three orthonormal
      # directions.
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix0.xaxis.to_a.pack('F*'))
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix0.yaxis.to_a.pack('F*'))
      Newton.userJointAddLinearRow(@joint_ptr, pos0, pos1, matrix0.zaxis.to_a.pack('F*'))
      # Get a point along the pin axis at some reasonable large distance from
      # the pivot.
      v1 = matrix0.zaxis
      v1.length = PIN_LENGTH
      q0 = (matrix0.origin + v1).to_a.pack('F*')
      v2 = matrix1.zaxis
      v2.length = PIN_LENGTH
      q1 = (matrix1.origin + v2).to_a.pack('F*')
      # Add two constraints row perpendicular to the pin vector.
      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix0.xaxis.to_a.pack('F*'))
      Newton.userJointAddLinearRow(@joint_ptr, q0, q1, matrix0.yaxis.to_a.pack('F*'))
      # Now get the ankle point
      v1 = matrix0.yaxis
      v1.length = PIN_LENGTH
      r0 = (matrix0.origin + v1).to_a.pack('F*')
      v2 = matrix1.yaxis
      v2.length = PIN_LENGTH
      r1 = (matrix1.origin + v2).to_a.pack('F*')
      Newton.userJointAddLinearRow(@joint_ptr, r0, r1, matrix0.xaxis.to_a.pack('F*'))
    end

    public

    # Set required disconnection force in Newtons.
    # @param [Numeric] force
    def breaking_force=(force)
      @breaking_force = breaking_force.to_f.abs
    end

  end # class Fixed
end # module MSPhysics
