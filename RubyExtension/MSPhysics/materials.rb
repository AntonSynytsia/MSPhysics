# @since 1.0.0
module MSPhysics::Materials

  @instances ||= {}

  class << self

    include Enumerable

    # @!visibility private
    def each(&block)
      @instances.values.each(&block)
    end

    # Add/Overwrite material to/in the materials list.
    # @param [Material] material
    # @return [Boolean] success
    def add(material)
      AMS.validate_type(material, MSPhysics::Material)
      @instances[material.name] = material
      true
    end

    # Remove material from the materials list.
    # @param [Material] material
    # @return [Boolean] success
    def remove(material)
      AMS.validate_type(material, MSPhysics::Material)
      @instances.delete(material)
    end

    # Remove material from the materials list by name.
    # @note Case sensitive.
    # @param [String] name
    # @return [Boolean] success
    def remove_by_name(name)
      @instances.delete(name) ? true : false
    end

    # Get material by name.
    # @note Case sensitive.
    # @param [String] name
    # @return [Material, nil]
    def material_by_name(name)
      @instances[name]
    end

    # Get all material names.
    # @return [Array<String>]
    def names
      @instances.keys
    end

  end # class << self
end # module MSPhysics::Materials
