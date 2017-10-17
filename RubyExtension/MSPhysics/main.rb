require 'MSPhysics.rb'

# Ensure standard Set is loaded
if Sketchup.version.to_i >= 14
  Sketchup.require 'set'
end

# Require all files
ext_dir = File.dirname(__FILE__)
ext_dir.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18

ext_manager = AMS::ExtensionManager.new(ext_dir, MSPhysics::VERSION)
if AMS::IS_PLATFORM_WINDOWS
  ext_manager.add_optional_library('libFLAC-8')
  ext_manager.add_optional_library('libmikmod-2')
  ext_manager.add_optional_library('libmodplug-1')
  ext_manager.add_optional_library('libogg')
  ext_manager.add_optional_library('libvorbis')
  ext_manager.add_optional_library('libvorbisfile-3')
  ext_manager.add_required_library('SDL2')
  ext_manager.add_required_library('SDL2_mixer')
  ext_manager.add_optional_library('smpeg2')
  ext_manager.add_required_library('newton')
else
  ext_manager.add_required_library('libSDL2-2.0.0')
  ext_manager.add_required_library('libSDL2_mixer-2.0.0')
  ext_manager.add_required_library('libnewton')
end
ext_manager.add_c_extension('msp_lib')
ext_manager.add_ruby_no_require('main')
ext_manager.add_ruby_no_require('main_entry')
ext_manager.add_ruby('entity')
ext_manager.add_ruby('collision')
ext_manager.add_ruby('contact')
ext_manager.add_ruby('hit')
ext_manager.add_ruby('script_exception')
ext_manager.add_ruby('common_context')
ext_manager.add_ruby('controller_context')
ext_manager.add_ruby('body_context')
ext_manager.add_ruby('body')
ext_manager.add_ruby('world')
ext_manager.add_ruby('material')
ext_manager.add_ruby('materials')
ext_manager.add_ruby('joint')
ext_manager.add_ruby('gear')
ext_manager.add_ruby('joint_hinge')
ext_manager.add_ruby('joint_motor')
ext_manager.add_ruby('joint_servo')
ext_manager.add_ruby('joint_corkscrew')
ext_manager.add_ruby('joint_ball_and_socket')
ext_manager.add_ruby('joint_universal')
ext_manager.add_ruby('joint_slider')
ext_manager.add_ruby('joint_piston')
ext_manager.add_ruby('joint_spring')
ext_manager.add_ruby('joint_up_vector')
ext_manager.add_ruby('joint_fixed')
ext_manager.add_ruby('joint_curvy_slider')
ext_manager.add_ruby('joint_curvy_piston')
ext_manager.add_ruby('joint_plane')
ext_manager.add_ruby('joint_point_to_point')
ext_manager.add_ruby('simulation')
ext_manager.add_ruby('settings')
ext_manager.add_ruby('dialog')
ext_manager.add_ruby('control_panel')
ext_manager.add_ruby('joint_tool')
ext_manager.add_ruby('joint_connection_tool')
ext_manager.add_ruby('gear_connection_tool')
ext_manager.add_ruby('replay')
ext_manager.add_ruby('scene_data')
ext_manager.require_all
ext_manager.clean_up(true)

# @since 1.0.0
module MSPhysics

  EPSILON = 1.0e-6

  SHAPES = {
    0 => 'Null',
    1 => 'Box',
    2 => 'Sphere',
    3 => 'Cone',
    4 => 'Cylinder',
    5 => 'Chamfer Cylinder',
    6 => 'Capsule',
    7 => 'Convex Hull',
    8 => 'Compound',
    9 => 'Static Mesh'
  }.freeze

  SHAPE_DIRS = {
    0 => 'X-axis',
    1 => 'Y-axis',
    2 => 'Z-axis'
  }.freeze

  JOINT_ID_TO_NAME = {
    0  => 'None',
    1  => 'Hinge',
    2  => 'Motor',
    3  => 'Servo',
    4  => 'Slider',
    5  => 'Piston',
    6  => 'UpVector',
    7  => 'Spring',
    8  => 'Corkscrew',
    9  => 'BallAndSocket',
    10 => 'Universal',
    11 => 'Fixed',
    12 => 'CurvySlider',
    13 => 'CurvyPiston',
    14 => 'Plane',
    15 => 'PointToPoint'
  }.freeze

  JOINT_ID_TO_FILE_NAME = {
    0  => 'none',
    1  => 'hinge',
    2  => 'motor',
    3  => 'servo',
    4  => 'slider',
    5  => 'piston',
    6  => 'up_vector',
    7  => 'spring',
    8  => 'corkscrew',
    9  => 'ball_and_socket',
    10 => 'universal',
    11 => 'fixed',
    12 => 'curvy_slider',
    13 => 'curvy_piston',
    14 => 'plane',
    15 => 'point_to_point'
  }.freeze

  JOINT_NAMES = JOINT_ID_TO_NAME.values.freeze
  JOINT_FILE_NAMES = JOINT_ID_TO_FILE_NAME.values.freeze

  MASS_CONTROLS = {
    1 => 'by Density',
    2 => 'by Mass'
  }.freeze

  MAGNET_MODES = {
    1 => 'Mode 1',
    2 => 'Mode 2'
  }.freeze

  BODY_TYPES = {
    0 => 'Dynamic',
    1 => 'Kinematic'
  }.freeze

  BODY_STATES = [
    'Ignore',
    'Static',
    'Frozen',
    'Magnetic',
    'Collidable',
    'Auto Sleep',
    'Continuous Collision',
    'Enable Friction',
    'Enable Script',
    'Enable Gravity',
    'Enable Thruster',
    'Enable Emitter',
    'Thruster Lock Axes',
    'Emitter Lock Axes',
    'Emitter Recoil'
  ].freeze

  BODY_CON_STATES = [
    'Clear Ignore Flag',
    'Movable',
    'Non-Frozen',
    'Non-Magnetic',
    'Non-Collidable',
    'No Auto Sleep',
    'No Continuous Collision',
    'Disable Friction',
    'Disable Script',
    'Disable Gravity',
    'Disable Thruster',
    'Disable Emitter',
    'No Thruster Lock Axes',
    'No Emitter Lock Axes',
    "No Emitter Recoil"
  ].freeze

  # Coefficients are defined from material to material contacts. Material to
  # another material coefficients are automatically calculated by averaging out
  # the coefficients of both.
  # Many coefficient values are estimated, averaged up, made up, and not accurate.
  # name => [ density (kg/m^3), static friction, kinetic friction, elasticity, softness ]
  MATERIALS = {
    'Aluminium'         => [2700, 0.42, 0.34, 0.30, 0.01],
    'Brass'             => [8730, 0.35, 0.24, 0.40, 0.01],
    'Brick'             => [1920, 0.60, 0.55, 0.20, 0.01],
    'Bronze'            => [8200, 0.36, 0.27, 0.60, 0.01],
    'Cadnium'           => [8640, 0.79, 0.46, 0.60, 0.01],
    'Cast Iron'         => [7300, 0.51, 0.40, 0.60, 0.01],
    'Chromium'          => [7190, 0.46, 0.30, 0.60, 0.01],
    'Cobalt'            => [8746, 0.56, 0.40, 0.60, 0.01],
    'Concrete'          => [2400, 0.62, 0.56, 0.20, 0.01],
    'Glass'             => [2800, 0.90, 0.72, 0.4, 0.01],
    'Copper'            => [8940, 0.55, 0.40, 0.60, 0.01],
    'Gold'              => [19320, 0.49, 0.39, 0.60, 0.01],
    'Graphite'          => [2230, 0.18, 0.14, 0.10, 0.01],
    'Ice'               => [916, 0.01, 0.01, 0.10, 0.01],
    'Nickel'            => [8900, 0.53, 0.44, 0.60, 0.01],
    'Plastic'           => [1000, 0.35, 0.30, 0.69, 0.01],
    'Rubber'            => [1100, 1.16, 1.16, 0.01, 0.01],
    'Silver'            => [10500, 0.50, 0.40, 0.60, 0.01],
    'Steel'             => [8050, 0.31, 0.23, 0.60, 0.01],
    'Teflon'            => [2170, 0.04, 0.03, 0.10, 0.01],
    'Titanium'          => [4500, 0.36, 0.30, 0.40, 0.01],
    'Tungsten Carbide'  => [19600, 0.22, 0.15, 0.40, 0.01],
    'Wood'              => [700, 0.50, 0.25, 0.40, 0.01],
    'Zinc'              => [7000, 0.60, 0.50, 0.60, 0.01]
  }.freeze

  DEFAULT_SIMULATION_SETTINGS = {
    :solver_model               => 16,      # 1 - 64
    :joint_algorithm            => 2,       # 0 - accurate; 1 - don't use; 2 - fast.
    :update_rate                => 2,       # 1 - 100
    :update_timestep            => 1/60.0,  # 1/30 - 1/1200, in seconds
    :gravity                    => -9.801,  # in m/s/s along Z-axis
    :material_thickness         => 0.002,   # thickness b/w 0.0 and 1/32 meters
    :contact_merge_tolerance    => 0.005,   # 0.001+
    :world_scale                => 9,       # 1 - 100
    :continuous_collision_check => false,   # boolean
    :full_screen_mode           => false,   # boolean
    :ignore_hidden_instances    => false,   # boolean
    :game_mode                  => false,   # boolean
    :hide_joint_layer           => false,   # boolean
    :undo_on_end                => false,   # boolean
    :key_nav_state              => 0,       # 0 - off; 1 - normal; 2 - upright.
    :key_nav_velocity           => 40.0,    # in m/s
    :key_nav_omega              => 1.0,     # in rad/s
    :key_nav_atime              => 0.15,    # in seconds
    :animate_scenes_state       => 0,       # 0 - off; 1 - one way; 2 - repeat forth and back; 3 - loop around.
    :animate_scenes_delay       => 0.0,     # in seconds
    :animate_scenes_reversed    => false    # boolean
  }.freeze

  DEFAULT_BODY_SETTINGS = {
    :type                   => 0,
    :shape_id               => 8,
    :shape_dir              => 0,
    :material_name          => 'Default',
    :mass_control           => 1,
    :density                => 700,
    :mass                   => 1.0,
    :static_friction        => 0.90,
    :kinetic_friction       => 0.50,
    :enable_friction        => true,
    :elasticity             => 0.40,
    :softness               => 0.10,
    :linear_damping         => 0.001,
    :angular_damping        => 0.001,
    :magnet_mode            => 1,
    :magnet_force           => 0.00,
    :magnet_range           => 0.00,
    :magnet_strength        => 0.00,
    :magnetic               => false,
    :enable_script          => true,
    :static                 => false,
    :frozen                 => false,
    :collidable             => true,
    :auto_sleep             => true,
    :continuous_collision   => false,
    :thruster_lock_axes     => true,
    :enable_thruster        => true,
    :emitter_lock_axes      => true,
    :emitter_recoil         => false,
    :emitter_rate           => 0.1,
    :emitter_lifetime       => 1.0,
    :emitter_delay          => 0.0,
    :enable_emitter         => true,
    :enable_gravity         => true
  }.freeze

  DEFAULT_BUOYANCY_PLANE_SETTINGS = {
    :material_name          => 'MSPhysics Buoyancy',
    :density                => 997.04,
    :viscosity              => 0.015,
    :current_x              => 0,
    :current_y              => 0,
    :current_z              => 0,
    :plane_size             => 10000,
    :color                  => Sketchup::Color.new(40,60,220),
    :alpha                  => 0.7
  }.freeze

  CURSORS = {
    :select                 => 0,
    :select_plus            => 0,
    :select_minus           => 0,
    :select_plus_minus      => 0,
    :hand                   => 0,
    :grab                   => 0,
    :click                  => 0,
    :target                 => 0
  }

  CURSOR_ORIGINS = {
    :select                 => [2,5],
    :select_plus            => [2,5],
    :select_minus           => [2,5],
    :select_plus_minus      => [2,5],
    :hand                   => [2,2],
    :grab                   => [2,2],
    :click                  => [2,2],
    :target                 => [15,15]
  }.freeze

  EMBEDDED_MUSIC_FORMATS = %w(wav aiff riff ogg voc flac mod it xm s3m m4a 669 med mp3 mp2 mid pat).freeze
  EMBEDDED_SOUND_FORMATS = %w(wav aiff riff ogg voc mp3 mp2).freeze

  JOYSTICK1_AXES = %w(leftx lefty leftz rightx righty rightz).freeze
  JOYSTICK2_AXES = %w(leftx lefty rightx righty).freeze
  JOYSTICK1_BUTTONS = %w(a b x y lb rb back start leftb rightb).freeze
  JOYSTICK2_BUTTONS = %w(x a b y lb rb lt rt back start leftb rightb).freeze

  DEFAULT_JOINT_SCALE = 1.0

  WATERMARK_COLOR = Sketchup::Color.new(126, 42, 168)

  DEFAULT_ANGLE_UNITS = 'deg'.freeze
  DEFAULT_POSITION_UNITS = 'cm'.freeze

  SCRIPT_NAME = 'MSPhysicsScript'.freeze
  CONTROLLER_NAME = 'MSPhysicsController'.freeze

  VIEW_UPDATE_TIMESTEP = 1.0 / 60.0
  VIEW_UPDATE_TIMESTEP_INV = 1.0 / VIEW_UPDATE_TIMESTEP

  POSITION_CONVERSION = {
    'mm' => 0.001,
    'cm' => 0.01,
    'dm' => 0.1,
    'm'  => 1.0,
    'in' => 0.0254,
    'ft' => 0.3048,
    'yd' => 0.9144
  }

  ANGLE_CONVERSION = {
    'deg' => 1.degrees.to_f,
    'rad' => 1.0
  }

  class << self

    # Get common attribute value from a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @param [Object] default_value The value to return if the attribute value
    #   is not found.
    # @return [Object] A common attribute value or nil if one of the entity
    #   attributes is different from another.
    def get_attribute(ents, handle, name, default_value = nil)
      vset = false
      value = nil
      ents.each { |e|
        v = e.get_attribute(handle, name, default_value)
        if vset
          return nil if v != value
        else
          value = v
          vset = true
        end
      }
      return value
    end

    # Assign attribute value to a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @param [Object] value Attribute value.
    # @return [void]
    # @note Manually wrap the operation.
    def set_attribute(ents, handle, name, value)
      ents.each { |e|
        e.set_attribute(handle, name, value)
      }
    end

    # Delete attribute value from a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @return [void]
    # @note Manually wrap the operation.
    def delete_attribute(ents, handle, name = nil)
      ents.each { |e|
        name ? e.delete_attribute(handle, name) : e.delete_attribute(handle)
      }
    end

    # Delete MSPhysics attributes from a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @return [void]
    # @note Manually wrap the operation.
    def delete_attributes(ents)
      ents.each { |e|
        e.delete_attribute('MSPhysics') if e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body'
        e.delete_attribute('MSPhysics Body')
        #~ e.delete_attribute('MSPhysics Joint')
        dict = e.attribute_dictionary('MSPhysics Joint')
        if dict
          dict.keys.each { k|
            e.delete_attribute('MSPhysics Joint', k) if k != 'Type'
          }
        end
        e.delete_attribute('MSPhysics Script')
        e.delete_attribute('MSPhysics Buoyancy Plane')
      }
    end

    # Delete MSPhysics attributes from all entities.
    # @return [void]
    # @note Manually wrap the operation.
    def delete_all_attributes
      model = Sketchup.active_model
      model.definitions.each { |d|
        delete_attributes(d.instances)
      }
      dicts = model.attribute_dictionaries
      if dicts
        dicts.delete('MSPhysics')
        dicts.delete('MSPhysics Sounds')
        dicts.delete('MSPhysics Replay')
      end
    end

    # Get physical joint scale.
    # @return [Numeric]
    def get_joint_scale
      Sketchup.active_model.get_attribute('MSPhysics', 'Joint Scale', DEFAULT_JOINT_SCALE).to_f
    end

    # Set physical joint scale.
    # @param [Numeric] scale A value between 0.01 and 100.00
    # @param [Boolean] wrap_op Whether to wrap in operation.
    # @return [Numeric]
    def set_joint_scale(scale, wrap_op = true)
      scale = AMS.clamp(scale.to_f, 0.01, 100.00)
      model = Sketchup.active_model
      if wrap_op
        op = 'MSPhysics - Scale Joints'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
      end
      model.set_attribute('MSPhysics', 'Joint Scale', scale)
      count = scale_joints(scale)
      model.commit_operation if wrap_op
      count
    end

    # Get version of the Newton Dynamics physics SDK.
    # @return [String]
    def newton_version
      MSPhysics::Newton.get_version
    end

    # Get float size of the Newton Dynamics physics SDK.
    # @return [Fixnum]
    def newton_float_size
      MSPhysics::Newton.get_float_size
    end

    # Get memory used by the Newton Dynamics physics SDK at the current time.
    # @return [Fixnum]
    def newton_memory_used
      MSPhysics::Newton.get_memory_used
    end

    # Create a watermark text.
    # @param [Fixnum] x X position on screen.
    # @param [Fixnum] y Y position on screen.
    # @param [String] text Watermark text.
    # @param [String] name Watermark name.
    # @param [String] component Watermark component.
    # @return [Sketchup::Text, nil] A text object if successful.
    # @note Manually wrap the operation.
    def add_watermark_text(x, y, text, name = 'Watermark', component = 'version_text.skp')
      ext_dir = File.dirname(__FILE__)
      ext_dir.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
      path = File.join(ext_dir, "models/#{component}")
      return unless File.exists?(path)
      model = Sketchup.active_model
      view = model.active_view
      cd = model.definitions.load(path)
      ray = view.pickray(x,y)
      loc = ray[0]+ray[1]
      ci = model.entities.add_instance(cd, Geom::Transformation.new(loc))
      tt = ci.explode.find { |e| e.is_a?(Sketchup::Text) }
      tt.set_text(text)
      tt.set_attribute('MSPhysics', 'Name', name.to_s)
      mat = model.materials['MSPWatermarkText']
      unless mat
        mat = model.materials.add('MSPWatermarkText')
        mat.color = WATERMARK_COLOR
      end
      tt.material = mat
      layer = model.layers['MSPWatermarkText']
      unless layer
        layer = model.layers.add('MSPWatermarkText')
        layer.color = WATERMARK_COLOR if Sketchup.version.to_i > 13
      end
      tt.layer = layer
      tt
    end

    # Create a watermark text without material or layer.
    # @param [Fixnum] x X position on screen.
    # @param [Fixnum] y Y position on screen.
    # @param [String] text Watermark text.
    # @param [String] name Watermark name.
    # @param [String] component Watermark component.
    # @return [Sketchup::Text, nil] A text object if successful.
    # @note Manually wrap the operation.
    def add_watermark_text2(x, y, text, name = 'Watermark', component = 'version_text.skp')
      ext_dir = File.dirname(__FILE__)
      ext_dir.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
      path = File.join(ext_dir, "models/#{component}")
      return unless File.exists?(path)
      model = Sketchup.active_model
      view = model.active_view
      cd = model.definitions.load(path)
      ray = view.pickray(x,y)
      loc = ray[0]+ray[1]
      ci = model.entities.add_instance(cd, Geom::Transformation.new(loc))
      tt = ci.explode.find { |e| e.is_a?(Sketchup::Text) }
      tt.set_text(text)
      tt.set_attribute('MSPhysics', 'Name', name.to_s)
      tt
    end

    # Get watermark text by name.
    # @param [String] name
    # @return [Sketchup::Text, nil] A text object if the text exists.
    def find_watermark_text_by_name(name)
      Sketchup.active_model.entities.each { |e|
        return e if e.is_a?(Sketchup::Text) && e.get_attribute('MSPhysics', 'Name') == name.to_s
      }
      nil
    end

    # Get all watermark texts.
    # @return [Array<Sketchup::Text>]
    def all_watermark_texts
      texts = []
      Sketchup.active_model.entities.each { |e|
        texts << e if e.is_a?(Sketchup::Text) && e.get_attribute('MSPhysics', 'Name') != nil
      }
      texts
    end

    # Displays MSPhysics version text.
    # @param [Boolean] wrap_op Whether to wrap in operation.
    # @return [Boolean] Whether the user chose to continue running.
    def verify_version(wrap_op = true)
      model = Sketchup.active_model
      mvers = model.get_attribute('MSPhysics', 'Version')
      cvers = MSPhysics::VERSION
      cver = cvers.gsub(/\./, '').to_i
      bmake_compatible = false
      if mvers.is_a?(String)
        mver = mvers.gsub(/\./, '').to_i
        if mver == cver
          return true
        elsif mver < cver
          bmake_compatible = true
        elsif mver > cver
          return false if ::UI.messagebox("This model was created with MSPhysics #{mvers}. Your version is #{cvers}. Using an outdated version may result in an improper behaviour! Would you like to continue?", MB_YESNO) == IDNO
        end
      end
      te = nil
      model.entities.each { |e|
        if e.is_a?(Sketchup::Text) && e.get_attribute('MSPhysics', 'Name') == 'Version Text'
          te = e
          break
        end
      }
      if wrap_op
        op = 'MSPhysics - Utilizing Version'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
      end
      make_compatible(false) if bmake_compatible
      if te
        te.set_text("Created with MSPhysics #{cvers}\n#{te.text}")
      else
        te = add_watermark_text(10, 10, "Created with MSPhysics #{cvers}", 'Version Text')
      end
      model.set_attribute('MSPhysics', 'Version', cvers)
      model.commit_operation if wrap_op
      true
    end

    # Make model of previous MSPhysics version compatible with this version.
    # @param [Boolean] wrap_op Whether to wrap in operation.
    # @return [void]
    def make_compatible(wrap_op = true)
      model = Sketchup.active_model
      bdict = 'MSPhysics Body'
      # Start operation
      if wrap_op
        op = 'MSPhysics - Making Compatible'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
      end
      # Make changes
      update_timestep = model.get_attribute('MSPhysics', 'Update Timestep', MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_timestep]).to_f
      model.definitions.each { |d|
        d.instances.each { |i|
          # Remove unused attributes
          i.delete_attribute(bdict, 'Connect Closest Joints')
          # Replace weight control
          weight_control = i.get_attribute(bdict, 'Weight Control')
          if weight_control
            if i.get_attribute(bdict, 'Mass Control').nil?
              i.set_attribute(bdict, 'Mass Control', weight_control == 'Mass' ? 2 : 1)
            end
            i.delete_attribute(bdict, 'Weight Control')
          end
          # Replace dynamic friction
          dynamic_friction = i.get_attribute(bdict, 'Dynamic Friction')
          if dynamic_friction
            if i.get_attribute(bdict, 'Kinetic Friction').nil?
              i.set_attribute(bdict, 'Kinetic Friction', dynamic_friction)
            end
            i.delete_attribute(bdict, 'Dynamic Friction')
          end
          # Update emitter to use seconds
          attr = i.get_attribute(bdict, 'Emitter Rate')
          i.set_attribute(bdict, 'Emitter Rate', attr.to_f * update_timestep) if attr
          attr = i.get_attribute(bdict, 'Emitter Delay')
          i.set_attribute(bdict, 'Emitter Delay', attr.to_f * update_timestep) if attr
          attr = i.get_attribute(bdict, 'Emitter Lifetime')
          i.set_attribute(bdict, 'Emitter Lifetime', attr.to_f * update_timestep) if attr
          # Update thruster lock axis to axes
          attr = i.get_attribute(bdict, 'Thruster Lock Axes')
          unless attr.nil?
            i.set_attribute(bdict, 'Thruster Lock Axes', attr)
            i.delete_attribute(bdict, 'Thruster Lock Axis')
          end
          # Update emitter lock axis to axes
          attr = i.get_attribute(bdict, 'Emitter Lock Axes')
          unless attr.nil?
            i.set_attribute(bdict, 'Emitter Lock Axes', attr)
            i.delete_attribute(bdict, 'Emitter Lock Axis')
          end
          # Replace Compound from CD shapes with default
          shape = i.get_attribute(bdict, 'Shape')
          if shape == 'Compound from CD'
            i.delete_attribute(bdict, 'Shape')
            shape = nil
          end
          # Replace shape with shape id to conserve size
          if shape.is_a?(String)
            found = false
            MSPhysics::SHAPES.each { |k, v|
              if shape == v
                i.set_attribute(bdict, 'Shape', k)
                found = true
                break
              end
            }
            i.delete_attribute(bdict, 'Shape') unless found
          end
        }
      }
      # Replace exact solver with iterative 4 passes and accurate joint solver
      if model.get_attribute('MSPhysics', 'Solver Model') == 0
        model.set_attribute('MSPhysics', 'Solver Model', 4)
        unless model.get_attribute('MSPhysics', 'Joint Algorithm')
          model.set_attribute('MSPhysics', 'Joint Algorithm', 0)
        end
      end
      # Use fast joint algorithm unless set
      unless model.get_attribute('MSPhysics', 'Joint Algorithm')
        model.set_attribute('MSPhysics', 'Joint Algorithm', 2)
      end
      # End operation
      model.commit_operation if wrap_op
    end

    # Create copy of a camera object.
    # @param [Sketchup::Camera] camera
    # @return [Sketchup::Camera]
    def duplicate_camera(camera)
      c = Sketchup::Camera.new(camera.eye, camera.target, camera.up)
      c.aspect_ratio = camera.aspect_ratio
      c.perspective = camera.perspective?
      if camera.perspective?
        c.focal_length = camera.focal_length
        c.fov = camera.fov
        c.image_width = camera.image_width
      else
        c.height = camera.height
      end
      c.description = camera.description
      return c
    end

    private

    def scale_joints(scale, entities = Sketchup.active_model.entities)
      model = Sketchup.active_model
      st = Geom::Transformation.scaling(scale)
      count = 0
      entities.each { |e|
        next if !e.is_a?(Sketchup::Group) && !e.is_a?(Sketchup::ComponentInstance)
        if e.get_attribute('MSPhysics', 'Type', nil) == 'Joint'
          jt = e.get_attribute('MSPhysics Joint', 'Type')
          if jt == 'CurvySlider' || jt == 'CurvyPiston'
            ents = e.is_a?(Sketchup::ComponentInstance) ? e.definition.entities : e.entities
            ents.each { |se|
              if se.is_a?(Sketchup::ComponentInstance) && se.definition.name =~ /curvy_slider|curvy_piston/
                tra = se.transformation
                t = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, tra.origin)
                se.transformation = t * st
                count += 1
                break
              end
            }
          else
            tra = e.transformation
            t = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, tra.origin)
            e.transformation = t * st
            count += 1
          end
        else
          ents = e.is_a?(Sketchup::ComponentInstance) ? e.definition.entities : e.entities
          count += scale_joints(scale, ents)
        end
      }
      count
    end

  end # class << self
end # module MSPhysics

unless file_loaded?(__FILE__)
  # Setup audio
  MSPhysics::SDL.init(MSPhysics::SDL::INIT_AUDIO | MSPhysics::SDL::INIT_JOYSTICK | MSPhysics::SDL::INIT_EVENTS)
  MSPhysics::Mixer.init(MSPhysics::Mixer::INIT_SUPPORTED)
  MSPhysics::Mixer.open_audio(22050, MSPhysics::Mixer::DEFAULT_FORMAT, 2, 1024)
  MSPhysics::Mixer.allocate_channels(16)
  Kernel.at_exit {
    MSPhysics::Music.destroy_all
    MSPhysics::Sound.destroy_all
    MSPhysics::Mixer.close_audio
    MSPhysics::Mixer.quit
    MSPhysics::SDL.quit
  }

  # Create cursors
  path = File.join(ext_dir, 'images/cursors')
  MSPhysics::CURSORS.keys.each { |name|
    pt = MSPhysics::CURSOR_ORIGINS[name]
    MSPhysics::CURSORS[name] = UI.create_cursor(File.join(path, name.to_s + '.png'), pt[0], pt[1])
  }

  # Initialize stuff
  MSPhysics::Settings.init
  MSPhysics::Dialog.init
  MSPhysics::Replay.init

  # Create some contact materials
  MSPhysics::MATERIALS.each { |name, data|
    mat = MSPhysics::Material.new(name, data[0], data[1], data[2], data[3], data[4])
    MSPhysics::Materials.add(mat)
  }

  # Choose icons path
  use_orig_icons = Sketchup.read_default('MSPhysics', 'Use Original Icons', false) ? true : false
  if Sketchup.version.to_i > 13 && !use_orig_icons
    simg_path = 'images/icons/'
    limg_path = 'images/icons/'
  else
    simg_path = 'images/small/'
    limg_path = 'images/large/'
  end

  # Create MSPhysics Simulation Toolbar
  sim = MSPhysics::Simulation
  sim_toolbar = UI::Toolbar.new('MSPhysics')

  cmd = UI::Command.new('Toggle UI') {
    MSPhysics::Dialog.open(!MSPhysics::Dialog.open?)
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Dialog.open? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.menu_text = cmd.tooltip = 'Toggle UI'
  cmd.status_bar_text = 'Show/hide MSPhysics UI.'
  cmd.small_icon = simg_path + 'ui.png'
  cmd.large_icon = limg_path + 'ui.png'
  sim_toolbar.add_item(cmd)

  cmd = UI::Command.new('Toggle Play') {
    if sim.active?
      sim.instance.toggle_play
    else
      sim.start(false) if MSPhysics.verify_version
    end
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    if sim.active?
      if sim.instance.started_from_selection?
        MF_GRAYED
      else
        sim.instance.playing? ? MF_CHECKED : MF_UNCHECKED
      end
    else
      MF_ENABLED
    end
  }
  cmd.menu_text = cmd.tooltip = 'Toggle Play'
  cmd.status_bar_text = 'Play/Pause simulation. This option starts simulation with all groups considered part of simulation.'
  cmd.small_icon = simg_path + 'toggle_play.png'
  cmd.large_icon = limg_path + 'toggle_play.png'
  sim_toolbar.add_item(cmd)

  cmd = UI::Command.new('Reset') {
    sim.reset
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    sim.active? ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Reset'
  cmd.status_bar_text = 'End simulation, reset positions, and erase copied/emitted bodies.'
  cmd.small_icon = simg_path + 'reset.png'
  cmd.large_icon = limg_path + 'reset.png'
  sim_toolbar.add_item(cmd)

  sim_toolbar.add_separator

  cmd = UI::Command.new('Toggle Play 2') {
    if  sim.active?
      sim.instance.toggle_play
    else
      sim.start(true) if MSPhysics.verify_version
    end
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    if sim.active?
      if sim.instance.started_from_selection?
        sim.instance.playing? ? MF_CHECKED : MF_UNCHECKED
      else
        MF_GRAYED
      end
    else
      MF_ENABLED
    end
  }
  cmd.menu_text = cmd.tooltip = 'Toggle Play 2'
  cmd.status_bar_text = 'Play/Pause simulation. This option starts simulation from selected groups/components. All non-selected instances will act as stationary bodies.'
  cmd.small_icon = simg_path + 'toggle_play2.png'
  cmd.large_icon = limg_path + 'toggle_play2.png'
  sim_toolbar.add_item(cmd)

  cmd = UI::Command.new('Stop') {
    sim.instance.reset_positions_on_end = false
    sim.instance.erase_instances_on_end = false
    sim.instance.reset_camera_on_end = false
    sim.reset
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    sim.active? ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Stop'
  cmd.status_bar_text = 'End simulation without resetting positions nor erasing emitted bodies.'
  cmd.small_icon = simg_path + 'stop.png'
  cmd.large_icon = limg_path + 'stop.png'
  sim_toolbar.add_item(cmd)

  sim_toolbar.show


  # Create MSPhysics Joints Toolbar
  joints_toolbar = UI::Toolbar.new('MSPhysics Joints')

  cmd = UI::Command.new('cmd') {
    tool = MSPhysics::JointConnectionTool
    tool.active? ? tool.deactivate : tool.activate
  }
  cmd.menu_text = cmd.tooltip = 'Joint Connection Tool'
  cmd.status_bar_text = 'Activate/Deactivate joint connection tool.'
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::JointConnectionTool.active? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = simg_path + 'toggle_connect.png'
  cmd.large_icon = limg_path + 'toggle_connect.png'
  joints_toolbar.add_item(cmd)

  cmd = UI::Command.new('Scale MSPhysics Joints') {
    scale = MSPhysics.get_joint_scale
    prompts = ['Scale']
    defaults = [sprintf("%0.2f", scale)]
    list = ['0.05|0.10|0.25|0.50|0.75|1.00|1.25|1.50|1.75|2.00|3.00|4.00|5.00|10.00']
    input = UI.inputbox(prompts, defaults, list, 'Scale MSPhysics Joints')
    next unless input
    scale = input[0].to_f
    count = MSPhysics.set_joint_scale(scale, true)
    UI.messagebox("Edited scale of #{count} MSPhysics joint(s).")
  }
  cmd.set_validation_proc {
    Sketchup.active_model ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Edit Joints Scale'
  cmd.status_bar_text = 'Change scale of MSPhysics joints.'
  cmd.small_icon = simg_path + 'scale_joints.png'
  cmd.large_icon = limg_path + 'scale_joints.png'
  joints_toolbar.add_item(cmd)

  joints_toolbar.add_separator

  MSPhysics::JOINT_ID_TO_NAME.each { |id, name|
    next if id < 1 || id > 14
    jt = nil
    cmd = UI::Command.new('cmd') {
      jt = MSPhysics::JointTool.new(id)
    }
    cmd.set_validation_proc {
      next MF_GRAYED unless Sketchup.active_model
      jt != nil && jt.active? ? MF_CHECKED : MF_UNCHECKED
    }
    cmd.menu_text = cmd.tooltip = name
    cmd.status_bar_text = "Add #{name} joint."
    cmd.small_icon = simg_path + MSPhysics::JOINT_ID_TO_FILE_NAME[id] + '.png'
    cmd.large_icon = limg_path + MSPhysics::JOINT_ID_TO_FILE_NAME[id] + '.png'
    joints_toolbar.add_item(cmd)
  }

  joints_toolbar.show


  # Create Replay Toolbar
  replay_toolbar = UI::Toolbar.new('MSPhysics Replay')

  cmd = UI::Command.new('Toggle Record') {
    MSPhysics::Replay.record_enabled = !MSPhysics::Replay.record_enabled?
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.record_enabled? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.menu_text = cmd.tooltip = 'Toggle Record'
  cmd.status_bar_text = 'Enable/disable replay animation recording.'
  cmd.small_icon = simg_path + 'replay_record.png'
  cmd.large_icon = limg_path + 'replay_record.png'
  replay_toolbar.add_item(cmd)

  cmd = UI::Command.new('Toggle Camera Replay') {
    state = !MSPhysics::Replay.camera_record_enabled?
    MSPhysics::Replay.camera_record_enabled = state
    MSPhysics::Replay.camera_replay_enabled = state
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.camera_record_enabled? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.menu_text = cmd.tooltip = 'Toggle Camera Replay'
  cmd.status_bar_text = 'Enable/disable camera replay.'
  cmd.small_icon = simg_path + 'replay_camera.png'
  cmd.large_icon = limg_path + 'replay_camera.png'
  replay_toolbar.add_item(cmd)

  cmd = UI::Command.new('Play') {
    if MSPhysics::Replay.active?
      if MSPhysics::Replay.paused? || MSPhysics::Replay.reversed?
        MSPhysics::Replay.reversed = false
        MSPhysics::Replay.play
      else
        MSPhysics::Replay.pause
      end
    else
      MSPhysics::Replay.reversed = false
      MSPhysics::Replay.start
      MSPhysics::Replay.play
    end
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    next MF_GRAYED unless MSPhysics::Replay.active_data_valid?
    if MSPhysics::Replay.active?
      MSPhysics::Replay.playing? && !MSPhysics::Replay.reversed? ? MF_CHECKED : MF_UNCHECKED
    else
      MF_ENABLED
    end
  }
  cmd.menu_text = cmd.tooltip = 'Play'
  cmd.status_bar_text = 'Play replay animation forward.'
  cmd.small_icon = simg_path + 'replay_play.png'
  cmd.large_icon = limg_path + 'replay_play.png'
  replay_toolbar.add_item(cmd)

  cmd = UI::Command.new('Reverse') {
    if MSPhysics::Replay.active?
      if MSPhysics::Replay.paused? || !MSPhysics::Replay.reversed?
        MSPhysics::Replay.reversed = true
        MSPhysics::Replay.play
      else
        MSPhysics::Replay.pause
      end
    else
      MSPhysics::Replay.reversed = true
      MSPhysics::Replay.start
      MSPhysics::Replay.play
    end
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    next MF_GRAYED unless MSPhysics::Replay.active_data_valid?
    if MSPhysics::Replay.active?
      MSPhysics::Replay.playing? && MSPhysics::Replay.reversed? ? MF_CHECKED : MF_UNCHECKED
    else
      MF_ENABLED
    end
  }
  cmd.menu_text = cmd.tooltip = 'Reverse'
  cmd.status_bar_text = 'Play replay animation backward.'
  cmd.small_icon = simg_path + 'replay_reverse.png'
  cmd.large_icon = limg_path + 'replay_reverse.png'
  replay_toolbar.add_item(cmd)

  cmd = UI::Command.new('Pause') {
    if MSPhysics::Replay.paused?
      MSPhysics::Replay.play
    else
      MSPhysics::Replay.pause
    end
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    next MF_GRAYED unless MSPhysics::Replay.active_data_valid?
    if MSPhysics::Replay.active?
      MSPhysics::Replay.paused? ? MF_CHECKED : MF_UNCHECKED
    else
      MF_GRAYED
    end
  }
  cmd.menu_text = cmd.tooltip = 'Pause'
  cmd.status_bar_text = 'Pause replay animation.'
  cmd.small_icon = simg_path + 'replay_pause.png'
  cmd.large_icon = limg_path + 'replay_pause.png'
  replay_toolbar.add_item(cmd)

  cmd = UI::Command.new('Reset') {
    MSPhysics::Replay.reset
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active? ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Reset'
  cmd.status_bar_text = 'Stop replay animation and reset positions.'
  cmd.small_icon = simg_path + 'replay_reset.png'
  cmd.large_icon = limg_path + 'replay_reset.png'
  replay_toolbar.add_item(cmd)

  cmd = UI::Command.new('Stop') {
    MSPhysics::Replay.stop
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active? ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Stop'
  cmd.status_bar_text = 'Stop replay animation, but avoid resetting positions.'
  cmd.small_icon = simg_path + 'replay_stop.png'
  cmd.large_icon = limg_path + 'replay_stop.png'
  replay_toolbar.add_item(cmd)

  inc_spd_cmd = UI::Command.new('Increase Speed') {
    v = MSPhysics::Replay.speed
    MSPhysics::Replay.speed = v + (v < 10.0 ? (v < 2.0 ? (v < 0.1 ? 0.01 : 0.1) : 1.0) : 10.0)
  }
  inc_spd_cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    inc_spd_cmd.status_bar_text = "Increase replay animation speed.    Speed: #{sprintf("%.2f", MSPhysics::Replay.speed)}"
    MF_ENABLED
  }
  inc_spd_cmd.menu_text = inc_spd_cmd.tooltip = 'Increase Speed'
  inc_spd_cmd.status_bar_text = "Increase replay animation speed.    Speed: #{sprintf("%.2f", MSPhysics::Replay.speed)}"
  inc_spd_cmd.small_icon = simg_path + 'replay_increase_speed.png'
  inc_spd_cmd.large_icon = limg_path + 'replay_increase_speed.png'
  replay_toolbar.add_item(inc_spd_cmd)

  dec_spd_cmd = UI::Command.new('Decrease Speed') {
    v = MSPhysics::Replay.speed
    MSPhysics::Replay.speed = v - (v > 10.0 ? 10.0 : (v > 2.0 ? 1.0 : (v > 0.1 ? 0.1 : 0.01)))
  }
  dec_spd_cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    dec_spd_cmd.status_bar_text = "Decrease replay animation speed.    Speed: #{sprintf("%.2f", MSPhysics::Replay.speed)}"
    MF_ENABLED
  }
  dec_spd_cmd.menu_text = dec_spd_cmd.tooltip = 'Decrease Speed'
  dec_spd_cmd.status_bar_text = "Decrease replay animation speed.    Speed: #{sprintf("%.2f", MSPhysics::Replay.speed)}"
  dec_spd_cmd.small_icon = simg_path + 'replay_decrease_speed.png'
  dec_spd_cmd.large_icon = limg_path + 'replay_decrease_speed.png'
  replay_toolbar.add_item(dec_spd_cmd)

  cmd = UI::Command.new('Clear Data') {
    begin
      MSPhysics::Replay.clear_active_data
      MSPhysics::Replay.clear_data_from_model(!MSPhysics::Simulation.active?, true)
      MSPhysics::Replay.clear_data_from_file
    rescue Exception => err
      msg = "An exception occurred while attempting to clear replay data!\n#{err.class}:\n#{err_message}"
      puts "#{msg}\nTrace:\n#{err_backtrace.join("\n")}\n\n"
      UI.messagebox(msg)
    end
  }
  cmd.set_validation_proc {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active_data_valid? && !MSPhysics::Replay.active? ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Clear Data'
  cmd.status_bar_text = 'Clear recorded data.'
  cmd.small_icon = simg_path + 'replay_destroy.png'
  cmd.large_icon = limg_path + 'replay_destroy.png'
  replay_toolbar.add_item(cmd)

  replay_toolbar.show


  # Create Edit Menus
  UI.add_context_menu_handler { |menu|
    model = Sketchup.active_model
    bodies = []
    joints = []
    buoyancy_planes = []
    use_all_opts = (model.active_entities == model.entities)
    model.selection.each { |e|
      if e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)
        case e.get_attribute('MSPhysics', 'Type', 'Body')
        when 'Body'
          bodies << e
        when 'Joint'
          joints << e
        when 'Buoyancy Plane'
          buoyancy_planes << e if use_all_opts
        end
      end
    }
    next if bodies.empty? && joints.empty? && buoyancy_planes.empty?
    msp_menu = menu.add_submenu('MSPhysics')
    if bodies.size > 0
      if joints.empty? && buoyancy_planes.empty?
        body_menu = msp_menu
      else
        text = bodies.size > 1 ? "#{bodies.size} Bodies" : "Body"
        body_menu = msp_menu.add_submenu(text)
      end

      state_menu = body_menu.add_submenu('State')
      state_menu.add_item('Default') {
        op = 'MSPhysics Body - Reset States'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        MSPhysics::BODY_STATES.each { |option|
          MSPhysics.delete_attribute(bodies, 'MSPhysics Body', option)
        }
        model.commit_operation
        MSPhysics::Dialog.update_body_state
      }
      state_menu.add_separator

      if (bodies.size == 1)
        MSPhysics::BODY_STATES.each { |option|
          default_state = MSPhysics::DEFAULT_BODY_SETTINGS[option.downcase.gsub(/\s/, '_').to_sym]
          item = state_menu.add_item(option) {
            op = 'MSPhysics Body - Change State'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            state = MSPhysics.get_attribute(bodies, 'MSPhysics Body', option, default_state) ? true : false
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', option, !state)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          state_menu.set_validation_proc(item) {
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', option, default_state) ? MF_CHECKED : MF_UNCHECKED
          }
          break unless use_all_opts
        }
      else
        index = 0
        MSPhysics::BODY_STATES.each { |option|
          state_menu.add_item(option) {
            op = 'MSPhysics Body - Change State'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', option, true)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          state_menu.add_item(MSPhysics::BODY_CON_STATES[index]) {
            op = 'MSPhysics Body - Change State'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', option, false)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          break unless use_all_opts
          index += 1
        }
      end

      if use_all_opts
        shape_menu = body_menu.add_submenu('Shape')
        shape_menu.add_item('Default') {
          op = 'MSPhysics Body - Reset Shape'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          MSPhysics.delete_attribute(bodies, 'MSPhysics Body', 'Shape')
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        shape_menu.add_separator
        MSPhysics::SHAPES.each { |id, name|
          item = shape_menu.add_item(name) {
            op = 'MSPhysics Body - Change Shape'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Shape', id)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          shape_menu.set_validation_proc(item) {
            shape = MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Shape',  MSPhysics::DEFAULT_BODY_SETTINGS[:shape_id])
            (shape == id || shape == name) ? MF_CHECKED : MF_UNCHECKED
          }
        }

        shape_dir_menu = body_menu.add_submenu('Shape Dir')
        shape_dir_menu.add_item('Default') {
          op = 'MSPhysics Body - Reset Shape Dir'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          MSPhysics.delete_attribute(bodies, 'MSPhysics Body', 'Shape Dir')
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        shape_dir_menu.add_separator
        MSPhysics::SHAPE_DIRS.each { |id, name|
          item = shape_dir_menu.add_item(name) {
            op = 'MSPhysics Body - Change Shape Dir'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Shape Dir', id)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          shape_dir_menu.set_validation_proc(item) {
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Shape Dir',  MSPhysics::DEFAULT_BODY_SETTINGS[:shape_dir]) == id ? MF_CHECKED : MF_UNCHECKED
          }
        }

        mat_menu = body_menu.add_submenu('Material')
        default_mat = MSPhysics::DEFAULT_BODY_SETTINGS[:material_name]
        item = mat_menu.add_item(default_mat) {
          op = 'MSPhysics Body - Change Material'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          ['Material', 'Density', 'Static Friction', 'Kinetic Friction', 'Enable Friction', 'Elasticity', 'Softness'].each { |option|
            MSPhysics.delete_attribute(bodies, 'MSPhysics Body', option)
          }
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        mat_menu.set_validation_proc(item) {
          MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Material', default_mat) == default_mat ? MF_CHECKED : MF_UNCHECKED
        }
        item = mat_menu.add_item('Custom') {
          op = 'MSPhysics Body - Change Material'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Material', 'Custom')
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        mat_menu.set_validation_proc(item) {
          MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Material', default_mat) == 'Custom' ? MF_CHECKED : MF_UNCHECKED
        }
        mat_menu.add_separator
        materials = MSPhysics::Materials.sort { |a, b| a.name <=> b.name }
        materials.each { |material|
          item = mat_menu.add_item(material.name) {
            op = 'MSPhysics Body - Change Material'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Material', material.name)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Density', material.density)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Static Friction', material.static_friction)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Kinetic Friction', material.kinetic_friction)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Enable Friction', true)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Elasticity', material.elasticity)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Softness', material.softness)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          mat_menu.set_validation_proc(item) {
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Material', default_mat) == material.name ? MF_CHECKED : MF_UNCHECKED
          }
        }

        mass_control_menu = body_menu.add_submenu('Mass Control')
        mass_control_menu.add_item('Default') {
          op = 'MSPhysics Body - Reset Mass Control'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          MSPhysics.delete_attribute(bodies, 'MSPhysics Body', 'Mass Control')
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        mass_control_menu.add_separator
        MSPhysics::MASS_CONTROLS.each { |id, name|
          item = mass_control_menu.add_item(name) {
            op = 'MSPhysics Body - Change Mass Control'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Mass Control', id)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          mass_control_menu.set_validation_proc(item) {
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Mass Control', MSPhysics::DEFAULT_BODY_SETTINGS[:mass_control]) == id ? MF_CHECKED : MF_UNCHECKED
          }
        }

        magnet_mode_menu = body_menu.add_submenu('Magnet Mode')
        magnet_mode_menu.add_item('Default') {
          op = 'MSPhysics Body - Reset Magnet Mode'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          MSPhysics.delete_attribute(bodies, 'MSPhysics Body', 'Magnet Mode')
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        magnet_mode_menu.add_separator
        MSPhysics::MAGNET_MODES.each { |id, name|
          item = magnet_mode_menu.add_item(name) {
            op = 'MSPhysics Body - Change Magnet Mode'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Magnet Mode', id)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          magnet_mode_menu.set_validation_proc(item) {
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Magnet Mode', MSPhysics::DEFAULT_BODY_SETTINGS[:magnet_mode]) == id ? MF_CHECKED : MF_UNCHECKED
          }
        }

        type_menu = body_menu.add_submenu('Type')
        type_menu.add_item('Default') {
          op = 'MSPhysics Body - Reset Type'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          MSPhysics.delete_attribute(bodies, 'MSPhysics Body', 'Type')
          model.commit_operation
          MSPhysics::Dialog.update_body_state
        }
        type_menu.add_separator
        MSPhysics::BODY_TYPES.each { |id, name|
          item = type_menu.add_item(name) {
            op = 'MSPhysics Body - Change Type'
            Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Type', id)
            model.commit_operation
            MSPhysics::Dialog.update_body_state
          }
          type_menu.set_validation_proc(item) {
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Type', MSPhysics::DEFAULT_BODY_SETTINGS[:type]) == id ? MF_CHECKED : MF_UNCHECKED
          }
        }
      end

      body_menu.add_item('Clear Connected Joints') {
        op = 'MSPhysics Body - Clear Connected Joints'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        MSPhysics.delete_attribute(bodies, 'MSPhysics Body', 'Connected Joints')
        model.commit_operation
      }
      body_menu.add_item('Reset Properties') {
        op = 'MSPhysics Body - Reset Properties'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        MSPhysics.delete_attribute(bodies, 'MSPhysics')
        bodies.each { |e|
          dict = e.attribute_dictionary('MSPhysics Body')
          if dict
            dict.keys.each { |k|
              next if k == 'Enable Script' || k == 'Connected Joints'
              e.delete_attribute('MSPhysics Body', k)
            }
          end
        }
        model.commit_operation
      }
      body_menu.add_item('Clear Script') {
        op = 'MSPhysics Body - Clear Script'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        MSPhysics.delete_attribute(bodies, 'MSPhysics Script')
        MSPhysics.delete_attribute(bodies, 'MSPhysics Script', 'Enable Script')
        model.commit_operation
      }
    end
    if joints.size > 0
      if bodies.empty? && buoyancy_planes.empty?
        joint_menu = msp_menu
      else
        text = joints.size > 1 ? "#{joints.size} Joints" : "Joint"
        joint_menu = msp_menu.add_submenu(text)
      end
      joint_menu.add_item('Make Unique ID') {
        op = 'MSPhysics Joint - Make Unique ID'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        MSPhysics.delete_attribute(joints, 'MSPhysics Joint', 'ID')
        joints.each { |joint|
          id = MSPhysics::JointTool.generate_uniq_id
          joint.set_attribute('MSPhysics Joint', 'ID', id)
        }
        model.commit_operation
      }
      if joints.size > 1
        joint_menu.add_item('Make Same ID') {
          op = 'MSPhysics Joint - Make Same ID'
          Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
          id = MSPhysics::JointTool.generate_uniq_id
          MSPhysics.set_attribute(joints, 'MSPhysics Joint', 'ID', id)
          model.commit_operation
        }
      end
      item = joint_menu.add_item('Connected Collide') {
        op = 'MSPhysics Joint - Make Same ID'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        state = MSPhysics.get_attribute(joints, 'MSPhysics Joint', 'Bodies Collidable', MSPhysics::Joint::DEFAULT_BODIES_COLLIDABLE) ? true : false
        MSPhysics.set_attribute(joints, 'MSPhysics Joint', 'Bodies Collidable', !state)
        model.commit_operation
      }
      joint_menu.set_validation_proc(item) {
        MSPhysics.get_attribute(joints, 'MSPhysics Joint', 'Bodies Collidable', MSPhysics::Joint::DEFAULT_BODIES_COLLIDABLE) ? MF_CHECKED : MF_UNCHECKED
      }
      joint_menu.add_item('Reset Properties') {
        op = 'MSPhysics Joint - Reset Properties'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        #~ MSPhysics.delete_attribute(joints, 'MSPhysics Joint')
        joints.each { |joint|
          dict = joint.attribute_dictionary('MSPhysics Joint')
          if dict
            dict.keys.each { |k|
              joint.delete_attribute('MSPhysics Joint', k) if k != 'Type' && k != 'ID'
            }
          end
        }
        model.commit_operation
      }
    end
    if buoyancy_planes.size > 0
      if bodies.empty? && joints.empty?
        bp_menu = msp_menu
      else
        text = buoyancy_planes.size > 1 ? "#{buoyancy_planes.size} Buoyancy Planes" : "Buoyancy Plane"
        bp_menu = msp_menu.add_submenu(text)
      end
      bp_menu.add_item('Properties') {
        default = MSPhysics::DEFAULT_BUOYANCY_PLANE_SETTINGS
        prompts = ['Density (kg/m³)', 'Viscosity (0.0 - 1.0)', 'Current X', 'Current Y', 'Current Z']
        dict = 'MSPhysics Buoyancy Plane'
        ddensity = MSPhysics.get_attribute(buoyancy_planes, dict, 'Density', default[:density])
        dviscosity = MSPhysics.get_attribute(buoyancy_planes, dict, 'Viscosity', default[:viscosity])
        dcurrent_x = MSPhysics.get_attribute(buoyancy_planes, dict, 'Current X', default[:current_x])
        dcurrent_y = MSPhysics.get_attribute(buoyancy_planes, dict, 'Current Y', default[:current_y])
        dcurrent_z = MSPhysics.get_attribute(buoyancy_planes, dict, 'Current Z', default[:current_z])
        defaults = [ddensity, dviscosity, dcurrent_x, dcurrent_y, dcurrent_z]
        input = UI.inputbox(prompts, defaults, 'Buoyancy Plane Properties')
        next unless input
        density = AMS.clamp(input[0].to_f, 0.001, nil)
        viscosity = AMS.clamp(input[1].to_f, 0, 1)
        current_x = input[2].to_f
        current_y = input[3].to_f
        current_z = input[4].to_f
        op = 'MSPhysics Buoyancy - Edit Plane Properties'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        MSPhysics.set_attribute(buoyancy_planes, dict, 'Density', density)
        MSPhysics.set_attribute(buoyancy_planes, dict, 'Viscosity', viscosity)
        MSPhysics.set_attribute(buoyancy_planes, dict, 'Current X', current_x)
        MSPhysics.set_attribute(buoyancy_planes, dict, 'Current Y', current_y)
        MSPhysics.set_attribute(buoyancy_planes, dict, 'Current Z', current_z)
        model.commit_operation
      }
      item = bp_menu.add_item('Ignore') {
        dict = 'MSPhysics Buoyancy Plane'
        op = 'MSPhysics Buoyancy - Change State'
        Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
        state = MSPhysics.get_attribute(buoyancy_planes, dict, 'Ignore', false) ? true : false
        MSPhysics.set_attribute(buoyancy_planes, dict, 'Ignore', !state)
        model.commit_operation
      }
      bp_menu.set_validation_proc(item) {
        MSPhysics.get_attribute(buoyancy_planes, 'MSPhysics Buoyancy Plane', 'Ignore', false) ? MF_CHECKED : MF_UNCHECKED
      }
    end
  }

  # Create Plugin Menus
  plugin_menu = UI.menu('Plugins').add_submenu('MSPhysics')

  item = plugin_menu.add_item('Export Replay to Images') {
    MSPhysics::Replay.export_to_images
  }
  plugin_menu.set_validation_proc(item) {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active_data_valid? ? MF_ENABLED : MF_GRAYED
  }

  item = plugin_menu.add_item('Export Replay to SKP') {
    MSPhysics::Replay.export_to_skp
  }
  plugin_menu.set_validation_proc(item) {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active_data_valid? ? MF_ENABLED : MF_GRAYED
  }

  item = plugin_menu.add_item('Export Replay to Kerkythea') {
    MSPhysics::Replay.export_to_kerkythea
  }
  plugin_menu.set_validation_proc(item) {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active_data_valid? ? MF_ENABLED : MF_GRAYED
  }

  item = plugin_menu.add_item('Export Replay to SkIndigo') {
    MSPhysics::Replay.export_to_skindigo
  }
  plugin_menu.set_validation_proc(item) {
    next MF_GRAYED unless Sketchup.active_model
    MSPhysics::Replay.active_data_valid? ? MF_ENABLED : MF_GRAYED
  }

  item = plugin_menu.add_item('Replay Settings') {
    str_yes = 'Yes'
    str_no = 'No'
    str_yn = str_yes + '|' + str_no
    prompts = ['Record Groups', 'Record Materials', 'Record Layers', 'Record Camera', 'Record Render', 'Record Shadow', 'Replay Groups', 'Replay Materials', 'Replay Layers', 'Replay Camera', 'Replay Render', 'Replay Shadow']
    defaults = [
      MSPhysics::Replay.groups_record_enabled? ? str_yes : str_no,
      MSPhysics::Replay.materials_record_enabled? ? str_yes : str_no,
      MSPhysics::Replay.layers_record_enabled? ? str_yes : str_no,
      MSPhysics::Replay.camera_record_enabled? ? str_yes : str_no,
      MSPhysics::Replay.render_record_enabled? ? str_yes : str_no,
      MSPhysics::Replay.shadow_record_enabled? ? str_yes : str_no,
      MSPhysics::Replay.groups_replay_enabled? ? str_yes : str_no,
      MSPhysics::Replay.materials_replay_enabled? ? str_yes : str_no,
      MSPhysics::Replay.layers_replay_enabled? ? str_yes : str_no,
      MSPhysics::Replay.camera_replay_enabled? ? str_yes : str_no,
      MSPhysics::Replay.render_replay_enabled? ? str_yes : str_no,
      MSPhysics::Replay.shadow_replay_enabled? ? str_yes : str_no
    ]
    drop_downs = [str_yn, str_yn, str_yn, str_yn, str_yn, str_yn, str_yn, str_yn, str_yn, str_yn, str_yn, str_yn]
    input = UI.inputbox(prompts, defaults, drop_downs, 'Replay Settings')
    next unless input
    MSPhysics::Replay.groups_record_enabled = input[0] == str_yes
    MSPhysics::Replay.materials_record_enabled = input[1] == str_yes
    MSPhysics::Replay.layers_record_enabled = input[2] == str_yes
    MSPhysics::Replay.camera_record_enabled = input[3] == str_yes
    MSPhysics::Replay.render_record_enabled = input[4] == str_yes
    MSPhysics::Replay.shadow_record_enabled = input[5] == str_yes
    MSPhysics::Replay.groups_replay_enabled = input[6] == str_yes
    MSPhysics::Replay.materials_replay_enabled = input[7] == str_yes
    MSPhysics::Replay.layers_replay_enabled = input[8] == str_yes
    MSPhysics::Replay.camera_replay_enabled = input[9] == str_yes
    MSPhysics::Replay.render_replay_enabled = input[10] == str_yes
    MSPhysics::Replay.shadow_replay_enabled = input[11] == str_yes
    MSPhysics::Replay.save_replay_settings(true)
  }
  plugin_menu.set_validation_proc(item) {
    Sketchup.active_model ? MF_ENABLED : MF_GRAYED
  }

  plugin_menu.add_separator

  item = plugin_menu.add_item('Create Buoyancy Plane') {
    default = MSPhysics::DEFAULT_BUOYANCY_PLANE_SETTINGS
    prompts = ['Density (kg/m³)', 'Viscosity (0.0 - 1.0)', 'Current X', 'Current Y', 'Current Z']
    defaults = [default[:density], default[:viscosity], default[:current_x], default[:current_y], default[:current_z]]
    input = UI.inputbox(prompts, defaults, 'Buoyancy Plane Properties')
    next unless input
    density = AMS.clamp(input[0].to_f, 0.001, nil)
    viscosity = AMS.clamp(input[1].to_f, 0, 1)
    current_x = input[2].to_f
    current_y = input[3].to_f
    current_z = input[4].to_f
    model = Sketchup.active_model
    op = 'MSPhysics Buoyancy - Create Plane'
    Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
    mat = model.materials[default[:material_name]]
    unless mat
      mat = model.materials.add(default[:material_name])
      mat.color = default[:color]
      mat.alpha = default[:alpha]
    end
    group = model.entities.add_group
    s = default[:plane_size] * 0.5
    group.entities.add_face([-s, -s, 0], [s, -s, 0], [s, s, 0], [-s, s, 0])
    group.material = mat
    dict = 'MSPhysics Buoyancy Plane'
    group.set_attribute('MSPhysics', 'Type', 'Buoyancy Plane')
    group.set_attribute(dict, 'Density', density)
    group.set_attribute(dict, 'Viscosity', viscosity)
    group.set_attribute(dict, 'Current X', current_x)
    group.set_attribute(dict, 'Current Y', current_y)
    group.set_attribute(dict, 'Current Z', current_z)
    model.commit_operation
  }
  plugin_menu.set_validation_proc(item) {
    Sketchup.active_model ? MF_ENABLED : MF_GRAYED
  }

  plugin_menu.add_separator

  item = plugin_menu.add_item('Select All Joints') {
    model = Sketchup.active_model
    model.selection.clear
    model.definitions.each { |d|
      d.instances.each { |i|
        model.selection.add(i) if i.get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
      }
    }
  }
  plugin_menu.set_validation_proc(item) {
    Sketchup.active_model ? MF_ENABLED : MF_GRAYED
  }


  plugin_menu.add_item('Make Compatible') {
    MSPhysics.make_compatible(true)
  }

  item = plugin_menu.add_item('Delete All Attributes') {
    msg = "This option removes all MSPhysics assigned properties, scripts, and record of connected joints.\n"
    msg << "Do you want to proceed?"
    choice = UI.messagebox(msg, MB_YESNO)
    if choice == IDYES
      model = Sketchup.active_model
      op = 'MSPhysics - Delete All Attributes'
      Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
      MSPhysics.delete_all_attributes
      model.commit_operation
    end
  }
  plugin_menu.set_validation_proc(item) {
    Sketchup.active_model ? MF_ENABLED : MF_GRAYED
  }

  item = plugin_menu.add_item('Purge Unused') {
    model = Sketchup.active_model
    op = 'MSPhysics - Purge Unused'
    Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
    model.definitions.purge_unused
    model.materials.purge_unused
    model.layers.purge_unused
    model.styles.purge_unused
    model.commit_operation
  }
  plugin_menu.set_validation_proc(item) {
    Sketchup.active_model ? MF_ENABLED : MF_GRAYED
  }

  plugin_menu.add_separator

  if Sketchup.version.to_i > 13
    item = plugin_menu.add_item('Use Original Icons') {
      cur_use_orig = Sketchup.read_default('MSPhysics', 'Use Original Icons', false) ? true : false
      Sketchup.write_default('MSPhysics', 'Use Original Icons', !cur_use_orig)
      if cur_use_orig == use_orig_icons
        UI.messagebox('The change will take place when SketchUp is re-launched.', MB_OK)
      end
    }
    plugin_menu.set_validation_proc(item) {
      Sketchup.read_default('MSPhysics', 'Use Original Icons', false) ? MF_CHECKED : MF_UNCHECKED
    }

    plugin_menu.add_separator
  end

  plugin_menu.add_item('Homepage') {
    UI.openURL("http://sketchucation.com/forums/viewtopic.php?f=323&t=56852")
  }

  plugin_menu.add_item('Wiki') {
    UI.openURL("https://github.com/AntonSynytsia/MSPhysics/wiki")
  }

  plugin_menu.add_item('Models') {
    UI.openURL("https://3dwarehouse.sketchup.com/search.html?q=msphysics&backendClass=entity")
  }

  plugin_menu.add_item('About') {
    msg = "#{MSPhysics::NAME} #{MSPhysics::VERSION} -- #{MSPhysics::RELEASE_DATE}\n"
    msg << "Powered by Newton Dynamics #{MSPhysics::Newton.get_version}\n"
    msg << "Copyright #{MSPhysics.extension.copyright}"
    UI.messagebox(msg)
  }

  file_loaded(__FILE__)
end
