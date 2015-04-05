module MSPhysics

  # @since 1.0.0
  class Music

    # @!visibility private
    @@instances = []

    class << self

      # Verify that music is valid.
      # @api private
      # @param [Music] music
      # @raise [TypeError] if music is invalid or destroyed.
      # @return [void]
      def validate(music)
        AMS.validate_type(music, MSPhysics::Music)
        unless music.is_valid?
          raise(TypeError, "Music #{music} is invalid/destroyed!", caller)
        end
      end

      # Determine if active music is playing.
      # @return [Boolean]
      def is_playing?
        MSPhysics::SDL::Mixer.is_music_playing
      end

      # Determine if active music is paused.
      # @return [Boolean]
      def is_paused?
        MSPhysics::SDL::Mixer.is_music_paused
      end

      # Determine if active music is stopped.
      # @return [Boolean]
      def is_stopped?
        (MSPhysics::SDL::Mixer.is_music_playing == false && MSPhysics::SDL::Mixer.is_music_paused == false)
      end

      # Determine if active music is fading in/out.
      # @return [Boolean]
      def is_fading?
        MSPhysics::SDL::Mixer.fading_music != 0
      end

      # @overload play()
      #   Resume active music.
      #   @return [nil]
      # @overload play(music)
      #   Play music from beginning with no repeat.
      #   @param [Music] music
      #   @return [Boolean] success
      #   @note Only one music can be played at a time. Active music is halted
      #     any time new music is played.
      # @overload play(music, repeat)
      #   Play music from beginning with repeat.
      #   @param [Music] music
      #   @param [Fixnum] repeat Number of times to repeat music. A repeat value
      #     of -1 will play music continuously. A repeat value of 0 will play
      #     sound once.
      #   @return [Boolean] success
      #   @note Only one music can be played at a time. Active music is halted
      #     any time new music is played.
      def play(*args)
        if args.size == 0
          MSPhysics::SDL::Mixer.resume_music
        elsif args.size == 1
          validate(args[0])
          MSPhysics::SDL::Mixer.set_music_cmd(nil)
          MSPhysics::SDL::Mixer.play_music(args[0].get_address, 0) == 0
        elsif args.size == 2
          validate(args[0])
          MSPhysics::SDL::Mixer.set_music_cmd(nil)
          MSPhysics::SDL::Mixer.play_music(args[0].get_address, args[1].to_i) == 0
        else
          raise(ArgumentError, "Expected 0..2 arguments, but got #{args.size}.", caller)
        end
      end

      # Pause active music.
      # @return [nil]
      def pause
        MSPhysics::SDL::Mixer.pause_music
      end

      # Resume active music. Same as {play} with no parameters.
      # @return [nil]
      def resume
        MSPhysics::SDL::Mixer.resume_music
      end

      # Stop active music.
      # @return [nil]
      def stop
        MSPhysics::SDL::Mixer.halt_music
      end

      # Start playing music from beginning with a fade-in effect.
      # @param [Music] music
      # @param [Fixnum] repeat Number of times to repeat music. A repeat value
      #   of -1 will play music continuously. A repeat value of 0 will play
      #   sound once.
      # @param [Fixnum] time Time in milliseconds for the fade-in effect to
      #   complete.
      # @return [Boolean] success
      def fade_in(music, repeat, time)
        validate(music)
        MSPhysics::SDL::Mixer.fade_in_music(music.get_address, repeat.to_i, time.to_i) == 0
      end

      # Fade out active playing music.
      # @param [Fixnum] time Time in milliseconds for the fade-out effect to
      #   complete.
      # @return [Boolean] success
      def fade_out(time)
        MSPhysics::SDL::Mixer.fade_out_music(time.to_i) == 1
      end

      # Get music volume.
      # @return [Fixnum] A value between 0 and 128.
      def get_volume
        MSPhysics::SDL::Mixer.volume_music(-1)
      end

      # Set music volume.
      # @param [Fixnum] volume New volume, a value between 0 and 128.
      # @return [Fixnum] Original volume, a value between 0 and 128.
      def set_volume(volume)
        volume = AMS.clamp(volume.to_i, 0, 128)
        MSPhysics::SDL::Mixer.volume_music(volume)
      end

      # Destroy all music.
      # @return [Fixnum] Number of music sounds destroyed.
      def destroy_all
        count = @@instances.size
        @@instances.each { |inst|
          inst.destroy
        }
        @@instances.clear
        count
      end

      # Get music by music name.
      # @param [String] name
      # @return [Music, nil]
      def get_by_name(name)
        @@instances.each { |inst|
          return inst if inst.get_name == name
        }
        nil
      end

    end # class << self

    # @overload initialize(path)
    #   Load music from directory.
    #   @param [String] path
    #   @raise [TypeError] if file path is invalid, or file is not a valid music
    #     file.
    # @overload initialize(buffer, size)
    #   Load music from buffer.
    #   @param [String] buffer
    #   @param [Fixnum] size
    #   @raise [TypeError] if buffer doesn't contain valid music data.
    #   @raise [TypeError] if mixer fails to load music from buffer.
    def initialize(*args)
      if args.size == 1
        path = args[0].to_s
        @address = MSPhysics::SDL::Mixer.load_music(path)
        if @address.null?
          raise(TypeError, "File path is invalid, or file is not a valid music file!", caller)
        end
      elsif args.size == 2
        @buffer = args[0].to_s
        size = args[1].to_i
        @rw = MSPhysics::SDL.rw_from_mem(@buffer, size)
        if @rw.null?
          raise(TypeError, "Invalid buffer!", caller)
        end
        @address = MSPhysics::SDL::Mixer.load_music_rw(@rw, 1)
        if @address.null?
          raise(TypeError, "Failed to load music from buffer!", caller)
        end
      else
        raise(ArgumentError, "Expected 1 or 2 arguments, but got #{args.size}.", caller)
      end
      @valid = true
      @name = ""
      @@instances << self
    end

    # Get music address.
    # @return [AMS::FFI::Pointer]
    def get_address
      Music.validate(self)
      @address
    end

    # Get music name.
    # @return [String]
    def get_name
      Music.validate(self)
      @name
    end

    # Set music name.
    # @param [String] name
    # @return [String] Newly assigned name.
    def set_name(name)
      Music.validate(self)
      @name = name.to_s
    end

    # Free music from memory.
    # @return [nil]
    def destroy
      Music.validate(self)
      MSPhysics::SDL::Mixer.free_music(@address)
      @address = nil
      @buffer = nil
      @rw = nil
      @valid = false
      @@instances.delete(self)
      nil
    end

    # Determine if music is valid.
    # @return [Boolean]
    def is_valid?
      @valid
    end

  end # class Music
end # module MSPhysics
