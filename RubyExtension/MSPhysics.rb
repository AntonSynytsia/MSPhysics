# ------------------------------------------------------------------------------
# **MSPhysics**
#
# Homepage
#   http://sketchucation.com/forums/viewtopic.php?f=323&t=56852
#
# Overview
#   MSPhysics is a physics simulation tool similar to SketchyPhysics. Unlike
#   SketchyPhysics, MSPhysics has a far more advanced scripting API. When it
#   came to creating FPS type games in SP, scripters were limited to having
#   control over user input. Scripters had to add extra work-around code in
#   order to gain control over mouse input, which was very time consuming and
#   not as reliable as wanted. MSPhysics, however, gives scripters full control
#   over all mouse and keyboard events, including the mouse wheel. Instead of
#   having keyboard keys serve as shortcuts, they can be intercepted and used as
#   game controls. Instead of having the mouse wheel serve as a shortcut for the
#   native zoom in/out operation, it can be intercepted and serve as a command
#   to switch weapons for instance. All such operations might seem a fantasy,
#   but thanks to Microsoft Windows API, which is heavily implemented in AMS
#   Library, gaining control over user input is possible. Along the lines,
#   MSPhysics uses NewtonDynamics 3.14 by Juleo Jerez in order to produce fast
#   and realistic physics effects. Compared to Newton 1.53, which is used by
#   SketchyPhysics, Newton 3.14 is faster and more advanced. In many ways the
#   goal of this project is to bring SketchyPhysics back to life.
#
# Access
#   - (Menu) Plugins → MSPhysics → [Option]
#   - (Context Menu) MSPhysics → [Option]
#   - MSPhysics Toolbars
#
# Compatibility and Requirements
#   - Microsoft Windows XP, Vista, 7, 8, or 10.
#     This plugin will not work on Mac OS X as many of the techniques and
#     features are achieved via the Windows API.
#   - SketchUp 6 or later. SU2016 64bit is recommended!
#   - AMS Library 3.2.0 or later.
#
# Version
#   - MSPhysics 0.7.3
#   - NewtonDynamics 3.14
#   - SDL 2.0.4
#   - SDL_mixer 2.0.1
#
# Release Date
#   May 25, 2016
#
# Licence
#   MIT © 2015-2016, Anton Synytsia
#
# Credits
#   - Julio Jerez for the NewtonDynamics physics engine.
#   - Chris Phillips for ideas from SketchyPhysics.
#	- István Nagy (PituPhysics) for examples and testing.
#
# ------------------------------------------------------------------------------

require 'sketchup.rb'
require 'extensions.rb'

load_me = true

# Verify operation system.
if RUBY_PLATFORM !~ /mswin|mingw/i
  load_me = false
  UI.messagebox("'MSPhysics' extension is not compatible on your platform. Only Microsoft Windows platforms are supported. In that case 'MSPhysics' will not be loaded.")
end

# Load and verify AMS Library.
if load_me
  begin
    require 'ams_Lib/main.rb'
    raise 'Outdated library!' if AMS::Lib::VERSION.to_f < 3.2
  rescue StandardError, LoadError
    load_me = false
    msg = "'MSPhysics' extension requires AMS Library version 3.2.0 or later. 'MSPhysics' extension will not work without the library installed. Would you like to get to the library's download page?"
    if UI.messagebox(msg, MB_YESNO) == IDYES
      UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
    end
  end
end

if load_me
  # @since 1.0.0
  module MSPhysics

    NAME         = 'MSPhysics'.freeze
    VERSION      = '0.7.3'.freeze
    RELEASE_DATE = 'May 25, 2016'.freeze

    # Create the extension.
    @extension = SketchupExtension.new NAME, 'MSPhysics/main.rb'

    desc = "MSPhysics is a physics simulation plugin similar to SketchyPhysics."

    # Attach some nice info.
    @extension.description = desc
    @extension.version     = VERSION
    @extension.copyright   = 'Anton Synytsia © 2015-2016'
    @extension.creator     = 'Anton Synytsia (anton.synytsia@gmail.com)'

    # Register and load the extension on start-up.
    Sketchup.register_extension @extension, true

    class << self

      # @!attribute [r] extension
      # Get MSPhysics extension.
      # @return [SketchupExtension]
      attr_reader :extension

    end # class << self
  end # module MSPhysics
end
