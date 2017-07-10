require 'MSPhysics.rb'

tload_me = true

# Load and verify AMS Library
begin
  require 'ams_Lib/main'
  tload_me = false if AMS::Lib::VERSION.to_f < 3.5
rescue LoadError
  tload_me = false
end

if tload_me
  dir = File.dirname(__FILE__)
  require File.join(dir, 'main')
else
  msg = "MSPhysics requires AMS Library, version 3.5.0 or later! This extension will not be loaded with the library not installed or outdated. Would you like to navigate to the library's download page?"
  tload_me = false
  if UI.messagebox(msg, MB_YESNO) == IDYES
    UI.openURL('http://sketchucation.com/forums/viewtopic.php?f=323&t=55067#p499835')
  end
end
