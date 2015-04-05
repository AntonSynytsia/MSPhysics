module MSPhysics; end

# @private
module MSPhysics::SDL

  dir = File.dirname(__FILE__)
  ops = (RUBY_PLATFORM =~ /mswin|mingw/i) ? 'win' : 'mac'
  bit = (Sketchup.respond_to?('is_64bit?') && Sketchup.is_64bit?) ? '64' : '32'

  extend AMS::FFI::Library

  ffi_lib File.join(dir, ops+bit, 'SDL2')

  INIT_TIMER            = 0x00000001
  INIT_AUDIO            = 0x00000010
  INIT_VIDEO            = 0x00000020 # SDL_INIT_VIDEO implies SDL_INIT_EVENTS
  INIT_JOYSTICK         = 0x00000200 # SDL_INIT_JOYSTICK implies SDL_INIT_EVENTS
  INIT_HAPTIC           = 0x00001000
  INIT_GAMECONTROLLER   = 0x00002000 # SDL_INIT_GAMECONTROLLER implies SDL_INIT_JOYSTICK
  INIT_EVENTS           = 0x00004000
  INIT_NOPARACHUTE      = 0x00100000 # Don't catch fatal signals
  INIT_EVERYTHING       = INIT_TIMER | INIT_AUDIO | INIT_VIDEO | INIT_EVENTS | INIT_JOYSTICK | INIT_HAPTIC |INIT_GAMECONTROLLER

  # General
  attach_function :init, :SDL_Init, [:uint32], :int
  attach_function :quit, :SDL_Quit, [], :void

  # File I/O Abstraction
  attach_function :alloc_rw, :SDL_AllocRW, [], :pointer
  attach_function :free_rw, :SDL_FreeRW, [:pointer], :void
  attach_function :rw_from_const_mem, :SDL_RWFromConstMem, [:pointer, :int], :pointer
  attach_function :rw_from_fp, :SDL_RWFromFP, [:pointer, :bool], :pointer
  attach_function :rw_from_file, :SDL_RWFromFile, [:pointer, :pointer], :pointer
  attach_function :rw_from_mem, :SDL_RWFromMem, [:pointer, :int], :pointer

end # module MSPhysics::SDL

# @private
module MSPhysics::SDL::Mixer

  dir = File.dirname(__FILE__)
  ops = (RUBY_PLATFORM =~ /mswin|mingw/i) ? 'win' : 'mac'
  bit = (Sketchup.respond_to?('is_64bit?') && Sketchup.is_64bit?) ? '64' : '32'
  path = File.join(dir, ops+bit)

  extend AMS::FFI::Library

  ffi_lib(
    File.join(path, 'SDL2'),
    File.join(path, 'libFLAC-8'),
    File.join(path, 'libmikmod-2'),
    File.join(path, 'libmodplug-1'),
    File.join(path, 'libogg-0'),
    File.join(path, 'libvorbis-0'),
    File.join(path, 'libvorbisfile-3'),
    File.join(path, 'smpeg2'),
    File.join(path, 'SDL2_mixer'))

  AUDIO_U8              = 0x0008 # Unsigned 8-bit samples
  AUDIO_S8              = 0x8008 # Signed 8-bit samples
  AUDIO_U16LSB          = 0x0010 # Unsigned 16-bit samples
  AUDIO_S16LSB          = 0x8010 # Signed 16-bit samples
  AUDIO_U16MSB          = 0x1010 # As above, but big-endian byte order
  AUDIO_S16MSB          = 0x9010 # As above, but big-endian byte order
  AUDIO_S32LSB          = 0x8020 # 32-bit integer samples
  AUDIO_S32MSB          = 0x9020 # As above, but big-endian byte order
  AUDIO_F32LSB          = 0x8120 # 32-bit floating point samples
  AUDIO_F32MSB          = 0x9120 # As above, but big-endian byte order

  use_lil = true

  if use_lil
    # Using lil-endian byte order as default
    AUDIO_U16SYS        = AUDIO_U16LSB
    AUDIO_S16SYS        = AUDIO_S16LSB
    AUDIO_S32SYS        = AUDIO_S32LSB
    AUDIO_F32SYS        = AUDIO_F32LSB
  else
    # Using big-endian byte order as default
    AUDIO_U16SYS        = AUDIO_U16MSB
    AUDIO_S16SYS        = AUDIO_S16MSB
    AUDIO_S32SYS        = AUDIO_S32MSB
    AUDIO_F32SYS        = AUDIO_F32MSB
  end

  DEFAULT_FORMAT        = AUDIO_S16SYS

  INIT_FLAC             = 0x0001
  INIT_MOD              = 0x0002
  INIT_MODPLUG          = 0x0004
  INIT_MP3              = 0x0008
  INIT_OGG              = 0x0010
  INIT_FLUIDSYNTH       = 0x0020
  INIT_EVERYTHING       = INIT_FLAC | INIT_MOD | INIT_MODPLUG | INIT_MP3 | INIT_OGG | INIT_FLUIDSYNTH

  # General
  attach_function :init, :Mix_Init, [:uint32], :int
  attach_function :quit, :Mix_Quit, [], :void
  attach_function :open_audio, :Mix_OpenAudio, [:int, :uint16, :int, :int], :int
  attach_function :close_audio, :Mix_CloseAudio, [], :void

  # Music
  attach_function :get_num_music_decoders, :Mix_GetNumMusicDecoders, [], :int
  attach_function :get_music_decoder, :Mix_GetMusicDecoder, [:int], :string
  attach_function :set_music_cmd, :Mix_SetMusicCMD, [:pointer], :int
  attach_function :load_music, :Mix_LoadMUS, [:pointer], :pointer
  attach_function :load_music_rw, :Mix_LoadMUS_RW, [:pointer, :int], :pointer
  attach_function :free_music, :Mix_FreeMusic, [:pointer], :void
  attach_function :play_music, :Mix_PlayMusic, [:pointer, :int], :int
  attach_function :pause_music, :Mix_PauseMusic, [], :void
  attach_function :resume_music, :Mix_ResumeMusic, [], :void
  attach_function :rewind_music, :Mix_RewindMusic, [], :void
  attach_function :halt_music, :Mix_HaltMusic, [], :void
  attach_function :fade_in_music, :Mix_FadeInMusic, [:pointer, :int, :int], :int
  attach_function :fade_in_music_pos, :Mix_FadeInMusicPos, [:pointer, :int, :int, :double], :int
  attach_function :fade_out_music, :Mix_FadeOutMusic, [:int], :int
  attach_function :set_music_pos, :Mix_SetMusicPosition, [:double], :int
  attach_function :get_music_type, :Mix_GetMusicType, [:pointer], :int
  attach_function :is_music_playing, :Mix_PlayingMusic, [], :bool
  attach_function :is_music_paused, :Mix_PausedMusic, [], :bool
  attach_function :volume_music, :Mix_VolumeMusic, [:int], :int
  attach_function :fading_music, :Mix_FadingMusic, [], :int
  callback :music_finished, [], :void
  attach_function :hook_music_finished, :Mix_HookMusicFinished, [:music_finished], :void

  # Samples
  attach_function :get_num_chunk_decoders, :Mix_GetNumChunkDecoders, [], :int
  attach_function :get_chunk_decoder, :Mix_GetChunkDecoder, [:int], :string
  attach_function :load_wav_rw, :Mix_LoadWAV_RW, [:pointer, :int], :pointer
  attach_function :quick_load_wav, :Mix_QuickLoad_WAV, [:pointer], :pointer
  attach_function :quick_load_raw, :Mix_QuickLoad_RAW, [:pointer, :uint32], :pointer
  attach_function :volume_chunk, :Mix_VolumeChunk, [:pointer, :int], :int
  attach_function :free_chunk, :Mix_FreeChunk, [:pointer], :void

  # Channels & Effects
  attach_function :allocate_channels, :Mix_AllocateChannels, [:int], :int
  attach_function :volume_channel, :Mix_Volume, [:int, :int], :int
  attach_function :play_channel_timed, :Mix_PlayChannelTimed, [:int, :pointer, :int, :int], :int
  attach_function :fade_in_channel_timed, :Mix_FadeInChannelTimed, [:int, :pointer, :int, :int, :int], :int
  attach_function :pause_channel, :Mix_Pause, [:int], :void
  attach_function :resume_channel, :Mix_Resume, [:int], :void
  attach_function :stop_channel, :Mix_HaltChannel, [:int], :void
  attach_function :expire_channel, :Mix_ExpireChannel, [:int, :int], :int
  attach_function :fade_out_channel, :Mix_FadeOutChannel, [:int, :int], :int
  attach_function :playing_channel, :Mix_Playing, [:int], :int
  attach_function :paused_channel, :Mix_Paused, [:int], :int
  attach_function :fading_channel, :Mix_FadingChannel, [:int], :int
  attach_function :get_channel_chunk, :Mix_GetChunk, [:int], :pointer
  attach_function :set_channel_panning, :Mix_SetPanning, [:int, :uint8, :uint8], :int
  attach_function :set_channel_distance, :Mix_SetDistance, [:int, :uint8], :int
  attach_function :set_channel_position, :Mix_SetPosition, [:int, :int16, :uint8], :int
  attach_function :set_reverse_stereo, :Mix_SetReverseStereo, [:int, :int], :int
  callback :channel_finished, [:int], :void
  attach_function :hook_channel_finished, :Mix_ChannelFinished, [:channel_finished], :void

end # module MSPhysics::SDL::Mixer
