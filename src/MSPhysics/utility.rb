module MSPhysics

  DEFAULT_SIMULATION_SETTINGS = {
    :solver_model       => 1,
    :friction_model     => 0,
    :update_timestep    => 1/60.0,
    :gravity            => -9.8,
    :material_thickness => 0.0
  }

  DEFAULT_BODY_SETTINGS = {
    :shape              => 'Convex Hull',
    :material_name      => 'Default (Wood)',
    :density            => 700,
    :static_friction    => 0.50,
    :dynamic_friction   => 0.25,
    :enable_friction    => true,
    :elasticity         => 0.40,
    :softness           => 0.15,
    :magnet_force       => 0.00,
    :magnet_range       => 0.00,
    :magnetic           => false,
    :enable_script      => true
  }

  CURSORS = {
    :select             => 0,
    :select_plus        => 0,
    :select_plus_minus  => 0,
    :hand               => 671,
    :target             => 0
  }

  # Create cursors
  dir = File.dirname(__FILE__)
  path = File.join(dir, 'images/cursors')
    names = [:select, :select_plus, :select_plus_minus]
    names.each { |name|
    CURSORS[name] = UI.create_cursor(File.join(path, name.to_s + '.png'), 5, 12)
  }
  CURSORS[:target] = UI.create_cursor(File.join(path, 'target.png'), 15, 15)


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

    # Delete attribute value from the collection of given entities.
    # @param [Sketchup::Entity, Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    def delete_attribute(ents, handle, name)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.delete_attribute(handle, name)
      }
    end

    # Delete MSPhysics attributes from a collection of entities.
    # @param [Sketchup::Entity, Array<Sketchup::Entity>] ents A collection of entities.
    def delete_attributes(ents)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.delete_attribute('MSPhysics') if e.get_attribute('MSPhysics', 'Type') != 'Joint'
        e.delete_attribute('MSPhysics Body')
        e.delete_attribute('MSPhysics Joint')
        e.delete_attribute('MSPhysics Script')
      }
    end

    # Delete MSPhysics attributes from all entities.
    def delete_all_attributes
      model = Sketchup.active_model
      model.definitions.each { |definition|
        delete_attributes(definition.instances)
      }
      model.attribute_dictionaries.delete('MSPhysics')
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

    # Get points on a 2D circle.
    # @param [Array<Numeric>] origin
    # @param [Numeric] radius
    # @param [Fixnum] num_seg Number of segments.
    # @param [Numeric] rot_angle Rotate angle in degrees.
    # @return [Array<Array<Numeric>>] An array of points on circle.
    def points_on_circle2d(origin, radius, num_seg = 16, rot_angle = 0)
      ra = rot_angle.degrees
      offset = Math::PI*2/num_seg.to_i
      pts = []
      for n in 0...num_seg.to_i
        angle = ra + (n*offset)
        x = Math.cos(angle)*radius
        y = Math.sin(angle)*radius
        pts << [x + origin[0], y + origin[1]]
      end
      pts
    end

    # Get points on a 3D circle.
    # @param [Array<Numeric>, Geom::Point3d] origin
    # @param [Array<Numeric>, Geom::Vector3d] normal
    # @param [Numeric] radius
    # @param [Fixnum] num_seg Number of segments.
    # @param [Numeric] rot_angle Rotate angle in degrees.
    # @return [Array<Geom::Point3d>] An array of points on circle.
    def points_on_circle3d(origin, radius, normal = [0,0,1], num_seg = 16, rot_angle = 0)
      # Get the x and y axes
      origin = Geom::Point3d.new(origin)
      axes = Geom::Vector3d.new(normal).axes
      xaxis = axes[0]
      yaxis = axes[1]
      xaxis.length = radius
      yaxis.length = radius
      # Compute points
      ra = rot_angle.degrees
      offset = Math::PI*2/num_seg.to_i
      pts = []
      for n in 0...num_seg.to_i
        angle = ra + (n*offset)
        cosa = Math.cos(angle)
        sina = Math.sin(angle)
        vec = Geom::Vector3d.linear_combination(cosa, xaxis, sina, yaxis)
        pts << origin + vec
      end
      pts
    end

    # Blend colors.
    # @param [Numeric] ratio between 0.0 and 1.0.
    # @param [Array<Sketchup::Color, String, Array>] colors An array of colors to blend.
    # @return [Sketchup::Color]
    def blend_colors(ratio, colors = ['white', 'black'])
      if colors.empty?
        raise ArgumentError, 'Expected at least one color, but got none.'
      end
      return Sketchup::Color.new(colors[0]) if colors.size == 1
      ratio = MSPhysics.clamp(ratio, 0, 1)
      cr = (colors.length-1)*ratio
      dec = cr-cr.to_i
      if dec == 0
        color = colors[cr]
      else
        a = colors[cr.to_i]
        b = colors[cr.ceil]
        a = Sketchup::Color.new(a) unless a.is_a?(Sketchup::Color)
        b = Sketchup::Color.new(b) unless b.is_a?(Sketchup::Color)
        a = a.to_a
        b = b.to_a
        color = []
        for i in 0..3
          color << ((b[i] - a[i])*dec + a[i]).round
        end
      end
      Sketchup::Color.new(color)
    end

  end # class << self
end # module MSPhysics
