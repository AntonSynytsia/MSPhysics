module MSPhysics
  class SimulationTool

    # @!visibility private
    VALID_TYPES = [Sketchup::Group, Sketchup::ComponentInstance]

    # Holds reference to {MSPhysics::Simulation} instance.
    $msp_simulation = nil
    # Holds reference to {MSPhysics::SimulationTool} instance.
    $msp_simulation_tool = nil

    def initialize
      @time = { :start => 0, :end => 0, :last => 0, :sim => 0, :total => 0 }
      @fps = { :val => 0, :update_rate => 10, :last => 0, :change => 0 }
      @note = 'Click and drag to move. Hold SHIFT while dragging to lift.'
      @note << '    PAUSE - toggle play.    ESC - quit.'
      @frame = 0
      @change = 0
      @paused = false
      @suspended = false
      @deactivated = false
      @pause_updated = false
      @animation_stop = false
      @mouse_enter = false
      @menu_enter = false
      @error = nil
      @mode = 0 # 0 - interaction mode, 1 - game mode.
      @simulation = Simulation.new
      @camera = {}
      @cursor_pos = [0,0]
      @ip1 = Sketchup::InputPoint.new
      @ip = Sketchup::InputPoint.new
      @picked = []
      @clicked = nil
      @selection = []
      @cursor_id = MSPhysics::CURSORS[:hand]
      @drag = {
        :line_width     => 2,
        :line_stipple   => '_',
        :point_size     => 10,
        :point_style    => 4,
        :point_color    => Sketchup::Color.new(255,0,0),
        :line_color     => Sketchup::Color.new(0,0,255)
      }
      @pause_tag = {
        :offset         => [[10,10], [50,10]],
        :points         => [[0,0],[30,0],[30,70],[0,70]],
        :color1         => [0,60,200,255],
        :color2         => [0,0,10,255],
        :stipple        => '',
        :width          => 2,
        :opacity        => 0,
        :rate           => 15
      }
      @deferred_tasks = []
    end

    # @!attribute [r] simulation
    #   @return [MSPhysics::Simulation]
    attr_reader :simulation

    # Get simulation time in frames.
    # @return [Fixnum]
    def frame
      @frame
    end

    # Get simulation update rate in frames per second.
    # @return [Numeric]
    def fps
      @fps[:val]
    end

    # Play simulation.
    # @return [Boolean] +true+ (if successful).
    def play
      return false unless @paused
      @paused = false
      call_event2(:onPlay)
      true
    end

    # Pause simulation.
    # @return [Boolean] +true+ (if successful).
    def pause
      return false if @paused
      @paused = true
      call_event2(:onPause)
      true
    end

    # Play/Pause simulation.
    # @return [Boolean] +true+ (if successful).
    def toggle_play
      @paused ? play : pause
    end

    # Determine whether simulation is playing.
    # @return [Boolean]
    def playing?
      !(@paused or @deactivated or @suspended)
    end

    # Determine whether simulation is paused.
    # @return [Boolean]
    def paused?
      (@paused or @deactivated or @suspended)
    end

    # Get cursor position in view coordinates.
    # @return [Array<Fixnum>] +[x,y]+
    def cursor_pos
      @cursor_pos
    end

    # Get simulation mode.
    # @return [Fixnum]
    #   +0+ - Interactive mode: The drag tool and rotating camera is enabled.
    #   +1+ - Game mode: The drag tool and rotating camera is disabled.
    def mode
      @mode
    end

    # Set simulation mode.
    # @param [Fixnum] mode
    #   +0+ - Interactive mode: The drag tool and rotating camera is enabled.
    #   +1+ - Game mode: The drag tool and rotating camera is disabled.
    def mode=(mode)
      @mode = (mode == 1 ? 1 : 0)
    end

    # Get cursor id.
    # @return [Fixnum]
    def cursor_id
      @cursor_id
    end

    # Set cursor id.
    # @param [Fixnum] id
    # @see MSPhysics::CURSORS
    # @example
    #   onStart {
    #     simulation_tool.cursor_id = MSPhysics::CURSORS[:target]
    #   }
    def cursor_id=(id)
      @cursor_id = id.to_i
    end

    private

    def update_status_text
      @change = @simulation.change if @frame % 10 == 0
      Sketchup.status_text = "Frame : #{@frame}    FPS : #{@fps[:val]}    Change : #{@change} ms    #{@note}" if @mouse_enter
    end

    def call_event(evt, *args)
      return if paused?
      begin
        @simulation.call_event(evt, *args)
      rescue Exception => e
        abort(e)
      end
    end

    def call_event2(evt, *args)
      begin
        @simulation.call_event(evt, *args)
      rescue Exception => e
        abort(e)
      end
    end

    def defer_task(&block)
      @deferred_tasks << block
    end

    def abort(e)
      @error = e
      self.class.reset
    end

    public

    # @!visibility private

    # SketchUp Tool Events

    def activate
      model = Sketchup.active_model
      view = model.active_view
      cam = view.camera
      # Close active path
      state = true
      while state
        state = model.close_active
      end
      # Wrap operations
      if Sketchup.version.to_i > 6
        model.start_operation('MSPhysics', true)
      else
        model.start_operation('MSPhysics')
      end
      # Save camera orientation
      @camera[:orig] = [cam.eye, cam.target, cam.up]
      # Activate tools
      AMS::InputProc.select_tool(self, true, false, false)
      AMS::Sketchup.add_observer(self)
      # Save original selection
      model.selection.to_a.each{ |e|
        # Use entity ID to get proper reference if entity was once deleted.
        @selection << e.entityID
      }
      # Create global variables for easy access
      $msp_simulation = @simulation
      $msp_simulation_tool = self
      # Clear selection
      model.selection.clear
      # Initialize timers
      t = Time.now
      @time[:start] = t
      @time[:last] = t
      @fps[:last] = t
      view.animation = self
      # Start simulation
      begin
        @simulation.do_on_start
      rescue Exception => e
        abort(e)
        return
      end
    end

    def deactivate(view)
      view.animation = nil
      # End simulation
      begin
        @simulation.do_on_end
      rescue Exception => e
        @error = e unless @error
      end
      # Clear global variables
      $msp_simulation = nil
      $msp_simulation_tool = nil
      # Remove observers and deselect tools
      AMS::Sketchup.remove_observer(self)
      AMS::InputProc.deselect_tool(self)
      # Use abort_operation rather than commit_operation.
      # Abort operation undoes most model changes made during simulation.
      begin
        Sketchup.active_model.abort_operation
      rescue Exception => e
        puts "Abort failed!\n #{e}"
      end
      # Reset entity transformations.
      begin
        @simulation.do_on_end_post_operation
      rescue Exception => e
        puts "An error occurred while resetting post operation data:\n#{e}\n#{e.backtrace[0..2].join("\n")}"
      end
      # Set camera to original placement.
      view.camera.set(*@camera[:orig])
      # Add original selection.
      sel = Sketchup.active_model.selection
      sel.clear
      to_select = []
      @selection.each{ |id|
        e = MSPhysics.get_entity_by_id(id)
        to_select << e if e
      }
      sel.add(to_select)
      # Clear instance
      @@instance = nil
      # Refresh view
      view.invalidate
      # Show info
      if @error
        msg = "MSPhysics Simulation was aborted due to an error!\n\n#{@error}"
        puts msg + "\n\n"
        UI.messagebox(msg)
        data = MSPhysics::BodyContext._error_reference
        Dialog.lead_to_error(data)
        MSPhysics::BodyContext._error_reference = nil
      else
        @time[:end] = Time.now
        @time[:total] = @time[:end] - @time[:start]
        @time[:sim] += @time[:end] - @time[:last] unless paused?
        average_fps = (@time[:sim].zero? ? 0 : (@frame / @time[:sim]).round)
        puts 'MSPhysics Simulation Results:'
        printf("  frames          : %d\n", @frame)
        printf("  average FPS     : %d\n", average_fps)
        printf("  simulation time : %.2f seconds\n", @time[:sim])
        printf("  total time      : %.2f seconds\n\n", @time[:total])
      end
      # Clear some variables
      @picked.clear
      @clicked = nil
      @selection.clear
    end

    def onCancel(reason, view)
      self.class.reset
    end

    def resume(view)
      @suspended = false
      if @camera[:follow] and @camera[:follow].valid?
        @camera[:offset] = view.camera.eye - @camera[:follow].bounds.center
      end
      update_status_text
      view.invalidate
    end

    def suspend(view)
      @suspended = true
    end

    def onMouseEnter(view)
      @mouse_enter = true
      return unless @menu_enter
      sel = Sketchup.active_model.selection
      sel.clear
      sel.add @camera[:follow] if @camera[:follow] and @camera[:follow].valid?
      sel.add @camera[:target] if @camera[:target] and @camera[:target].valid?
      @menu_enter = false
    end

    def onMouseLeave(view)
      @mouse_enter = false
    end

    def onMouseMove(flags, x, y, view)
      @cursor_pos = [x,y]
      call_event(:onMouseMove, x, y)
      return unless self.class.active?
      @ip.pick view, x, y
      return if @ip == @ip1
      @ip1.copy! @ip
      # view.tooltip = @ip1.tooltip
      return if @mode == 1 or @picked.empty?
      if @picked[0].valid?
        begin
          @picked[0].call_event(:onDrag) if @picked[4] != @frame
        rescue Exception => e
          abort(e)
          return
        end
        return unless self.class.active?
        @picked[4] = @frame
      else
        @picked.clear
        return
      end
      cam = view.camera
      line = [cam.eye, @ip1.position]
      if AMS::Keyboard.key_down?('shift')
        normal = view.camera.zaxis
        normal.z = 0
        normal.normalize!
      else
        normal = Z_AXIS
      end
      plane = [@picked[2], normal]
      vector = cam.eye.vector_to(@ip1.position)
      theta = vector.angle_between(normal).radians
      if (90 - theta).abs > 1
        pt = Geom.intersect_line_plane(line, plane)
        v = cam.eye.vector_to(pt)
        @picked[2] = pt if cam.zaxis.angle_between(v).radians < 90
      end
      #@picked[2] = @ip1.position
    end

    def onLButtonDown(flags, x, y, view)
      @ip1.pick view, x, y
      return unless @ip1.valid?
      pos = @ip1.position
      # Use raytest as it determines positions more accurate than inputpoint.
      res = view.model.raytest( view.pickray(x,y) )
      pos = res[0] if res
      ph = view.pick_helper
      ph.do_pick x,y
      ent = ph.best_picked
      return unless VALID_TYPES.include?(ent.class)
      body = @simulation.get_body_by_entity(ent)
      return unless body
=begin
      # Correct input point position if is not located on the picked entity.
      # 1. Transform input point position relative to the deepest element
      # coordinate system.
      path = ph.path_at(0)[0..-2]
      path.each { |e|
        pos.transform!(e.transformation.inverse)
      }
      deepest = ph.leaf_at(0)
      # 2. Verify that input point is located on the picked entity. If point
      # is not on the entity, then use deepest element position as the new
      # reference to the point.
      case deepest
      when Sketchup::ConstructionPoint
        pos = deepest.position
      when Sketchup::Edge
        unless MSPhysics::Geometry.is_point_on_edge?(pos, deepest)
          pos = MSPhysics::Geometry.calc_edge_centre(deepest)
        end
      when Sketchup::Face
        unless MSPhysics::Geometry.is_point_on_face?(pos, deepest)
          pos = MSPhysics::Geometry.calc_face_centre(deepest)
        end
      end
      # 3. Transform input point back into global coordinate system for
      # implementation.
      path.reverse.each { |e|
        pos.transform!(e.transformation)
      }
=end
      # Call the onClick event.
      begin
        @clicked = body
        @clicked.call_event(:onClick, pos.clone)
      rescue Exception => e
        abort(e)
        return
      end unless @paused
      return unless self.class.active?
      # Pick body if the body is not static.
      return if body.static? or @mode == 1
      pick_pt = pos.transform(ent.transformation.inverse)
      cc = body.get_continuous_collision_mode
      body.set_continuous_collision_mode(true)
      @picked = [body, pick_pt, pos, cc]
      view.lock_inference
    end

    def onLButtonUp(flags, x, y, view)
      unless @picked.empty?
        @picked[0].set_continuous_collision_mode(@picked[3]) if @picked[0].valid?
        @picked.clear
      end
      begin
        @clicked.call_event(:onUnclick)
      rescue Exception => e
        abort(e)
      end if @clicked and @clicked.valid?
      @clicked = nil
    end

    def nextFrame(view)
      # Call all deferred tasks.
      @deferred_tasks.each { |task| task.call }
      @deferred_tasks.clear
      return false unless self.class.active?
      # Handle simulation play/pause events.
      if @paused or @suspended or @deactivated
        unless @pause_updated
          t = Time.now
          @time[:sim] += t - @time[:last]
          @fps[:change] += t - @fps[:last]
          @pause_updated = true
          begin
            @clicked.call_event(:onUnclick)
          rescue Exception => e
            abort(e)
            return
          end if @clicked and @clicked.valid?
          @clicked = nil
          return false unless self.class.active?
        end
        view.show_frame
        return true
      end
      if @pause_updated
        t = Time.now
        @time[:last] = t
        @fps[:last] = t
        @pause_updated = false
      end
      # Update newton world.
      begin
        @simulation.do_on_update(@frame)
      rescue Exception => e
        abort(e)
        return
      end if @frame > 0
      return false unless self.class.active?
      # Update camera
      cam = view.camera
      ent = @camera[:follow]
      if ent
        if ent.deleted?
          @camera[:follow] = nil
        else
          eye = ent.bounds.center + @camera[:offset]
          tar = eye + cam.direction.to_a
          cam.set(eye, tar, [0,0,1])
        end
      end
      ent = @camera[:target]
      if ent
        if ent.deleted?
          @camera[:target] = nil
        else
          dir = cam.eye.vector_to(ent.bounds.center)
          cam.set(cam.eye, dir, [0,0,1])
        end
      end
      # Process dragged body
      unless @picked.empty?
        if @picked[0].valid?
          Body.apply_pick_force(@picked[0], @picked[1], @picked[2], 100, 10)
        else
          @picked.clear
        end
      end
      # Update FPS
      if @frame % @fps[:update_rate] == 0
        @fps[:change] += Time.now - @fps[:last]
        @fps[:val] = ( @fps[:change] == 0 ? 0 : (@fps[:update_rate] / @fps[:change]).round )
        @fps[:last] = Time.now
        @fps[:change] = 0
      end
      update_status_text
      @frame += 1
      view.show_frame
      true
    end

    def stop
      @animation_stop = true
      Sketchup.active_model.active_view.show_frame
    end

    def draw(view)
      @simulation.bb.clear
      if @animation_stop
        view.animation = self
        @animation_stop = false
      end
      animate_pause = false
      if @paused or @suspended or @deactivated
        animate_pause = true
        v = @pause_tag[:opacity]
        v += @pause_tag[:rate]
        v = 240 if v > 240
        @pause_tag[:opacity] = v
      elsif @pause_tag[:opacity] > 0
        animate_pause = true
        v = @pause_tag[:opacity]
        v -= @pause_tag[:rate]
        v = 0 if v < 0
        @pause_tag[:opacity] = v
      end
      if animate_pause
        @pause_tag[:offset].each { |ox,oy|
          pts = []
          @pause_tag[:points].each { |x,y|
            pts << [x+ox, y+oy]
          }
          c1 = @pause_tag[:color1]
          c1[3] = @pause_tag[:opacity].to_i
          c2 = @pause_tag[:color2]
          c2[3] = @pause_tag[:opacity].to_i
          view.drawing_color = c1
          view.draw2d(GL_POLYGON, pts)
          view.drawing_color = c2
          view.line_stipple = @pause_tag[:stipple]
          view.line_width = @pause_tag[:width]
          view.draw2d(GL_LINE_LOOP, pts)
        }
      end
      unless @picked.empty?
        if @picked[0].entity.deleted?
          @picked.clear
        else
          pt1 = @picked[1].transform(@picked[0].entity.transformation)
          pt2 = @picked[2]
          @simulation.bb.add(pt2)
          view.line_width = @drag[:line_width]
          view.line_stipple = @drag[:line_stipple]
          view.drawing_color = @drag[:line_color]
          view.draw_line(pt1, pt2)
          view.line_stipple = ''
          view.draw_points(pt1, @drag[:point_size], @drag[:point_style], @drag[:point_color])
        end
      end
      # @ip1.draw(view) if (@ip1.valid? and @ip1.display?)
      begin
        @simulation.do_on_draw(view)
      rescue Exception => e
        abort(e)
      end
    end

    def onSetCursor
      UI.set_cursor(@cursor_id)
    end

    def getInstructorContentDirectory
    end

    def getMenu(menu)
      menu.add_item(self.paused? ? 'Play' : 'Pause'){
        self.toggle_play
      }
      menu.add_item('Reset'){
        self.class.reset
      }
      menu.add_separator

      model = Sketchup.active_model
      view = model.active_view
      sel = model.selection
      ph = view.pick_helper
      ph.do_pick *@cursor_pos
      ent = ph.best_picked
      return unless VALID_TYPES.include?(ent.class)
      body = @simulation.get_body_by_entity(ent)
      return unless body
      @menu_enter = true
      sel.add ent
      item = menu.add_item('Camera Follow'){
        next unless body.valid?
        if @camera[:follow] == ent
          @camera[:follow] = nil
        else
          @camera[:follow] = ent
          @camera[:offset] = view.camera.eye - ent.bounds.center
        end
      }
      menu.set_validation_proc(item){
        @camera[:follow] == ent ? MF_CHECKED : MF_UNCHECKED
      }
      item = menu.add_item('Camera Target'){
        next unless body.valid?
        if @camera[:target] == ent
          @camera[:target] = nil
        else
          @camera[:target] = ent
        end
      }
      menu.set_validation_proc(item){
        @camera[:target] == ent ? MF_CHECKED : MF_UNCHECKED
      }
      if @camera[:target] or @camera[:follow]
        menu.add_item('Camera Clear'){
          @camera[:follow] = nil
          @camera[:target] = nil
        }
      end
      menu.add_item('Freeze Body'){
        next unless body.valid?
        body.frozen = true
      }
      menu.add_item('Destroy Body'){
        next unless body.valid?
        body.destroy(true)
      }
    end

    def getExtents
      if Sketchup.version.to_i > 6
        Sketchup.active_model.entities.each { |ent|
          @simulation.bb.add(ent.bounds)
        }
      end
      @simulation.bb
    end


    # AMS User Input Events

    def hk_activate
    end

    def hk_deactivate
      defer_task { self.class.reset }
    end


    def hk_onKeyDown(key, val, char)
      defer_task {
        case key
        when 'escape'
          self.class.reset
          next
        when 'pause'
          toggle_play
        end
        call_event(:onKeyDown, key, val, char)
      }
      1
    end

    def hk_onKeyExtended(key, val, char)
      defer_task { call_event(:onKeyExtended, key, val, char) }
      1
    end

    def hk_onKeyUp(key, val, char)
      defer_task { call_event(:onKeyUp, key, val, char) }
      1
    end


    def hk_onLButtonDown(x,y)
      defer_task { call_event(:onLButtonDown, x, y) }
      0
    end

    def hk_onLButtonUp(x,y)
      defer_task { call_event(:onLButtonUp, x, y) }
      0
    end

    def hk_onLButtonDoubleClick(x,y)
      defer_task { call_event(:onLButtonDoubleClick, x, y) }
      0
    end


    def hk_onRButtonDown(x,y)
      defer_task { call_event(:onRButtonDown, x, y) }
      # Prevent the menu from showing up if user selects anything, other than
      # simulation bodies.
      if @mode == 0 and !@suspended
        view = Sketchup.active_model.active_view
        ph = view.pick_helper
        ph.do_pick x,y
        ent = ph.best_picked
        return @simulation.get_body_by_entity(ent) ? 0 : 1
      end
      @mode
    end

    def hk_onRButtonUp(x,y)
      defer_task { call_event(:onRButtonUp, x, y) }
      @mode
    end

    def hk_onRButtonDoubleClick(x,y)
      defer_task { call_event(:onRButtonDoubleClick, x, y) }
      # Prevent the menu from showing up if user selects anything, other than
      # simulation bodies.
      if @mode == 0 and !@suspended
        view = Sketchup.active_model.active_view
        ph = view.pick_helper
        ph.do_pick x,y
        ent = ph.best_picked
        return @simulation.get_body_by_entity(ent) ? 0 : 1
      end
      @mode
    end


    def hk_onMButtonDown(x,y)
      defer_task { call_event(:onMButtonDown, x, y) }
      @mode
    end

    def hk_onMButtonUp(x,y)
      defer_task { call_event(:onMButtonUp, x, y) }
      @mode
    end

    def hk_onMButtonDoubleClick(x,y)
      defer_task { call_event(:onMButtonDoubleClick, x, y) }
      @mode
    end


    def hk_onXButtonDown(x,y)
      defer_task { call_event(:onXButtonDown, x, y) }
      0
    end

    def hk_onXButtonUp(x,y)
      defer_task { call_event(:onXButtonUp, x, y) }
      0
    end

    def hk_onXButtonDoubleClick(x,y)
      defer_task { call_event(:onXButtonDoubleClick, x, y) }
      0
    end


    def hk_onMouseWheelRotate(x,y, dir)
      defer_task { call_event(:onMouseWheelRotate, x, y, dir) }
      @mode
    end

    def hk_onMouseWheelTilt(x,y, dir)
      defer_task { call_event(:onMouseWheelTilt, x, y, dir) }
      0
    end


    # AMS SketchUp Window Observers

    def swo_activate
    end

    def swo_deactivate
      defer_task { self.class.reset }
    end

    def swo_mw_onActivate
      @deactivated = false
    end

    def swo_mw_onDeactivate
      @deactivated = true unless AMS::Sketchup.active?
    end


    # @!visibility private
    @@instance = nil

    class << self

      # Start simulation.
      # @return [Boolean] +true+ (if successful).
      def start
        return false if @@instance
        @@instance = SimulationTool.new
        Sketchup.active_model.select_tool(@@instance)
        true
      end

      # Reset simulation.
      # @return [Boolean] +true+ (if successful).
      def reset
        return false unless @@instance
        Sketchup.active_model.select_tool(nil)
        @@instance = nil
        # GC.start
        true
      end

      # Determine whether simulation is running.
      # @return [Boolean]
      def active?
        @@instance ? true : false
      end

      # Determine whether simulation is not running.
      # @return [Boolean]
      def inactive?
        @@instance ? false : true
      end

      # Get simulation tool instance.
      # @return [SimulationTool, NilClass]
      def instance
        @@instance
      end

    end # proxy class
  end # class SimulationTool

end # module MSPhysics
