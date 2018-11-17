# @since 1.0.0
class MSPhysics::Simulation < MSPhysics::Entity

  @@instance = nil

  class << self

    # Get {Simulation} instance.
    # @return [Simulation, nil]
    def instance
      @@instance
    end

    # Determine if simulation is running.
    # @return [Boolean]
    def active?
      @@instance ? true : false
    end

    # Start simulation.
    # @param [Boolean] from_selection
    #   * Pass true to start simulation from selection and have all
    #     non-selected groups act stationary.
    #   * Pass false to start simulation will all bodies being considered.
    # @return [Boolean] success
    def start(from_selection = false)
      return false if @@instance
      MSPhysics::Replay.reset
      @@instance = self.new(from_selection)
      Sketchup.active_model.select_tool(@@instance)
      true
    end

    # End simulation.
    # @return [Boolean] success
    def reset
      return false unless @@instance
      Sketchup.active_model.select_tool(nil)
      @@instance = nil
      true
    end

    # Create a Simulation instance for external frame-by-frame.
    # @param [Boolean] from_selection
    #   * Pass true to start simulation from selection and have all
    #     non-selected groups act stationary.
    #   * Pass false to start simulation will all bodies being considered.
    # @return [Simulation, nil] A Simulation instance if successful.
    def external_control(from_selection = false)
      return if @@instance
      MSPhysics::Replay.reset
      @@instance = self.new(from_selection)
      @@instance.configure_for_external_control
      Sketchup.active_model.select_tool(@@instance)
      @@instance
    end

  end # class << self

  # @param [Boolean] from_selection Pass true to start simulation from selection.
  def initialize(from_selection = false)
    @started_from_selection = from_selection ? true : false
    @selected_ents = []
    @world = nil
    @update_rate = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_rate]
    @update_timestep = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_timestep]
    @update_timestep_inv = 1.0 / @update_timestep
    @mode = 0
    @frame = 0
    @time_info = { :start => 0, :end => 0, :last => 0, :sim => 0, :total => 0 }
    @fps_info = { :fps => 0, :update_rate => 10, :last => 0, :change => 0 }
    @animation_enabled = true
    @animation_stopped = false
    @camera = { :original => {}, :follow => nil, :target => nil, :offset => nil }
    @cursor_id = MSPhysics::CURSORS[:hand]
    @original_cursor_id = @cursor_id
    @cursor_pos = Geom::Point3d.new(0,0,0)
    @interactive_note = "Interactive mode: Click and drag a physics body to move. Hold SHIFT while dragging to lift."
    @game_note = "Game mode: All control over bodies and camera via mouse is restricted as the mouse is reserved for gaming."
    @general_note = "PAUSE - toggle play  ESC - reset"
    @fullscreen_note = {
      :time => nil,
      :text => "Fullscreen Detected\nPress ESC to Reset Simulation",
      :font => "Verdana",
      :align => TextAlignCenter,
      :color => Sketchup::Color.new(255,255,255),
      :size => 12,
      :italic => false,
      :bold => false,
      :duration => 5,
      :background => Sketchup::Color.new(10,10,10, 200),
      :bgw => 300,
      :bgh => 80,
      :bvo => 20,
      :hratio => 0.5,
      :vratio => 0.05
    }
    @paused = false
    @pause_updated = false
    @suspended = false
    @mouse_over = true
    @menu_entered = false
    @menu_entered2 = false
    @ip = Sketchup::InputPoint.new
    @picked = {}
    @clicked = nil
    @error = nil
    @saved_transformations = {}
    @log_line = { :ent => nil, :mat => nil, :log => [], :limit => 20 }
    @display_note = { :ent => nil, :mat => nil }
    @fancy_note_defaults = {
      :font => "Ariel",
      :size => 11,
      :bold => false,
      :italic => false,
      :align => TextAlignLeft,
      :color => Sketchup::Color.new(255,255,255,255),
      :background => Sketchup::Color.new(0,140,255,220),
      :padding => 10,
      :hratio => 0.0,
      :vratio => 0.0,
      :duration => 5,
      :fade => 0.25,
      :twr => 0.6,
      :thr => 1.5
    }
    @fancy_note = nil
    @prev_fancy_note = nil
    @emitted_bodies = {}
    @created_entities = {}
    @bb = Geom::BoundingBox.new
    @draw_queue = []
    @points_queue = []
    @ccm = false
    @show_bodies = true
    @hidden_entities = []
    @timers_started = false
    @contact_points = {
      :show           => false,
      :point_size     => 3,
      :point_style    => 2,
      :point_color    => Sketchup::Color.new(109, 206, 255)
    }
    @contact_forces = {
      :show           => false,
      :line_width     => 1,
      :line_stipple   => '',
      :line_color     => Sketchup::Color.new(247, 40, 85)
    }
    @aabb = {
      :show           => false,
      :line_width     => 1,
      :line_stipple   => '',
      :line_color     => Sketchup::Color.new(68, 53, 165)
    }
    @collision_wireframe = {
      :show           => false,
      :line_width     => 1,
      :line_stipple   => '',
      :active         => Sketchup::Color.new(221, 38, 165),
      :sleeping       => Sketchup::Color.new(255, 255, 100),
      :show_edges     => nil,
      :show_profiles  => nil
    }
    @axes = {
      :show           => false,
      :line_width     => 2,
      :line_stipple   => '',
      :size           => 20,
      :xaxis          => Sketchup::Color.new(255, 0, 0),
      :yaxis          => Sketchup::Color.new(0, 255, 0),
      :zaxis          => Sketchup::Color.new(0, 0, 255)
    }
    @pick_and_drag = {
      :line_width     => 1,
      :line_stipple   => '_',
      :line_color     => Sketchup::Color.new(250, 10, 10),
      :point_width    => 2,
      :point_size     => 10,
      :point_style    => 4,
      :point_color    => Sketchup::Color.new(4, 4, 4),
      :vline_width    => 1,
      :vline_stipple1 => '',
      :vline_stipple2 => '-',
      :vline_color    => Sketchup::Color.new(0, 40, 255)
    }
    @controller_context = MSPhysics::ControllerContext.new
    @thrusters = {}
    @emitters = {}
    @buoyancy_planes = {}
    @controlled_joints = {}
    @scene_info = { :active => false, :data1 => nil, :data2 => nil, :transition_time => 0, :elasted_time => 0, :timer => nil }
    @scene_anim_info = { :state => 0, :data => {}, :transition_time => 0, :elasted_time => 0, :ref_time => 0, :tabs_size => nil, :active_tab => nil, :tab_dir => 1 }
    @cc_bodies = []
    @particles = []
    @particle_def2d = {}
    @particle_def3d = {}
    @dp_particle_instances = []
    @particles_visible = true
    @undo_on_reset = false
    @joystick_data = {}
    @joybutton_data = {}
    @joypad_data = 0
    @simulation_started = false
    @reset_positions_on_end = true
    @erase_instances_on_end = true
    @reset_camera_on_end = true
    @joint_layer_orig_visible = true
    @frame_change_observer_id = nil
    @key_nav_veloc = Geom::Vector3d.new(0,0,0)
    @key_nav_omega = Geom::Vector3d.new(0,0,0)
  end

  # @!attribute [r] world
  #   Get simulation world.
  #   @return [World]

  # @!attribute [r] frame
  #   Get simulation frame.
  #   @return [Integer]

  # @!attribute [r] fps
  #   Get simulation update rate in frames per second.
  #   @return [Integer]


  attr_reader :world, :frame, :fps

  # @!visibility private
  attr_reader :joystick_data, :joybutton_data, :joypad_data

  # @!group Simulation Control Functions

  # Determine whether simulation started from selection.
  # @return [Boolean] true if simulation started from selection; false if
  #   simulation started with all groups being considered.
  def started_from_selection?
    @started_from_selection
  end

  # Play simulation.
  # @return [Boolean] success
  def play
    return false unless @paused
    @paused = false
    call_event(:onPlay)
    true
  end

  # Pause simulation.
  # @return [Boolean] success
  def pause
    return false if @paused
    @paused = true
    call_event(:onPause)
    true
  end

  # Play/pause simulation.
  # @return [Boolean] success
  def toggle_play
    @paused ? play : pause
  end

  # Determine if simulation is playing.
  # @return [Boolean]
  def playing?
    !@paused
  end

  # Determine if simulation is paused.
  # @return [Boolean]
  def paused?
    @paused
  end

  # Get simulation update rate, the number of times to update newton world
  # per frame.
  # @return [Integer] A value between 1 and 100.
  def update_rate
    @update_rate
  end

  # Set simulation update rate, the number of times to update newton world
  # per frame.
  # @param [Integer] rate A value between 1 and 100.
  def update_rate=(rate)
    @update_rate = AMS.clamp(rate.to_i, 1, 100)
  end

  # Get the inverse of simulation update timestep.
  # @return [Numeric]
  def update_timestep_inv
    @update_timestep_inv
  end

  # Get simulation update timestep in seconds.
  # @return [Numeric]
  def update_timestep
    @update_timestep
  end

  # Set simulation update time step in seconds.
  # @param [Numeric] timestep This value is clamped between +1/1200.0+ and
  #   +1/30.0+. Normal update time step is +1/60.0+.
  def update_timestep=(timestep)
    @update_timestep = AMS.clamp(timestep, 1/1200.0, 1/30.0)
    @update_timestep_inv = 1.0 / @update_timestep
  end

  # Get simulation mode.
  # @return [Integer] Returns one of the following values:
  # * 0 - Interactive mode: The pick and drag tool and orbiting camera via the
  #   middle mouse button is enabled.
  # * 1 - Game mode: The pick and drag tool and orbiting camera via the middle
  #   mouse button is disabled.
  def mode
    @mode
  end

  # Set simulation mode.
  # @param [Integer] value Pass one of the following values:
  # * 0 - Interactive mode: The pick and drag tool and orbiting camera via the
  #   middle mouse button is enabled.
  # * 1 - Game mode: The pick and drag tool and orbiting camera via the middle
  #   mouse button is disabled.
  def mode=(value)
    @mode = value == 1 ? 1 : 0
  end

  # @!endgroup
  # @!group Cursor Functions

  # Get active cursor.
  # @return [Integer] Cursor id.
  def cursor
    @cursor_id
  end

  # Set active cursor.
  # @example
  #   onStart {
  #     # Set game mode.
  #     simulation.mode = 1
  #     # Set target cursor.
  #     simulation.cursor = MSPhysics::CURSORS[:target]
  #   }
  # @param [Integer] id Cursor id.
  # @return [Integer] The new cursor id.
  # @see MSPhysics::CURSORS
  def cursor=(id)
    @cursor_id = id.to_i
    onSetCursor
    @cursor_id
  end

  # Get cursor position in view coordinates.
  # @return [Array<Integer>] +[x,y]+
  def get_cursor_pos
    [@cursor_pos.x, @cursor_pos.y]
  end

  # Set cursor position in view coordinates.
  # @param [Integer] x
  # @param [Integer] y
  # @note Windows only!
  def set_cursor_pos(x, y)
    if AMS::IS_PLATFORM_WINDOWS
      @cursor_pos.x = x
      @cursor_pos.y = y
      AMS::Cursor.set_pos(x, y, 2)
    end
  end

  # Show/hide mouse cursor.
  # @param [Boolean] state
  # @note Windows only!
  def cursor_visible=(state)
    if AMS::IS_PLATFORM_WINDOWS
      AMS::Cursor.show(state)
    end
  end

  # Determine whether cursor is visible.
  # @return [Boolean]
  # @note Windows only!
  def cursor_visible?
    if AMS::IS_PLATFORM_WINDOWS
      AMS::Cursor.is_visible?
    end
  end

  # @!endgroup
  # @!group Mode and Debug Draw Functions

  # Determine whether the scenes animation is active.
  # @return [Boolean]
  def scenes_animating?
    return false if @scene_anim_info[:state] == 0
    et = @scene_anim_info[:ref_time] - MSPhysics::Settings.animate_scenes_delay
    return et >= 0
  end

  # Determine whether the scene transitioning is active.
  # @return [Boolean]
  def scenes_transitioning?
    @scene_info[:active]
  end

  # Stimulate automatic scene transitioning.
  # @note Other settings like, delay and reverse mode, are acquired from the
  #   settings.
  # @param [Integer] state
  #   * 0 - off/stop
  #   * 1 - one way
  #   * 2 - repeat forth and back
  #   * 3 - loop around
  # @return [Boolean] success
  def animate_scenes(state)
    state = AMS.clamp(state.to_i, 0, 3)
    return false if state == @scene_anim_info[:state]
    if state == 0
      @scene_anim_info[:state] = state
      @scene_anim_info[:data].clear
      @scene_anim_info[:transition_time] = 0
      @scene_anim_info[:elasted_time] = 0
      @scene_anim_info[:ref_time] = 0
      @scene_anim_info[:tabs_size] = nil
      @scene_anim_info[:active_tab] = nil
      @scene_anim_info[:tab_dir] = 1
      return true
    elsif @scene_anim_info[:state] != 0
      @scene_anim_info[:state] = state
      @scene_anim_info[:tab_dir] = 1
      return true
    else
      # Obtain default page parameters
      model = Sketchup.active_model
      default_delay_time = model.options['SlideshowOptions']['SlideTime']
      default_transition_time = model.options['PageOptions']['TransitionTime']
      # Abort any current scene transitioning
      @scene_info[:active] = false
      @scene_info[:data1] = nil
      @scene_info[:data2] = nil
      @scene_info[:transition_time] = 0
      @scene_info[:elasted_time] = 0
      # Gather scene information
      @scene_anim_info[:transition_time] = 0
      tab_index = 0
      tabs_size = model.pages.size
      model.pages.each { |page|
        dt = (page.delay_time < 0 ? default_delay_time : page.delay_time).to_f
        tt = (page.transition_time < 0 ? default_transition_time : page.transition_time).to_f
        @scene_anim_info[:data][tab_index] = {
          :sdelay => @scene_anim_info[:transition_time],
          :edelay => @scene_anim_info[:transition_time] + dt,
          :stransition => @scene_anim_info[:transition_time] + dt,
          :etransition => @scene_anim_info[:transition_time] + dt + tt,
          :scene => MSPhysics::SceneData.new(page),
          :page => page,
          :delay_time => dt,
          :transition_time => tt
        }
        @scene_anim_info[:transition_time] += dt + tt
        tab_index += 1
      }
      # Verify
      if @scene_anim_info[:transition_time] < MSPhysics::EPSILON
        @scene_anim_info[:transition_time] = 0
        @scene_anim_info[:data].clear
        return false
      end
      # Record additional info
      @scene_anim_info[:state] = state
      @scene_anim_info[:elasted_time] = MSPhysics::Settings.animate_scenes_reversed? ? @scene_anim_info[:transition_time] : 0
      @scene_anim_info[:ref_time] = 0
      @scene_anim_info[:tabs_size] = tabs_size
      @scene_anim_info[:active_tab] = nil
      @scene_anim_info[:tab_dir] = 1
      # Return success
      return true
    end
  end

  # Set view full screen.
  # @param [Boolean] state
  # @param [Boolean] include_floating_windows Whether to include floating
  #   toolbars and windows in the operation.
  # @return [Boolean] success
  # @example
  #   onStart {
  #     simulation.view_full_screen(true)
  #   }
  #   onEnd {
  #     simulation.view_full_screen(false)
  #   }
  # @note Windows only!
  def view_full_screen(state, include_floating_windows = true)
    return false unless AMS::IS_PLATFORM_WINDOWS
    AMS::Sketchup.show_toolbar_container(5, !state, false)
    AMS::Sketchup.show_scenes_bar(!state, false)
    AMS::Sketchup.show_status_bar(!state, false)
    AMS::Sketchup.set_viewport_border(!state)
    r1 = AMS::Sketchup.set_menu_bar(!state)
    r2 = AMS::Sketchup.switch_full_screen(state)
    AMS::Sketchup.refresh unless r1 || r2
    if include_floating_windows
      AMS::Sketchup.show_dialogs(!state)
      AMS::Sketchup.show_toolbars(!state)
    end
    @fullscreen_note[:time] = state ? 0.0 : nil
    true
  end

  # Enable/disable the drawing of collision contact points.
  # @param [Boolean] state
  def contact_points_visible=(state)
    @contact_points[:show] = state ? true : false
  end

  # Determine whether the drawing of collision contact points is enabled.
  # @return [Boolean]
  def contact_points_visible?
    @contact_points[:show]
  end

  # Enable/disable the drawing of collision contact forces.
  # @param [Boolean] state
  def contact_forces_visible=(state)
    @contact_forces[:show] = state ? true : false
  end

  # Determine whether the drawing of collision contact forces is enabled.
  # @return [Boolean]
  def contact_forces_visible?
    @contact_forces[:show]
  end

  # Enable/disable the drawing of world axes aligned bounding box for all
  # bodies.
  # @param [Boolean] state
  def aabb_visible=(state)
    @aabb[:show] = state ? true : false
  end

  # Determine whether the drawing of world axes aligned bounding box, for all
  # bodies, is enabled.
  # @return [Boolean]
  def aabb_visible?
    @aabb[:show]
  end

  # Enable/disable the drawing of collision wireframe for all bodies.
  # @param [Boolean] state
  def collision_wireframe_visible=(state)
    state = state ? true : false
    return state if state == @collision_wireframe[:show]
    ro = Sketchup.active_model.rendering_options
    if state
      @collision_wireframe[:show_edges] = ro['EdgeDisplayMode']
      @collision_wireframe[:show_profiles] = ro['DrawSilhouettes']
      ro['EdgeDisplayMode'] = false
      ro['DrawSilhouettes'] = false
    else
      ro['EdgeDisplayMode'] = @collision_wireframe[:show_edges]
      ro['DrawSilhouettes'] = @collision_wireframe[:show_profiles]
    end
    @collision_wireframe[:show] = state
  end

  # Determine whether the drawing of collision wireframe, for all bodies, is
  # enabled.
  # @return [Boolean]
  def collision_wireframe_visible?
    @collision_wireframe[:show]
  end

  # Enable/disable the drawing of centre of mass axes for all bodies.
  # @param [Boolean] state
  def axes_visible=(state)
    @axes[:show] = state ? true : false
  end

  # Determine whether the drawing of centre of mass axes, for all bodies, is
  # enabled.
  # @return [Boolean]
  def axes_visible?
    @axes[:show]
  end

  # Enable/disable the continuous collision check for all bodies. Continuous
  # collision check prevents bodies from passing each other at high speeds.
  # @param [Boolean] state
  def continuous_collision_check_enabled=(state)
    @ccm = state ? true : false
    world_address = @world.address
    MSPhysics::Newton.enable_object_validation(false)
    if @ccm
      body_address = MSPhysics::Newton::World.get_first_body(world_address)
      while body_address
        unless MSPhysics::Newton::Body.get_continuous_collision_state(body_address)
          MSPhysics::Newton::Body.set_continuous_collision_state(body_address, true)
          @cc_bodies << body_address unless @cc_bodies.include?(body_address)
        end
        body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
      end
    else
      @cc_bodies.each { |body_address|
        next unless MSPhysics::Newton::Body.is_valid?(body_address)
        MSPhysics::Newton::Body.set_continuous_collision_state(body_address, false)
      }
      @cc_bodies.clear
    end
    MSPhysics::Newton.enable_object_validation(true)
  end

  # Determine whether the continuous collision check, for all bodies, is
  # intended to be enabled. Continuous collision check prevents bodies from
  # passing each other at high speeds.
  # @return [Boolean]
  def continuous_collision_check_enabled?
    @ccm
  end

  # Show/hide all groups/components associated with the bodies.
  # @param [Boolean] state
  def bodies_visible=(state)
    @show_bodies = state ? true : false
    if @show_bodies
      @hidden_entities.each { |e|
        e.visible = true if e.valid?
      }
      @hidden_entities.clear
    else
      world_address = @world.address
      MSPhysics::Newton.enable_object_validation(false)
      body_address = MSPhysics::Newton::World.get_first_body(world_address)
      while body_address
        data = MSPhysics::Newton::Body.get_user_data(body_address)
        if data.is_a?(MSPhysics::Body) && data.group.visible?
          data.group.visible = false
          @hidden_entities << data.group
        end
        body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
      end
      MSPhysics::Newton.enable_object_validation(true)
    end
    @show_bodies
  end

  # Determine whether all groups/components associated with the bodies are
  # intended to be visible.
  # @return [Boolean]
  def bodies_visible?
    @show_bodies
  end

  # @!endgroup
  # @!group Body/Group Functions

  # Reference body by group/component.
  # @param [Sketchup::Group, Sketchup::ComponentInstance] group
  # @return [Body, nil]
  def find_body_by_group(group)
    AMS.validate_type(group, Sketchup::Group, Sketchup::ComponentInstance)
    data = MSPhysics::Newton::Body.get_body_data_by_group(@world.address, group)
    data.is_a?(MSPhysics::Body) ? data : nil
  end

  # Reference body by group name.
  # @param [String] name Group name.
  # @return [Body, nil] A body or nil if not found.
  def find_body_by_name(name)
    MSPhysics::Newton::World.get_bodies(@world.address) { |ptr, data|
      return data if data.is_a?(MSPhysics::Body) && data.group && data.group.valid? && data.group.name == name
      nil
    }
    nil
  end

  # Reference all bodies by group name.
  # @param [String] name Group name.
  # @return [Array<Body>]
  def find_bodies_by_name(name)
    MSPhysics::Newton::World.get_bodies(@world.address) { |ptr, data|
      data.is_a?(MSPhysics::Body) && data.group && data.group.valid? && data.group.name == name ? data : nil
    }
  end

  # Reference group by name.
  # @param [String] name Group name.
  # @param [Sketchup::Entities, nil] entities Entities to search within. Pass
  #   +nil+ to search within the model entities.
  # @return [Sketchup::Group, Sketchup::ComponentInstance, nil] A group or nil
  #   if not found.
  def find_group_by_name(name, entities = nil)
    (entities ? entities : Sketchup.active_model.entities).each { |e|
      return e if (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && e.name == name
    }
    nil
  end

  # Reference all groups by name.
  # @param [String] name Group name.
  # @param [Sketchup::Entities, nil] entities Entities to search within. Pass
  #   +nil+ to search within the model entities.
  # @return [Array<Sketchup::Group, Sketchup::ComponentInstance>] An array of
  #   groups/components.
  def find_groups_by_name(name, entities = nil)
    groups = []
    (entities ? entities : Sketchup.active_model.entities).each { |e|
      groups << e if (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && e.name == name
    }
    groups
  end

  # Reference joint associated with a group.
  # @param [Sketchup::Group, Sketchup::ComponentInstance] group
  # @return [Joint, nil] A joint or nil if not found.
  def find_joint_by_group(group)
    AMS.validate_type(group, Sketchup::Group, Sketchup::ComponentInstance)
    MSPhysics::Newton::Joint.get_joints_by_group(group) { |ptr, data|
      return data if data.is_a?(MSPhysics::Joint) && data.world == @world
      nil
    }
    nil
  end

  # Reference all joints associated with a group.
  # @param [Sketchup::Group, Sketchup::ComponentInstance] group
  # @return [Array<Joint>]
  def find_joints_by_group(group)
    AMS.validate_type(group, Sketchup::Group, Sketchup::ComponentInstance)
    MSPhysics::Newton::Joint.get_joints_by_group(group) { |ptr, data|
      data.is_a?(MSPhysics::Joint) && data.world == @world ? data : nil
    }
  end

  # Reference joint by name.
  # @param [String] name Joint Name.
  # @return [Joint, nil] A joint or nil if not found.
  def find_joint_by_name(name)
    MSPhysics::Newton::World.get_joints(@world.address) { |ptr, data|
      return data if data.is_a?(MSPhysics::Joint) && data.name == name
      nil
    }
    nil
  end

  # Reference all joints by name.
  # @param [String] name Joint Name.
  # @return [Array<Joint>]
  def find_joints_by_name(name)
    MSPhysics::Newton::World.get_joints(@world.address) { |ptr, data|
      data.is_a?(MSPhysics::Joint) && data.name == name ? data : nil
    }
  end

  # Add a group/component to simulation.
  # @raise [TypeError] if the specified entity is already part of simulation.
  # @raise [TypeError] if the entity doesn't meet demands for being a valid
  #   physics body.
  # @raise [MSPhysics::ScriptException] if there is an error in body script.
  # @param [Sketchup::Group, Sketchup::ComponentInstance] group
  # @return [Body]
  def add_group(group)
    AMS.validate_type(group, Sketchup::Group, Sketchup::ComponentInstance)
    if find_body_by_group(group)
      raise(TypeError, "Entity #{group} is already part of simulation!", caller)
    end
    default = MSPhysics::DEFAULT_BODY_SETTINGS
    bdict = 'MSPhysics Body'
    btype = group.get_attribute(bdict, 'Type', default[:type]).to_i
    shape = group.get_attribute(bdict, 'Shape', default[:shape_id])
    if shape.is_a?(String)
      MSPhysics::SHAPES.each { |k, v|
        if shape == v
          shape = k
          break
        end
      }
      shape = default[:shape_id] if shape.is_a?(String)
    end
    shape_dir = group.get_attribute(bdict, 'Shape Dir', default[:shape_dir]).to_i
    if shape_dir == 1
      collision_transform = Geom::Transformation.new(Y_AXIS.reverse, X_AXIS, Z_AXIS, ORIGIN)
    elsif shape_dir == 2
      collision_transform = Geom::Transformation.new(Z_AXIS, Y_AXIS, X_AXIS.reverse, ORIGIN)
    else
      collision_transform = nil
    end
    body = MSPhysics::Body.new(@world, group, shape, collision_transform, btype)
    if group.get_attribute(bdict, 'Mass Control', default[:mass_control]).to_i == 2
      body.mass = group.get_attribute(bdict, 'Mass', default[:mass]).to_f
    else
      body.density = group.get_attribute(bdict, 'Density', default[:density]).to_f
    end
    body.static_friction = group.get_attribute(bdict, 'Static Friction', default[:static_friction]).to_f
    body.kinetic_friction = group.get_attribute(bdict, 'Kinetic Friction', default[:kinetic_friction]).to_f
    body.elasticity = group.get_attribute(bdict, 'Elasticity', default[:elasticity]).to_f
    body.softness = group.get_attribute(bdict, 'Softness', default[:softness]).to_f
    body.friction_enabled = group.get_attribute(bdict, 'Enable Friction', default[:enable_friction])
    body.magnet_mode = group.get_attribute(bdict, 'Magnet Mode', default[:magnet_mode])
    body.magnet_force = group.get_attribute(bdict, 'Magnet Force', default[:magnet_force]).to_f
    body.magnet_range = group.get_attribute(bdict, 'Magnet Range', default[:magnet_range]).to_f
    body.magnet_strength = group.get_attribute(bdict, 'Magnet Strength', default[:magnet_strength]).to_f
    body.static = group.get_attribute(bdict, 'Static', default[:static])
    body.frozen = group.get_attribute(bdict, 'Frozen', default[:frozen])
    body.magnetic = group.get_attribute(bdict, 'Magnetic', default[:magnetic])
    body.collidable = group.get_attribute(bdict, 'Collidable', default[:collidable])
    body.auto_sleep_enabled = group.get_attribute(bdict, 'Auto Sleep', default[:auto_sleep])
    body.continuous_collision_check_enabled = group.get_attribute(bdict, 'Continuous Collision', default[:continuous_collision])
    ld = group.get_attribute(bdict, 'Linear Damping', default[:linear_damping]).to_f
    body.set_linear_damping(ld,ld,ld)
    ad = group.get_attribute(bdict, 'Angular Damping', default[:angular_damping]).to_f
    body.set_angular_damping(ad,ad,ad)
    body.gravity_enabled = group.get_attribute(bdict, 'Enable Gravity', default[:enable_gravity])
    if group.get_attribute(bdict, 'Enable Script', default[:enable_script])
      script = group.get_attribute('MSPhysics Script', 'Value')
      begin
        body.context.eval_script(script, MSPhysics::SCRIPT_NAME, 1)
      rescue Exception => err
        ref = nil
        test = MSPhysics::SCRIPT_NAME + ':'
        err_message = err.message
        err_backtrace = err.backtrace
        unless AMS::IS_RUBY_VERSION_18
          err_message.force_encoding('UTF-8')
          err_backtrace.each { |i| i.force_encoding('UTF-8') }
        end
        err_backtrace.each { |location|
          if location.include?(test)
            ref = location
            break
          end
        }
        ref = err_message if !ref && err_message.include?(test)
        line = ref ? ref.split(test, 2)[1].split(/\:/, 2)[0].to_i : nil
        msg = "#{err.class.to_s[0] =~ /a|e|i|o|u/i ? 'An' : 'A'} #{err.class} has occurred while evaluating entity script#{line ? ', line ' + line.to_s : nil}:\n#{err_message}"
        raise MSPhysics::ScriptException.new(msg, err_backtrace, group, line)
      end if script.is_a?(String)
    end
    if group.get_attribute(bdict, 'Enable Thruster', default[:enable_thruster])
      controller = group.get_attribute(bdict, 'Thruster Controller')
      if controller.is_a?(String) && !controller.empty?
        @thrusters[body] = {
          :controller => controller,
          :lock_axes => group.get_attribute(bdict, 'Thruster Lock Axes', default[:thruster_lock_axes])
        }
      end
    end
    if group.get_attribute(bdict, 'Enable Emitter', default[:enable_emitter])
      controller = group.get_attribute(bdict, 'Emitter Controller')
      if controller.is_a?(String) && !controller.empty?
        @emitters[body] = {
          :controller => controller,
          :lock_axes => group.get_attribute(bdict, 'Emitter Lock Axes', default[:emitter_lock_axes]),
          :recoil => group.get_attribute(bdict, 'Emitter Recoil', default[:emitter_recoil]),
          :rate => AMS.clamp(group.get_attribute(bdict, 'Emitter Rate', default[:emitter_rate]), MSPhysics::EPSILON, nil),
          :lifetime => AMS.clamp(group.get_attribute(bdict, 'Emitter Lifetime', default[:emitter_lifetime]), 0.0, nil),
          :delay => AMS.clamp(group.get_attribute(bdict, 'Emitter Delay', default[:emitter_delay]), 0.0, nil),
          :flags => nil
        }
      end
    end
    @saved_transformations[group] = group.transformation
    body
  end

  # Remove a group/component from simulation.
  # @param [Sketchup::Group, Sketchup::ComponentInstance] group
  # @return [Boolean] success
  def remove_group(group)
    AMS.validate_type(group, Sketchup::Group, Sketchup::ComponentInstance)
    body = find_body_by_group(group)
    return false unless body
    body.destroy
    true
  end

  # @overload emit_body(body, force, lifetime)
  #   Create a copy of a body and apply force to it.
  #   @param [Body] body The body to emit.
  #   @param [Geom::Vector3d, Array<Numeric>] force The force to apply in
  #     Newtons.
  #   @param [Numeric] lifetime The lifetime in seconds. Pass zero to have the
  #     emitted body live endlessly.
  # @overload emit_body(body, transformation, force, lifetime)
  #   Create a copy of a body at a specific transformation and apply force to
  #   it.
  #   @param [Body] body The body to emit.
  #   @param [Geom::Transformation, Array<Numeric>] transformation
  #   @param [Geom::Vector3d, Array<Numeric>] force The force to apply in
  #     Newtons.
  #   @param [Numeric] lifetime The lifetime in seconds. Pass zero to have the
  #     emitted body live endlessly.
  # @return [Body] A new, emitted body.
  # @example
  #   onTick {
  #     # Emit body every 0.25 seconds if key 'space' is down.
  #     if key('space') == 1 && singular_repeater(0.25) == 1
  #       force = AMS::Geometry.scale_vector(this.get_matrix.yaxis, this.mass * 100)
  #       simulation.emit_body(this, force, 3)
  #     end
  #   }
  def emit_body(*args)
    if args.size == 3
      body, force, lifetime = args
    elsif args.size == 4
      body, tra, force, lifetime = args
    else
      raise(ArgumentError, "Wrong number of arguments! Expected 3..4 arguments but got #{args.size}.", caller)
    end
    new_body = args.size == 3 ? body.copy(true, 0) : body.copy(tra, true, 0)
    new_body.static = false
    new_body.collidable = true
    new_body.continuous_collision_check_enabled = true
    new_body.add_force(force)
    @emitted_bodies[new_body] = lifetime.zero? ? nil : @world.time + lifetime
    @created_entities[new_body.group] = true
    new_body
  end

  # Destroy all emitted bodies and the entities associated with them.
  # @return [Integer] The number of emitted bodies destroyed.
  def destroy_all_emitted_bodies
    count = 0
    @emitted_bodies.each { |body, life|
      if body.valid?
        body.destroy
        count += 1
      end
    }
    @emitted_bodies.clear
    @created_entities.each { |e, s|
      e.erase! if e.valid?
    }
    @created_entities.clear
    count
  end

  # Erase group/component when simulation resets. This method is commonly used
  # for copied bodies. <tt>Body.#copy</tt> method doesn't register created
  # entity to the "erase" queue. When simulation resets created entities
  # remain un-deleted. To erase these entities, one could simply use this
  # method.
  # @param [Sketchup::Drawingelement] entity
  # @return [void]
  # @example Erasing copied entities.
  #   onTick {
  #     if frame % 10 == 0 && key('space') == 1
  #       pt = Geom::Point3d.new(rand(1000), rand(1000), rand(1000))
  #       tra = Geom::Transformation.new(pt)
  #       body = this.copy(tra, true, 0)
  #       simulation.erase_on_end(body.group)
  #     end
  #   }
  def erase_on_end(entity)
    AMS.validate_type(entity, Sketchup::Drawingelement)
    @created_entities[entity] = true
  end

  # @!endgroup
  # @!group Text Control Functions

  # Display text on screen in logged form.
  # @param [String] text
  # @param [Sketchup::Color] color Text color.
  # @return [String] Displayed text
  def log_line(text, color = MSPhysics::WATERMARK_COLOR)
    model = Sketchup.active_model
    if @log_line[:mat].nil? || @log_line[:mat].deleted?
      @log_line[:mat] = model.materials.add('MSPLogLine')
    end
    if @log_line[:ent].nil? || @log_line[:ent].deleted?
      @log_line[:ent] = MSPhysics.add_watermark_text2(10, 50, '', 'LogLine')
      @log_line[:ent].material = @log_line[:mat]
    end
    color = Sketchup::Color.new(color) unless color.is_a?(Sketchup::Color)
    @log_line[:mat].color = color if @log_line[:mat].color.to_i != color.to_i
    @log_line[:log] << text.to_s
    @log_line[:log].shift if @log_line[:log].size > @log_line[:limit]
    @log_line[:ent].set_text(@log_line[:log].join("\n"))
  end

  # Get log-line text limit.
  # @return [Integer]
  def log_line_limit
    @log_line[:limit]
  end

  # Set log-line text limit.
  # @param [Integer] limit Desired limit, a value between 1 and 1000.
  def log_line_limit=(limit)
    @log_line[:limit] = AMS.clamp(limit, 1, 1000)
    ls = @log_line[:log].size
    if ls > @log_line[:limit]
      @log_line[:log] = @log_line[:log][ls-@log_line[:limit]...ls]
      if @log_line[:ent] != nil && @log_line[:ent].valid?
        @log_line[:ent].set_text(@log_line[:log].join("\n"))
      end
    end
  end

  # Clear log-line text.
  # @return [void]
  def clear_log_line
    if @log_line[:ent] != nil && @log_line[:ent].valid?
      @log_line[:ent].set_text("")
    end
    @log_line[:log].clear
  end

  # Display text on screen.
  # @param [String] text A text to display.
  # @param [Sketchup::Color] color Text color.
  # @return [String] Displayed text
  def display_note(text, color = MSPhysics::WATERMARK_COLOR)
    model = Sketchup.active_model
    if @display_note[:mat].nil? || @display_note[:mat].deleted?
      @display_note[:mat] = model.materials.add('MSPDisplayNote')
    end
    if @display_note[:ent].nil? || @display_note[:ent].deleted?
      @display_note[:ent] = MSPhysics.add_watermark_text2(10, 30, '', 'DisplayNote')
      @display_note[:ent].material = @display_note[:mat]
    end
    color = Sketchup::Color.new(color) unless color.is_a?(Sketchup::Color)
    @display_note[:mat].color = color if @display_note[:mat].color.to_i != color.to_i
    @display_note[:ent].set_text(text.to_s)
  end

  # Clear display-note text.
  # @return [void]
  def clear_display_note
    if @display_note[:ent] != nil && @display_note[:ent].valid?
      @display_note[:ent].set_text("")
    end
  end

  # Display a fancy text on screen.
  # @param [String] text
  # @param [Hash] opts
  # @option opts [String] :font ("Ariel") Text font.
  # @option opts [Integer] :size (11) Font size in pixels.
  # @option opts [Boolean] :bold (false) Whether to have the text bold.
  # @option opts [Boolean] :italic (false) Whether to have the text
  #   italicized.
  # @option opts [Integer] :align (TextAlignLeft) Text float:
  #   <tt>TextAlignLeft</tt>, <tt>TextAlignCenter</tt>, or
  #   <tt>TextAlignRight</tt>.
  # @option opts [Sketchup::Color] :color Text color.
  # @option opts [Sketchup::Color, nil] :background Background color.
  #   Pass +nil+ to have a text without background.
  # @option opts [Integer] :padding Background padding in pixels.
  # @option opts [Numeric] :hratio (0.0) Horizontal position ratio on
  #   screen.
  # @option opts [Numeric] :vratio (0.0) Vertical position ratio on
  #   screen.
  # @option opts [Numeric] :duration (5) Display duration in seconds.
  # @option opts [Numreic] :fade (0.25) Fade in and fade out times in seconds.
  # @option opts [Numeric] :twr (0.6) Text width ratio in pixels in case size
  #   computation is not successful.
  # @option opts [Numeric] :thr (1.5) Text height ratio in pixels in case size
  #   computation is not successful.
  # @return [void]
  def fancy_note(text, opts = {})
    clear_fancy_note
    data = {}
    data[:text] = text.to_s
    data[:font] = opts.has_key?(:font) ? opts[:font].to_s : @fancy_note_defaults[:font]
    data[:size] = opts.has_key?(:size) ? opts[:size].to_i : @fancy_note_defaults[:size]
    data[:bold] = opts.has_key?(:bold) ? opts[:bold] : @fancy_note_defaults[:bold]
    data[:italic] = opts.has_key?(:italic) ? opts[:italic] : @fancy_note_defaults[:italic]
    data[:align] = opts.has_key?(:align) ? opts[:align].to_i : @fancy_note_defaults[:align]
    data[:color] = opts.has_key?(:color) ? Sketchup::Color.new(opts[:color]) : @fancy_note_defaults[:color]
    if opts.has_key?(:background)
      data[:background] = opts[:background] ? Sketchup::Color.new(opts[:background]) : nil
    else
      data[:background] = @fancy_note_defaults[:background]
    end
    data[:padding] = opts.has_key?(:padding) ? opts[:padding].to_i : @fancy_note_defaults[:padding]
    data[:hratio] = opts.has_key?(:hratio) ? opts[:hratio].to_f : @fancy_note_defaults[:hratio]
    data[:vratio] = opts.has_key?(:vratio) ? opts[:vratio].to_f : @fancy_note_defaults[:vratio]
    data[:duration] = opts.has_key?(:duration) ? AMS.clamp(opts[:duration].to_f, 0.01, nil) : @fancy_note_defaults[:duration]
    data[:fade] = opts.has_key?(:fade) ? AMS.clamp(opts[:fade].to_f, 0.0, nil) : @fancy_note_defaults[:fade]
    data[:twr] = opts.has_key?(:twr) ? opts[:twr].to_f : @fancy_note_defaults[:twr]
    data[:thr] = opts.has_key?(:thr) ? opts[:thr].to_f : @fancy_note_defaults[:thr]
    unless MSPhysics::ControlPanel.open?
      MSPhysics::ControlPanel.open
      MSPhysics::ControlPanel.hide
    end
    size = MSPhysics::ControlPanel.compute_text_size(data[:text], data)
    unless size
      num_lines = 1
      for i in 0...data[:text].size
        num_lines += 1 if data[:text][i] == "\n"
      end
      size = [data[:twr] * data[:size] * data[:text].size, data[:thr] * data[:size] * num_lines]
    end
    data[:time] = 0.0
    data[:sx] = size.x + data[:padding] * 2
    data[:sy] = size.y + data[:padding] * 2
    data[:color2] = Sketchup::Color.new(data[:color])
    data[:background2] = data[:background] ? Sketchup::Color.new(data[:background]) : nil
    data[:alpha] = 0.0
    @fancy_note = data
  end

  # Clear fancy note.
  # @return [void]
  def clear_fancy_note
    return unless @fancy_note
    @prev_fancy_note = @fancy_note
    @fancy_note = nil
    if @prev_fancy_note.empty? || @prev_fancy_note[:fade] < MSPhysics::EPSILON
      @prev_fancy_note = nil
    elsif @prev_fancy_note[:time] > @prev_fancy_note[:fade]
      @prev_fancy_note[:time] = @prev_fancy_note[:fade]
    end
  end

  # @!endgroup
  # @!group Viewport Drawing Functions

  # Draw 2D geometry into view.
  # @param [String, Symbol] type Drawing type. Use one of the following:
  #   * <tt>"points"</tt> - Draw a collection of points. Each vertex is
  #     treated as a single point. Vertex n defines point n. N points are
  #     drawn.
  #   * <tt>"lines"</tt> - Draw a collection of independent lines. Each pair
  #     of vertices is treated as a single line. Vertices 2n-1 and 2n define
  #     line n. N/2 lines are drawn.
  #   * <tt>"line_strip"</tt> - Draw a connected group of line segments from
  #     the first vertex to the last. Vertices n and n+1 define line n. N-1
  #     lines are drawn.
  #   * <tt>"line_loop"</tt> - Draw a connected group of line segments from
  #     the first vertex to the last, then back to the first. Vertices n and
  #     n+1 define line n. The last line, however, is defined by vertices N
  #     and 1. N lines are drawn.
  #   * <tt>"triangles"</tt> - Draw a group of independent triangles. Each
  #     triplet of vertices is considered a single triangle. Vertices 3n-2,
  #     3n-1, and 3n define triangle n. N/3 triangles are drawn.
  #   * <tt>"triangle_strip"</tt> - Draw a connected group of triangles. One
  #     triangle is defined for each vertex presented after the first two
  #     vertices. For odd n, vertices n, n+1, and n+2 define triangle n. For
  #     even n, vertices n+1, n, and n+2 define triangle n. N-2 triangles are
  #     drawn.
  #   * <tt>"triangle_fan"</tt> - Draw a connected group of triangles. One
  #     triangle is defined for each vertex presented after the first two
  #     vertices. Vertices 1, n+1, and n+2 define triangle n. N-2 triangles
  #     are drawn.
  #   * <tt>"quads"</tt> - Draw a collection of independent quadrilaterals. A
  #     group of four vertices is treated as a single quadrilateral. Vertices
  #     4n-3, 4n-2, 4n-1, and 4n define quadrilateral n. N/4 quadrilaterals
  #     are drawn.
  #   * <tt>"quad_strip"</tt> - Draw a collection of connected quadrilaterals.
  #     One quadrilateral is defined for each pair of vertices presented after
  #     the first pair. Vertices 2n-1, 2n, 2n+2, and 2n+1 define quadrilateral n.
  #     N/2-1 quadrilaterals are drawn. Note that the order in which
  #     vertices are used to construct a quadrilateral from strip data is
  #     different from that used with independent data.
  #   * <tt>"polygon"</tt> - Draws a single convex polygon. Vertices 1 through
  #     N define this polygon.
  # @param [Array<Geom::Point3d, Array<Numeric>>] points An array of points.
  # @param [Sketchup::Color, Array, String] color Drawing color.
  # @param [Integer] width Line width in pixels.
  # @param [String] stipple Line stipple. Use one of the following:
  #  * <tt>"."</tt> - dotted line
  #  * <tt>"-"</tt> - short-dashed line
  #  * <tt>"_"</tt> - long-dashed line
  #  * <tt>"-.-"</tt> - dash dot dash line
  #  * <tt>""</tt> - solid line
  # @return [void]
  def draw2d(type, points, color = 'black', width = 1, stipple = '')
    type = case type.to_s.downcase.gsub(/\s/, '_').to_sym
      when :points
        GL_POINTS
      when :lines
        GL_LINES
      when :line_strip
        GL_LINE_STRIP
      when :line_loop
        GL_LINE_LOOP
      when :triangles
        GL_TRIANGLES
      when :triangle_strip
        GL_TRIANGLE_STRIP
      when :triangle_fan
        GL_TRIANGLE_FAN
      when :quads
        GL_QUADS
      when :quad_strip
        GL_QUAD_STRIP
      when :polygon
        GL_POLYGON
    else
      raise(TypeError, 'Invalid type!', caller)
    end
    @draw_queue << [type, points, color, width, stipple, 0]
  end

  # Draw 3D geometry into view.
  # @param (see #draw2d)
  # @return (see #draw2d)
  def draw3d(type, points, color = 'black', width = 1, stipple = '')
    type = case type.to_s.downcase.gsub(/\s/, '_').to_sym
      when :points
        GL_POINTS
      when :lines
        GL_LINES
      when :line_strip
        GL_LINE_STRIP
      when :line_loop
        GL_LINE_LOOP
      when :triangles
        GL_TRIANGLES
      when :triangle_strip
        GL_TRIANGLE_STRIP
      when :triangle_fan
        GL_TRIANGLE_FAN
      when :quads
        GL_QUADS
      when :quad_strip
        GL_QUAD_STRIP
      when :polygon
        GL_POLYGON
    else
      raise(TypeError, 'Invalid type!', caller)
    end
    @draw_queue << [type, points, color, width, stipple, 1]
  end

  # Draw 3D points with custom style.
  # @param [Array<Geom::Point3d, Array<Numeric>>] points An array of points.
  # @param [Integer] size Point size in pixels.
  # @param [Integer] style Point style. Use one of the following:
  #   0. none
  #   1. open square
  #   2. filled square
  #   3. + cross
  #   4. x cross
  #   5. star
  #   6. open triangle
  #   7. filled triangle
  # @param [Sketchup::Color, Array, String] color Point color.
  # @param [Integer] width Line width in pixels.
  # @param [String] stipple Line stipple. Use one of the following:
  #  * <tt>"."</tt> - dotted line
  #  * <tt>"-"</tt> - short-dashed line
  #  * <tt>"_"</tt> - long-dashed line
  #  * <tt>"-.-"</tt> - dash dot dash line
  #  * <tt>""</tt> - solid line
  # @return [void]
  def draw_points(points, size = 1, style = 0, color = 'black', width = 1, stipple = '')
    @points_queue << [points, size, style, color, width, stipple]
  end

  # @!endgroup

  # @!group Music, Sound, and MIDI Functions

  # Play embedded sound by name.
  # @note If this function succeeds, it returns a channel the sound was
  #   registered to play on. The returned channel can be adjusted to desired
  #   volume and panning.
  # @note On Windows, this can load WAVE, AIFF, RIFF, OGG, and VOC formats.
  #   Mac OS X is limited to WAVE sounds.
  # @example Play 3D effect when space is pressed.
  #   onKeyDown { |key, value, char|
  #     if key == 'space'
  #       channel = simulation.play_sound("MyEffect1", -1, 0)
  #       max_hearing_range = 1000 # Set hearing range to 1000 meters.
  #       simulation.position_sound(channel, this.get_position(1), max_hearing_range)
  #     end
  #   }
  # @param [String] name The name of embedded sound.
  # @param [Integer] channel The channel to play the sound at. Pass -1 to play
  #   sound at the available channel.
  # @param [Integer] repeat The number of times to play the sound plus one.
  #   Pass -1 to play sound infinite times.
  # @return [Integer, nil] A channel the sound is set to be played on or nil if
  #   mixer failed to play sound.
  # @raise [TypeError] if sound is invalid.
  def play_sound(name, channel = -1, repeat = 0)
    sound = MSPhysics::Sound.get_by_name(name)
    unless sound
      type = Sketchup.active_model.get_attribute('MSPhysics Sound Types', name, nil)
      unless type
        raise(TypeError, "Sound with name \"#{name}\" doesn't exist!", caller)
      end
      unless MSPhysics::EMBEDDED_SOUND_FORMATS.include?(type)
        raise(TypeError, "Sound format is not supported!", caller)
      end
      data = Sketchup.active_model.get_attribute('MSPhysics Sounds', name, nil)
      unless data
        raise(TypeError, "Sound with name \"#{name}\" doesn't exist!", caller)
      end
      buf = data.pack('l*')
      sound = MSPhysics::Sound.create_from_buffer(buf, buf.size)
      MSPhysics::Sound.set_name(sound, name)
    end
    MSPhysics::Sound.play(sound, channel, repeat)
  end

  # Play sound from path.
  # @note If this function succeeds, it returns a channel the sound was
  #   registered to play on. The returned channel can be adjusted to desired
  #   volume and panning.
  # @note On Windows, this can load WAVE, AIFF, RIFF, OGG, VOC, and FLAC
  #   formats. Mac OS X is limited to WAVE sounds.
  # @param [String] path Full path of the sound.
  # @param [Integer] channel The channel to play the sound at. Pass -1 to play
  #   sound at the available channel.
  # @param [Integer] repeat The number of times to play the sound plus one.
  #   Pass -1 to play sound infinite times.
  # @return [Integer, nil] A channel the sound is set to be played on or nil if
  #   mixer failed to play sound.
  # @raise [TypeError] if sound is invalid.
  def play_sound2(path, channel = -1, repeat = 0)
    sound = MSPhysics::Sound.create_from_dir(path)
    MSPhysics::Sound.play(sound, channel, repeat)
  end

  # Stop the currently playing sound at channel.
  # @param [Integer] channel The channel returned by {#play_sound} or
  #   {#play_sound2} functions. Pass -1 to stop all sounds.
  # @return [Boolean] success
  def stop_sound(channel)
    MSPhysics::Sound.stop(channel)
    true
  end

  # Set sound 3D position.
  # @note Sound volume and panning is adjusted automatically with respect to
  #   camera orientation. You don't need to call this function every frame if
  #   sound remains in constant position. You do, however, need to call this
  #   function if sound position changes.
  # @param [Integer] channel The channel the sound is being played on.
  # @param [Geom::Point3d, Array<Numeric>] pos Sound position in global space.
  # @param [Numeric] max_hearing_range The maximum hearing range of the sound
  #   in meters.
  # @return [Boolean] success
  def position_sound(channel, pos, max_hearing_range = 100)
    MSPhysics::Sound.set_position_3d(channel, pos, max_hearing_range)
  end

  # Play embedded music by name. This can load WAVE, AIFF, RIFF, OGG, FLAC,
  # MOD, IT, XM, and S3M formats.
  # @example Start playing music when simulation starts.
  #   onStart {
  #     simulation.play_music("MyBackgroundMusic", -1)
  #   }
  # @param [String] name The name of embedded music.
  # @param [Integer] repeat The number of times to play the music plus one.
  #   Pass -1 to play music infinite times.
  # @return [Boolean] success
  # @raise [TypeError] if music is invalid.
  def play_music(name, repeat = 0)
    music = MSPhysics::Music.get_by_name(name)
    unless music
      type = Sketchup.active_model.get_attribute('MSPhysics Sound Types', name, nil)
      unless type
        raise(TypeError, "Music with name \"#{name}\" doesn't exist!", caller)
      end
      unless MSPhysics::EMBEDDED_MUSIC_FORMATS.include?(type)
        raise(TypeError, "Music format is not supported!", caller)
      end
      data = Sketchup.active_model.get_attribute('MSPhysics Sounds', name, nil)
      unless data
        raise(TypeError, "Music with name \"#{name}\" doesn't exist!", caller)
      end
      buf = data.pack('l*')
      music = MSPhysics::Music.create_from_buffer(buf, buf.size)
      MSPhysics::Music.set_name(music, name)
    end
    MSPhysics::Music.play(music, repeat)
  end

  # Play music from path. This can load WAVE, AIFF, RIFF, OGG, FLAC, MOD, IT,
  # XM, and S3M formats.
  # @param [String] path Full path of the music.
  # @param [Integer] repeat The number of times to play the music plus one.
  #   Pass -1 to play music infinite times.
  # @return [Boolean] success
  # @raise [TypeError] if music is invalid.
  def play_music2(path, repeat = 0)
    music = MSPhysics::Music.create_from_dir(path)
    MSPhysics::Music.play(music, repeat)
  end

  # Stop the currently playing music.
  # @return [nil]
  def stop_music
    MSPhysics::Music.stop
  end

  # Play MIDI note.
  # @note Setting channel to 9 will play midi notes from the "General MIDI
  #   Percussion Key Map." Any other channel will play midi notes from the
  #   "General MIDI Instrument Patch Map". If channel is set to 9, the
  #   instrument parameter will have no effect and the note parameter will be
  #   used to play particular percussion sound, if note's value is between 27
  #   and 87. According to my experiments, values outside that 27-87 range
  #   won't yield any sounds.
  # @note Some instruments have notes that never seem to end. For this reason
  #   it might come in handy to use {#stop_midi_note} function when needed.
  # @param [Integer] instrument A value between 0 and 127. See link below for
  #   supported instruments and their identifiers.
  # @param [Integer] note A value between 0 and 127. Each instrument has a
  #   maximum of 128 notes.
  # @param [Integer] channel A value between 0 and 15. Each note has a maximum
  #   of 16 channels. To play multiple sounds of same type at the same time,
  #   change channel value to an unused one. Remember that channel 9 is
  #   subjected to different instrument patch and it will change the behaviour
  #   of this function; see note above.
  # @param [Integer] volume A value between 0 and 127. 0 means quiet/far and
  #   127 means close/loud.
  # @return [Integer, nil] Midi note ID or nil if MIDI interface failed to play
  #   the note.
  # @see http://wiki.fourthwoods.com/midi_file_format#general_midi_instrument_patch_map General MIDI Instrument Patch Map
  def play_midi_note(instrument, note = 63, channel = 0, volume = 127)
    AMS::MIDI.play_note(instrument, note, channel, volume);
  end

  # Stop MIDI note.
  # @param [Integer] id A MIDI note identifier returned by the
  #   {#play_midi_note} function. Pass -1 to stop all midi notes.
  # @return [Boolean] success
  def stop_midi_note(id)
    if id == -1
      for i in 0..15
        AMS::MIDI.change_channel_controller(i, 0x7B, 0)
      end
      AMS::MIDI.reset
    else
      AMS::MIDI.stop_note(id)
    end
  end

  # Set MIDI note position in 3D space.
  # @note Sound volume and panning is not adjusted automatically with respect
  #   to camera orientation. It is required to manually call this function
  #   every frame until the note is stopped or has finished playing. Sometimes
  #   it's just enough to call this function once after playing the note.
  #   Other times, when the note is endless or pretty long, it might be useful
  #   to update position of the note every frame until the note ends or is
  #   stopped. Meantime, there is no function to determine when the note ends.
  #   It is up to the user to decide for how long to call this function or
  #   when to stop calling this function.
  # @note When it comes to setting 3D positions of multiple sounds, make sure
  #   to play each sound on separate channel. That is, play sound 1 on channel
  #   0, sound 2 on channel 1, sound 3 on channel 2, and etcetera until
  #   channel gets to 15, as there are only 15 channels available. Read the
  #   note below to find out why each sound is supposed to be played on
  #   separate channel. I think it would make more sense if the function was
  #   renamed to <tt>set_midi_channel_position</tt> and had the 'id' parameter
  #   replaced with 'channel'.
  # @note This function works by adjusting panning and volume of the note's
  #   and instrument's channel, based on camera's angle and distance to the
  #   origin of the sound. Now, there is only one function that adjusts stereo
  #   and panning, but it adjusts panning and volume of all notes and
  #   instruments that are played on same channel. As of my research, I
  #   haven't found a way to adjust panning and volume of channel that belongs
  #   to particular note and instrument. There's only a function that can
  #   adjust panning and volume of channel that belongs to all notes and
  #   instruments that are played on particular channel. For instance, if you
  #   play instrument 1 and instrument 2 both on channel zero, they will still
  #   play simultaneously, without cancelling out each other, as if they are
  #   playing on separate channels, but when it comes to adjusting panning and
  #   volume of one of them, the properties of both sounds will be adjusted.
  #   This means that this function is only limited to playing 16 3D sounds,
  #   with each sound played on different channel. Otherwise, sounds played on
  #   same channel at different locations, will endup being tuned as if they
  #   are playing from the same location.
  # @example Play 3D note.
  #   onKeyDown { |k,v,c|
  #     if k == 'space'
  #       id = simulation.play_midi_note(2, 63, 0, 127)
  #       simulation.position_midi_note(id, this.get_position(1), 100) if id
  #     end
  #   }
  # @param [Integer] id A MIDI note identifier returned by the
  #   {#play_midi_note} function.
  # @param [Geom::Point3d, Array<Numeric>] pos MIDI note position in global
  #   space.
  # @param [Numeric] max_hearing_range MIDI note maximum hearing range in
  #   meters.
  # @return [Boolean] success
  def position_midi_note(id, pos, max_hearing_range = 100)
    AMS::MIDI.set_note_position(id, pos, max_hearing_range)
  end

  # @!endgroup
  # @!group Particle Effects

  # Create a new particle.
  # @param [Hash] opts Particle options.
  # @option opts [Geom::Point3d, Array] :position (ORIGIN) Starting position.
  #   Position is altered by particle velocity and time.
  # @option opts [Geom::Vector3d, Array] :velocity (nil) Starting velocity
  #   in inches per second. Pass nil if velocity is not necessary.
  # @option opts [Numeric] :velocity_damp (0.0) Velocity damping,
  #   a value between 0.0 and 1.0.
  # @option opts [Geom::Vector3d, Array] :gravity (nil) Gravitational
  #   acceleration in inches per second per second. Pass nil if gravity is not
  #   necessary.
  # @option opts [Numeric] :radius (0.1) Starting radius in inches, a value
  #   between 1.0e-4 and 1.0e6. Radius is influenced by the scale option.
  # @option opts [Numeric] :scale (1.01) Radius scale ratio per second, a
  #   value between 1.0e-4 and 1.0e6. If radius becomes less than 0.001 or more
  #   than 10000, the particle is automatically destroyed.
  # @option opts [Sketchup::Color, Array, String, Integer] :color1 ('Gray')
  #   Starting color.
  # @option opts [Sketchup::Color, Array, String, Integer] :color2 (nil) Ending
  #   color. Pass nil to have the ending color remain same as the starting
  #   color.
  # @option opts [Numeric] :alpha1 (1.0) Starting opacity, a value between
  #   0.0 and 1.0.
  # @option opts [Numeric] :alpha2 (nil) Ending opacity, a value between 0.0
  #   and 1.0. Pass nil if ending opacity is ought to be the same as the
  #   starting opacity.
  # @option opts [Numeric] :fade (0.0) A time ratio it should take the effect
  #   to fade into the starting opacity and fade out from the ending opacity,
  #   a value between 0.0 and 1.0.
  # @option opts [Integer] :lifetime (3.0) Particle lifetime in seconds, a value
  #   greater than zero.
  # @option opts [Integer] :num_seg (16) Number of segments the particle is to
  #   consist of, a value between 3 and 360.
  # @option opts [Numeric] :rot_angle (0.0) Rotate angle in radians.
  # @option opts [Integer] :type (1)
  #   1. Defines a 2D circular particle that is drawn through view drawing
  #      functions. This type is fast, but particle shade and shadow is not
  #      present. Also, this particle doesn't blend quite well with other
  #      particles of this type.
  #   2. Defines a 2D circular particle that is created from SketchUp
  #      geometry. This type is normal, and guarantees good, balanced results.
  #   3. Defines a 3D spherical particle that is crated from SketchUp
  #      geometry. This type is slow, but it guarantees best results.
  # @return [void]
  def create_particle(opts)
    if opts[:type] == 1
      MSPhysics::C::Particle.create(opts)
      return
    end
    opts2 = {
      :position       => opts[:position] ? Geom::Point3d.new(opts[:position]) : Geom::Point3d.new(0, 0, 0),
      :velocity       => opts[:velocity] ? Geom::Vector3d.new(opts[:velocity]) : nil,
      :velocity_damp  => opts[:velocity_damp] ? AMS.clamp(opts[:velocity_damp].to_f, 0.0, 1.0) : 0.0,
      :gravity        => opts[:gravity] ? Geom::Vector3d.new(opts[:gravity]) : nil,
      :radius         => opts[:radius] ? AMS.clamp(opts[:radius].to_f, 1.0e-4, 1.0e6) : 0.1,
      :scale          => opts[:scale] ? AMS.clamp(opts[:scale].to_f, 1.0e-4, 1.0e6) : 1.01,
      :color1         => opts[:color1] ? Sketchup::Color.new(opts[:color1]) : Sketchup::Color.new('Gray'),
      :color2         => opts[:color2] ? Sketchup::Color.new(opts[:color2]) : nil,
      :alpha1         => opts[:alpha1] ? AMS.clamp(opts[:alpha1].to_f, 0.0, 1.0) : 1.0,
      :alpha2         => opts[:alpha2] ? AMS.clamp(opts[:alpha2].to_f, 0.0, 1.0) : nil,
      :fade           => opts[:fade] ? AMS.clamp(opts[:fade].to_f, 0.0, 1.0) : 0.0,
      :lifetime       => opts[:lifetime] ? AMS.clamp(opts[:lifetime].to_f, 1.0e-6, nil) : 3.0,
      :num_seg        => opts[:num_seg] ? AMS.clamp(opts[:num_seg].to_i, 3, 360) : 16,
      :rot_angle      => opts[:rot_angle] ? opts[:rot_angle].to_f : 0.0,
      :type           => opts[:type] ? AMS.clamp(opts[:type].to_i, 1, 3) : 1
    }
    opts2[:life_start] = @world.time
    opts2[:life_end] = opts2[:life_start] + opts2[:lifetime]
    opts2[:color] = Sketchup::Color.new(opts2[:color1])
    opts2[:color].alpha = opts2[:fade].zero? ? opts2[:alpha1] : 0.0
    @particles << opts2
    model = Sketchup.active_model
    default_layer = model.layers[0]
    cd = nil
    if opts2[:type] == 3 # 3D entity
      cd = @particle_def3d[opts2[:num_seg]]
      if cd.nil? || cd.deleted?
        cd = model.definitions.add("AP3D_#{opts2[:num_seg]}")
        @particle_def3d[opts2[:num_seg]] = cd
        e = cd.entities
        c1 = e.add_circle(ORIGIN, X_AXIS, 1, opts2[:num_seg])
        c2 = e.add_circle([0,0,-10], Z_AXIS, 1, opts2[:num_seg])
        c1.each { |edge| edge.hidden = true }
        f = e.add_face(c1)
        f.followme(c2)
        c2.each { |edge| edge.erase! }
        # Create an instance to ensure the definition is not erased.
        instance = model.entities.add_instance(cd, Geom::Transformation.new())
        instance.layer = default_layer if (instance.layer != default_layer)
        instance.visible = false
        instance.set_attribute('MSPhysics', 'Type', 'Particle')
        instance.set_attribute('MSPhysics Body', 'Ignore', true)
        @dp_particle_instances << instance
      end
      normal = Geom::Vector3d.new(Math.cos(opts2[:rot_angle]), Math.sin(opts2[:rot_angle]), 0)
    else # 2D entity
      cd = @particle_def2d[opts2[:num_seg]]
      if cd.nil? || cd.deleted?
        cd = model.definitions.add("MSP_P2D_#{opts2[:num_seg]}")
        @particle_def2d[opts2[:num_seg]] = cd
        e = cd.entities
        c = e.add_circle(ORIGIN, Z_AXIS, 1, opts2[:num_seg])
        c.each { |edge| edge.hidden = true }
        e.add_face(c)
        # Create an instance to ensure the definition is not erased.
        instance = model.entities.add_instance(cd, Geom::Transformation.new())
        instance.layer = default_layer if (instance.layer != default_layer)
        instance.visible = false
        instance.set_attribute('MSPhysics', 'Type', 'Particle')
        instance.set_attribute('MSPhysics Body', 'Ignore', true)
        @dp_particle_instances << instance
      end
      eye = model.active_view.camera.eye
      normal = (eye == opts2[:position]) ? Z_AXIS : opts2[:position].vector_to(eye)
    end
    tra1 = Geom::Transformation.new(opts[:position], normal)
    tra2 = Geom::Transformation.rotation(ORIGIN, Z_AXIS, opts[:rot_angle])
    tra3 = Geom::Transformation.scaling(opts[:radius])
    tra = tra1*tra2*tra3
    opts2[:material] = model.materials.add('FX')
    opts2[:material].color = opts2[:color]
    opts2[:material].alpha = opts2[:color].alpha / 255.0
    opts2[:group] = model.entities.add_instance(cd, tra)
    opts2[:group].material = opts2[:material]
    opts2[:group].layer = default_layer if (opts2[:group].layer != default_layer)
    opts2[:group].visible = false unless @particles_visible
    #opts2[:group].receives_shadows = false if opts2[:group].receives_shadows?
    #opts2[:group].casts_shadows = false if opts2[:group].casts_shadows?
  end

  # Get number of particles.
  # @return [Integer]
  def particles_count
    @particles.size + MSPhysics::C::Particle.size
  end

  # Remove all particles.
  def clear_particles
    model = Sketchup.active_model
    mats = model.materials
    @particles.each { |opts|
      if opts[:group] && opts[:group].valid?
        opts[:group].material = nil
        opts[:group].erase!
      end
      mats.remove(opts[:material]) if mats.respond_to?(:remove)
    }
    @particles.clear
    MSPhysics::C::Particle.destroy_all
  end

  # Show/hide particles.
  # @param [Boolean] state
  def particles_visible=(state)
    state = state ? true : false
    @particles_visible = state
    @particles.each { |opts|
      next if opts[:type] == 1
      opts[:group].visible = state if opts[:group].valid? && opts[:group].visible? != state
    }
  end

  # Determine whether particles are visible.
  # @return [Boolean]
  def particles_visible?
    @particles_visible
  end

  # @!endgroup
  # @!group Advanced

  # Determine whether the undo command is ought to be triggered after the
  # simulation resets.
  # @return [Boolean]
  def undo_on_reset?
    @undo_on_reset
  end

  # Enable/disable the undo commend that is ought to be triggered after the
  # simulation resets.
  # @param [Boolean] state
  def undo_on_reset=(state)
    @undo_on_reset = state ? true : false
  end

  # Determine whether the reseting of group/component transformations when
  # simulation ends is enabled.
  # @return [Boolean]
  def reset_positions_on_end?
    @reset_positions_on_end
  end

  # Enable/disable the reseting of group/component transformations when
  # simulation ends.
  # @param [Boolean] state
  def reset_positions_on_end=(state)
    @reset_positions_on_end = state ? true : false
  end

  # Determine whether the reseting of camera when simulation ends is enabled.
  # @return [Boolean]
  def reset_camera_on_end?
    @reset_camera_on_end
  end

  # Enable/disable the reseting of camera when simulation ends.
  # @param [Boolean] state
  def reset_camera_on_end=(state)
    @reset_camera_on_end = state ? true : false
  end

  # Determine whether the deletion of emitted/copied/split bodies and
  # particles when simulation ends is enabled.
  # @return [Boolean]
  def erase_instances_on_end?
    @erase_instances_on_end
  end

  # Enable/disable the deletion of emitted/copied/split bodies and particles
  # when simulation ends.
  # @param [Boolean] state
  def erase_instances_on_end=(state)
    @erase_instances_on_end = state ? true : false
  end

  # @!endgroup

  private

  def update_joy_data
    @joystick_data.clear
    @joybutton_data.clear
    @joypad_data = 0
    return if MSPhysics::Joystick.get_num_joysticks == 0
    joys = MSPhysics::Joystick.get_open_joysticks
    joy = joys.empty? ?  MSPhysics::Joystick.open(0) : joys[0]
    return unless joy
    MSPhysics::Joystick.update
    axes_count = MSPhysics::Joystick.get_num_axes(joy)
    buttons_count = MSPhysics::Joystick.get_num_buttons(joy)
    if (axes_count == 6)
      # Using XInput Controller
      index = 0
      JOYSTICK1_AXES.each { |axis_name|
        if index < axes_count
          v = MSPhysics::Joystick.get_axis(joy, index)
          r = v < 0 ? v / 32768.0 : v / 32767.0
          @joystick_data[axis_name] = r
        end
        index += 1
      }
      index = 0
      JOYSTICK1_BUTTONS.each { |button_name|
        if index < buttons_count
          @joybutton_data[button_name] = MSPhysics::Joystick.get_button(joy, index)
        end
        index += 1
      }
      # Link lt and rt buttons with leftz and rightz axes
      @joybutton_data['lt'] = (@joystick_data['leftz'].to_f * 0.5 + 0.5).round
      @joybutton_data['rt'] = (@joystick_data['rightz'].to_f * 0.5 + 0.5).round
    else
      # Using Dual Controller
      index = 0
      JOYSTICK2_AXES.each { |axis_name|
        if index < axes_count
          v = MSPhysics::Joystick.get_axis(joy, index)
          r = v < 0 ? v / 32768.0 : v / 32767.0
          @joystick_data[axis_name] = r
        end
        index += 1
      }
      index = 0
      JOYSTICK2_BUTTONS.each { |button_name|
        if index < buttons_count
          @joybutton_data[button_name] = MSPhysics::Joystick.get_button(joy, index)
        end
        index += 1
      }
      # Link leftz and rightz axes with lt and rt buttons
      @joystick_data['leftz'] = @joybutton_data['lt'].to_f * 2 - 1
      @joystick_data['rightz'] =  @joybutton_data['rt'].to_f * 2 - 1
    end
    if MSPhysics::Joystick.get_num_hats(joy) > 0
      @joypad_data = MSPhysics::Joystick.get_hat(joy, 0)
    end
  end

  def update_particles
    model = Sketchup.active_model
    mats = model.materials
    eye = model.active_view.camera.eye
    MSPhysics::C::Particle.update_all(@update_timestep)
    @particles.reject! { |opts|
      # Compute particle time
      rtime = @world.time - opts[:life_start]
      # Control radius
      opts[:radius] *= opts[:scale]
      # Check if need to delete the particle
      if opts[:group].deleted? || opts[:material].deleted? || rtime > opts[:lifetime] || opts[:radius] < 1.0e-3 || opts[:radius] > 1.0e6
        if opts[:group].valid?
          opts[:group].material = nil
          opts[:group].erase!
        end
        mats.remove(opts[:material]) if opts[:material].valid? && mats.respond_to?(:remove)
        next true
      end
      # Transition color
      ratio = rtime / opts[:lifetime]
      if opts[:color2]
        c = opts[:color]
        c1 = opts[:color1]
        c2 = opts[:color2]
        c.red = c1.red + ((c2.red - c1.red) * ratio).to_i
        c.green = c1.green + ((c2.green - c1.green) * ratio).to_i
        c.blue = c1.blue + ((c2.blue - c1.blue) * ratio).to_i
      end
      # Transition opacity
      if opts[:alpha2]
        if opts[:fade].zero?
          opts[:color].alpha = opts[:alpha1] + (opts[:alpha2] - opts[:alpha1]) * ratio
        else
          fh = opts[:fade] * 0.5
          if ratio < fh
            r = rtime / (opts[:lifetime] * fh)
            opts[:color].alpha = opts[:alpha1] * r
          elsif ratio >= (1.0 - fh)
            r = (opts[:lifetime] - rtime) / (opts[:lifetime] * fh)
            opts[:color].alpha = opts[:alpha2] * r
          else
            fl = opts[:lifetime] * opts[:fade]
            r = (rtime - fl * 0.5) / (opts[:lifetime] - fl)
            opts[:color].alpha = opts[:alpha1] + (opts[:alpha2] - opts[:alpha1]) * r
          end
        end
      else
        if opts[:fade].zero?
          opts[:color].alpha = opts[:alpha1]
        else
          fh = opts[:fade] * 0.5
          if ratio < fh
            r = rtime / (opts[:lifetime] * fh)
            opts[:color].alpha = opts[:alpha1] * r
          elsif ratio >= (1.0 - fh)
            r = (opts[:lifetime] - rtime) / (opts[:lifetime] * fh).to_f
            opts[:color].alpha = opts[:alpha1] * r
          else
            opts[:color].alpha = opts[:alpha1]
          end
        end
      end
      # Control velocity and position
      pos = opts[:position]
      vel = opts[:velocity]
      if vel
        gra = opts[:gravity]
        if gra
          vel.x += gra.x * @update_timestep
          vel.y += gra.y * @update_timestep
          vel.z += gra.z * @update_timestep
        end
        if opts[:velocity_damp] != 0
          s = 1.0 - opts[:velocity_damp]
          vel.x *= s
          vel.y *= s
          vel.z *= s
        end
        pos.x += vel.x * @update_timestep
        pos.y += vel.y * @update_timestep
        pos.z += vel.z * @update_timestep
      end
      opts[:material].color = opts[:color]
      opts[:material].alpha = opts[:color].alpha / 255.0
      if opts[:type] == 3
        normal = Geom::Vector3d.new(Math.cos(opts[:rot_angle]), Math.sin(opts[:rot_angle]), 0)
      else
        normal = (eye == pos) ? Z_AXIS : pos.vector_to(eye)
      end
      tra1 = Geom::Transformation.new(pos, normal)
      tra2 = Geom::Transformation.rotation(ORIGIN, Z_AXIS, opts[:rot_angle])
      tra3 = Geom::Transformation.scaling(opts[:radius])
      opts[:group].move!(tra1*tra2*tra3)
      false
    }
  rescue Exception => err
    err_message = err.message
    err_backtrace = err.backtrace
    unless AMS::IS_RUBY_VERSION_18
      err_message.force_encoding('UTF-8')
      err_backtrace.each { |i| i.force_encoding('UTF-8') }
    end
    puts "An exception occurred while updating particles.\n#{err.class}:\n#{err_message}\nTrace:\n#{err_backtrace.join("\n")}"
  end

  def draw_particles(view, bb)
    return unless @particles_visible
    MSPhysics::C::Particle.draw_all(view, bb)
  end

  # @return [Boolean] success
  def update_scenes_animation
    return false if @scene_anim_info[:state] == 0
    @scene_anim_info[:ref_time] += MSPhysics::VIEW_UPDATE_TIMESTEP
    return false if @scene_anim_info[:ref_time] < MSPhysics::Settings.animate_scenes_delay
    tab_dir = MSPhysics::Settings.animate_scenes_reversed? ? -@scene_anim_info[:tab_dir] : @scene_anim_info[:tab_dir]
    last_elasted_time = @scene_anim_info[:elasted_time]
    @scene_anim_info[:elasted_time] += MSPhysics::VIEW_UPDATE_TIMESTEP * tab_dir
    case @scene_anim_info[:state]
      when 1
        if @scene_anim_info[:elasted_time] < 0
          if last_elasted_time == 0
            self.animate_scenes(0)
            return false
          else
            @scene_anim_info[:elasted_time] = 0
          end
        elsif @scene_anim_info[:elasted_time] > @scene_anim_info[:transition_time]
          if last_elasted_time == @scene_anim_info[:transition_time]
            self.animate_scenes(0)
            return false
          else
            @scene_anim_info[:elasted_time] = @scene_anim_info[:transition_time]
          end
        end
      when 2
        if @scene_anim_info[:elasted_time] < 0
          @scene_anim_info[:elasted_time] = @scene_anim_info[:elasted_time].abs
          @scene_anim_info[:tab_dir] = -@scene_anim_info[:tab_dir]
          tab_dir = -tab_dir
        elsif @scene_anim_info[:elasted_time] > @scene_anim_info[:transition_time]
          @scene_anim_info[:elasted_time] = @scene_anim_info[:transition_time] * 2 - @scene_anim_info[:elasted_time]
          @scene_anim_info[:tab_dir] = -@scene_anim_info[:tab_dir]
          tab_dir = -tab_dir
        end
      when 3
        if @scene_anim_info[:elasted_time] < 0
          @scene_anim_info[:elasted_time] += @scene_anim_info[:transition_time]
        elsif @scene_anim_info[:elasted_time] > @scene_anim_info[:transition_time]
          @scene_anim_info[:elasted_time] -= @scene_anim_info[:transition_time]
        end
    else
      self.animate_scenes(0)
      return false
    end
    @scene_anim_info[:data].each { |tab, data|
      unless data[:page].valid?
        self.animate_scenes(0)
        return false
      end
    }
    @scene_anim_info[:data].each { |tab, data|
      if @scene_anim_info[:elasted_time] >= data[:sdelay] && @scene_anim_info[:elasted_time] <= data[:edelay]
        if @scene_anim_info[:active_tab] != tab
          odt = data[:page].delay_time
          ott = data[:page].transition_time
          data[:page].delay_time = 0
          data[:page].transition_time = 0
          Sketchup.active_model.pages.selected_page = data[:page]
          data[:page].delay_time = odt
          data[:page].transition_time = ott
          @scene_anim_info[:active_tab] = tab
        end
        return true
      elsif @scene_anim_info[:elasted_time] >= data[:stransition] && @scene_anim_info[:elasted_time] <= data[:etransition]
        if @scene_anim_info[:active_tab] != tab
          odt = data[:page].delay_time
          ott = data[:page].transition_time
          data[:page].delay_time = 0
          data[:page].transition_time = 0
          Sketchup.active_model.pages.selected_page = data[:page]
          data[:page].delay_time = odt
          data[:page].transition_time = ott
          @scene_anim_info[:active_tab] = tab
        end
        if @scene_anim_info[:state] == 3
          if tab_dir > 0
            from_tab = tab
            to_tab = tab + 1
            to_tab = 0 if to_tab == @scene_anim_info[:tabs_size]
            from_data = @scene_anim_info[:data][from_tab]
            to_data =  @scene_anim_info[:data][to_tab]
            return true if from_data[:transition_time] < MSPhysics::EPSILON
            ratio = (@scene_anim_info[:elasted_time] - from_data[:stransition]) / from_data[:transition_time]
          else
            from_tab = tab + 1
            to_tab = tab
            from_tab = 0 if from_tab == @scene_anim_info[:tabs_size]
            from_data = @scene_anim_info[:data][from_tab]
            to_data =  @scene_anim_info[:data][to_tab]
            return true if to_data[:transition_time] < MSPhysics::EPSILON
            ratio = (to_data[:etransition] - @scene_anim_info[:elasted_time]) / to_data[:transition_time]
          end
        else
          if tab_dir > 0
            from_tab = tab
            to_tab = tab + 1
            return true if to_tab == @scene_anim_info[:tabs_size]
            from_data = @scene_anim_info[:data][from_tab]
            to_data =  @scene_anim_info[:data][to_tab]
            return true if from_data[:transition_time] < MSPhysics::EPSILON
            ratio = (@scene_anim_info[:elasted_time] - from_data[:stransition]) / from_data[:transition_time]
          else
            from_tab = tab + 1
            to_tab = tab
            return true if from_tab == @scene_anim_info[:tabs_size]
            from_data = @scene_anim_info[:data][from_tab]
            to_data =  @scene_anim_info[:data][to_tab]
            return true if to_data[:transition_time] < MSPhysics::EPSILON
            ratio = (to_data[:etransition] - @scene_anim_info[:elasted_time]) / to_data[:transition_time]
          end
        end
        ratio = 0.5 * Math.sin(Math::PI * (ratio - 0.5)) + 0.5
        from_data[:scene].transition(to_data[:scene], ratio)
        return true
      end
    }
    return false
  end

  # @return [Boolean] success
  def update_scene_transitioning
    return false unless @scene_info[:active]
    @scene_info[:elasted_time] += MSPhysics::VIEW_UPDATE_TIMESTEP
    r1 = AMS.clamp(@scene_info[:elasted_time] / @scene_info[:transition_time], 0.0, 1.0)
    r2 = 0.5 * Math.sin(Math::PI * (r1 - 0.5)) + 0.5
    @scene_info[:data1].transition(@scene_info[:data2], r2)
    if r1 >= 1.0
      @scene_info[:active] = false
      @scene_info[:data1] = nil
      @scene_info[:data2] = nil
      @scene_info[:transition_time] = 0
      @scene_info[:elasted_time] = 0
    end
    return true
  end

  # @return [Boolean] success
  def update_camera_track_follow
    view = Sketchup.active_model.active_view
    if @camera[:follow]
      if @camera[:follow].deleted?
        @camera[:follow] = nil
      else
        eye = @camera[:follow].bounds.center + @camera[:offset]
        tar = eye + view.camera.direction
        view.camera.set(eye, tar, Z_AXIS.parallel?(view.camera.direction) ? Y_AXIS : Z_AXIS)
      end
    end
    if @camera[:target]
      if @camera[:target].deleted?
        @camera[:target] = nil
      else
        eye = view.camera.eye
        dir = eye.vector_to(@camera[:target].bounds.center)
        tar = eye + dir
        view.camera.set(eye, tar, Z_AXIS.parallel?(dir) ? Y_AXIS : Z_AXIS)
      end
    end
    return @camera[:target] && @camera[:follow] ? true : false
  end

  # @return [Boolean] success
  def update_keyboard_navigation
    return false if MSPhysics::Settings.key_nav_state == 0
    camera = Sketchup.active_model.active_view.camera
    if MSPhysics::Settings.key_nav_state == 1
      camera_tra = Geom::Transformation.new(camera.xaxis, camera.zaxis, camera.yaxis, camera.eye)
      # Motion
      if !AMS::IS_PLATFORM_WINDOWS || AMS::Sketchup.is_main_window_active?
        dir = Geom::Vector3d.new(AMS::Keyboard.key('d') - AMS::Keyboard.key('a'), AMS::Keyboard.key('w') - AMS::Keyboard.key('s'), AMS::Keyboard.key('e') - AMS::Keyboard.key('q'))
      else
        dir = Geom::Vector3d.new(0,0,0)
      end
      dir_len = dir.length.to_f
      if dir_len > MSPhysics::EPSILON
        des_veloc = AMS::Geometry.scale_vector(dir, MSPhysics::Settings.key_nav_velocity / dir_len).transform(camera_tra)
      else
        des_veloc = dir
      end
      if MSPhysics::Settings.key_nav_atime > MSPhysics::EPSILON
        des_accel = AMS::Geometry.scale_vector(des_veloc - @key_nav_veloc, MSPhysics::VIEW_UPDATE_TIMESTEP_INV)
        accel_len = des_accel.length.to_f
        max_accel = MSPhysics::Settings.key_nav_velocity / MSPhysics::Settings.key_nav_atime
        if accel_len > max_accel
          des_accel = AMS::Geometry.scale_vector(des_accel, max_accel / accel_len)
        end
        @key_nav_veloc += AMS::Geometry.scale_vector(des_accel, MSPhysics::VIEW_UPDATE_TIMESTEP)
      else
        @key_nav_veloc = des_veloc
      end
      key_nav_veloc_len = @key_nav_veloc.length.to_f
      # Rotation
      if !AMS::IS_PLATFORM_WINDOWS || AMS::Sketchup.is_main_window_active?
        rls = AMS::Keyboard.key('right') - AMS::Keyboard.key('left')
        dir = Geom::Vector3d.new(AMS::Keyboard.key('up') - AMS::Keyboard.key('down'), AMS::Keyboard.control_down? ? rls : 0, AMS::Keyboard.control_up? ? -rls : 0)
      else
        dir = Geom::Vector3d.new(0,0,0)
      end
      dir_len = dir.length.to_f
      if dir_len > MSPhysics::EPSILON
        des_omega = AMS::Geometry.scale_vector(dir, MSPhysics::Settings.key_nav_omega / dir_len).transform(camera_tra)
      else
        des_omega = dir
      end
      if MSPhysics::Settings.key_nav_atime > MSPhysics::EPSILON
        des_alpha = AMS::Geometry.scale_vector(des_omega - @key_nav_omega, MSPhysics::VIEW_UPDATE_TIMESTEP_INV)
        alpha_len = des_alpha.length.to_f
        max_alpha = MSPhysics::Settings.key_nav_omega / MSPhysics::Settings.key_nav_atime
        if alpha_len > max_alpha
          des_alpha = AMS::Geometry.scale_vector(des_alpha, max_alpha / alpha_len)
        end
        @key_nav_omega += AMS::Geometry.scale_vector(des_alpha, MSPhysics::VIEW_UPDATE_TIMESTEP)
      else
        @key_nav_omega = des_omega
      end
      key_nav_omega_len = @key_nav_omega.length.to_f
      # Update camera if necessary
      return false if key_nav_veloc_len < MSPhysics::EPSILON && key_nav_omega_len < MSPhysics::EPSILON
      eye = camera_tra.origin + AMS::Geometry.scale_vector(@key_nav_veloc, MSPhysics::VIEW_UPDATE_TIMESTEP * 39.37)
      if key_nav_omega_len < MSPhysics::EPSILON
        camera_tra3 = camera_tra
      else
        parent_tra = Geom::Transformation.rotation(eye, AMS::Geometry.scale_vector(@key_nav_omega, 1.0 / key_nav_omega_len), key_nav_omega_len * MSPhysics::VIEW_UPDATE_TIMESTEP)
        camera_tra3 = parent_tra * camera_tra
      end
      camera.set(eye, eye + camera_tra3.yaxis, camera_tra3.zaxis)
    else
      # Unroll the camera matrix, so that its side is perpendicular to global
      # Z-axis; its front should remain the same.
      yaxis = camera.zaxis
      if yaxis.z.abs > 0.999995
        # If camera's front is parallel to global Z-axis, we know that camera's
        # side is already perpendicular to global Z-axis.
        xaxis = camera.xaxis
        zaxis = camera.yaxis
      else
        # Otherwise, make camera's X-axis perpendicular to global Z-axis.
        xaxis = yaxis.cross(camera.yaxis.z < 0 ? Z_AXIS.reverse : Z_AXIS).normalize
        zaxis = xaxis.cross(yaxis).normalize
      end
      camera_tra = Geom::Transformation.new(xaxis, yaxis, zaxis, camera.eye)
      # Now make another matrix whose Z-axis is parallel to global Z-axis.
      new_zaxis = zaxis.z > 0 ? Z_AXIS : Z_AXIS.reverse
      camera_tra2 = Geom::Transformation.new(xaxis, xaxis.cross(new_zaxis).normalize, new_zaxis, camera.eye)
      # Motion
      if !AMS::IS_PLATFORM_WINDOWS || AMS::Sketchup.is_main_window_active?
        dir = Geom::Vector3d.new(AMS::Keyboard.key('d') - AMS::Keyboard.key('a'), AMS::Keyboard.key('s') - AMS::Keyboard.key('w'), AMS::Keyboard.key('e') - AMS::Keyboard.key('q'))
      else
        dir = Geom::Vector3d.new(0,0,0)
      end
      dir_len = dir.length.to_f
      if dir_len > MSPhysics::EPSILON
        des_veloc = AMS::Geometry.scale_vector(dir, MSPhysics::Settings.key_nav_velocity / dir_len).transform(camera_tra2)
      else
        des_veloc = dir
      end
      if MSPhysics::Settings.key_nav_atime > MSPhysics::EPSILON
        des_accel = AMS::Geometry.scale_vector(des_veloc - @key_nav_veloc, MSPhysics::VIEW_UPDATE_TIMESTEP_INV)
        accel_len = des_accel.length.to_f
        max_accel = MSPhysics::Settings.key_nav_velocity / MSPhysics::Settings.key_nav_atime
        if accel_len > max_accel
          des_accel = AMS::Geometry.scale_vector(des_accel, max_accel / accel_len)
        end
        @key_nav_veloc += AMS::Geometry.scale_vector(des_accel, MSPhysics::VIEW_UPDATE_TIMESTEP)
      else
        @key_nav_veloc = des_veloc
      end
      key_nav_veloc_len = @key_nav_veloc.length.to_f
      # Rotation
      if !AMS::IS_PLATFORM_WINDOWS || AMS::Sketchup.is_main_window_active?
        dir = Geom::Vector3d.new(AMS::Keyboard.key('up') - AMS::Keyboard.key('down'), 0, AMS::Keyboard.key('left') - AMS::Keyboard.key('right'))
      else
        dir = Geom::Vector3d.new(0,0,0)
      end
      dir_len = dir.length.to_f
      if dir_len > MSPhysics::EPSILON
        des_omega = AMS::Geometry.scale_vector(dir, MSPhysics::Settings.key_nav_omega / dir_len).transform(camera_tra)
      else
        des_omega = dir
      end
      if MSPhysics::Settings.key_nav_atime > MSPhysics::EPSILON
        des_alpha = AMS::Geometry.scale_vector(des_omega - @key_nav_omega, MSPhysics::VIEW_UPDATE_TIMESTEP_INV)
        alpha_len = des_alpha.length.to_f
        max_alpha = MSPhysics::Settings.key_nav_omega / MSPhysics::Settings.key_nav_atime
        if alpha_len > max_alpha
          des_alpha = AMS::Geometry.scale_vector(des_alpha, max_alpha / alpha_len)
        end
        @key_nav_omega += AMS::Geometry.scale_vector(des_alpha, MSPhysics::VIEW_UPDATE_TIMESTEP)
      else
        @key_nav_omega = des_omega
      end
      key_nav_omega_len = @key_nav_omega.length.to_f
      # Update camera if necessary
      return false if key_nav_veloc_len < MSPhysics::EPSILON && key_nav_omega_len < MSPhysics::EPSILON
      eye = camera_tra.origin + AMS::Geometry.scale_vector(@key_nav_veloc, MSPhysics::VIEW_UPDATE_TIMESTEP * 39.37)
      if key_nav_omega_len > MSPhysics::EPSILON
        parent_tra = Geom::Transformation.rotation(eye, AMS::Geometry.scale_vector(@key_nav_omega, 1.0 / key_nav_omega_len), key_nav_omega_len * MSPhysics::VIEW_UPDATE_TIMESTEP)
        camera_tra = parent_tra * camera_tra
      end
      camera.set(eye, eye + camera_tra.yaxis, camera_tra.zaxis)
    end
    return true
  end

  def update_fullscreen_note
    return unless @fullscreen_note[:time]
    @fullscreen_note[:time] += MSPhysics::VIEW_UPDATE_TIMESTEP
    if @fullscreen_note[:time] > @fullscreen_note[:duration]
      @fullscreen_note[:time] = nil
    end
  end

  def update_fancy_note
    if @prev_fancy_note
      @prev_fancy_note[:time] -= MSPhysics::VIEW_UPDATE_TIMESTEP
      if @prev_fancy_note[:time] > 0.0
        @prev_fancy_note[:alpha] = @prev_fancy_note[:time] / @prev_fancy_note[:fade]
        @prev_fancy_note[:color].alpha = (@prev_fancy_note[:color2].alpha * @prev_fancy_note[:alpha]).to_i
        if @prev_fancy_note[:background]
          @prev_fancy_note[:background].alpha = (@prev_fancy_note[:background2].alpha * @prev_fancy_note[:alpha]).to_i
        end
        return
      end
      @prev_fancy_note = nil
    end
    return unless @fancy_note
    @fancy_note[:time] += MSPhysics::VIEW_UPDATE_TIMESTEP
    total = @fancy_note[:fade] * 2 + @fancy_note[:duration]
    if @fancy_note[:time] > total
      @fancy_note = nil
      return
    end
    if @fancy_note[:fade] < MSPhysics::EPSILON
      @fancy_note[:alpha] = 1.0
    else
      if @fancy_note[:time] < @fancy_note[:fade]
        @fancy_note[:alpha] = @fancy_note[:time] / @fancy_note[:fade]
      elsif @fancy_note[:time] <= @fancy_note[:fade] + @fancy_note[:duration]
        @fancy_note[:alpha] = 1.0
      else
        @fancy_note[:alpha] = (total - @fancy_note[:time]) / @fancy_note[:fade]
      end
    end
    @fancy_note[:color].alpha = (@fancy_note[:color2].alpha * @fancy_note[:alpha]).to_i
    if @fancy_note[:background]
      @fancy_note[:background].alpha = (@fancy_note[:background2].alpha * @fancy_note[:alpha]).to_i
    end
  end

  def do_on_update(view)
    if @error
      self.class.reset
      return false
    end
    # Update control panel opacity
    MSPhysics::ControlPanel.udpdate_opacity
    # Handle simulation play/pause events.
    if @paused
      unless @pause_updated
        @pause_updated = true
        @time_info[:sim] += Time.now - @time_info[:last]
        @fps_info[:change] += Time.now - @fps_info[:last]
        MSPhysics::Music.pause
        MSPhysics::Sound.pause(-1)
      end
      view.show_frame(0.25)
      return true
    end
    if @pause_updated
      @time_info[:last] = Time.now
      @fps_info[:last] = Time.now
      @pause_updated = false
      MSPhysics::Music.resume
      MSPhysics::Sound.resume(-1)
    end
    # Clear drawing queues
    @bb.clear
    @draw_queue.clear
    @points_queue.clear
    # Update joy data
    update_joy_data
    # Increment frame
    @frame += 1
    # Call onPreFrame event
    call_event(:onPreFrame)
    return false unless self.class.active?
    # Update world update_rate times
    world_address = @world.address
    @update_rate.times {
      # Get world time
      world_time = @world.time
      # Call onPreUpdate event
      call_event(:onPreUpdate)
      return false unless self.class.active?
      # Update key sliders
      MSPhysics::CommonContext.update_key_sliders(@update_timestep)
      # Process thrusters
      @thrusters.reject! { |body, data|
        next true unless body.valid?
        value = nil
        begin
          value = @controller_context.eval_script(data[:controller], CONTROLLER_NAME, 0)
        rescue Exception => err
          err_message = err.message
          err_message.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
          puts "An exception occurred while evaluating thruster controller!\nController:\n#{data[:controller]}\n#{err.class}:\n#{err_message}"
        end
        return false unless self.class.active?
        next true unless body.valid?
        begin
          if value.is_a?(Numeric)
            value = Geom::Vector3d.new(0, 0, value)
          elsif value.is_a?(Array) && value.x.is_a?(Numeric) && value.y.is_a?(Numeric) && value.z.is_a?(Numeric)
            value = Geom::Vector3d.new(value)
          elsif !value.is_a?(Geom::Vector3d)
            next false
          end
          if data[:lock_axes]
            body.add_force(value.transform(body.normal_matrix))
          else
            body.add_force(value)
          end
        rescue Exception => err
          err_message = err.message
          err_message.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
          puts "An exception occurred while assigning thruster controller!\nController:\n#{data[:controller]}\n#{err.class}:\n#{err_message}"
        end
        false
      }
      # Process emitters
      @emitters.reject! { |body, data|
        next true unless body.valid?
        value = nil
        begin
          value = @controller_context.eval_script(data[:controller], CONTROLLER_NAME, 0)
        rescue Exception => err
          err_message = err.message
          err_message.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
          puts "An exception occurred while evaluating emitter controller!\nController:\n#{data[:controller]}\n#{err.class}:\n#{err_message}"
        end
        return false unless self.class.active?
        next true unless body.valid?
        begin
          if value.is_a?(Numeric)
            value = Geom::Vector3d.new(0, 0, value)
          elsif value.is_a?(Array) && value.x.is_a?(Numeric) && value.y.is_a?(Numeric) && value.z.is_a?(Numeric)
            value = Geom::Vector3d.new(value)
          elsif !value.is_a?(Geom::Vector3d)
            next false
          end
          if value.length != 0
            value = AMS::Geometry.scale_vector(value, @update_timestep_inv)
            value.transform!(body.normal_matrix) if data[:lock_axes]
            rel_world_time = world_time - data[:delay]
            if rel_world_time > 0
              res = (rel_world_time / data[:rate]).to_i
              if data[:flags] != res
                self.emit_body(body, value, data[:lifetime])
                body.add_force(value.reverse) if data[:recoil]
                data[:flags] = res
              end
            end
          end
        rescue Exception => err
          err_message = err.message
          err_message.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
          puts "An exception occurred while assigning emitter controller!\nController:\n#{data[:controller]}\n#{err.class}:\n#{err_message}"
        end
        false
      }
      # Process buoyancy planes
      MSPhysics::Newton.enable_object_validation(false)
      @buoyancy_planes.reject! { |entity, data|
        next true unless entity.valid?
        tra = entity.transformation
        body_address = MSPhysics::Newton::World.get_first_body(world_address)
        while body_address
          unless MSPhysics::Newton::Body.is_static?(body_address)
            MSPhysics::Newton::Body.apply_buoyancy(body_address, tra.origin, tra.zaxis, data[:density], data[:linear_viscosity], data[:angular_viscosity], data[:linear_current].transform(tra), data[:angular_current].transform(tra), @update_timestep)
          end
          body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
        end
        false
      }
      MSPhysics::Newton.enable_object_validation(true)
      # Update controlled joints
      @controlled_joints.reject! { |joint, data|
        next true if !joint.valid?
        if data.is_a?(Array)
          controller = data[0]
          ratio = data[1]
        else
          controller = data
          ratio = 1
        end
        value = nil
        begin
          value = @controller_context.eval_script(controller, CONTROLLER_NAME, 0)
        rescue Exception => err
          err_message = err.message
          err_message.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
          puts "An exception occurred while evaluating joint controller!\nController:\n#{controller}\n#{err.class}:\n#{err_message}"
        end
        return false unless self.class.active?
        next true if !joint.valid?
        begin
          if joint.is_a?(Servo) || joint.is_a?(Piston) || joint.is_a?(CurvyPiston)
            if value.is_a?(Numeric)
              joint.controller = value * ratio
            elsif value.nil?
              joint.controller = nil
            end
          elsif joint.is_a?(UpVector)
            if (value.is_a?(Array) || value.is_a?(Geom::Vector3d)) && value.x.is_a?(Numeric) && value.y.is_a?(Numeric) && value.z.is_a?(Numeric)
              joint.set_pin_dir(value)
            end
          elsif value.is_a?(Numeric)
            joint.controller = value * ratio
          end
        rescue Exception => err
          err_message = err.message
          err_message.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
          puts "An exception occurred while assigning joint controller!\nController:\n#{controller}\n#{err.class}:\n#{err_message}"
        end
        false
      }
      # Process dragged body
      unless @picked.empty?
        if @picked[:body].valid?
          pick_pt = @picked[:loc_pick_pt].transform(@picked[:body].get_matrix)
          ray = view.pickray(@cursor_pos.x, @cursor_pos.y)
          view_normal = view.camera.zaxis
          normal = nil
          if AMS::Keyboard.shift_down?
            if view_normal.dot(Z_AXIS).abs < 0.999995
              normal = view_normal
              normal.z = 0
              normal.normalize!
            end
            if !@picked[:shift_down]
              @picked[:plane_origin] = Geom::Point3d.new(@picked[:dest_pt])
              @picked[:shift_down] = true
            end
          else
            if pick_pt.z > ray[0].z
              normal = Z_AXIS
            else
              normal = Z_AXIS.reverse
            end
            if @picked[:shift_down]
              @picked[:plane_origin] = Geom::Point3d.new(@picked[:dest_pt])
              @picked[:shift_down] = false
            end
          end
          if normal && ray[1].dot(normal) > 0.02
            plane = [@picked[:plane_origin], normal]
            @picked[:dest_pt] = Geom.intersect_line_plane(ray, plane)
          end
          MSPhysics::Newton::Body.apply_pick_and_drag(@picked[:body].address, pick_pt, @picked[:dest_pt], 0.2, 0.5, @update_timestep)
        else
          @picked.clear
          self.cursor = @original_cursor_id
        end
      end
      # Update newton world
      @world.update(@update_timestep)
      # Call onUpdate event
      call_event(:onUpdate)
      return false unless self.class.active?
      # Call onTouch event
      count = MSPhysics::Newton::World.get_touch_data_count(world_address)
      for i in 0...count
        MSPhysics::Newton.enable_object_validation(false)
        data = MSPhysics::Newton::World.get_touch_data_at(world_address, i)
        if MSPhysics::Newton::Body.is_valid?(data[0]) && MSPhysics::Newton::Body.is_valid?(data[1])
          body1 = MSPhysics::Newton::Body.get_user_data(data[0])
          body2 = MSPhysics::Newton::Body.get_user_data(data[1])
        else
          MSPhysics::Newton.enable_object_validation(true)
          next
        end
        MSPhysics::Newton.enable_object_validation(true)
        if body1.is_a?(MSPhysics::Body) && body2.is_a?(MSPhysics::Body)
          begin
            body1.context.call_event(:onTouch, body2, data[2], data[3], data[4], data[5])
          rescue Exception => err
            abort(err)
          end
          return false unless self.class.active?
        end
      end
      # Call onTouching event
      count = MSPhysics::Newton::World.get_touching_data_count(world_address)
      for i in 0...count
        MSPhysics::Newton.enable_object_validation(false)
        data = MSPhysics::Newton::World.get_touching_data_at(world_address, i)
        if MSPhysics::Newton::Body.is_valid?(data[0]) && MSPhysics::Newton::Body.is_valid?(data[1])
          body1 = MSPhysics::Newton::Body.get_user_data(data[0])
          body2 = MSPhysics::Newton::Body.get_user_data(data[1])
        else
          MSPhysics::Newton.enable_object_validation(true)
          next
        end
        MSPhysics::Newton.enable_object_validation(true)
        if body1.is_a?(MSPhysics::Body) && body2.is_a?(MSPhysics::Body)
          begin
            body1.context.call_event(:onTouching, body2)
          rescue Exception => err
            abort(err)
          end
          return false unless self.class.active?
        end
      end
      # Call onUntouch event
      count = MSPhysics::Newton::World.get_untouch_data_count(world_address)
      for i in 0...count
        MSPhysics::Newton.enable_object_validation(false)
        data = MSPhysics::Newton::World.get_untouch_data_at(world_address, i)
        if MSPhysics::Newton::Body.is_valid?(data[0]) && MSPhysics::Newton::Body.is_valid?(data[1])
          body1 = MSPhysics::Newton::Body.get_user_data(data[0])
          body2 = MSPhysics::Newton::Body.get_user_data(data[1])
        else
          MSPhysics::Newton.enable_object_validation(true)
          next
        end
        MSPhysics::Newton.enable_object_validation(true)
        if body1.is_a?(MSPhysics::Body) && body2.is_a?(MSPhysics::Body)
          begin
            body1.context.call_event(:onUntouch, body2)
          rescue Exception => err
            abort(err)
          end
          return false unless self.class.active?
        end
      end
      # Call onPostUpdate event
      call_event(:onPostUpdate)
      return false unless self.class.active?
      # Process emitted bodies.
      world_time = @world.time
      @emitted_bodies.reject! { |body, life_end|
        next true unless body.valid?
        if life_end && world_time > life_end
          @created_entities.delete(body.group)
          body.destroy(true)
          true
        else
          false
        end
      }
    }
    # Update group transformations
    MSPhysics::Newton.enable_object_validation(false)
    body_address = MSPhysics::Newton::World.get_first_body(world_address)
    while body_address
      if MSPhysics::Newton::Body.matrix_changed?(body_address)
        data = MSPhysics::Newton::Body.get_user_data(body_address)
        if data.is_a?(MSPhysics::Body) && data.group.valid?
          data.group.move!(data.get_matrix)
        end
      end
      body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
    end
    MSPhysics::Newton::World.clear_matrix_change_record(world_address)
    MSPhysics::Newton.enable_object_validation(true)
    # Update particles
    update_particles
    # Call onTick event
    call_event(:onTick)
    return false unless self.class.active?
    # Call onPostFrame event
    call_event(:onPostFrame)
    return false unless self.class.active?
    # Process 3D sounds.
    MSPhysics::Sound.update_effects
    # Update Camera
    update_scenes_animation || update_scene_transitioning || update_camera_track_follow || update_keyboard_navigation
    # Update notes
    update_fullscreen_note
    update_fancy_note
    # Update FPS
    if @frame % @fps_info[:update_rate] == 0
      @fps_info[:change] += Time.now - @fps_info[:last]
      @fps_info[:fps] = ( @fps_info[:change] == 0 ? 0 : (@fps_info[:update_rate] / @fps_info[:change]).round )
      @fps_info[:last] = Time.now
      @fps_info[:change] = 0
    end
    # Update status bar text
    update_status_text
    # Record replay animation
    if MSPhysics::Replay.record_enabled?
      MSPhysics::Replay.record_all(@frame)
    end
    # Redraw view
    view.show_frame
    # Continue to next frame
    return true
  end

  def draw_contact_points(view)
    return unless @contact_points[:show]
    world_address = @world.address
    MSPhysics::Newton.enable_object_validation(false)
    body_address = MSPhysics::Newton::World.get_first_body(world_address)
    while body_address
      points = MSPhysics::Newton::Body.get_contact_points(body_address, true)
      if points.size > 0
        view.draw_points(points, @contact_points[:point_size], @contact_points[:point_style], @contact_points[:point_color])
      end
      body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
    end
    MSPhysics::Newton.enable_object_validation(true)
  end

  def draw_contact_forces(view)
    return unless @contact_forces[:show]
    view.drawing_color = @contact_forces[:line_color]
    view.line_width = @contact_forces[:line_width]
    view.line_stipple = @contact_forces[:line_stipple]
    world_address = @world.address
    MSPhysics::Newton.enable_object_validation(false)
    body_address = MSPhysics::Newton::World.get_first_body(world_address)
    while body_address
      mass = MSPhysics::Newton::Body.get_mass(body_address)
      if mass > 0
        MSPhysics::Newton::Body.get_contacts(body_address, false) { |ptr, data, point, normal, force, speed|
          view.draw(GL_LINES, [point, point + AMS::Geometry.scale_vector(force, 1.0 / mass)])
          nil
        }
      end
      body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
    end
    MSPhysics::Newton.enable_object_validation(true)
  end

  def draw_collision_wireframe(view)
    return unless @collision_wireframe[:show]
    MSPhysics::Newton::World.draw_collision_wireframe(@world.address, view, @bb, @collision_wireframe[:sleeping], @collision_wireframe[:active], @collision_wireframe[:line_width], @collision_wireframe[:line_stipple])
  end

  def draw_axes(view)
    return unless @axes[:show]
    view.line_width = @axes[:line_width]
    view.line_stipple = @axes[:line_stipple]
    world_address = @world.address
    MSPhysics::Newton.enable_object_validation(false)
    body_address = MSPhysics::Newton::World.get_first_body(world_address)
    while body_address
      pos = MSPhysics::Newton::Body.get_position(body_address, 1)
      tra = MSPhysics::Newton::Body.get_matrix(body_address)
      # Draw xaxis
      l = tra.xaxis
      l.length = @axes[:size]
      pt = pos + l
      view.drawing_color = @axes[:xaxis]
      view.draw_line(pos, pt)
      # Draw yaxis
      l = tra.yaxis
      l.length = @axes[:size]
      pt = pos + l
      view.drawing_color = @axes[:yaxis]
      view.draw_line(pos, pt)
      # Draw zaxis
      l = tra.zaxis
      l.length = @axes[:size]
      pt = pos + l
      view.drawing_color = @axes[:zaxis]
      view.draw_line(pos, pt)
      # Get next body
      body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
    end
    MSPhysics::Newton.enable_object_validation(true)
  end

  def draw_aabb(view)
    return unless @aabb[:show]
    view.drawing_color = @aabb[:line_color]
    view.line_width = @aabb[:line_width]
    view.line_stipple = @aabb[:line_stipple]
    world_address = @world.address
    MSPhysics::Newton.enable_object_validation(false)
    body_address = MSPhysics::Newton::World.get_first_body(world_address)
    while body_address
      min, max = MSPhysics::Newton::Body.get_aabb(body_address)
      view.draw(GL_LINE_LOOP, [min, [min.x, max.y, min.z], [max.x, max.y, min.z], [max.x, min.y, min.z]])
      view.draw(GL_LINE_LOOP, [[min.x, min.y, max.z], [min.x, max.y, max.z], max, [max.x, min.y, max.z]])
      view.draw(GL_LINES, [min, [min.x, min.y, max.z], [min.x, max.y, min.z], [min.x, max.y, max.z], [max.x, max.y, min.z], max, [max.x, min.y, min.z], [max.x, min.y, max.z]])
      body_address = MSPhysics::Newton::World.get_next_body(world_address, body_address)
    end
    MSPhysics::Newton.enable_object_validation(true)
  end

  def draw_pick_and_drag(view)
    return if @picked.empty?
    if @picked[:body].valid?
      pt1 = @picked[:loc_pick_pt].transform(@picked[:body].get_matrix)
      pt2 = @picked[:dest_pt]
      @bb.add(pt2)
      view.line_width = @pick_and_drag[:line_width]
      view.line_stipple = @pick_and_drag[:line_stipple]
      view.drawing_color = @pick_and_drag[:line_color]
      view.draw_line(pt1, pt2)
      view.line_width = @pick_and_drag[:point_width]
      view.line_stipple = ''
      view.draw_points(pt1, @pick_and_drag[:point_size], @pick_and_drag[:point_style], @pick_and_drag[:point_color])
      if AMS::Keyboard.shift_down?
        view.line_width = @pick_and_drag[:vline_width]
        view.line_stipple = pt2.z > 0 ? @pick_and_drag[:vline_stipple1] : @pick_and_drag[:vline_stipple2]
        view.drawing_color = @pick_and_drag[:vline_color]
        view.draw(GL_LINES, [pt2.x, pt2.y, 0], pt2)
      end
    else
      @picked.clear
      self.cursor = @original_cursor_id
    end
  end

  def draw_queues(view)
    @draw_queue.each { |type, points, color, width, stipple, nmode|
      view.drawing_color = color
      view.line_width = width
      view.line_stipple = stipple
      if nmode == 1
        @bb.add(points)
        view.draw(type, points)
      else
        view.draw2d(type, points)
      end
    }
    @points_queue.each { |points, size, style, color, width, stipple|
      @bb.add(points)
      view.line_width = width
      view.line_stipple = stipple
      view.draw_points(points, size, style, color)
    }
  rescue Exception => err
    @draw_queue.clear
    @points_queue.clear
  end

  def draw_fullscreen_note(view)
    return unless @fullscreen_note[:time]
    pt = Geom::Point3d.new(view.vpwidth * @fullscreen_note[:hratio], view.vpheight * @fullscreen_note[:vratio], 0)
    min = Geom::Point3d.new(pt.x - @fullscreen_note[:bgw] * 0.5, pt.y - @fullscreen_note[:bgh] * 0.5 + @fullscreen_note[:bvo], 0)
    max = Geom::Point3d.new(pt.x + @fullscreen_note[:bgw] * 0.5, pt.y + @fullscreen_note[:bgh] * 0.5 + @fullscreen_note[:bvo], 0)
    pts = [min, Geom::Point3d.new(min.x, max.y, 0), max, Geom::Point3d.new(max.x, min.y, 0)]
    view.drawing_color = @fullscreen_note[:background]
    view.draw2d(GL_POLYGON, pts)
    view.draw_text(pt, @fullscreen_note[:text], @fullscreen_note)
  end

  def draw_fancy_note(view)
    if @prev_fancy_note
      dp = @prev_fancy_note[:padding] * 2
      if @prev_fancy_note[:align] == TextAlignLeft
        hoffset = 0
      elsif @prev_fancy_note[:align] == TextAlignCenter
        hoffset = (@prev_fancy_note[:sx] - dp) * 0.5
      else
        hoffset = (@prev_fancy_note[:sx] - dp)
      end
      px = (view.vpwidth - @prev_fancy_note[:sx]) * @prev_fancy_note[:hratio]
      py = (view.vpheight - @prev_fancy_note[:sy]) * @prev_fancy_note[:vratio]
      if @prev_fancy_note[:background]
        min = Geom::Point3d.new(px, py, 0)
        max = Geom::Point3d.new(px + @prev_fancy_note[:sx], py + @prev_fancy_note[:sy], 0)
        pts = [min, Geom::Point3d.new(min.x, max.y, 0), max, Geom::Point3d.new(max.x, min.y, 0)]
        view.drawing_color = @prev_fancy_note[:background]
        view.draw2d(GL_POLYGON, pts)
      end
      pt = Geom::Point3d.new(px + @prev_fancy_note[:padding] + hoffset, py + @prev_fancy_note[:padding], 0)
      view.draw_text(pt, @prev_fancy_note[:text], @prev_fancy_note)
    elsif @fancy_note
      dp = @fancy_note[:padding] * 2
      if @fancy_note[:align] == TextAlignLeft
        hoffset = 0
      elsif @fancy_note[:align] == TextAlignCenter
        hoffset = (@fancy_note[:sx] - dp) * 0.5
      else
        hoffset = (@fancy_note[:sx] - dp)
      end
      px = (view.vpwidth - @fancy_note[:sx]) * @fancy_note[:hratio]
      py = (view.vpheight - @fancy_note[:sy]) * @fancy_note[:vratio]
      if @fancy_note[:background]
        min = Geom::Point3d.new(px, py, 0)
        max = Geom::Point3d.new(px + @fancy_note[:sx], py + @fancy_note[:sy], 0)
        pts = [min, Geom::Point3d.new(min.x, max.y, 0), max, Geom::Point3d.new(max.x, min.y, 0)]
        view.drawing_color = @fancy_note[:background]
        view.draw2d(GL_POLYGON, pts)
      end
      pt = Geom::Point3d.new(px + @fancy_note[:padding] + hoffset, py + @fancy_note[:padding], 0)
      view.draw_text(pt, @fancy_note[:text], @fancy_note)
    end
  end

  def update_status_text
    if @mouse_over && !@suspended
      Sketchup.set_status_text("Frame: #{@frame}   Time: #{sprintf("%.2f", @world.time)} s   FPS: #{@fps_info[:fps]}   Threads: #{@world.cur_threads_count}   #{@mode == 0 ? @interactive_note : @game_note}   #{@general_note}", SB_PROMPT)
    end
  end

  def call_event(evt, *args)
    return if @world.nil? || !@world.valid?
    @world.bodies.each { |body|
      next if !body.valid?
      body.context.call_event(evt, *args)
      return if @world.nil? || !@world.valid?
    }
  rescue Exception => err
    abort(err)
  end

  def call_event2(evt, *args)
    return if @world.nil? || !@world.valid?
    @world.bodies.each { |body|
      next if !body.valid?
      body.context.call_event(evt, *args)
      return if @world.nil? || !@world.valid?
    }
  rescue Exception => err
    @error = err
  end

  def abort(err)
    @error = err
    self.class.reset
  end

  def init_joint(joint_ent, pin_matrix, child_body, parent_body)
    jdict = 'MSPhysics Joint'
    jtype = joint_ent.get_attribute(jdict, 'Type')
    attr = joint_ent.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS)
    ang_ratio = MSPhysics::ANGLE_CONVERSION[attr]
    ang_ratio = 1.0 unless ang_ratio
    iang_ratio = 1.0 / ang_ratio
    attr = joint_ent.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS)
    pos_ratio = MSPhysics::POSITION_CONVERSION[attr]
    pos_ratio = 1.0 unless pos_ratio
    ipos_ratio = 1.0 / pos_ratio
    if AMS::Geometry.is_matrix_flipped?(pin_matrix)
      pin_matrix = Geom::Transformation.new(pin_matrix.xaxis.reverse, pin_matrix.yaxis, pin_matrix.zaxis, pin_matrix.origin)
    end
    case jtype
    when 'Fixed'
      attr = joint_ent.get_attribute(jdict, 'Adjust To', 0)
      if (attr == 2 && parent_body)
        centre = parent_body.get_position(1)
        pin_matrix = Geom::Transformation.new(pin_matrix.xaxis, pin_matrix.yaxis, pin_matrix.zaxis, centre)
      elsif (attr == 1)
        centre = child_body.get_position(1)
        pin_matrix = Geom::Transformation.new(pin_matrix.xaxis, pin_matrix.yaxis, pin_matrix.zaxis, centre)
      end
      joint = MSPhysics::Fixed.new(@world, parent_body, pin_matrix, joint_ent)
    when 'Hinge'
      joint = MSPhysics::Hinge.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min', MSPhysics::Hinge::DEFAULT_MIN * iang_ratio)
      joint.min = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Max', MSPhysics::Hinge::DEFAULT_MAX * iang_ratio)
      joint.max = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits', MSPhysics::Hinge::DEFAULT_LIMITS_ENABLED)
      joint.limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Mode', MSPhysics::Hinge::DEFAULT_MODE)
      joint.mode = attr.to_i
      attr = joint_ent.get_attribute(jdict, 'Friction', MSPhysics::Hinge::DEFAULT_FRICTION)
      joint.friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Accel', MSPhysics::Hinge::DEFAULT_ACCEL)
      joint.accel = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Damp', MSPhysics::Hinge::DEFAULT_DAMP)
      joint.damp = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Strength', MSPhysics::Hinge::DEFAULT_STRENGTH)
      joint.strength = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Spring Constant', MSPhysics::Hinge::DEFAULT_SPRING_CONSTANT)
      joint.spring_constant = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Spring Drag', MSPhysics::Hinge::DEFAULT_SPRING_DRAG)
      joint.spring_drag = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Start Angle', MSPhysics::Hinge::DEFAULT_START_ANGLE * iang_ratio)
      joint.start_angle = attr.to_f * ang_ratio
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'Motor'
      joint = MSPhysics::Motor.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Accel', MSPhysics::Motor::DEFAULT_ACCEL)
      joint.accel = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Damp', MSPhysics::Motor::DEFAULT_DAMP)
      joint.damp = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Enable Free Rotate', MSPhysics::Motor::DEFAULT_FREE_ROTATE_ENABLED)
      joint.free_rotate_enabled = attr
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'Servo'
      joint = MSPhysics::Servo.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min', MSPhysics::Servo::DEFAULT_MIN * iang_ratio)
      joint.min = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Max', MSPhysics::Servo::DEFAULT_MAX * iang_ratio)
      joint.max = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits', MSPhysics::Servo::DEFAULT_LIMITS_ENABLED)
      joint.limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Rate', MSPhysics::Servo::DEFAULT_RATE * iang_ratio)
      joint.rate = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Power', MSPhysics::Servo::DEFAULT_POWER)
      joint.power = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Reduction Ratio', MSPhysics::Servo::DEFAULT_REDUCTION_RATIO)
      joint.reduction_ratio = attr.to_f
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = [controller, ang_ratio]
      end
    when 'Slider'
      joint = MSPhysics::Slider.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min', MSPhysics::Slider::DEFAULT_MIN * ipos_ratio)
      joint.min = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Max', MSPhysics::Slider::DEFAULT_MAX * ipos_ratio)
      joint.max = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits', MSPhysics::Slider::DEFAULT_LIMITS_ENABLED)
      joint.limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Friction', MSPhysics::Slider::DEFAULT_FRICTION)
      joint.friction = attr.to_f
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'Piston'
      joint = MSPhysics::Piston.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min', MSPhysics::Piston::DEFAULT_MIN * ipos_ratio)
      joint.min = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Max', MSPhysics::Piston::DEFAULT_MAX * ipos_ratio)
      joint.max = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits', MSPhysics::Piston::DEFAULT_LIMITS_ENABLED)
      joint.limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Rate', MSPhysics::Piston::DEFAULT_RATE * ipos_ratio)
      joint.rate = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Power', MSPhysics::Piston::DEFAULT_POWER)
      joint.power = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Reduction Ratio', MSPhysics::Piston::DEFAULT_REDUCTION_RATIO)
      joint.reduction_ratio = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Controller Mode', MSPhysics::Piston::DEFAULT_CONTROLLER_MODE)
      joint.controller_mode = attr.to_i
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = [controller, joint.controller_mode == 0 ? pos_ratio : 1]
      end
    when 'Spring'
      joint = MSPhysics::Spring.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min', MSPhysics::Spring::DEFAULT_MIN * ipos_ratio)
      joint.min = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Max', MSPhysics::Spring::DEFAULT_MAX * ipos_ratio)
      joint.max = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits', MSPhysics::Spring::DEFAULT_LIMITS_ENABLED)
      joint.limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Enable Rotation', MSPhysics::Spring::DEFAULT_ROTATION_ENABLED)
      joint.rotation_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Mode', MSPhysics::Spring::DEFAULT_MODE)
      joint.mode = attr.to_i
      attr = joint_ent.get_attribute(jdict, 'Accel', MSPhysics::Spring::DEFAULT_ACCEL)
      joint.accel = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Damp', MSPhysics::Spring::DEFAULT_DAMP)
      joint.damp = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Strength', MSPhysics::Spring::DEFAULT_STRENGTH)
      joint.strength = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Spring Constant', MSPhysics::Spring::DEFAULT_SPRING_CONSTANT)
      joint.spring_constant = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Spring Drag', MSPhysics::Spring::DEFAULT_SPRING_DRAG)
      joint.spring_drag = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Start Position', MSPhysics::Spring::DEFAULT_START_POSITION * ipos_ratio)
      joint.start_position = attr.to_f * pos_ratio
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'UpVector'
      joint = MSPhysics::UpVector.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Accel', MSPhysics::UpVector::DEFAULT_ACCEL)
      joint.accel = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Damp', MSPhysics::UpVector::DEFAULT_DAMP)
      joint.damp = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Strength', MSPhysics::UpVector::DEFAULT_STRENGTH)
      joint.strength = attr.to_f
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'Corkscrew'
      joint = MSPhysics::Corkscrew.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min Position', MSPhysics::Corkscrew::DEFAULT_MIN_POSITION * ipos_ratio)
      joint.min_position = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Max Position', MSPhysics::Corkscrew::DEFAULT_MAX_POSITION * ipos_ratio)
      joint.max_position = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Linear Limits', MSPhysics::Corkscrew::DEFAULT_LINEAR_LIMITS_ENABLED)
      joint.linear_limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Linear Friction', MSPhysics::Corkscrew::DEFAULT_LINEAR_FRICTION)
      joint.linear_friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Min Angle', MSPhysics::Corkscrew::DEFAULT_MIN_ANGLE * iang_ratio)
      joint.min_angle = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Max Angle', MSPhysics::Corkscrew::DEFAULT_MAX_ANGLE * iang_ratio)
      joint.max_angle = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Angular Limits', MSPhysics::Corkscrew::DEFAULT_ANGULAR_LIMITS_ENABLED)
      joint.angular_limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Angular Friction', MSPhysics::Corkscrew::DEFAULT_ANGULAR_FRICTION)
      joint.angular_friction = attr.to_f
    when 'BallAndSocket'
      joint = MSPhysics::BallAndSocket.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Max Cone Angle', MSPhysics::BallAndSocket::DEFAULT_MAX_CONE_ANGLE * iang_ratio)
      joint.max_cone_angle = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Cone Limits', MSPhysics::BallAndSocket::DEFAULT_CONE_LIMITS_ENABLED)
      joint.cone_limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Min Twist Angle', MSPhysics::BallAndSocket::DEFAULT_MIN_TWIST_ANGLE * iang_ratio)
      joint.min_twist_angle = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Max Twist Angle', MSPhysics::BallAndSocket::DEFAULT_MAX_TWIST_ANGLE * iang_ratio)
      joint.max_twist_angle = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Twist Limits', MSPhysics::BallAndSocket::DEFAULT_TWIST_LIMITS_ENABLED)
      joint.twist_limits_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Friction', MSPhysics::BallAndSocket::DEFAULT_FRICTION)
      joint.friction = attr.to_f
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'Universal'
      joint = MSPhysics::Universal.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Min1', MSPhysics::Universal::DEFAULT_MIN * iang_ratio)
      joint.min1 = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Max1', MSPhysics::Universal::DEFAULT_MAX * iang_ratio)
      joint.max1 = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits1', MSPhysics::Universal::DEFAULT_LIMITS_ENABLED)
      joint.limits1_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Min2', MSPhysics::Universal::DEFAULT_MIN * iang_ratio)
      joint.min2 = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Max2', MSPhysics::Universal::DEFAULT_MAX * iang_ratio)
      joint.max2 = attr.to_f * ang_ratio
      attr = joint_ent.get_attribute(jdict, 'Enable Limits2', MSPhysics::Universal::DEFAULT_LIMITS_ENABLED)
      joint.limits2_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Friction', MSPhysics::Universal::DEFAULT_FRICTION)
      joint.friction = attr.to_f
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
    when 'CurvySlider'
      joint = MSPhysics::CurvySlider.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Enable Alignment', MSPhysics::CurvySlider::DEFAULT_ALIGNMENT_ENABLED)
      joint.alignment_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Enable Rotation', MSPhysics::CurvySlider::DEFAULT_ROTATION_ENABLED)
      joint.rotation_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Enable Loop', MSPhysics::CurvySlider::DEFAULT_LOOP_ENABLED)
      joint.loop_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Linear Friction', MSPhysics::CurvySlider::DEFAULT_LINEAR_FRICTION)
      joint.linear_friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Angular Friction', MSPhysics::CurvySlider::DEFAULT_ANGULAR_FRICTION)
      joint.angular_friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Alignment Power', MSPhysics::CurvySlider::DEFAULT_ALIGNMENT_POWER)
      joint.alignment_power = attr.to_f
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = controller
      end
      MSPhysics::JointConnectionTool.get_points_on_curve(joint_ent, parent_body ? parent_body.group : nil).each { |point|
        joint.add_point(point)
      }
    when 'CurvyPiston'
      joint = MSPhysics::CurvyPiston.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Enable Alignment', MSPhysics::CurvyPiston::DEFAULT_ALIGNMENT_ENABLED)
      joint.alignment_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Enable Rotation', MSPhysics::CurvyPiston::DEFAULT_ROTATION_ENABLED)
      joint.rotation_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Enable Loop', MSPhysics::CurvyPiston::DEFAULT_LOOP_ENABLED)
      joint.loop_enabled = attr
      attr = joint_ent.get_attribute(jdict, 'Angular Friction', MSPhysics::CurvyPiston::DEFAULT_ANGULAR_FRICTION)
      joint.angular_friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Rate', MSPhysics::CurvyPiston::DEFAULT_RATE * ipos_ratio)
      joint.rate = attr.to_f * pos_ratio
      attr = joint_ent.get_attribute(jdict, 'Power', MSPhysics::CurvyPiston::DEFAULT_POWER)
      joint.power = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Alignment Power', MSPhysics::CurvyPiston::DEFAULT_ALIGNMENT_POWER)
      joint.alignment_power = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Reduction Ratio', MSPhysics::CurvyPiston::DEFAULT_REDUCTION_RATIO)
      joint.reduction_ratio = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Controller Mode', MSPhysics::CurvyPiston::DEFAULT_CONTROLLER_MODE)
      joint.controller_mode = attr.to_i
      controller = joint_ent.get_attribute(jdict, 'Controller')
      if controller.is_a?(String) && !controller.empty?
        @controlled_joints[joint] = [controller, joint.controller_mode == 0 ? pos_ratio : 1]
      end
      MSPhysics::JointConnectionTool.get_points_on_curve(joint_ent, parent_body ? parent_body.group : nil).each { |point|
        joint.add_point(point)
      }
    when 'Plane'
      joint = MSPhysics::Plane.new(@world, parent_body, pin_matrix, joint_ent)
      attr = joint_ent.get_attribute(jdict, 'Linear Friction', MSPhysics::Plane::DEFAULT_LINEAR_FRICTION)
      joint.linear_friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Angular Friction', MSPhysics::Plane::DEFAULT_ANGULAR_FRICTION)
      joint.angular_friction = attr.to_f
      attr = joint_ent.get_attribute(jdict, 'Enable Rotation', MSPhysics::Plane::DEFAULT_ROTATION_ENABLED)
      joint.rotation_enabled = attr
    else
      return
    end
    attr = joint_ent.name.to_s
    attr = joint_ent.get_attribute(jdict, 'ID').to_s if attr.empty?
    joint.name = attr
    attr = joint_ent.get_attribute(jdict, 'Stiffness', MSPhysics::Joint::DEFAULT_STIFFNESS)
    joint.stiffness = attr.to_f
    attr = joint_ent.get_attribute(jdict, 'Bodies Collidable', MSPhysics::Joint::DEFAULT_BODIES_COLLIDABLE)
    joint.bodies_collidable = attr
    attr = joint_ent.get_attribute(jdict, 'Breaking Force', MSPhysics::Joint::DEFAULT_BREAKING_FORCE)
    joint.breaking_force = attr.to_f
    joint.solver_model = MSPhysics::Settings.joint_algorithm
    joint.connect(child_body)
    joint
  end

  def init_joints
    MSPhysics::JointConnectionTool.get_all_connections(true).each { |jinfo|
      begin
        init_joint(jinfo[0], jinfo[1], jinfo[2], jinfo[3])
      rescue Exception => err
        err_message = err.message
        err_backtrace = err.backtrace
        unless AMS::IS_RUBY_VERSION_18
          err_message.force_encoding('UTF-8')
          err_backtrace.each { |i| i.force_encoding('UTF-8') }
        end
        puts "An exception occurred while creating a joint from #{jinfo[0]}!\n#{err.class}:\n#{err_message}\nTrace:\n#{err_backtrace.join("\n")}"
      end
    }
  end

  public

  # @!visibility private

  # SketchUp Tool Events

  def activate
    model = Sketchup.active_model
    view = model.active_view
    camera = view.camera
    default_sim = MSPhysics::DEFAULT_SIMULATION_SETTINGS
    default_buoyancy = MSPhysics::DEFAULT_BUOYANCY_PLANE_SETTINGS
    # Close active path
    if model.active_entities != model.entities
      state = true
      while state
        state = model.close_active
      end
    end
    # Wrap operations
    op = 'MSPhysics Simulation'
    Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
    # Record starting selection
    model.selection.each { |e|
      if (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && e.parent == model
        @selected_ents << e
      end
    }
    # Clear selection
    model.selection.clear
    # Update dialog state
    MSPhysics::Dialog.update_state
    # Stop any playing sounds and music
    MSPhysics::Sound.destroy_all
    MSPhysics::Music.destroy_all
    # Save camera orientation
    opts = @camera[:original]
    opts[:eye] = camera.eye
    opts[:target] = camera.target
    opts[:up] = camera.up
    opts[:aspect_ratio] = camera.aspect_ratio
    opts[:perspective] = camera.perspective?
    if opts[:perspective]
      opts[:focal_length] = camera.focal_length
      opts[:fov] = camera.fov
      opts[:image_width] = camera.image_width
    else
      opts[:height] = camera.height
    end
    # Activate observer
    AMS::Sketchup.add_observer(self) if AMS::IS_PLATFORM_WINDOWS
    # Activate page observer (for OS X)
    @frame_change_observer_id = ::Sketchup::Pages.add_frame_change_observer(self)
    # Configure Settings
    @update_rate = MSPhysics::Settings.update_rate
    @update_timestep = MSPhysics::Settings.update_timestep
    @update_timestep_inv = 1.0 / @update_timestep
    # Create world
    @world = MSPhysics::World.new()
    destructor = Proc.new { self.class.reset }
    MSPhysics::Newton::World.set_destructor_proc(@world.address, destructor)
    # Enable Newton object validation
    MSPhysics::Newton.enable_object_validation(true)
    # Clear collision cache
    MSPhysics::Collision.clear_cache
    # Add entities
    ents = model.entities.to_a
    ents.each { |entity|
      next unless entity.is_a?(Sketchup::Group) || entity.is_a?(Sketchup::ComponentInstance)
      next unless entity.valid?
      next if (!entity.visible? || !entity.layer.visible?) && MSPhysics::Settings.ignore_hidden_instances?
      type = entity.get_attribute('MSPhysics', 'Type', 'Body')
      if type == 'Body'
        next if entity.get_attribute('MSPhysics Body', 'Ignore')
        begin
          if @started_from_selection
            body = add_group(entity)
            unless @selected_ents.include?(entity)
              body.static = true
            end
          else
            add_group(entity)
          end
        rescue MSPhysics::ScriptException => err
          abort(err)
          return
        rescue StandardError => err
          index = ents.index(entity)
          err_message = err.message
          err_backtrace = err.backtrace
          unless AMS::IS_RUBY_VERSION_18
            err_message.force_encoding('UTF-8')
            err_backtrace.each { |i| i.force_encoding('UTF-8') }
          end
          #~ puts "Entity at index #{index} was not added to simulation:\n#{err.class}:\n#{err_message}\nTrace:\n#{err_backtrace.join("\n")}\n\n"
          puts "Entity at index #{index} was not added to simulation:\n#{err.class}:\n#{err_message}\n#{err.backtrace.join("\n")}\n"
        end
      elsif type == 'Buoyancy Plane'
        dict = 'MSPhysics Buoyancy Plane'
        unless entity.get_attribute(dict, 'Ignore', false)
          density = entity.get_attribute(dict, 'Density')
          density = default_buoyancy[:density] unless density.is_a?(Numeric)
          viscosity = entity.get_attribute(dict, 'Viscosity')
          viscosity = default_buoyancy[:viscosity] unless viscosity.is_a?(Numeric)
          current_x = entity.get_attribute(dict, 'Current X')
          current_x = default_buoyancy[:current_x] unless current_x.is_a?(Numeric)
          current_y = entity.get_attribute(dict, 'Current Y')
          current_y = default_buoyancy[:current_y] unless current_y.is_a?(Numeric)
          current_z = entity.get_attribute(dict, 'Current Z')
          current_z = default_buoyancy[:current_z] unless current_z.is_a?(Numeric)
          @buoyancy_planes[entity] = {
            :density => AMS.clamp(density, 0.001, nil),
            :linear_viscosity => AMS.clamp(viscosity, 0.0, 1.0),
            :angular_viscosity => AMS.clamp(viscosity, 0.0, 1.0),
            :linear_current => Geom::Vector3d.new(current_x, current_y, current_z),
            :angular_current => Geom::Vector3d.new(0, 0, 0)
          }
        end
      end
      return unless @world
    }
    # Clear collision cache again
    MSPhysics::Collision.clear_cache
    # Transform all entities once to register into the undo operation.
    @saved_transformations.each { |e, t|
      e.transformation = t if e.valid?
    }
    # Create Joints
    init_joints
    # Apply settings
    jlayer = model.layers['MSPhysics Joints']
    @joint_layer_orig_visible = (jlayer && jlayer.visible?) ? true : false
    jlayer.visible = false if jlayer && jlayer.visible? && MSPhysics::Settings.hide_joint_layer_enabled?
    @world.solver_model = MSPhysics::Settings.solver_model
    @world.set_gravity(0, 0, MSPhysics::Settings.gravity)
    @world.material_thickness = MSPhysics::Settings.material_thickness
    #~@world.max_threads_count = @world.max_possible_threads_count # C++ Extension unsafe with multithreading the moment
    @world.contact_merge_tolerance = default_sim[:contact_merge_tolerance]
    self.continuous_collision_check_enabled = MSPhysics::Settings.continuous_collision_check_enabled?
    self.view_full_screen(true) if MSPhysics::Settings.full_screen_mode_enabled?
    self.mode = MSPhysics::Settings.game_mode_enabled? ?  1 : 0
    self.undo_on_reset = MSPhysics::Settings.undo_on_end_enabled?
    self.animate_scenes(MSPhysics::Settings.animate_scenes_state)
    self.update_timestep = MSPhysics::Settings.update_timestep
    self.update_rate = MSPhysics::Settings.update_rate
    self.collision_wireframe_visible = MSPhysics::Settings.collision_wireframe_visible?
    self.axes_visible = MSPhysics::Settings.axes_visible?
    self.aabb_visible = MSPhysics::Settings.aabb_visible?
    self.contact_points_visible = MSPhysics::Settings.contact_points_visible?
    self.contact_forces_visible = MSPhysics::Settings.contact_forces_visible?
    self.bodies_visible = MSPhysics::Settings.bodies_visible?
    # Reinitialize joystick
    MSPhysics::SDL.quit_sub_system(MSPhysics::SDL::INIT_JOYSTICK)
    MSPhysics::SDL.init_sub_system(MSPhysics::SDL::INIT_JOYSTICK)
    # Close SP MIDI device if open
    if defined?(MIDIator::Interface.midiInterface) && MIDIator::Interface.midiInterface.respond_to?(:close)
      MIDIator::Interface.midiInterface.close
    end
    # Open MIDI device
    AMS::MIDI.open_device()
    # Initialize timers
    @time_info[:start] = Time.now
    @time_info[:last] = Time.now
    @fps_info[:last] = Time.now
    @timers_started = true
    # Camera follow and track from scenes
    page = model.pages.selected_page
    if page
      desc = page.description.downcase
      sentences = desc.split(/\./)
      sentences.each { |sentence|
        words = sentence.strip.split(/\s/)
        words.reject! { |word| word.empty? }
        if words.size >= 3 && words[0] == 'camera'
          if words[1] == 'follow'
            ent = find_group_by_name(words[2])
            if ent
              @camera[:follow] = ent
              @camera[:offset] = view.camera.eye - ent.bounds.center
            end
          elsif words[1] == 'track'
            ent = find_group_by_name(words[2])
            @camera[:target] = ent if ent
          end
        end
      }
    end
    # Call onStart event
    call_event(:onStart)
    return unless self.class.active?
    # Display control panel on Max OS X
    if !MSPhysics::ControlPanel.open? && !AMS::IS_PLATFORM_WINDOWS
      MSPhysics::ControlPanel.open
    end
    # Start the animation
    view.animation = @animation_enabled ? self : nil
    # Refresh view
    view.invalidate
    # Status indicator
    @simulation_started = true
  end

  def deactivate(view)
    model = view.model
    camera = view.camera
    # Call onEnd event
    orig_error = @error
    call_event(:onEnd)
    @error = orig_error if orig_error
    # Set time end
    end_time = Time.now
    # Destroy all emitted bodies
    if @erase_instances_on_end
      destroy_all_emitted_bodies
    end
    # Remove particles
    if @erase_instances_on_end
      clear_particles
    else
      MSPhysics::C::Particle.destroy_all
    end
    # Destroy world
    @world.destroy if @world.valid?
    @world = nil
    # Erase log-line and display-note
    if @log_line[:ent] != nil && @log_line[:ent].valid?
      @log_line[:ent].material = nil
      @log_line[:ent].erase!
    end
    if @log_line[:mat] != nil && @log_line[:mat].valid? && model.materials.respond_to?(:remove)
      model.materials.remove(@log_line[:mat])
    end
    @log_line.clear
    if @display_note[:ent] != nil && @display_note[:ent].valid?
      @display_note[:ent].material = nil
      @display_note[:ent].erase!
    end
    if @display_note[:mat] != nil && @display_note[:mat].valid? && model.materials.respond_to?(:remove)
      model.materials.remove(@display_note[:mat])
    end
    @display_note.clear
    # Reset entity transformations
    if @reset_positions_on_end
      @saved_transformations.each { |e, t|
        e.move!(t) if e.valid?
      }
    end
    @saved_transformations.clear
    # Show hidden entities
    @hidden_entities.each { |e|
      e.visible = true if e.valid? && !e.visible?
    }
    @hidden_entities.clear
    # Undo changed style made by the show collision function
    self.collision_wireframe_visible = false
    # Show cursor if hidden
    AMS::Cursor.show(true) if AMS::IS_PLATFORM_WINDOWS
    # Close control panel
    MSPhysics::ControlPanel.close
    MSPhysics::ControlPanel.remove_sliders
    # Clear variables of the common context
    MSPhysics::CommonContext.reset_variables
    # Remove observer
    AMS::Sketchup.remove_observer(self) if AMS::IS_PLATFORM_WINDOWS
    # Deactivate page observer (for OS X)
    if @frame_change_observer_id
      ::Sketchup::Pages.remove_frame_change_observer(@frame_change_observer_id)
      @frame_change_observer_id = nil
    end
    # Unset from fullscreen mode
    view_full_screen(false) if MSPhysics::Settings.full_screen_mode_enabled?
    # Recover joint layer
    if @joint_layer_orig_visible
      jlayer = model.layers['MSPhysics Joints']
      jlayer.visible = true if jlayer && !jlayer.visible?
    end
    # Reset camera orientation
    if @reset_camera_on_end
      opts = @camera[:original]
      camera.set(opts[:eye], opts[:target], opts[:up])
      #camera.aspect_ratio = opts[:aspect_ratio]
      camera.perspective = opts[:perspective]
      if opts[:perspective]
        camera.focal_length = opts[:focal_length]
        camera.fov = opts[:fov]
        camera.image_width = opts[:image_width]
      else
        camera.height = opts[:height]
      end
    end
    # Restore original selection
    model.selection.clear
    @selected_ents.each { |e|
      model.selection.add(e) if e.valid?
    }
    @selected_ents.clear
    # Ask user if replay animation needs saving.
    bsave_replay = @error.nil? && @timers_started && MSPhysics::Replay.recorded_data_valid? && ::UI.messagebox("Would you like to save the recorded simulation for replay?", MB_YESNO) == IDYES
    # This indicates whether to delete particle definition preserving instances.
    # This is done sooner than later to have it all under one operation.
    unless bsave_replay
      # Erase all added particle definition preserving instances
      @dp_particle_instances.each { |inst|
        inst.erase! if inst.valid?
      }
      @dp_particle_instances.clear
    end
    # Close joystick
    MSPhysics::Joystick.close_all_joysticks
    # Stop any playing sounds and music
    MSPhysics::Sound.destroy_all
    MSPhysics::Music.destroy_all
    # Close MIDI device
    AMS::MIDI.close_device()
    # Stop any running animation
    view.animation = nil
    # Purge unused definitions
    model.definitions.purge_unused
    # Make sure the undo called next does not undo a previous operation.
    model.entities.add_cpoint(ORIGIN) if @undo_on_reset
    # Finish all operations
    model.commit_operation
    # Undo all changes
    Sketchup.undo if @undo_on_reset
    # Free some variables
    @controller_context = nil
    @emitters.clear
    @thrusters.clear
    @buoyancy_planes.clear
    @controlled_joints.clear
    @cc_bodies.clear
    @particles.clear
    @particle_def2d.clear
    @particle_def3d.clear
    @simulation_started = false
    # Show info
    if @error
      err_message = @error.message
      err_backtrace = @error.backtrace
      unless AMS::IS_RUBY_VERSION_18
        err_message.force_encoding('UTF-8')
        err_backtrace.each { |i| i.force_encoding('UTF-8') }
      end
      msg = "MSPhysics Simulation has been aborted due to an error!\n#{@error.class}:\n#{err_message}"
      puts "#{msg}\nTrace:\n#{err_backtrace.join("\n")}\n\n"
      ::UI.messagebox(msg)
      if @error.is_a?(MSPhysics::ScriptException)
        MSPhysics::Dialog.locate_error(@error)
      end
    elsif @timers_started
      @time_info[:end] = end_time
      @time_info[:total] = @time_info[:end] - @time_info[:start]
      @time_info[:sim] += @time_info[:end] - @time_info[:last] unless @paused
      average_fps = (@time_info[:sim].zero? ? 0 : (@frame / @time_info[:sim]).round)
      puts 'MSPhysics Simulation Results:'
      printf("  frames          : %d\n", @frame)
      printf("  average FPS     : %d\n", average_fps)
      printf("  simulation time : %.2f seconds\n", @time_info[:sim])
      printf("  total time      : %.2f seconds\n\n", @time_info[:total])
      # Save replay animation data
      if bsave_replay
        MSPhysics::Replay.save_recorded_data
        if MSPhysics::Replay.camera_data_valid? && ::UI.messagebox("Would you like to smoothen recorded camera?", MB_YESNO) == IDYES
          MSPhysics::Replay.smoothen_camera_data(100)
        end
        begin
          MSPhysics::Replay.save_data_to_file(true)
        rescue Exception => err
          msg = "An exception occurred while attempting to save replay data to file!\n#{err.class}:\n#{err_message}"
          puts "#{msg}\nTrace:\n#{err_backtrace.join("\n")}\n\n"
          ::UI.messagebox(msg)
        end
      end
      # Update UI
      MSPhysics::Dialog.update_state
    else
      puts "MSPhysics Simulation was aborted before fully starting."
      MSPhysics::Dialog.update_state
    end
    MSPhysics::Replay.clear_recorded_data
    # Start garbage collection
    ::ObjectSpace.garbage_collect
    # Reset instance
    @@instance = nil
  end

  def onCancel(reason, view)
    self.class.reset
  end

  def resume(view)
    @suspended = false
    if @camera[:follow] && @camera[:follow].valid?
      @camera[:offset] = view.camera.eye - @camera[:follow].bounds.center
    end
    update_status_text
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def suspend(view)
    @suspended = true
  end

  def stop
    @animation_stopped = true
    if @animation_enabled
      Sketchup.active_model.active_view.invalidate
    end
  end

  def onMouseEnter(view)
    @mouse_over = true
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onMouseLeave(view)
    @mouse_over = false
  end

  def onMouseMove(flags, x, y, view)
    @cursor_pos.x = x
    @cursor_pos.y = y
    call_event(:onMouseMove, x, y, view) unless @paused
    return unless self.class.active?
  end

  def onLButtonDown(flags, x, y, view)
    sel = Sketchup.active_model.selection
    if @paused
      sel.clear
      return
    end
    @ip.pick(view, x, y)
    unless @ip.valid?
      sel.clear
      return
    end
    pick_pt = @ip.position
    # Use ray test as it determines positions more accurate than input point.
    ray = view.pickray(x,y)
    res = view.model.raytest(ray)
    if res
      pick_pt = res[0]
      ent = res[1][0]
    else
      ph = view.pick_helper
      ph.do_pick(x,y)
      ent = ph.best_picked
    end
    unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
      sel.clear
      return
    end
    body = find_body_by_group(ent)
    return unless body
    # Call the onClick event.
    begin
      @clicked = body
      @clicked.context.call_event(:onClick, Geom::Point3d.new(pick_pt))
    rescue Exception => err
      abort(err)
      return
    end
    return unless self.class.active?
    # Pick body if the body is not static.
    return if body.static? || @mode == 1
    loc_pick_pt = pick_pt.transform(body.get_matrix.inverse)
    body.continuous_collision_check_enabled = true
    @picked[:body] = body
    @picked[:loc_pick_pt] = loc_pick_pt
    @picked[:dest_pt] = pick_pt
    @picked[:plane_origin] = Geom::Point3d.new(pick_pt)
    @picked[:shift_down] = false
    @picked[:ccc] = body.continuous_collision_check_enabled?
    @picked[:orig_selected] = sel.include?(ent)
    sel.add(ent)
    @original_cursor_id = @cursor_id
    self.cursor = MSPhysics::CURSORS[:grab]
    view.lock_inference
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onLButtonDoubleClick(flags, x, y, view)
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onLButtonUp(flags, x, y, view)
    unless @picked.empty?
      if @picked[:body].valid?
        @picked[:body].continuous_collision_check_enabled = @picked[:ccc]
        if @picked[:body].group.valid?
          view.model.selection.remove(@picked[:body].group) unless @picked[:orig_selected]
        end
      end
      @picked.clear
      self.cursor = @original_cursor_id
    end
    begin
      @clicked.context.call_event(:onUnclick)
    rescue Exception => err
      abort(err)
    end if @clicked and @clicked.valid?
    @clicked = nil
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onRButtonDown(flags, x, y, view)
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onRButtonDoubleClick(flags, x, y, view)
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onRButtonUp(flags, x, y, view)
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  # Implementing this will disable the orbit tool, which is not what we want.
  #~ def onMButtonDown(flags, x, y, view)
    #~ MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  #~ end

  def onMButtonDoubleClick(flags, x, y, view)
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def onMButtonUp(flags, x, y, view)
    MSPhysics::ControlPanel.bring_to_front unless AMS::IS_PLATFORM_WINDOWS
  end

  def getMenu(menu)
    @menu_entered2 = true

    menu.add_item(@paused ? 'Play' : 'Pause') {
      self.toggle_play
    }
    menu.add_item('Reset') {
      self.class.reset
    }

    model = Sketchup.active_model
    view = model.active_view
    sel = model.selection
    ph = view.pick_helper
    ph.do_pick(@cursor_pos.x, @cursor_pos.y)
    ent = ph.best_picked

    menu.add_separator

    if ent.visible?
      item = menu.add_item('Hide Entity') {
        ent.visible = false
      }
    else
      item = menu.add_item('Show Entity') {
        ent.visible = true
      }
    end

    return unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
    menu.add_separator
    sel.add(ent)
    item = menu.add_item('Camera Follow') {
      next unless ent.valid?
      if @camera[:follow] == ent
        @camera[:follow] = nil
      else
        @camera[:follow] = ent
        @camera[:offset] = view.camera.eye - ent.bounds.center
      end
    }
    menu.set_validation_proc(item) {
      @camera[:follow] == ent ? MF_CHECKED : MF_UNCHECKED
    }
    item = menu.add_item('Camera Target') {
      next unless ent.valid?
      if @camera[:target] == ent
        @camera[:target] = nil
      else
        @camera[:target] = ent
      end
    }
    menu.set_validation_proc(item) {
      @camera[:target] == ent ? MF_CHECKED : MF_UNCHECKED
    }
    item = menu.add_item('Camera Follow and Target') {
      next unless ent.valid?
      if @camera[:follow] == ent && @camera[:target] == ent
        @camera[:follow] = nil
        @camera[:target] = nil
        @camera[:follow] = nil
      else
        @camera[:follow] = ent
        @camera[:target] = ent
        @camera[:offset] = view.camera.eye - ent.bounds.center
      end
    }
    menu.set_validation_proc(item) {
      @camera[:follow] == ent && @camera[:target] == ent ? MF_CHECKED : MF_UNCHECKED
    }
    if @camera[:target] != nil || @camera[:follow] != nil
      menu.add_item('Camera Clear') {
        @camera[:follow] = nil
        @camera[:target] = nil
      }
    end

    body = find_body_by_group(ent)
    return unless body
    menu.add_separator
    menu.add_item('Freeze Body') {
      next unless body.valid?
      body.frozen = true
    }
  end

  def onSetCursor
    ::UI.set_cursor(@cursor_id)
  end

  def getInstructorContentDirectory
  end

  def getExtents
    if Sketchup.version.to_i > 6
      Sketchup.active_model.entities.each { |e|
        next if e.is_a?(Sketchup::Text) && !e.has_leader?
        if e.visible? && e.layer.visible?
          ebb = e.bounds
          if ebb.valid?
            c = ebb.center
            next if c.x.abs > 1.0e10 || c.y.abs > 1.0e10 || c.z.abs > 1.0e10
            @bb.add(ebb)
          end
        end
      }
    end
    @bb.empty? ? Sketchup.active_model.bounds : @bb
  end

  def draw(view)
    return if @error || !@simulation_started
    if @animation_stopped && @animation_enabled
      view.animation = self
      @animation_stopped = false
    end
    draw_contact_points(view)
    draw_contact_forces(view)
    draw_collision_wireframe(view)
    draw_axes(view)
    draw_aabb(view)
    draw_pick_and_drag(view)
    draw_particles(view, @bb)
    draw_fullscreen_note(view)
    draw_fancy_note(view)
    draw_queues(view)
    return unless self.class.active?
    view.drawing_color = 'black'
    view.line_width = 5
    view.line_stipple = ''
    call_event2(:onDraw, view, @bb)
  end

  # AMS SketchUp Observer

  def swo_activate
  end

  def swo_deactivate
    self.class.reset
  end


  def swo_on_post_enter_menu
    @menu_entered = true
  end

  def swo_on_post_exit_menu
    @menu_entered = false
    if @menu_entered2
      sel = Sketchup.active_model.selection
      sel.clear
      sel.add @camera[:follow] if @camera[:follow] && @camera[:follow].valid?
      sel.add @camera[:target] if @camera[:target] && @camera[:target].valid?
      @menu_entered2 = false
    end
  end

  def swp_on_key_down(key, val, char)
    return if @menu_entered
    case key
      when 'escape'
        self.class.reset
        return
      when 'pause'
        toggle_play
    end
    call_event(:onKeyDown, key, val, char) unless @paused
    1
  end

  def swp_on_key_extended(key, val, char)
    return if @menu_entered
    call_event(:onKeyExtended, key, val, char) unless @paused
    1
  end

  def swp_on_key_up(key, val, char)
    return if @menu_entered
    call_event(:onKeyUp, key, val, char) unless @paused
    1
  end


  def swp_on_lbutton_down(x,y)
    call_event(:onLButtonDown, x, y) unless @paused
    0
  end

  def swp_on_lbutton_up(x,y)
    call_event(:onLButtonUp, x, y) unless @paused
    0
  end

  def swp_on_lbutton_double_click(x,y)
    call_event(:onLButtonDoubleClick, x, y) unless @paused
    0
  end


  def swp_on_rbutton_down(x,y)
    call_event(:onRButtonDown, x, y) unless @paused
    # Prevent the menu from showing up if user selects anything, other than
    # simulation bodies.
    if @mode == 0 and !@suspended
      ph = Sketchup.active_model.active_view.pick_helper
      ph.do_pick x,y
      ent = ph.best_picked
      return 1 unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
      return find_body_by_group(ent) ? 0 : 1
    end
    @mode
  end

  def swp_on_rbutton_up(x,y)
    call_event(:onRButtonUp, x, y) unless @paused
    @mode
  end

  def swp_on_rbutton_double_click(x,y)
    call_event(:onRButtonDoubleClick, x, y) unless @paused
    @mode
  end


  def swp_on_mbutton_down(x,y)
    unless @paused
      call_event(:onMButtonDown, x, y)
      return 1 if self.scenes_transitioning? || self.scenes_animating?
    end
    @mode
  end

  def swp_on_mbutton_up(x,y)
    unless @paused
      call_event(:onMButtonUp, x, y)
      return 1 if self.scenes_transitioning? || self.scenes_animating?
    end
    @mode
  end

  def swp_on_mbutton_double_click(x,y)
    unless @paused
      call_event(:onMButtonDoubleClick, x, y)
      return 1 if self.scenes_transitioning? || self.scenes_animating?
    end
    @mode
  end


  def swp_on_xbutton1_down(x,y)
    call_event(:onXButton1Down, x, y) unless @paused
    0
  end

  def swp_on_xbutton1_up(x,y)
    call_event(:onXButton1Up, x, y) unless @paused
    0
  end

  def swp_on_xbutton1_double_click(x,y)
    call_event(:onXButton1DoubleClick, x, y) unless @paused
    0
  end


  def swp_on_xbutton2_down(x,y)
    call_event(:onXButton2Down, x, y) unless @paused
    0
  end

  def swp_on_xbutton2_up(x,y)
    call_event(:onXButton2Up, x, y) unless @paused
    0
  end

  def swp_on_xbutton2_double_click(x,y)
    call_event(:onXButton2DoubleClick, x, y) unless @paused
    0
  end


  def swp_on_mouse_wheel_rotate(x,y, dir)
    unless @paused
      call_event(:onMouseWheelRotate, x, y, dir)
      return 1 if self.scenes_transitioning? || self.scenes_animating?
    end
    @mode
  end

  def swp_on_mouse_wheel_tilt(x,y, dir)
    call_event(:onMouseWheelTilt, x, y, dir) unless @paused
    0
  end

  def swp_on_page_selected(page1, page2, tab1, tab2)
    if @scene_anim_info[:state] == 0
      dtt = Sketchup.active_model.options['PageOptions']['TransitionTime']
      tt = (page2.transition_time < 0 ? dtt : page2.transition_time).to_f
      if tt < MSPhysics::EPSILON
        odt = page2.delay_time
        ott = page2.transition_time
        page2.delay_time = 0
        page2.transition_time = 0
        Sketchup.active_model.pages.selected_page = page2
        page2.delay_time = odt
        page2.transition_time = ott
      else
        AMS::Sketchup.activate_scenes_bar_tab(tab2)
        @scene_info[:active] = true
        @scene_info[:data1] = MSPhysics::SceneData.new()
        @scene_info[:data2] = MSPhysics::SceneData.new(page2)
        @scene_info[:transition_time] = tt
        @scene_info[:elasted_time] = 0
      end
    end
    1
  end

  def frameChange(from_scene, to_scene, percent_done)
    if @scene_anim_info[:state] == 0
      dtt = Sketchup.active_model.options['PageOptions']['TransitionTime']
      tt = (to_scene.transition_time < 0 ? dtt : to_scene.transition_time).to_f
      if tt < MSPhysics::EPSILON
        if @scene_info[:active]
          @scene_info[:active] = false
          @scene_info[:data1] = nil
          @scene_info[:data2] = nil
          @scene_info[:transition_time] = 0
          @scene_info[:elasted_time] = 0
        end
      else
        @scene_info[:active] = true
        @scene_info[:data1] = MSPhysics::SceneData.new()
        @scene_info[:data2] = MSPhysics::SceneData.new(to_scene)
        @scene_info[:transition_time] = tt
        @scene_info[:elasted_time] = 0
      end
    end
  end

  def nextFrame(view)
    do_on_update(view)
  end

  def configure_for_external_control
    @animation_enabled = false
    @mode = 1
  end

  def next_frames(frame_count, view = nil)
    for i in 0..frame_count
      view = Sketchup.active_model.active_view unless view
      do_on_update(view)
    end
  end

end # class MSPhysics::Simulation
