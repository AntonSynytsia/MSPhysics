require 'sketchup.rb'
require 'extensions.rb'

tload_me = true

# Load and verify AMS Library
begin
  require 'ams_Lib/main'
  if AMS::Lib::VERSION.to_f < 3.4
    tload_me = false
  end
rescue LoadError
  tload_me = false
end
unless tload_me
  msg = "The AMS Window Settings extension requires AMS Library 3.4.0 or later! This extension will not load with the library not installed or outdated. Would you like to navigate to the library's download page?"
  tload_me = false
  if UI.messagebox(msg, MB_YESNO) == IDYES
    UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
  end
end

# Load the extension
if tload_me
  # @since 1.0.0
  module MSPhysics

    NAME         = 'MSPhysics'.freeze
    VERSION      = '0.9.9'.freeze
    RELEASE_DATE = 'December 22, 2016'.freeze

    # Create the extension.
    @extension = SketchupExtension.new(NAME, 'MSPhysics/main')

    # Attach some nice info.
    @extension.description = "A realtime physics simulation tool."
    @extension.version     = VERSION
    @extension.copyright   = 'Anton Synytsia © 2014-2016'
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
end # if tload_me
