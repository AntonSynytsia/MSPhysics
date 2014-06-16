module MSPhysics
  class UpVector < Joint

    # @param [Array<Numeric>, Geom::Vector3d] pin_dir Pivot direction in global
    #   space.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    def initialize(pin_dir, child)
      super([0,0,0], pin_dir, nil, child, 6)
    end

    def submit_constraints(timestep)
      matrix0 = @child.get_matrix(0)*@local_matrix0
      matrix1 = @parent ? @parent.get_matrix(0)*@local_matrix1 : @local_matrix1

      lateral_dir = matrix0.zaxis * matrix1.zaxis
      mag = lateral_dir % lateral_dir
      if mag > 1.0e-6
        # If the side vector is not zero, it means the body has rotated.
        mag = mag**0.5
        lateral_dir = MSPhysics.scale_vector(lateral_dir, 1.0 / mag)
        angle = Math.asin(mag)
        # Add an angular constraint to correct the error angle.
        Newton.userJointAddAngularRow(@joint_ptr, angle, lateral_dir.to_a.pack('F*'))
        Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
        # In theory only one correction is needed, but this produces instability
        # as the body may move sideways. Add a lateral correction prevent this
        # from happening.
        front_dir = lateral_dir * matrix1.zaxis
        Newton.userJointAddAngularRow(@joint_ptr, 0.0, front_dir.to_a.pack('F*'))
        Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      else
        # If the angle error is very small then two angular correction along the
        # plane axis will do the trick.
        Newton.userJointAddAngularRow(@joint_ptr, 0.0, matrix0.yaxis.to_a.pack('F*'))
        Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
        Newton.userJointAddAngularRow(@joint_ptr, 0.0, matrix0.xaxis.to_a.pack('F*'))
        Newton.userJointSetRowStiffness(@joint_ptr, @stiffness)
      end
    end

  end # class UpVector
end # module MSPhysics
