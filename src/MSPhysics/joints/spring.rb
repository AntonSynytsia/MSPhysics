module MSPhysics
  class Spring < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    def initialize(pos, pin_dir, parent, child)
      super(pos, pin_dir, parent, child, 6)
    end

	private

    def submit_constraints(timestep)
      matrix0 = @child.get_matrix(0)*@local_matrix0
      matrix1 = @parent ? @parent.get_matrix(0)*@local_matrix1 : @local_matrix1
	end
	
	public
	
  end # class Spring
end # module MSPhysics
