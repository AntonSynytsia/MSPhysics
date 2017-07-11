require 'sketchup.rb'
require 'extensions.rb'

# @since 1.0.0
module MSPhysics

  NAME         = 'MSPhysics'.freeze
  VERSION      = '1.0.1'.freeze
  RELEASE_DATE = 'July 11, 2017'.freeze

  # Create the extension.
  @extension = ::SketchupExtension.new(NAME, 'MSPhysics/main_entry')

  # Attach some nice info.
  @extension.description = "A realtime physics simulation tool, similar to SketchyPhysics."
  @extension.version     = VERSION
  @extension.copyright   = 'MIT © 2014-2017, Anton Synytsia'
  @extension.creator     = 'Anton Synytsia (anton.synytsia@gmail.com)'

  # Register and load the extension on start-up.
  ::Sketchup.register_extension(@extension, true)

  class << self

    # @!attribute [r] extension
    # Get MSPhysics extension.
    # @return [SketchupExtension]
    attr_reader :extension

  end # class << self
end # module MSPhysics
