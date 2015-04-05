require 'MSPhysics.rb'

unless defined?(MSPhysics)
  msg = 'You cannot load MSPhysics extension until SketchUp meets all the plugin compatibility demands!'
  raise(LoadError, msg, caller)
end

dir = File.dirname(__FILE__)
ops = (RUBY_PLATFORM =~ /mswin|mingw/i) ? 'win' : 'mac'
bit = (Sketchup.respond_to?('is_64bit?') && Sketchup.is_64bit?) ? '64' : '32'
ver = (RUBY_VERSION =~ /1.8/) ? '1.8' : '2.0'

require File.join(dir, ops+bit, ver, 'msp_lib')

require File.join(dir, 'geometry.rb')
require File.join(dir, 'group.rb')
require File.join(dir, 'collision.rb')
require File.join(dir, 'contact.rb')
require File.join(dir, 'hit.rb')
require File.join(dir, 'script_exception.rb')
require File.join(dir, 'slider.rb')
require File.join(dir, 'common.rb')
require File.join(dir, 'controller.rb')
require File.join(dir, 'body.rb')
require File.join(dir, 'world.rb')
require File.join(dir, 'material.rb')
require File.join(dir, 'materials.rb')
require File.join(dir, 'joint.rb')
require File.join(dir, 'joint_hinge.rb')
require File.join(dir, 'joint_motor.rb')
require File.join(dir, 'joint_servo.rb')
require File.join(dir, 'joint_corkscrew.rb')
require File.join(dir, 'joint_ball_and_socket.rb')
require File.join(dir, 'joint_universal.rb')
require File.join(dir, 'joint_slider.rb')
require File.join(dir, 'joint_piston.rb')
require File.join(dir, 'joint_spring.rb')
require File.join(dir, 'joint_up_vector.rb')
require File.join(dir, 'sdl.rb')
require File.join(dir, 'music.rb')
require File.join(dir, 'sound.rb')
require File.join(dir, 'midi.rb')
require File.join(dir, 'simulation.rb')
require File.join(dir, 'settings.rb')
require File.join(dir, 'dialog.rb')
require File.join(dir, 'control_panel.rb')

# @since 1.0.0
module MSPhysics

  DEFAULT_SIMULATION_SETTINGS = {
    :solver_model           => 1,
    :friction_model         => 0,
    :update_rate            => 1,
    :update_timestep        => 1/60.0,
    :gravity                => [0.0, 0.0, -9.8],
    :material_thickness     => 1/256.0
  }

  DEFAULT_BODY_SETTINGS = {
    :shape                  => 'Compound',
    :material_name          => 'Default',
    :density                => 700,
    :static_friction        => 0.90,
    :dynamic_friction       => 0.50,
    :enable_friction        => true,
    :elasticity             => 0.40,
    :softness               => 0.10,
    :linear_damping         => 0.10,
    :angular_damping        => 0.10,
    :magnet_force           => 0.00,
    :magnet_range           => 0.00,
    :magnetic               => false,
    :enable_script          => true,
    :static                 => false,
    :frozen                 => false,
    :collidable             => true,
    :auto_sleep             => true,
    :continuous_collision   => false,
    :thruster_lock_axis     => true,
    :emitter_lock_axis      => true,
    :emitter_rate           => 10,
    :emitter_lifetime       => 100
  }

  DEFAULT_BUOYANCY_PLANE_SETTINGS = {
    :density                => 997.04,
    :viscosity              => 0.005,
    :plane_size             => 10000,
    :color                  => Sketchup::Color.new(0,140,255),
    :alpha                  => 0.7,
    :material_name          => 'MSPhysics Buoyancy'
  }

  SCRIPT_NAME = 'MSPhysicsScript'.freeze
  CONTROLLER_NAME = 'MSPhysicsController'.freeze

  CURSORS = {
    :select                 => 0,
    :select_plus            => 0,
    :select_plus_minus      => 0,
    :hand                   => 671,
    :target                 => 0
  }

  class << self

    # Get common attribute value from a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @param [Object] default_value The value to return if the attribute value
    #   is not found.
    # @return [String, nil] A common attribute value or +nil+ if one of the
    #   entity attributes is different from another.
    def get_attribute(ents, handle, name, default_value = nil)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      value = nil
      ents.each { |e|
        v = e.get_attribute(handle, name, nil)
        value = v if value == nil
        return default_value if value != v
      }
      value == nil ? default_value : value
    end

    # Assign attribute value to a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @param [String] value Attribute value.
    # @return [void]
    def set_attribute(ents, handle, name, value)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.set_attribute(handle, name, value)
      }
    end

    # Delete attribute value from a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @param [String] handle Dictionary name.
    # @param [String] name Attribute name.
    # @return [void]
    def delete_attribute(ents, handle, name)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.delete_attribute(handle, name)
      }
    end

    # Delete MSPhysics attributes from a collection of entities.
    # @param [Array<Sketchup::Entity>] ents A collection of entities.
    # @return [void]
    def delete_attributes(ents)
      unless ents.is_a?(Array)
        ents = ents.respond_to?(:to_a) ? ents.to_a : [ents]
      end
      ents.each { |e|
        e.delete_attribute('MSPhysics') if e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body'
        e.delete_attribute('MSPhysics Body')
        e.delete_attribute('MSPhysics Joint')
        e.delete_attribute('MSPhysics Script')
		e.delete_attribute('MSPhysics Script')
      }
    end

    # Delete MSPhysics attributes from all entities.
    # @return [void]
    def delete_all_attributes
      model = Sketchup.active_model
      model.definitions.each { |definition|
        delete_attributes(definition.instances)
      }
      model.attribute_dictionaries.delete('MSPhysics')
	  model.attribute_dictionaries.delete('MSPhysics Sounds')
    end

    # Get version of the Newton Dynamics physics SDK.
    # @return [Fixnum]
    def get_newton_version
      MSPhysics::Newton.get_version
    end

    # Get float size of the Newton Dynamics physics SDK.
    # @return [Fixnum]
    def get_newton_float_size
      MSPhysics::Newton.get_float_size
    end

    # Get memory used by the Newton Dynamics physics SDK at the current time.
    # @return [Fixnum]
    def get_newton_memory_used
      MSPhysics::Newton.get_memory_used
    end

  end # class << self

end # module MSPhysics

unless file_loaded?(__FILE__)

  # Setup audio
  sdl = MSPhysics::SDL
  mix = MSPhysics::SDL::Mixer
  sdl.init(sdl::INIT_EVERYTHING)
  mix.init(mix::INIT_EVERYTHING)
  mix.open_audio(22050, mix::DEFAULT_FORMAT, 2, 1024)
  mix.allocate_channels(20)
  Kernel.at_exit {
    MSPhysics::Music.destroy_all
    mix.close_audio
    mix.quit
    sdl.quit
  }

  # Create cursors
  path = File.join(dir, 'images/cursors')
    names = [:select, :select_plus, :select_plus_minus]
    names.each { |name|
    MSPhysics::CURSORS[name] = UI.create_cursor(File.join(path, name.to_s + '.png'), 5, 12)
  }
  MSPhysics::CURSORS[:target] = UI.create_cursor(File.join(path, 'target.png'), 15, 15)

  # Create some materials
  # Coefficients are defined from material to material contacts. Material to
  # another material coefficients are automatically calculated by averaging out
  # the coefficients of both.
  # [ name, density (kg/m^3), static friction, kinetic friction, elasticity, softness ]
  # Many coefficient values are estimated, averaged up, made up, and not accurate.
  mats = [
    ['Aluminium', 2700, 0.42, 0.34, 0.30, 0.01],
    ['Brass', 8730, 0.35, 0.24, 0.40, 0.01],
    ['Brick', 1920, 0.60, 0.55, 0.20, 0.01],
    ['Bronze', 8200, 0.36, 0.27, 0.60, 0.01],
    ['Cadnium', 8640, 0.79, 0.46, 0.60, 0.01],
    ['Cast Iron', 7300, 0.51, 0.40, 0.60, 0.01],
    ['Chromium', 7190, 0.46, 0.30, 0.60, 0.01],
    ['Cobalt', 8746, 0.56, 0.40, 0.60, 0.01],
    ['Concrete', 2400, 0.62, 0.56, 0.20, 0.01],
    ['Glass', 2800, 0.90, 0.72, 0.4, 0.01],
    ['Copper', 8940, 0.55, 0.40, 0.60, 0.01],
    ['Gold', 19320, 0.49, 0.39, 0.60, 0.01],
    ['Graphite', 2230, 0.18, 0.14, 0.10, 0.01],
    ['Ice', 916, 0.01, 0.01, 0.10, 0.01],
    ['Nickel', 8900, 0.53, 0.44, 0.60, 0.01],
    ['Plastic', 1000, 0.35, 0.30, 0.69, 0.01],
    ['Rubber', 1100, 1.16, 1.16, 0.90, 0.01],
    ['Silver', 10500, 0.50, 0.40, 0.60, 0.01],
    ['Steel', 8050, 0.31, 0.23, 0.60, 0.01],
    ['Teflon', 2170, 0.04, 0.03, 0.10, 0.01],
    ['Titanium', 4500, 0.36, 0.30, 0.40, 0.01],
    ['Tungsten Carbide', 19600, 0.22, 0.15, 0.40, 0.01],
    ['Wood', 700, 0.50, 0.25, 0.40, 0.01],
    ['Zinc', 7000, 0.60, 0.50, 0.60, 0.01]
  ]
  mats.each { |args|
    mat = MSPhysics::Material.new(*args)
    MSPhysics::Materials.add(mat)
  }

  # Create MSPhysics Simulation Toolbar
  sim = MSPhysics::Simulation
  sim_toolbar = UI::Toolbar.new 'MSPhysics Simulation'

  cmd = UI::Command.new('Toggle UI'){
    MSPhysics::Dialog.show( !MSPhysics::Dialog.is_visible? )
  }
  cmd.set_validation_proc {
    MSPhysics::Dialog.is_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.menu_text = cmd.tooltip = 'Toggle UI'
  cmd.status_bar_text = 'Show/Hide MSPhysics UI.'
  cmd.small_icon = 'images/small/ui.png'
  cmd.large_icon = 'images/large/ui.png'
  sim_toolbar.add_item(cmd)

  cmd = UI::Command.new('Toggle Play'){
    sim.is_active? ? sim.instance.toggle_play : sim.start
  }
  cmd.set_validation_proc {
    if sim.is_active?
      sim.instance.is_playing? ? MF_CHECKED : MF_UNCHECKED
    else
      MF_ENABLED
    end
  }
  cmd.menu_text = cmd.tooltip = 'Toggle Play'
  cmd.status_bar_text = 'Play/Pause simulation.'
  cmd.small_icon = 'images/small/play.png'
  cmd.large_icon = 'images/large/play.png'
  sim_toolbar.add_item(cmd)

  cmd = UI::Command.new('Reset'){
    sim.reset
  }
  cmd.set_validation_proc {
    sim.is_active? ? MF_ENABLED : MF_GRAYED
  }
  cmd.menu_text = cmd.tooltip = 'Reset'
  cmd.status_bar_text = 'Reset simulation.'
  cmd.small_icon = 'images/small/reset.png'
  cmd.large_icon = 'images/large/reset.png'
  sim_toolbar.add_item(cmd)

  sim_toolbar.show

  # Create Edit Menus
  UI.add_context_menu_handler { |menu|
    model = Sketchup.active_model
    bodies = []
    joints = []
    buoyancy_planes = []
    model.selection.each { |e|
      next unless e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)
      case e.get_attribute('MSPhysics', 'Type', 'Body')
      when 'Body'
        bodies << e
      when 'Joint'
        joints << e
      when 'Buoyancy Plane'
        buoyancy_planes << e
      end
    }
    next if bodies.empty? && joints.empty? && buoyancy_planes.empty?
    msp_menu = menu.add_submenu('MSPhysics')
    if bodies.size > 0
      state_menu = msp_menu.add_submenu('State')
      state_options = ['Ignore']
      if model.active_entities == model.entities
        state_options.concat ['Static', 'Frozen', 'Magnetic', 'Collidable', 'Auto Sleep', 'Continuous Collision', 'Enable Friction', 'Enable Script']

        shape_menu = msp_menu.add_submenu('Shape')
        default_shape = MSPhysics::DEFAULT_BODY_SETTINGS[:shape]
        ['Box', 'Sphere', 'Cone', 'Cylinder', 'Chamfer Cylinder', 'Capsule', 'Convex Hull', 'Null', 'Compound', 'Compound from CD', 'Static Mesh'].each { |shape|
          item = shape_menu.add_item(shape){
            if Sketchup.version.to_i > 6
              model.start_operation('MSPhysics Change Shape', true)
            else
              model.start_operation('MSPhysics Change Shape')
            end
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Shape', shape)
            model.commit_operation
            MSPhysics::Dialog.update_state
          }
          shape_menu.set_validation_proc(item){
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Shape', default_shape) == shape ? MF_CHECKED : MF_UNCHECKED
          }
        }

        mat_menu = msp_menu.add_submenu('Material')
        default_mat = MSPhysics::DEFAULT_BODY_SETTINGS[:material_name]
        item = mat_menu.add_item(default_mat){
          if Sketchup.version.to_i > 6
            model.start_operation('MSPhysics Change Material', true)
          else
            model.start_operation('MSPhysics Change Material')
          end
          ['Material', 'Density', 'Static Friction', 'Dynamic Friction', 'Enable Friction', 'Elasticity', 'Softness'].each { |option|
            MSPhysics.delete_attribute(bodies, 'MSPhysics Body', option)
          }
          model.commit_operation
          MSPhysics::Dialog.update_state
        }
        mat_menu.set_validation_proc(item){
          MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Material', default_mat) == default_mat ? MF_CHECKED : MF_UNCHECKED
        }
        item = mat_menu.add_item('Custom'){
          if Sketchup.version.to_i > 6
            model.start_operation('MSPhysics Change Material', true)
          else
            model.start_operation('MSPhysics Change Material')
          end
          MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Material', 'Custom')
          model.commit_operation
          MSPhysics::Dialog.update_state
        }
        mat_menu.set_validation_proc(item){
          MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Material', default_mat) == 'Custom' ? MF_CHECKED : MF_UNCHECKED
        }
        mat_menu.add_separator
        materials = MSPhysics::Materials.sort { |a, b| a.get_name <=> b.get_name }
        materials.each { |material|
          item = mat_menu.add_item(material.get_name){
            if Sketchup.version.to_i > 6
              model.start_operation('MSPhysics Change Material', true)
            else
              model.start_operation('MSPhysics Change Material')
            end
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Material', material.get_name)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Density', material.get_density)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Static Friction', material.get_static_friction)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Dynamic Friction', material.get_dynamic_friction)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Enable Friction', true)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Elasticity', material.get_elasticity)
            MSPhysics.set_attribute(bodies, 'MSPhysics Body', 'Softness', material.get_softness)
            model.commit_operation
            MSPhysics::Dialog.update_state
          }
          mat_menu.set_validation_proc(item){
            MSPhysics.get_attribute(bodies, 'MSPhysics Body', 'Material', default_mat) == material.get_name ? MF_CHECKED : MF_UNCHECKED
          }
        }
      end
      state_options.each { |option|
        default_state = MSPhysics::DEFAULT_BODY_SETTINGS[option.downcase.gsub(' ', '_').to_sym]
        item = state_menu.add_item(option){
          if Sketchup.version.to_i > 6
            model.start_operation('MSPhysics Change State', true)
          else
            model.start_operation('MSPhysics Change State')
          end
          state = MSPhysics.get_attribute(bodies, 'MSPhysics Body', option, default_state) ? true : false
          MSPhysics.set_attribute(bodies, 'MSPhysics Body', option, !state)
          model.commit_operation
          MSPhysics::Dialog.update_state
        }
        state_menu.set_validation_proc(item){
          MSPhysics.get_attribute(bodies, 'MSPhysics Body', option, default_state) ? MF_CHECKED : MF_UNCHECKED
        }
      }
    end
    if joints.size > 0
    end
    if buoyancy_planes.size > 0
      msp_menu.add_item("Edit Buoyancy Plane Properties"){
        default = MSPhysics::DEFAULT_BUOYANCY_PLANE_SETTINGS
        prompts = ['Density', 'Viscosity']
        default_density = MSPhysics.get_attribute(buoyancy_planes, 'MSPhysics Buoyancy Plane', 'Density', default[:density])
        default_viscosity = MSPhysics.get_attribute(buoyancy_planes, 'MSPhysics Buoyancy Plane', 'Viscosity', default[:viscosity])
        defaults = [default_density, default_viscosity]
        input = Dialog.inputbox(prompts, defaults, 'Buoyancy Plane Properties')
        next unless input
        if Sketchup.version.to_i > 6
          model.start_operation('MSPhysics Edit Buoyancy Plane Properties', true)
        else
          model.start_operation('MSPhysics Edit Buoyancy Plane Properties')
        end
        MSPhysics.set_attribute(buoyancy_planes, 'MSPhysics Buoyancy Plane', 'Density', AMS.clamp(input[0].to_f, 0.001, nil))
        MSPhysics.set_attribute(buoyancy_planes, 'MSPhysics Buoyancy Plane', 'Viscosity', AMS.clamp(input[1].to_f, 0, 1))
        model.commit_operation
      }
    end
  }

  # Create Plugin Menus
  plugin_menu = UI.menu('Plugins').add_submenu('MSPhysics')
  plugin_menu.add_item('Add Buoyancy Plane'){
    default = MSPhysics::DEFAULT_BUOYANCY_PLANE_SETTINGS
    prompts = ['Density', 'Viscosity']
    defaults = [default[:density], default[:viscosity]]
    input = UI.inputbox(prompts, defaults, 'Buoyancy Plane Properties')
    next unless input
    model = Sketchup.active_model
    if Sketchup.version.to_i > 6
      model.start_operation('MSPhysics Add Buoyancy Plane', true)
    else
      model.start_operation('MSPhysics Add Buoyancy Plane')
    end
    mat = model.materials[default[:material_name]]
    unless mat
      mat = model.materials.add(default[:material_name])
      mat.color = default[:color]
      mat.alpha = default[:alpha]
    end
    group = model.entities.add_group
    s = default[:plane_size] * 0.5
    group.entities.add_face([-s*0.5, -s*0.5, 0], [s*0.5, -s*0.5, 0], [s*0.5, s*0.5, 0], [-s*0.5, s*0.5, 0])
    group.material = mat
    density = AMS.clamp(input[0].to_f, 0.001, nil)
    viscosity = AMS.clamp(input[1].to_f, 0, 1)
    group.set_attribute('MSPhysics', 'Type', 'Buoyancy Plane')
    group.set_attribute('MSPhysics Buoyancy Plane', 'Density', density)
    group.set_attribute('MSPhysics Buoyancy Plane', 'Viscosity', viscosity)
    model.commit_operation
  }
  plugin_menu.add_item('Delete All Attributes'){
    msg = "This option removes all MSPhysics assigned properties and scripts.\n"
    msg << "Do you want to proceed?"
    choice = UI.messagebox(msg, MB_YESNO)
    if choice == IDYES
      model = Sketchup.active_model
      if Sketchup.version.to_i > 6
        model.start_operation('MSPhysics Delete All Attributes', true)
      else
        model.start_operation('MSPhysics Delete All Attributes')
      end
      MSPhysics.delete_all_attributes
      model.commit_operation
    end
  }
  plugin_menu.add_item('About'){
    msg = "MSPhysics #{MSPhysics::VERSION}, #{MSPhysics::RELEASE_DATE}\n"
    msg << "Powered by the Newton Dynamics #{MSPhysics.get_newton_version} physics SDK by Juleo Jerez.\n"
    msg << "Copyright MIT © 2015, Anton Synytsia\n"
    UI.messagebox(msg)
  }

  file_loaded(__FILE__)
end
