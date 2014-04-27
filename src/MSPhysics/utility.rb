module MSPhysics

  # @!visibility private
  ATTR_NAME = 'MSPhysics'.freeze

  class << self

    # Get common attribute value from a collection of entities.
    # @param [Array] ents A collection of entities.
    # @param [String] name Attribute name.
    # @param [Object] default_value The value to return if the attribute value
    #   is not found.
    # @return [String, NilClass] Common attribute value, or +nil+ if one of the
    #   entity attributes is different.
    def get_attribute(ents, name, default_value = nil)
      name = name.to_s
      ents = [ents] unless ents.is_a?(Array)
      shape = nil
      ents.each { |e|
        next unless [Sketchup::Group, Sketchup::ComponentInstance].include?(e.class)
        s = e.get_attribute(ATTR_NAME, name, nil)
        shape ||= s
        return if s != shape
      }
      return default_value unless shape
      shape
    end

    # Assign attribute value to a collection of entities.
    # @param [Array] ents A collection of entities.
    # @param [String] name Attribute name.
    # @param [String] value Attribute value.
    def set_attribute(ents, name, value)
      name = name.to_s
      ents = [ents] unless ents.is_a?(Array)
      ents.each { |e|
        next unless [Sketchup::Group, Sketchup::ComponentInstance].include?(e.class)
        e.set_attribute(ATTR_NAME, name, value)
      }
    end

    # Delete MSPhysics attributes from a collection of entities.
    # @param [Array] ents A collection of entities.
    def delete_attributes(ents)
      ents = [ents] unless ents.is_a?(Array)
      ents.each { |e|
        e.delete_attribute(ATTR_NAME)
      }
    end

    # Delete MSPhysics attributes from all entities.
    def delete_all_attributes
      Sketchup.active_model.entities.each { |e|
        e.delete_attribute(ATTR_NAME)
      }
    end

    # Get newton physics engine version number.
    # @return [Numeric]
    def get_newton_version
      Newton.worldGetVersion*0.01
    end

    # Clamp value between min and max.
    # @param [Numeric] value
    # @param [Numeric, NilClass] min Pass +nil+ to have no min limit.
    # @param [Numeric, NilClass] max Pass +nil+ to have no max limit.
    # @return [Numeric]
    def clamp(value, min, max)
      value = min if min and value < min
      value = max if max and value > max
      value
    end

  end # class << self
end # module MSPhysics
