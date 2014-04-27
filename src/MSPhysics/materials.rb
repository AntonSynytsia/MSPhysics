module MSPhysics
  module Materials

    # @!visibility private
    @instances = []

    class << self

      # Add material to the materials list.
      # @param [Material] mat
      # @return [Boolean] +true+ (if successful).
      def add(mat)
        return false unless mat.is_a?(MSPhysics::Material)
        return false if @instances.include?(mat)
        remove_by_name(mat.name)
        @instances << self
      end

      # Remove material from the materials list.
      # @param [Material] mat
      # @return [Boolean] +true+ (if successful).
      def remove(mat)
        @instances.delete(mat)
      end

      # Remove material from the materials list by name.
      # @param [String] name
      # @return [Boolean] +true+ (if successful).
      def remove_by_name(name)
        name = name.to_s.downcase
        found_mat = nil
        @instances.each { |mat|
          next if mat.name != name
          found_mat = mat
          break
        }
        @instances.delete(found_mat) ? true : false
      end

      # Get material by name.
      # @param [String] name
      # @return [Material, NilClass]
      def get(name)
        name = name.to_s.downcase
        @instances.each { |mat|
          return mat if mat.name == name
        }
        nil
      end

      # Get a list of all existing material names.
      # @return [Array<String>]
      def get_names
        names = []
        @instances.each { |mat|
          names << mat.name
        }
        names
      end

    end # proxy class
  end # module Materials
end # module MSPhysics
