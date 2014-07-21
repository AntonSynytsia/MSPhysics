module MSPhysics
  class Contact

    # @param [AMS::FFI::Pointer] material_ptr
    # @param [AMS::FFI::Pointer] body_ptr
    # @param [Body] toucher
    def initialize(material_ptr, body_ptr, toucher)
      buf1 = 0.chr*12
      buf2 = 0.chr*12
      @body = toucher
      Newton.materialGetContactPositionAndNormal(material_ptr, body_ptr, buf1, buf2)
      @position = Conversion.convert_point(buf1.unpack('F*'), :m, :in)
      @normal = Geom::Vector3d.new(buf2.unpack('F*'))
      buf3 = 0.chr*12
      Newton.materialGetContactForce(material_ptr, body_ptr, buf3)
      @force = Geom::Vector3d.new(buf3.unpack('F*'))
      @speed = Newton.materialGetContactNormalSpeed(material_ptr)
    end

    # @!attribute [r] body
    #   Get contact touching body.
    #   @return [Body]

    # @!attribute [r] position
    #   Get contact position in global space.
    #   @return [Geom::Point3d]

    # @!attribute [r] normal
    #   Get contact direction.
    #   @return [Geom::Vector3d]

    # @!attribute [r] force
    #   Get contact force in Newtons.
    #   @return [Geom::Vector3d]

    # @!attribute [r] speed
    #   Get contact speed in meters per second.
    #   @return [Numeric]


    attr_reader :body, :position, :normal, :force, :speed

  end # class Contact

  # @private
  class Contact2 < Contact

    def initialize(toucher, pos, normal)
      @body = toucher
      @position = Conversion.convert_point(pos, :m, :in)
      @normal = Geom::Vector3d.new(normal)
      @force = Geom::Vector3d.new(0,0,0)
      @speed = 0
    end

  end # class Contact2
end # module MSPhysics
