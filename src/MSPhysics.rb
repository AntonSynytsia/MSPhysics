# ------------------------------------------------------------------------------
# **MSPhysics**
#
# Homepage
#   http://sketchucation.com/forums/viewtopic.php?f=323&t=56852
#
# Overview
#   MSPhysics is a physics simulation tool for SketchUp. MSPhysics uses
#   NewtonDynamics physics engine by Juleo Jerez in order to produce fast and
#   reliable physics effects. In many ways the goal of this project is to bring
#   SketchyPhysics back to life.
#
# Access
#   - (Menu) Plugins → MSPhysics → [Option]
#   - (Context Menu) MSPhysics → [Option]
#   - MSPhysics Toolbars
#
# Compatibility and Requirements
#   - Microsoft Windows XP, Vista, 7, or 8.
#     This plugin will not work on Mac OS X as many of the techniques and
#     features are achieved via the mighty Windows API.
#   - SketchUp 8 or later. Latest SU version is always recommended!
#     To get it working on SU6 or SU7 you must upgrade SU Ruby core to 1.8.6 or
#     1.8.7. See plugin homepage for ruby upgrade instructions.
#   - AMS Library 2.2.0 or later.
#
# Version
#   - 0.2.0
#   - NewtonDynamics 3.13
#
# Release Date
#   April 05, 2015
#
# Licence
#   MIT © 2015, Anton Synytsia
#
# Credits
#   - Juleo Jerez for the NewtonDynamics physics engine.
#   - Chris Phillips for ideas from SketchyPhysics.
#
# ------------------------------------------------------------------------------

require 'sketchup.rb'
require 'extensions.rb'

load_me = true

# Verify operation system.
if RUBY_PLATFORM !~ /mswin|mingw/i
  load_me = false
  msg = "'MSPhysics' extension can operate on MSFT Windows based platforms only! "
  msg << "Your operating system doesn't smell like Windows to me."
  UI.messagebox(msg)
  dir = File.dirname(__FILE__)
  File.rename(__FILE__, File.join(dir, 'MSPhysics.rb!'))
end

=begin
# Check whether SU is using Ruby core 1.8.6 or later.
if load_me && RUBY_VERSION.delete('.').to_i < 186
  msg = "MSPhysics extension requires Ruby core 1.8.6 or later! "
  msg << "Upgrade SketchUp to the most recent Ruby 1.8.6/1.8.7 and restart! "
  msg << "Click OK to launch a browser to the Ruby upgrade page."
  if UI.messagebox(msg, MB_OKCANCEL) == IDOK
    UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=56852')
  end
  load_me = false
end
=end

# Load and verify AMS Library.
begin
  require 'ams_Lib/main.rb'
  raise 'Outdated library!' if AMS::Lib::VERSION.to_f < 2.2
rescue StandardError, LoadError
  msg = "'MSPhysics' extension requires AMS Library version 2.2.0 or later! "
  msg << "This extension will not work without the library installed. "
  msg << "Would you like to get to the library's download page?"
  load_me = false
  if UI.messagebox(msg, MB_YESNO) == IDYES
    UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
  end
end if load_me

if load_me

  # @since 1.0.0
  module MSPhysics

    NAME         = 'MSPhysics'.freeze
    VERSION      = '0.2.0'.freeze
    RELEASE_DATE = 'April 05, 2015'.freeze

    # Create the extension.
    @extension = SketchupExtension.new NAME, 'MSPhysics/main.rb'

    desc = "MSPhysics is a physics simulation tool similar to SketchyPhysics."

    # Attach some nice info.
    @extension.description = desc
    @extension.version     = VERSION
    @extension.copyright   = 'Anton Synytsia © 2015'
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
