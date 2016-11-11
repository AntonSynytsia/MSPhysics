# ------------------------------------------------------------------------------
# **MSPhysics**
#
# Homepage
#   http://sketchucation.com/forums/viewtopic.php?f=323&t=56852
#
# Overview
#   MSPhysics is a real-time physics simulation plugin for SketchUp, similar to
#   SketchyPhysics. Unlike SketchyPhysics, MSPhsyics is a written completely
#   from scratch, integrating the latest NewtonDynamics physics SDK. The
#   differences between the two is that MSPhysics is a lot faster, has a far
#   more advanced scripting API, and comes with a reliable Replay animation
#   tool. Furthermore MSPhysics uses AMS Library, which on the Windows side,
#   allows taking control over user input, such as mouse and keyboard, and
#   switching SketchUp fullscreen. With such advantage, creating FPS type games
#   in MSPhysics, without interference of various keyboard/mouse shortcuts, is
#   now a possibility. At the moment, the project is under development,
#   particularly within the Wiki branch. All in all huge credit goes to Julio
#   Jerez for writing the NewtonDynamics physics engine. As well as to those who
#   contributed in making the project a reality.
#
# Access
#   - (Menu) Extensions → MSPhysics → [Option]
#   - (Context Menu) MSPhysics → [Option]
#   - MSPhysics Toolbars
#
# Compatibility and Requirements
#   - Microsoft Windows XP or later / Mac OS X 10.5+
#   - SketchUp 6 or later. SU2016 64bit is recommended!
#   - AMS Library 3.4.0+
#
# Version
#   - MSPhysics 0.9.7
#   - NewtonDynamics 3.14
#   - SDL 2.0.5
#   - SDL_mixer 2.0.1
#
# Release Date
#   November 10, 2016
#
# Licence
#   MIT © 2015-2016, Anton Synytsia
#
# Credits
#   - Julio Jerez for the NewtonDynamics physics engine.
#   - Chris Phillips for ideas from SketchyPhysics.
#   - István Nagy (PituPhysics) and Faust07 for testing.
#
# ------------------------------------------------------------------------------

require 'sketchup.rb'
require 'extensions.rb'

load_me = true

# Load and verify AMS Library.
if load_me
  begin
    require 'ams_Lib/main.rb'
    raise 'Outdated library!' if AMS::Lib::VERSION.to_f < 3.4
  rescue StandardError, LoadError
    load_me = false
    msg = "MSPhysics extension requires AMS Library, version 3.4.0 or later. MSPhysics will not work without the library installed. Would you like to visit the library's download page?"
    if UI.messagebox(msg, MB_YESNO) == IDYES
      UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
    end
  end
end

if load_me
  # @since 1.0.0
  module MSPhysics

    NAME         = 'MSPhysics'.freeze
    VERSION      = '0.9.7'.freeze
    RELEASE_DATE = 'November 10, 2016'.freeze

    # Create the extension.
    @extension = SketchupExtension.new(NAME, 'MSPhysics/main.rb')

    desc = "A realtime physics simulation plugin similar to SketchyPhysics."

    # Attach some nice info.
    @extension.description = desc
    @extension.version     = VERSION
    @extension.copyright   = 'Anton Synytsia © 2015-2016'
    @extension.creator     = 'Anton Synytsia (anton.synytsia@gmail.com)'

    # Register and load the extension on start-up.
    Sketchup.register_extension(@extension, true)

    class << self

      # @!attribute [r] extension
      # Get MSPhysics extension.
      # @return [SketchupExtension]
      attr_reader :extension

    end # class << self
  end # module MSPhysics
end
