module MSPhysics

  # @since 1.0.0
  class Gear < Entity

    DEFAULT_RATIO = 1.0

    class << self

      # Verify that gear is valid.
      # @api private
      # @param [Gear] gear
      # @param [World, nil] world A world the gear ought to belong to or +nil+.
      # @raise [TypeError] if gear is invalid or destroyed.
      # @return [void]
      def validate(gear, world = nil)
        AMS.validate_type(gear, MSPhysics::Gear)
        unless gear.valid?
          raise(TypeError, "Gear #{gear} is invalid/destroyed!", caller)
        end
        if world != nil
          AMS.validate_type(world, MSPhysics::World)
          if gear.world.address != world.address
            raise(TypeError, "Gear #{gear} belongs to a different world!", caller)
          end
        end
      end

      # Get gear by address.
      # @param [Integer] address
      # @return [Gear, nil] A Gear object if successful.
      # @raise [TypeError] if the address is invalid.
      def gear_by_address(address)
        data = MSPhysics::Newton::Gear.get_user_data(address.to_i)
        data.is_a?(MSPhysics::Gear) ? data : nil
      end

      # Get all gears.
      # @note Gears that do not have a {Gear} instance are not included in the
      #   array.
      # @return [Array<Gear>]
      def all_gears
        MSPhysics::Newton.get_all_gears() { |ptr, data| data.is_a?(MSPhysics::Gear) ? data : nil }
      end

    end # class << self

    # @param [MSPhysics::World] world
    # @param [MSPhysics::Joint] joint1
    # @param [MSPhysics::Joint] joint2
    def initialize(world, joint1, joint2)
      MSPhysics::World.validate(world)
      MSPhysics::Joint.validate(joint1, world)
      MSPhysics::Joint.validate(joint2, world)
      @address = MSPhysics::Newton::Gear.create(world.address, joint1.address, joint2.address)
      MSPhysics::Newton::Gear.set_user_data(@address, self)
      MSPhysics::Newton::Gear.set_ratio(@address, DEFAULT_RATIO)
    end

    # Determine whether gear is valid.
    # @return [Boolean]
    def valid?
      MSPhysics::Newton::Gear.is_valid?(@address)
    end

    # Get pointer the gear.
    # @return [Integer]
    def address
      @address
    end

    # Get the world the gear is associated to.
    # @return [MSPhysics::World]
    def world
      world_address = MSPhysics::Newton::Gear.get_world(@address)
      MSPhysics::Newton::Gear.get_user_data(world_address)
    end

    # Get the first joint the gear is linked to.
    # @return [MSPhysics::Joint]
    def joint1
      joint1_address = MSPhysics::Newton::Gear.get_joint1(@address)
      MSPhysics::Newton::Joint.get_user_data(joint1_address)
    end

    # Get the second joint the gear is linked to.
    # @return [MSPhysics::Joint]
    def joint2
      joint2_address = MSPhysics::Newton::Gear.get_joint2(@address)
      MSPhysics::Newton::Joint.get_user_data(joint2_address)
    end

    # Destroy gear.
    # @return [void]
    def destroy
      MSPhysics::Newton::Gear.destroy(@address)
    end

    # Get gear ratio.
    # @return [Numeric]
    def ratio
      MSPhysics::Newton::Gear.get_ratio(@address)
    end

    # Set gear ratio.
    # @param [Numeric] value
    def ratio=(value)
      MSPhysics::Newton::Gear.set_ratio(@address, value)
    end

  end # class Gear
end # module MSPhysics
