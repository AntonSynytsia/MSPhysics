module MSPhysics
  class << self

    # Validate object type.
    # @param [Object] obj
    # @param [Object] type
    # @raise [ArgumentError] if object class doesn't match the specified type.
    def validate_type(obj, type)
      return if obj.is_a?(type)
      raise ArgumentError, "Expected #{type}, but got #{obj.class}."
    end

    # Get common attribute value from a collection of entities.
    # @param [Sketchup::Entity, Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @param [Object] default_value The value to return if the attribute value
    #   is not found.
    # @return [String, NilClass] Common attribute value, or +nil+ if one of the
    #   entity attributes is different.
    def get_attribute(ents, handle, name, default_value = nil)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      shape = nil
      ents.each { |e|
        s = e.get_attribute(handle, name, nil)
        shape ||= s
        return if s != shape
      }
      return default_value unless shape
      shape
    end

    # Assign attribute value to a collection of entities.
    # @param [Sketchup::Entity, Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @param [String] value Attribute value.
    def set_attribute(ents, handle, name, value)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.set_attribute(handle, name, value)
      }
    end

    # Delete MSPhysics attributes from a collection of entities.
    # @param [Sketchup::Entity, Array<Sketchup::Entity>] ents A collection of entities.
    def delete_attributes(ents)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.delete_attribute('MSPhysics')
        e.delete_attribute('MSPhysics Body')
        e.delete_attribute('MSPhysics Joint')
      }
    end

    # Delete MSPhysics attributes from all entities.
    def delete_all_attributes
      delete_attributes(Sketchup.active_model.entities)
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

    # Get numeric value sign.
    # @param [Numeric] value
    # @return [Fixnum] -1, 0, or 1
    def sign(value)
      value.zero? ? 0 : (value > 0 ? 1 : -1)
    end

    # Scale vector.
    # @param [Array<Numeric>, Geom::Vector3d] vector
    # @param [Numeric] scale
    # @return [Geom::Vector3d]
    def scale_vector(vector, scale)
      Geom::Vector3d.new(vector[0]*scale, vector[1]*scale, vector[2]*scale)
    end

    # Get least value of the two values.
    # @param [Numeric] a
    # @param [Numeric] b
    # @return [Numeric]
    def min(a, b)
      a < b ? a : b
    end

    # Get greatest value of the two values.
    # @param [Numeric] a
    # @param [Numeric] b
    # @return [Numeric]
    def max(a, b)
      a > b ? a : b
    end

    # Get entity by entity ID.
    # @param [Fixnum] id
    # @return [Sketchup::Entity, NilClass]
    def get_entity_by_id(id)
      Sketchup.active_model.entities.each { |e|
        return e if e.entityID == id
      }
      nil
    end

    # Get entity type.
    # @param [Sketchup::Entity] e
    # @return [String, NilClass]
    def get_entity_type(e)
      type = e.get_attribute('MSPhysics', 'Type', nil)
      if type.nil? && [Sketchup::Group, Sketchup::ComponentInstance].include?(e.class)
        type = 'Body'
      end
      type
    end

    # Set entity type.
    # @param [Sketchup::Entity] e
    # @param [String] type
    def set_entity_type(e, type)
      e.set_attribute('MSPhysics', 'Type', type.to_s)
    end

  end # class << self
end # module MSPhysics
