# ------------------------------------------------------------------------------
# ** MSPhysics **
#
# Overview
#   MSPhysics is a physics simulation tool for SketchUp. MSPhysics uses
#   NewtonDynamics physics engine by Juleo Jerez in order to produce fast and
#   reliable physics effects. In many ways the goal of this project is to bring
#   SketchyPhysics back to life.
#
# Homepage
#   http://sketchucation.com/forums/viewtopic.php?f=323&t=56852
#
# Access
#   * (Menu) Plugins → MSPhysics → [option]
#   * MSPhysics Toolbars
#
# Compatibility and Requirements
#   * Microsoft Windows XP, Vista, 7, or 8. This plugin will not work for Macs
#     as many of the techniques and features are achieved via the Windows API,
#     which are not available on the other operating systems.
#   * SketchUp 6 or later. SU 2014 is highly recommended!
#   * Ruby core 1.8.7 for SU2013 and below. See plugin homepage for ruby upgrade
#     instructions.
#   * AMS_Library 1.0.9 +.
#
# Version
#   * 0.2.0
#   * NewtonDynamics 3.12
#
# Release Date
#   June 26, 2014
#
# Licence
#   MIT © 2014, Anton Synytsia
#
# Credits
#   * Juleo Jerez for the NewtonDynamics physics engine.
#   * Chris Phillips for SketchyPhysics.
#
# ------------------------------------------------------------------------------

require 'sketchup.rb'
require 'extensions.rb'

dir = File.dirname(__FILE__)
lib = File.join(dir, 'ams_Lib.rb')
win = ( RUBY_PLATFORM =~ /mswin|mingw/i )

continue = true

# Check operating system, and make decisions prior to loading.
unless win
  msg = "It seems your operating system is not Microsoft Windows. "
  msg << "MSPhysics extension might not operate on your OS.\n"
  msg << "Load the extension anyway?"
  input = UI.messagebox(msg, MB_YESNO)
  continue = false if (input == IDNO)
end

# Check whether SU is using Ruby core 1.8.7 or later.
if continue and RUBY_VERSION.delete('.').to_i < 187
  msg = "MSPhysics extension requires Ruby 1.8.7 or later! "
  msg << "Upgrade SketchUp to the most recent Ruby 1.8.7, and restart!\n"
  msg << "Click OK to launch a browser to the Ruby upgrade page."
  input = UI.messagebox(msg, MB_OKCANCEL)
  if input == IDOK
    UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
  end
  continue = false
end

# Check for AMS Library.
if continue
  if File.exists?(lib)
    require 'ams_Lib.rb'
    if AMS::Lib::VERSION.delete('.').to_i < 109
      msg = "MSPhysics extension requires AMS Library version 1.0.9 or later! "
      msg << "Download AMS Library, extract it into the Plugins folder, and restart.\n"
      msg << "Click OK to proceed to the library's homepage."
      input = UI.messagebox(msg, MB_OKCANCEL)
      if input == IDOK
        UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
      end
      continue = false
    end
  else
    msg = "MSPhysics extension requires AMS Library! "
    msg << "Download AMS Library, extract it into the Plugins folder, and then restart.\n"
    msg << "Click OK to proceed to the library's homepage."
    input = UI.messagebox(msg, MB_OKCANCEL)
    if input == IDOK
      UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
    end
    continue = false
  end
end

module MSPhysics

  NAME         = 'MSPhysics'.freeze
  VERSION      = '0.2.0'.freeze
  RELEASE_DATE = 'June 26, 2014'.freeze

  # Create the extension.
  @extension = SketchupExtension.new NAME, 'MSPhysics/main.rb'

  desc = "MSPhysics is a physics simulation tool, similar to SketchyPhysics."

  # Attach some nice info.
  @extension.description = desc
  @extension.version     = VERSION
  @extension.copyright   = 'Anton Synytsia © 2014'
  @extension.creator     = 'Anton Synytsia (anton.synytsia@gmail.com)'

  # Register and load the extension on start-up.
  Sketchup.register_extension @extension, true

  class << self
    attr_reader :extension
  end

end if continue
