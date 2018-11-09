require 'sketchup.rb'
require 'extensions.rb'


# @since 1.0.0
module MSPhysics

  NAME         = 'MSPhysics'.freeze
  VERSION      = '1.1.0'.freeze
  RELEASE_DATE = 'January 1, 2019'.freeze

  # Create the extension.
  dir = ::File.expand_path(::File.dirname(__FILE__))
  @extension = ::SketchupExtension.new(NAME, ::File.join(dir, 'MSPhysics/main_entry'))

  # Attach some nice info.
  @extension.description = "A realtime physics simulator."
  @extension.version     = VERSION
  @extension.copyright   = '2014-2018, Anton Synytsia'
  @extension.creator     = 'Anton Synytsia'

  # Register and load the extension on start-up.
  ::Sketchup.register_extension(@extension, true)

  class << self

    # @!attribute [r] extension
    # Get MSPhysics extension.
    # @return [SketchupExtension]
    attr_reader :extension

  end # class << self
end # module MSPhysics
