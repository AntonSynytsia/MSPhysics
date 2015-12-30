module MSPhysics

  # @since 1.0.0
  class Hit

    # @param [Body] body
    # @param [Geom::Point3d, Array<Numeric>] point
    # @param [Geom::Vector3d, Array<Numeric>] normal
    def initialize(body, point, normal)
      @body = body
      @point = Geom::Point3d.new(point)
      @normal = Geom::Vector3d.new(normal)
    end

    # @!attribute [r] body
    #   @return [Body]

    # @!attribute [r] point
    #   @return [Geom::Point3d]

    # @!attribute [r] normal
    #   @return [Geom::Vector3d]


    attr_reader :body, :point, :normal

  end # class Hit
end # module MSPhysics
