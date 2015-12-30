module MSPhysics

  # The Contact class represents an individual collision contact.
  # @since 1.0.0
  class Contact

    # @param [Body] toucher
    # @param [Geom::Point3d, Array<Numeric>] point Contact point.
    # @param [Geom::Vector3d, Array<Numeric>] normal Contact normal.
    # @param [Geom::Vector3d, Array<Numeric>] force Contact force.
    # @param [Numeric] speed Contact speed.
    def initialize(toucher, point, normal, force, speed)
      @toucher = toucher
      @point = Geom::Point3d.new(point.to_a)
      @normal = Geom::Vector3d.new(normal.to_a).normalize
      @force = Geom::Vector3d.new(force.to_a)
      @speed = speed.to_f
    end

    # @!attribute [r] toucher
    #   Get contact body.
    #   @return [Body]

    # @!attribute [r] point
    #   Get contact point.
    #   @return [Geom::Point3d]

    # @!attribute [r] normal
    #   Get contact normal.
    #   @return [Geom::Vector3d]

    # @!attribute [r] force
    #   Get contact force in Newtons.
    #   @return [Geom::Vector3d]

    # @!attribute [r] speed
    #   Get contact speed in meters per second.
    #   @return [Numeric]


    attr_reader :toucher, :point, :normal, :force, :speed

  end # class Contact
end # module MSPhysics
