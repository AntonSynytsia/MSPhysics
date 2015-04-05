module MSPhysics

  # @since 1.0.0
  class Sound

    # @!visibility private
    @@instances = []

    class << self

      # Verify that sound is valid.
      # @api private
      # @param [Sound] sound
      # @raise [TypeError] if sound is invalid or destroyed.
      # @return [void]
      def validate(sound)
        AMS.validate_type(sound, MSPhysics::Sound)
        unless sound.is_valid?
          raise(TypeError, "Sound #{sound} is invalid/destroyed!", caller)
        end
      end

      # Get the maximum number of channels can be played simultaneously.
      # @return [Fixnum]
      def get_num_channels
        MSPhysics::SDL::Mixer.allocate_channels(-1)
      end

      # Set the maximum number of channels can be played simultaneously.
      # @param [Fixnum] max_channels
      # @return [Fixnum] Number of channels allocated.
      def set_num_channels(max_channels)
        max_channels = AMS.clamp(max_channels.to_i, 1, 100)
        MSPhysics::SDL::Mixer.allocate_channels(max_channels)
      end

      # Resume all sounds.
      # @return [Fixnum] Number of sounds resumed.
      def resume_all
      end

      alias play_all resume_all

      # Pause all sounds.
      # @return [Fixnum] Number of sounds paused.
      def pause_all
      end

      # Stop all sounds.
      # @return [Fixnum] Number of sounds stopped.
      def stop_all
      end

      # Destroy all sounds.
      # @return [Fixnum] Number of sounds destroyed.
      def destroy_all
        count = @@instances.size
        @@instances.each { |inst|
          inst.destroy
        }
        @@instances.clear
        count
      end

      # Get sound by sound name.
      # @param [String] name
      # @return [Sound, nil]
      def get_by_name(name)
        @@instances.each { |inst|
          return inst if inst.get_name == name
        }
        nil
      end

    end # class << self

    # @overload initialize(path)
    #   Load sound from directory.
    #   @param [String] path
    #   @raise [TypeError] if file path is invalid.
    #   @raise [TypeError] if mixer fails to load sound from file.
    # @overload initialize(buffer, size)
    #   Load sound from buffer.
    #   @param [String] buffer
    #   @param [Fixnum] size
    #   @raise [TypeError] if buffer doesn't contain valid sound data.
    #   @raise [TypeError] if mixer fails to load sound from buffer.
    def initialize(*args)
      if args.size == 1
        path = args[0].to_s
        @rw = MSPhysics::SDL.rw_from_file(path, "rb")
        if @rw.null?
          raise(TypeError, "Invalid path!", caller)
        end
        @address = MSPhysics::SDL::Mixer.load_wav_rw(@rw, 1)
        if @address.null?
          raise(TypeError, "Failed to load sound from file!", caller)
        end
      elsif args.size == 2
        @buffer = args[0].to_s
        size = args[1].to_i
        @rw = MSPhysics::SDL.rw_from_mem(@buffer, size)
        if @rw.null?
          raise(TypeError, "Invalid buffer!", caller)
        end
        @address = MSPhysics::SDL::Mixer.load_wav_rw(@rw, 1)
        if @address.null?
          raise(TypeError, "Failed to load sound from buffer!", caller)
        end
      else
        raise(ArgumentError, "Expected 1 or 2 arguments, but got #{args.size}.", caller)
      end
      @valid = true
      @name = ""
      @channel = nil
      @@instances << self
    end

	private
	
	def update_channel
	  if @channel
	    chunk = MSPhysics::SDL::Mixer.get_chunk(@channel)
		if chunk == @address
		  
		end
	  end
	end
	
	public
	
    # Get sound address.
    # @return [AMS::FFI::Pointer]
    def get_address
      Sound.validate(self)
      @address
    end

    # Get sound name.
    # @return [String]
    def get_name
      Sound.validate(self)
      @name
    end

    # Set sound name.
    # @param [String] name
    # @return [String] Newly assigned name.
    def set_name(name)
      Sound.validate(self)
      @name = name.to_s
    end

    # Determine if sound is playing.
    # @return [Boolean]
    def is_playing?
    end

    # Determine if sound is paused.
    # @return [Boolean]
    def is_paused?
    end

    # Determine if sound is stopped.
    # @return [Boolean]
    def is_stopped?
    end

    # Determine if sound is fading.
    # @return [Boolean]
    def is_fading?
    end

    # Play sound.
    # @param [Fixnum] repeat Number of times to repeat music. A repeat value
    #   of -1 will play music continuously. A repeat value of 0 will play
    #   sound once.
	# @return [Boolean] success
    def play(repeat = 0)
      @channel = MSPhysics::SDL::Mixer.play_channel_timed(-1, @address, repeat, -1)
    end

    # Pause sound.
    def pause
	  
    end

    # Resume sound.
    def resume
    end

    # Stop sound.
    def stop
    end

    # Start playing sound from beginning with a fade-in effect.
    def fade_in
    end

    # Fade out active playing sound.
    def fade_out
    end

    # Get sound volume.
    def get_volume
    end

    # Set sound volume
	# @param [Fixnum] volume A value between 0 (quiet) and 128 (loud). A volume
	#   of zero will not 
    def set_volume(volume)
    end

	# Get sound distance. This is like volume, however, volume cannot go to full
	# silence even if set to zero.
	def get_distance
	end
	
	def set_distance
	end
	
    def get_panning
    end

    def set_panning()
    end

	# @return 
    def get_position
    end

    def set_position(pos, max_hearing_range)
    end

    # Free sound from memory.
    # @return [nil]
    def destroy
      Sound.validate(self)
      MSPhysics::SDL::Mixer.free_chunk(@address)
      @address = nil
      @buffer = nil
      @rw = nil
      @valid = false
      @@instances.delete(self)
      nil
    end

    # Determine if sound is valid.
    # @return [Boolean]
    def is_valid?
      @valid
    end

  end # class Sound
end # module MSPhysics
