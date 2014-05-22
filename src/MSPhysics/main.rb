require 'MSPhysics.rb'

msg = 'You cannot load MSPhysics extension until SketchUp meets all the plugin compatibility demands!'
raise(msg) unless defined?(MSPhysics::NAME)

require 'ams_Lib.rb'
AMS::Lib.require_all
AMS::Lib.require_library 'ams_ffi.rb'

dir = File.dirname(__FILE__)

require File.join(dir, 'group.rb')
require File.join(dir, 'geometry.rb')
require File.join(dir, 'conversion.rb')
require File.join(dir, 'utility.rb')
require File.join(dir, 'newton.rb')
require File.join(dir, 'collision.rb')
require File.join(dir, 'contact.rb')
require File.join(dir, 'simple_contact.rb')
require File.join(dir, 'hit.rb')
require File.join(dir, 'material.rb')
require File.join(dir, 'materials.rb')
require File.join(dir, 'common_context.rb')
require File.join(dir, 'body_observer.rb')
require File.join(dir, 'body.rb')
require File.join(dir, 'body_context.rb')
require File.join(dir, 'joint.rb')
require File.join(dir, 'animation.rb')
require File.join(dir, 'simulation.rb')
require File.join(dir, 'simulation_tool.rb')

unless file_loaded?(__FILE__)

  sim_tool = MSPhysics::SimulationTool
  plugin_menu = UI.menu('Plugins').add_submenu('MSPhysics')
  sim_menu = plugin_menu.add_submenu('Simulation')
  toolbar = UI::Toolbar.new 'MSPhysics Simulation'


  cmd = UI::Command.new('cmd'){
    if sim_tool.active?
      sim_tool.instance.toggle_play
    else
      sim_tool.start
    end
  }
  cmd.menu_text = cmd.tooltip = 'Toggle Play'
  cmd.status_bar_text = 'Play/Pause simulation.'
  cmd.set_validation_proc {
    next MF_UNCHECKED unless sim_tool.active?
    sim_tool.instance.playing? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/play.png'
  cmd.large_icon = 'images/large/play.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    sim_tool.reset
  }
  cmd.menu_text = cmd.tooltip = 'Reset'
  cmd.status_bar_text = 'Reset simulation.'
  cmd.set_validation_proc {
    MF_GRAYED unless sim_tool.active?
  }
  cmd.small_icon = 'images/small/reset.png'
  cmd.large_icon = 'images/large/reset.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    next unless sim_tool.active?
    sim = sim_tool.instance.simulation
    sim.record_animation = !sim.animation_recording?
  }
  cmd.menu_text = cmd.tooltip = 'Record'
  cmd.status_bar_text = 'Toggle record simulation.'
  cmd.set_validation_proc {
    next MF_GRAYED unless sim_tool.active?
    sim_tool.instance.simulation.animation_recording? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/record.png'
  cmd.large_icon = 'images/large/record.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  toolbar.add_separator
  sim_menu.add_separator


  solver_menu = sim_menu.add_submenu('Solver Model')
  solver_models = [
    ['Exact', 'Use exact solver model when precision is more important than speed.', 0],
    ['Ineractive: 1 Pass', 'Use ineractive when speed is more important than precision.', 1],
    ['Ineractive: 2 Passes', 'Use ineractive when speed is more important than precision.', 2],
    ['Ineractive: 4 Passes', 'Use ineractive when speed is more important than precision.', 4],
    ['Ineractive: 8 Passes', 'Use ineractive when speed is more important than precision.', 8]
  ]
  solver_models.each { |name, description, model|
    cmd = UI::Command.new('cmd'){
      next unless sim_tool.active?
      sim = sim_tool.instance.simulation
      sim.set_solver_model(model)
    }
    cmd.menu_text = cmd.tooltip = name
    cmd.status_bar_text = description
    cmd.set_validation_proc {
      next MF_GRAYED if sim_tool.inactive?
      sim = sim_tool.instance.simulation
      sim.get_solver_model == model ? MF_CHECKED : MF_UNCHECKED
    }
    solver_menu.add_item(cmd)
  }


  speed_menu = sim_menu.add_submenu('Speed')
  update_rates = [
      ['Double', 1/32.0],
      ['Normal', 1/64.0],
      ['x 1/2', 1/128.0],
      ['x 1/4', 1/256.0],
      ['x 1/8', 1/512.0],
      ['x 1/16', 1/1024.0]
  ]
  update_rates.each { |name, step|
    item = speed_menu.add_item(name){
      next unless sim_tool.active?
      sim = sim_tool.instance.simulation
      sim.set_update_timestep(step)
    }
    speed_menu.set_validation_proc(item){
      next MF_GRAYED if sim_tool.inactive?
      sim = sim_tool.instance.simulation
      sim.get_update_timestep == step ? MF_CHECKED : MF_UNCHECKED
    }
  }


  sim_menu.add_separator


  cmd = UI::Command.new('cmd'){
    next unless sim_tool.active?
    sim = sim_tool.instance.simulation
    sim.show_collision(!sim.collision_visible?)
  }
  cmd.menu_text = cmd.tooltip = 'Collision Wireframe'
  cmd.status_bar_text = "Show/Hide bodies' collision wireframe."
  cmd.set_validation_proc {
    next MF_GRAYED if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.collision_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/collision wireframe.png'
  cmd.large_icon = 'images/large/collision wireframe.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    next if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.show_axis(!sim.axis_visible?)
  }
  cmd.menu_text = cmd.tooltip = 'Centre of Mass'
  cmd.status_bar_text = "Show/Hide bodies' centre of mass axis."
  cmd.set_validation_proc {
    next MF_GRAYED if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.axis_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/centre of mass.png'
  cmd.large_icon = 'images/large/centre of mass.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    next if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.show_bounding_box(!sim.bounding_box_visible?)
  }
  cmd.menu_text = cmd.tooltip = 'Bounding Box'
  cmd.status_bar_text = "Show/Hide bodies' world axis aligned bounding box (AABB)."
  cmd.set_validation_proc {
    next MF_GRAYED if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.bounding_box_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/bounding box.png'
  cmd.large_icon = 'images/large/bounding box.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    next if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.show_contact_points(!sim.contact_points_visible?)
  }
  cmd.menu_text = cmd.tooltip = 'Contact Points'
  cmd.status_bar_text = 'Show/Hide body contact points.'
  cmd.set_validation_proc {
    next MF_GRAYED if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.contact_points_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/contact points.png'
  cmd.large_icon = 'images/large/contact points.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    next if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.show_contact_forces(!sim.contact_forces_visible?)
  }
  cmd.menu_text = cmd.tooltip = 'Contact Forces'
  cmd.status_bar_text = 'Show/Hide body contact forces.'
  cmd.set_validation_proc {
    next MF_GRAYED if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.contact_forces_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/contact force.png'
  cmd.large_icon = 'images/large/contact force.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    next if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.show_bodies(!sim.bodies_visible?)
  }
  cmd.menu_text = cmd.tooltip = 'Bodies'
  cmd.status_bar_text = 'Show/Hide all bodies.'
  cmd.set_validation_proc {
    next MF_GRAYED if sim_tool.inactive?
    sim = sim_tool.instance.simulation
    sim.bodies_visible? ? MF_CHECKED : MF_UNCHECKED
  }
  cmd.small_icon = 'images/small/bodies.png'
  cmd.large_icon = 'images/large/bodies.png'
  toolbar.add_item(cmd)
  sim_menu.add_item(cmd)


  cmd = UI::Command.new('cmd'){
    msg = "This option removes all MSPhysics assigned properties and scripts.\n"
    msg << 'Are you sure you want to delete them?'
    choice = UI.messagebox(msg, MB_YESNO)
    MSPhysics.delete_all_attributes if choice == IDYES
  }
  cmd.menu_text = cmd.tooltip = 'Delete All Attributes'
  cmd.status_bar_text = 'Removes MSPhysics assigned properties and scripts from all entities.'
  plugin_menu.add_item(cmd)


  toolbar.show


  UI.add_context_menu_handler { |menu|
    model = Sketchup.active_model
    ents = model.selection.to_a
    active = Sketchup.version.to_i > 6 ? model.active_path : nil
    parent = nil
    if active.is_a?(Array)
      next if active.size > 1
      parent = active[0]
      next if MSPhysics.get_attribute(parent, 'Ignore')
      shape = MSPhysics.get_attribute(parent, 'Shape')
      next if shape != 'Compound'
    end
    found = false
    ents.each { |ent|
      if [Sketchup::Group, Sketchup::ComponentInstance].include?(ent.class)
        found = true
        break
      end
    }
    next unless found

    msp_menu = menu.add_submenu('MSPhysics')
    state_menu = msp_menu.add_submenu('State')
    shape_menu = msp_menu.add_submenu('Shape')

    options = ['Ignore', 'No Collision']
    options.concat ['Static', 'Frozen'] unless parent
    options.each { |option|
      item = state_menu.add_item(option){
        state = MSPhysics.get_attribute(ents, option) ? true : false
        MSPhysics.set_attribute(ents, option, !state)
      }
      state_menu.set_validation_proc(item){
        MSPhysics.get_attribute(ents, option) ? MF_CHECKED : MF_UNCHECKED
      }
    }

    MSPhysics::Collision::SHAPES.each { |shape|
      words = shape.to_s.split('_')
      for i in 0...words.size
        words[i].capitalize!
      end
      option = words.join(' ')
      if parent
        next if ['Null', 'Compound', 'Compound From Mesh', 'Static Mesh'].include?(option)
      end
      item = shape_menu.add_item(option){
        MSPhysics.set_attribute(ents, 'Shape', option)
      }
      shape_menu.set_validation_proc(item){
        MSPhysics.get_attribute(ents, 'Shape', 'Convex Hull') == option ? MF_CHECKED : MF_UNCHECKED
      }
    }

    msp_menu.add_item('Delete Attributes'){
      msg = "This option removes MSPhysics assigned properties and scripts from the selected bodies.\n"
      msg << 'Are you sure you want to delete them?'
      choice = UI.messagebox(msg, MB_YESNO)
      MSPhysics.delete_attributes(ents) if choice == IDYES
    }
  }

  file_loaded(__FILE__)
end
