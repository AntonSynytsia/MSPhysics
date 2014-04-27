module MSPhysics
  class Hit

    # @param [Body] body
    # @param [Geom::Point3d] point
    # @param [Geom::Vector3d] normal
    def initialize(body, point, normal)
      @body = body
      @point = point
      @normal = normal
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
