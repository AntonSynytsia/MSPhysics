module MSPhysics

  # @since 1.0.0
  class Entity

    def to_s
      sprintf("#<%s:0x%014x>", self.class, self.object_id << 1)
    end

    def inspect
      sprintf("#<%s:0x%014x>", self.class, self.object_id << 1)
    end

  end # class Entity
end # module MSPhysics
