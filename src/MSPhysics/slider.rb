module MSPhysics

  # @!visibility private
  @@instances ||= {}

  # Usually, a slider is created through a <tt>MSPhysics::Common.#slider</tt>
  # command. To tick slider value or get slider information, you can always use
  # <tt>MSPhysics::Slider.get_by_name(slider_name)</tt> method to get Slider
  # instance, and then access desired slider functions.
  # @since 1.0.0
  class Slider

    class << self

      # Verify that slider is valid.
      # @api private
      # @param [Slider] slider
      # @return [void]
      # @raise [TypeError] if the slider is invalid or destroyed.
      def validate(slider)
        AMS.validate_type(slider, MSPhysics::Slider)
        unless slider.is_valid?
          raise(TypeError, "Slider #{slider} is invalid/destroyed!", caller)
        end
      end

      # Get range slider by name.
      # @param [String] name Slider name.
      # @return [Slider, nil] A slider object or nil if no slider with given
      #   name exists.
      def get_by_name(name)
        @@instances[name]
      end

      # Destroy all sliders.
      # @return [Fixnum] Number of sliders destroyed.
      def destroy_all
        size = @@instances.size
        @@instances.each { |name, inst|
          inst.destroy
        }
        @@instances.clear
        size
      end

    end # class << self

    # Create a new slider.
    # @param [String] name Slider name.
    # @param [Numeric] default_value Starting value.
    # @param [Numeric] min Minimum value.
    # @param [Numeric] max Maximum value.
    # @param [Numeric] step Snap step.
    def initialize(name, default_value = 0, min = 0, max = 1, step = 0)
      @name = name.to_s
      if @sliders[@name]
        raise(TypeError, "Slider with given name already exists", caller)
      end
      @min = min.to_f
      @max = AMS.clamp(max.to_f, @min, nil)
      @step = AMS.clamp(step.to_f, 0, nil)
      default_value = AMS.clamp(default_value.to_f, @min, @max)
      @valid = true
      @instances[@name] = self
    end

    # Get slider name.
    def get_name
      Slider.validate(self)
      @name
    end

    # Get slider minimum value.
    # @return [Numeric]
    def get_min
      Slider.validate(self)
      @min
    end

    # Set slider minimum value.
    # @param [Numeric] value
    # @return [Numeric] The new value.
    def set_min(value)
      Slider.validate(self)
      @min = AMS.clamp(value.to_f, nil, @max)
    end

    # Get slider maximum value.
    # @return [Numeric]
    def get_max
      Slider.validate(self)
      @max
    end

    # Set slider maximum value.
    # @param [Numeric] value
    # @return [Numeric] The new value.
    def set_max(value)
      Slider.validate(self)
      @max = AMS.clamp(value.to_f, @min, nil)
    end

    # Get slider snap step.
    # @return [Numeric]
    def get_step
      Slider.validate(self)
      @step
    end

    # Set slider snap step.
    # @param [Numeric] value
    # @return [Numeric] The new value.
    def set_step(value)
      Slider.validate(self)
      @step = AMS.clamp(value.to_f, 0, nil)
    end

    # Get slider value.
    # @return [Numeric]
    def get_value
      Slider.validate(self)
    end

    # Set slider value.
    # @param [Numeric] value
    # @return [Numeric] The new value.
    def set_value(value)
      Slider.validate(self)
    end

    # Destroy slider.
    # @note Calling any methods after a slider is destroyed, except for the
    #   {#is_valid?} method, will result in a TypeError.
    # @return [void]
    def destroy
      Slider.validate(self)
      @valid = false
    end

    # Determine if slider is not destroyed.
    # @return [Boolean]
    def is_valid?
      @valid
    end

  end # class Slider
end # module MSPhysics
