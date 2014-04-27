module MSPhysics
  module BodyObserver

    @instances = []

    class << self

      # Add an observer
      # @param [Object] object
      # @return [Boolean] +true+ (if successful).
      def add_observer(object)
        return false if @instances.include?(object)
        @instances << object
        true
      end

      # Remove an observer
      # @param [Object] object
      # @return [Boolean] +true+ (if successful).
      def remove_observer(object)
        @instances.delete(self)
      end

      # @!visibility private
      def call_event(event, *args)
        @instances.reject! { |instance|
          next unless instance.respond_to?(event)
          begin
            instance.method(event).call(*args)
            false
          rescue Exception => e
            puts "#{e}\n#{$@[0]}"
            true
          end
        }
      end
    end

    # Called when the body is created.
    # @param [Body] body
    def on_create(body)
    end

    # Called when the body is destroyed.
    # @param [Body] body
    def on_destroy(body)
    end

  end # module BodyObserver
end # module MSPhysics
