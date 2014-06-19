module MSPhysics
  class JointConnectionTool

    # @!visibility private
    @@instance = nil

    class << self

      # Activate the joint connection tool.
      def activate
        return if @@instance
        Sketchup.active_model.select_tool(self.new)
      end

      # Deactivate the joint connection tool.
      def deactivate
        return unless @@instance
        Sketchup.active_model.select_tool(nil)
      end

      # Determine whether the joint connection tool is active.
      def active?
        @@instance ? true : false
      end

    end

    def initialize
      model = Sketchup.active_model
      # Close active path
      state = true
      while state
        state = model.close_active
      end
      model.selection.clear
      @ctrl_down    = false
      @shift_down   = false
      @parent       = nil
      @picked       = nil
      @connected    = []
      @cursor_id    = MSPhysics::CURSORS[:select]
      @body_color   = [100,0,160]
      @joint_color  = [255,255,0]
      @line_width1  = 3
      @line_width2  = 2
      @pins         = [0,1,1,3,3,2,2,0, 4,5,5,7,7,6,6,4, 0,4,1,5,2,6,3,7]
    end

    private

    def refresh_viewport
      Sketchup.active_model.active_view.invalidate
      onSetCursor
    end

    def add_to_connected(ent)
      if @cursor_id == MSPhysics::CURSORS[:select_plus]
        unless @connected.include?(ent)
          @connected << ent
        end
      else
        unless @connected.delete(ent)
          @connected << ent
        end
      end
    end

    public

    # Get all valid joints.
    # @return [Array<Sketchup::ComponentInstance>]
    def get_all_joints
      joints = []
      Sketchup.active_model.entities.each { |ent|
        type = MSPhysics.get_entity_type(ent)
        if type == 'Body'
          ents = ent.respond_to?(:definition) ? ent.definition.entities : ent.entities
          ents.each { |e|
            t = MSPhysics.get_entity_type(e)
            joints << e if t == 'Joint'
          }
        elsif type == 'Joint'
          joints << ent
        end
      }
      joints
    end

    # Get joint entities connected to the specified body entity.
    # @param [Sketchup::Group, Sketchup::ComponentDefinition] picked_body
    # @return [Array<Sketchup::ComponentInstance>]
    def get_connected_joints(picked_body)
      body_id = picked_body.entityID
      joints = []
      get_all_joints.each { |ent|
        data = ent.get_attribute('MSPhysics Joint', 'Connected', '[]')
        begin
          connected = eval(data)
          raise unless connected.is_a?(Array)
        rescue Exception => e
          ent.set_attribute('MSPhysics Joint', 'Connected', '[]')
          connected = []
        end
        joints << ent if connected.include?(body_id)
      }
      joints
    end

    # Get body entities connected to the specified joint entity.
    # @param [Array<Sketchup::ComponentInstance>] picked_joint
    # @return [Array<Sketchup::Group, Sketchup::ComponentInstance>]
    def get_connected_bodies(picked_joint)
      data = picked_joint.get_attribute('MSPhysics Joint', 'Connected', '[]')
      begin
        connected = eval(data)
        raise unless connected.is_a?(Array)
      rescue Exception => e
        picked_joint.set_attribute('MSPhysics Joint', 'Connected', '[]')
        connected = []
      end
      bodies = []
      connected.each { |id|
        body = MSPhysics.get_entity_by_id(id)
        bodies << body if body
      }
      bodies
    end

    # Set body entities connected to the specified joint.
    # @param [Sketchup::ComponentInstance] picked_joint
    # @param [Sketchup::Group, Sketchup::ComponentInstance] connected_bodies
    def set_connected_bodies(picked_joint, connected_bodies)
      ids = []
      connected_bodies.each { |ent|
        ids << ent.entityID
      }
      picked_joint.set_attribute('MSPhysics Joint', 'Connected', ids.inspect)
    end

    # @!visibility private


    def activate
      msg = 'Click to select body or joint. Use CTRL/SHIFT to connect/disconnect.'
      Sketchup.set_status_text(msg, SB_PROMPT)
      @@instance = self
    end

    def deactivate(view)
      view.invalidate
      @@instance = nil
    end

    def onMouseEnter(view)
      refresh_viewport
    end

    def resume(view)
      @ctrl_down = false
      @shift_down = false
      refresh_viewport
    end

    def draw(view)
      return unless @picked
      type = MSPhysics.get_entity_type(@picked)
      color1 = type == 'Body' ? @body_color : @joint_color
      color2 = type == 'Body' ? @joint_color : @body_color
      # Draw bounding box of picked body from 12 edges.
      view.line_width = @line_width1
      view.drawing_color = color1
      edges = []
      e = @picked.respond_to?(:definition) ? @picked.definition : @picked.entities[0].parent
      bb = e.bounds
      @pins.each { |n|
        pt = bb.corner(n)
        pt.transform! @picked.transformation
        edges << pt
      }
      edges.each do |pt|
        pt.transform! @parent.transformation
      end if @parent
      view.draw(GL_LINES, edges)
      # Draw bounding box of connected bodies from 12 edges.
      view.line_width = @line_width2
      view.drawing_color = color2
      @connected.each { |ent|
        edges = []
        e = ent.respond_to?(:definition) ? ent.definition : ent.entities[0].parent
        bb = e.bounds
        @pins.each { |n|
          pt = bb.corner(n)
          pt.transform! ent.transformation
          edges << pt
        }
        parents = ent.parent.is_a?(Sketchup::Model) ? nil : ent.parent.instances
        unless parents
          view.draw(GL_LINES, edges)
          next
        end
        parents.each { |parent|
          pts = []
          edges.each { |pt| pts << pt.transform(parent.transformation) }
          view.draw(GL_LINES, pts)
        }
      }
    end

    def onSetCursor
      UI.set_cursor(@cursor_id)
    end

    def onKeyDown(key, rpt, flags, view)
      if key == COPY_MODIFIER_KEY && rpt == 1
        @ctrl_down = true
        @cursor_id = MSPhysics::CURSORS[:select_plus]
        refresh_viewport
      end
      if key == CONSTRAIN_MODIFIER_KEY && rpt == 1
        @shift_down = true
        @cursor_id = MSPhysics::CURSORS[:select_plus_minus]
        refresh_viewport
      end
    end

    def onKeyUp(key, rpt, flags, view)
      if key == COPY_MODIFIER_KEY
        @ctrl_down = false
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_plus_minus] : MSPhysics::CURSORS[:select]
        refresh_viewport
      end
      if key == CONSTRAIN_MODIFIER_KEY
        @shift_down = false
        @cursor_id = @ctrl_down ? MSPhysics::CURSORS[:select_plus] : MSPhysics::CURSORS[:select]
        refresh_viewport
      end
    end

    def onLButtonDown(flags, x, y, view)
      ph = view.pick_helper
      ph.do_pick(x,y)
      path = ph.path_at(0)
      if @ctrl_down or @shift_down
        return unless path
        return unless @picked
        case MSPhysics.get_entity_type(@picked)
        when 'Body'
          if (path[0] == @picked ||
              (path[0].respond_to?(:definition) && @picked.respond_to?(:definition) &&
              path[0].definition == @picked.definition))
            UI.beep
            return
          end
          to_connect = nil
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body'
            if MSPhysics.get_entity_type(path[1]) == 'Joint'
              to_connect = path[1]
            else
              UI.beep
              return
            end
          elsif type == 'Joint'
            to_connect = path[0]
          end
          if to_connect
            add_to_connected(to_connect)
            connected_bodies = get_connected_bodies(to_connect)
            if @connected.include?(to_connect)
              connected_bodies.push(@picked) unless connected_bodies.include?(@picked)
            else
              connected_bodies.delete(@picked)
            end
            set_connected_bodies(to_connect, connected_bodies)
          end
        when 'Joint'
          # Make sure user doesn't connect to self.
          if (path[0] == @picked ||
              (@picked.parent.is_a?(Sketchup::ComponentDefinition) && @picked.parent.instances.include?(path[0])) ||
              MSPhysics.get_entity_type(path[0]) != 'Body')
            UI.beep
            return
          end
          add_to_connected(path[0])
          set_connected_bodies(@picked, @connected)
        end
      else
        unless path
          @parent = nil
          @picked = nil
          @connected.clear
          refresh_viewport
          return
        end
        if [Sketchup::Group, Sketchup::ComponentInstance].include?(path[0].class)
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body'
            if MSPhysics.get_entity_type(path[1]) == 'Joint'
              @parent = path[0]
              @picked = path[1]
              @connected = get_connected_bodies(@picked)
            else
              @parent = nil
              @picked = path[0]
              @connected = get_connected_joints(@picked)
            end
          elsif type == 'Joint'
            @parent = nil
            @picked = path[0]
            @connected = get_connected_bodies(@picked)
          else
            @parent = nil
            @picked = nil
            @connected.clear
            refresh_viewport
            return
          end
        else
          @parent = nil
          @picked = nil
          @connected.clear
        end
      end
      refresh_viewport
    end

    def onLButtonDoubleClick(flags, x, y, view)
      onLButtonDown(flags, x, y, view)
    end

  end # class JointConnectionTool
end # module MSPhysics
