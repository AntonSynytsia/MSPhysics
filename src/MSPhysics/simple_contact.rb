module MSPhysics
  class SimpleContact

    # @param [AMS::FFI::Pointer] material_ptr
    # @param [AMS::FFI::Pointer] body_ptr
    def initialize(material_ptr, body_ptr)
      point = 0.chr*12
      normal = 0.chr*12
      Newton.materialGetContactPositionAndNormal(material_ptr, body_ptr, point, normal)
      @position = Conversion.convert_point(point.unpack('F*'), :m, :in)
      @normal = Geom::Vector3d.new(normal.unpack('F*'))
      force = 0.chr*12
      Newton.materialGetContactForce(material_ptr, body_ptr, force)
      @force = Geom::Vector3d.new(force.unpack('F*'))
    end

    # @!attribute [r] position
    #   @return [Geom::Point3d]

    # @!attribute [r] normal
    #   @return [Geom::Vector3d]

    # @!attribute [r] force
    #   @return [Geom::Vector3d]


    attr_reader :position, :normal, :force

  end # class SimpleContact
end # module MSPhysics
