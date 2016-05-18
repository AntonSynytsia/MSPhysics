module MSPhysics

  # @since 1.0.0
  class JointConnectionTool

    # @!visibility private
    @@instance = nil

    class << self

      # Activate joint connection tool.
      # @return [Boolean] success
      def activate
        return false if @@instance
        Sketchup.active_model.select_tool(self.new)
        true
      end

      # Deactivate joint connection tool.
      # @return [Boolean] success
      def deactivate
        return false unless @@instance
        Sketchup.active_model.select_tool(nil)
        true
      end

      # Determine whether joint connection tool is active.
      # @return [Boolean]
      def is_active?
        @@instance ? true : false
      end


      # Get all connected and potentially connected joints.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Boolean] consider_world Whether to consider if entities have a
      #   body context.
      # @return [Array] An array of two elements. The first element contains an
      #   array of connected joints and their data. The second element contains
      #   an array of potentially connected joints and their data. Each joint
      #   data represents an array containing joint group, joint parent group,
      #   and joint transformation in global space.
      def get_connected_joints(body, consider_world = false)
        data = [[], []]
        sim_inst = MSPhysics::Simulation.instance
        ids = body.get_attribute('MSPhysics Body', 'Connected Joints')
        return data if (!ids.is_a?(Array) ||
          body.get_attribute('MSPhysics Body', 'Ignore', false) ||
          (consider_world && (sim_inst.nil? || sim_inst.get_body_by_group(body).nil?)))
        ids = ids.grep(Fixnum)
        if body.get_attribute('MSPhysics Body', 'Connect Closest Joints', MSPhysics::DEFAULT_BODY_SETTINGS[:connect_closest_joinst])
          bbsc = {}
          Sketchup.active_model.entities.each { |ent|
            next if ((!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) ||
              ent.get_attribute('MSPhysics', 'Type', 'Body') != 'Body')
            bbsc[ent] = MSPhysics::Group.get_bounding_box_from_faces(ent, true, ent.transformation) { |e|
              e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
            }.center
          }
          body_center = bbsc[body]
          Sketchup.active_model.entities.each { |ent|
            next if (!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) || ent == body
            type = ent.get_attribute('MSPhysics', 'Type', 'Body')
            if type == 'Body'
              next if (ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
                (consider_world && sim_inst.get_body_by_group(ent).nil?))
              ptra = ent.transformation
              cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
              cents.each { |cent|
                next if ((!cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)) ||
                  cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint')
                id = cent.get_attribute('MSPhysics Joint', 'ID')
                next unless ids.include?(id)
                jtra = ptra * MSPhysics::Geometry.extract_matrix_scale(cent.transformation)
                dist = jtra.origin.distance(body_center)
                dist = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist).to_f : dist.round(3)
                jconnected = true
                Sketchup.active_model.entities.each { |ent2|
                  next if ((!ent2.is_a?(Sketchup::Group) && !ent2.is_a?(Sketchup::ComponentInstance)) ||
                    ent2.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' ||
                    ent2.get_attribute('MSPhysics Body', 'Ignore', false) ||
                    ent2 == ent ||
                    ent2 == body ||
                    (consider_world && sim_inst.get_body_by_group(ent2).nil?))
                  ids2 = ent2.get_attribute('MSPhysics Body', 'Connected Joints')
                  if ids2.is_a?(Array) && ids2.include?(id)
                    dist2 = jtra.origin.distance(bbsc[ent2])
                    dist2 = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist2).to_f : dist2.round(3)
                    if dist2 < dist
                      jconnected = false
                      break
                    end
                  end
                }
                data[jconnected ? 0 : 1] << [cent, ent, jtra]
              }
            elsif type == 'Joint'
              id = ent.get_attribute('MSPhysics Joint', 'ID')
              next unless ids.include?(id)
              jtra = MSPhysics::Geometry.extract_matrix_scale(ent.transformation)
              dist = jtra.origin.distance(body_center)
              dist = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist).to_f : dist.round(3)
              jconnected = true
              Sketchup.active_model.entities.each { |ent2|
                next if ((!ent2.is_a?(Sketchup::Group) && !ent2.is_a?(Sketchup::ComponentInstance)) ||
                  ent2.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' ||
                  ent2.get_attribute('MSPhysics Body', 'Ignore', false) ||
                  ent2 == body ||
                  (consider_world && sim_inst.get_body_by_group(ent2).nil?))
                ids2 = ent2.get_attribute('MSPhysics Body', 'Connected Joints')
                if ids2.is_a?(Array) && ids2.include?(id)
                  dist2 = jtra.origin.distance(bbsc[ent2])
                  dist2 = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist2).to_f : dist2.round(3)
                  if dist2 < dist
                    jconnected = false
                    break
                  end
                end
              }
              data[jconnected ? 0 : 1] << [ent, nil, jtra]
            end
          }
        else
          Sketchup.active_model.entities.each { |ent|
            next if (!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) || ent == body
            type = ent.get_attribute('MSPhysics', 'Type', 'Body')
            if type == 'Body'
              next if (ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
                (consider_world && sim_inst.get_body_by_group(ent).nil?))
              ptra = ent.transformation
              cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
              cents.each { |cent|
                next if ((!cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)) ||
                  cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint' ||
                  !ids.include?(cent.get_attribute('MSPhysics Joint', 'ID')))
                jtra = ptra * MSPhysics::Geometry.extract_matrix_scale(cent.transformation)
                data[0] << [cent, ent, jtra]
              }
            elsif type == 'Joint'
              next unless ids.include?(ent.get_attribute('MSPhysics Joint', 'ID'))
              jtra = MSPhysics::Geometry.extract_matrix_scale(ent.transformation)
              data[0] << [ent, nil, jtra]
            end
          }
        end
        data
      end

      # Get all connected and potentially connected bodies.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jparent
      # @param [Boolean] consider_world Whether to consider if entities have a
      #   body context.
      # @return [Array] An array of two elements. The first element contains an
      #   array of connected bodies. The second element contains an array of
      #   potentially connected bodies.
      def get_connected_bodies(joint, jparent, consider_world = false)
        data = [[], []]
        sim_inst = MSPhysics::Simulation.instance
        return data if consider_world && sim_inst.nil?
        id = joint.get_attribute('MSPhysics Joint', 'ID', nil)
        return data unless id.is_a?(Fixnum)
        return data if (jparent &&
          (jparent.get_attribute('MSPhysics Body', 'Ignore', false) ||
          (consider_world && sim_inst.get_body_by_group(jparent).nil?)))
        bodies = {}
        jorigin = joint.transformation.origin
        jorigin.transform!(jparent.transformation) if jparent
        # Get all connected bodies.
        Sketchup.active_model.entities.each { |ent|
          next if ((!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) ||
            ent.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' ||
            ent == jparent ||
            ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
            (consider_world && sim_inst.get_body_by_group(ent).nil?))
          ids = ent.get_attribute('MSPhysics Body', 'Connected Joints')
          if ids.is_a?(Array) && ids.include?(id)
            bb = MSPhysics::Group.get_bounding_box_from_faces(ent, true, ent.transformation) { |e|
              e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
            }
            dist = jorigin.distance(bb.center)
            bodies[ent] = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist).to_f : dist.round(3)
          end
        }
        # Filter out closest bodies.
        bodies.each { |ent, dist|
          if ent.get_attribute('MSPhysics Body', 'Connect Closest Joints', MSPhysics::DEFAULT_BODY_SETTINGS[:connect_closest_joinst])
            found_closer = false
            bodies.each { |ent2, dist2|
              if ent2 != ent && dist2 < dist
                found_closer = true
                break
              end
            }
            data[found_closer ? 1 : 0] << ent
          else
            data[0] << ent
          end
        }
        data
      end

      # Get joint by its id.
      # @param [Fixnum] joint_id
      # @return [Array] An array of joint data. Each joint data represents an
      #   array of two elements. The first element of joint data is joint
      #   entity. The second element of joint data is joint parent entity.
      def get_joints_by_id(joint_id)
        data = []
        Sketchup.active_model.entities.each { |ent|
          next if !ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)
          type = ent.get_attribute('MSPhysics', 'Type', 'Body')
          if type == 'Body'
            next if ent.get_attribute('MSPhysics Body', 'Ignore', false)
            cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
            cents.each { |cent|
              next if !cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)
              next if cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint'
              id = cent.get_attribute('MSPhysics Joint', 'ID')
              data << [cent, ent] if id == joint_id
            }
          elsif type == 'Joint'
            id = ent.get_attribute('MSPhysics Joint', 'ID')
            data << [ent, nil] if id == joint_id
          end
        }
        data
      end

      # Get all joint ids connected to a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @return [Array<Fixnum>]
      def get_connected_joint_ids(body)
        ids = body.get_attribute('MSPhysics Body', 'Connected Joints')
        ids.is_a?(Array) ? ids.grep(Fixnum).uniq : []
      end

      # Set connected joint ids of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Array<Fixnum>] ids
      def set_connected_joint_ids(body, ids)
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
      end

      # Connect joint id to a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Fixnum] id
      # @return [Array<Fixnum>] The new joint ids of a group/component.
      def connect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids << id unless ids.include?(id)
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

      # Connect joint id from a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Fixnum] id
      # @return [Array<Fixnum>] The new joint ids of a group/component.
      def disconnect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids.delete(id)
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

      # Toggle connect joint id to a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Fixnum] id
      # @return [Array<Fixnum>] The new joint ids of a group/component.
      def toggle_connect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids.include?(id) ? ids.delete(id) : ids << id
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

    end # class << self

    def initialize
      model = Sketchup.active_model
      # Close active path
      state = true
      while state
        state = model.close_active
      end
      model.selection.clear
      @ctrl_down = false
      @shift_down = false
      @parent = nil
      @picked = nil
      @picked_type = nil
      @identical_picked_joints = []
      @connected = []
      @cursor_id = MSPhysics::CURSORS[:select]
      @color = {
        :picked     => Sketchup::Color.new(0, 0, 255),
        :identical  => Sketchup::Color.new(0, 0, 255),
        :connected  => Sketchup::Color.new(0, 225, 0),
        :potential  => Sketchup::Color.new(200, 40, 250)
      }
      @line_width = {
        :picked     => 2,
        :identical  => 2,
        :connected  => 2,
        :potential  => 2
      }
      @line_stipple = {
        :picked     => '',
        :identical  => '-',
        :connected  => '',
        :potential  => '_'
      }
      @pins = [0,1,1,3,3,2,2,0, 4,5,5,7,7,6,6,4, 0,4,1,5,2,6,3,7]
      @scale = 1.02
    end

    private

    def refresh_viewport
      Sketchup.active_model.active_view.invalidate
      onSetCursor
    end

    def toggle_attach_joint(body, joint)
      op = 'MSPhysics - Connecting Joint'
      model = Sketchup.active_model
      Sketchup.version.to_i > 6 ? model.start_operation(op, true) : model.start_operation(op)
      id = joint.get_attribute('MSPhysics Joint', 'ID', nil)
      if !id.is_a?(Fixnum)
        id = JointTool.generate_uniq_id
        joint.set_attribute('MSPhysics Joint', 'ID', id)
      end
      case @cursor_id
        when MSPhysics::CURSORS[:select_plus]
          JointConnectionTool.connect_joint_id(body, id)
        when MSPhysics::CURSORS[:select_minus]
          JointConnectionTool.disconnect_joint_id(body, id)
        when MSPhysics::CURSORS[:select_plus_minus]
          JointConnectionTool.toggle_connect_joint_id(body, id)
      end
      model.commit_operation
    end

    public
    # @!visibility private


    def activate
      msg = 'Click to select body or joint. Use CTRL, SHIFT, or both to connect/disconnect.   BLUE = Selected   GREEN = Connected   DASHED MAGENTA = Potentially Connected   DASHED BLUE = Secondary selected joints with same ID'
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
      if !@picked.valid? || (@parent && !@parent.valid?)
        @parent = nil
        @picked = nil
        @picked_type = nil
        @identical_picked_joints.clear
        @connected.clear
        return
      end
      # Draw picked
      view.line_width = @line_width[:picked]
      view.drawing_color = @color[:picked]
      view.line_stipple = @line_stipple[:picked]
      edges = []
      definition = @picked.respond_to?(:definition) ? @picked.definition : @picked.entities[0].parent
      bb = definition.bounds
      center = bb.center
      picked_tra = @picked.transformation
      @pins.each { |n|
        pt = bb.corner(n)
        v = center.vector_to(pt)
        for i in 0..2; v[i] *= @scale end
        pt = center + v
        pt.transform!(picked_tra)
        edges << pt
      }
      if @parent
        parent_tra = @parent.transformation
        edges.each { |pt| pt.transform!(parent_tra) }
      end
      view.draw(GL_LINES, edges)
      # Draw identical picked joints
      view.line_width = @line_width[:identical]
      view.drawing_color = @color[:identical]
      view.line_stipple = @line_stipple[:identical]
      @identical_picked_joints.each { |joint, jparent|
        next if joint.deleted? || (jparent && jparent.deleted?) || (joint == @picked && jparent == @parent)
        edges = []
        definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
        bb = definition.bounds
        center = bb.center
        joint_tra = joint.transformation
        @pins.each { |n|
          pt = bb.corner(n)
          v = center.vector_to(pt)
          for i in 0..2; v[i] *= @scale end
          pt = center + v
          pt.transform!(joint_tra)
          edges << pt
        }
        if jparent
          jparent_tra = jparent.transformation
          edges.each { |pt| pt.transform!(jparent_tra) }
        end
        view.draw(GL_LINES, edges)
      }
      # Draw connected and potentially connected
      if @picked_type == 'Body'
        # Draw connected joints
        view.line_width = @line_width[:connected]
        view.drawing_color = @color[:connected]
        view.line_stipple = @line_stipple[:connected]
        @connected[0].each { |joint, jparent, jtra|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          bb = definition.bounds
          center = bb.center
          joint_tra = joint.transformation
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(joint_tra)
            edges << pt
          }
          if jparent
            jparent_tra = jparent.transformation
            edges.each { |pt| pt.transform!(jparent_tra) }
          end
          view.draw(GL_LINES, edges)
        }
        # Draw potentially connected joints
        view.line_width = @line_width[:potential]
        view.drawing_color = @color[:potential]
        view.line_stipple = @line_stipple[:potential]
        @connected[1].each { |joint, jparent, jtra|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          bb = definition.bounds
          center = bb.center
          joint_tra = joint.transformation
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(joint_tra)
            edges << pt
          }
          if jparent
            jparent_tra = jparent.transformation
            edges.each { |pt| pt.transform!(jparent_tra) }
          end
          view.draw(GL_LINES, edges)
        }
      elsif @picked_type == 'Joint'
        # Draw connected bodies
        view.line_width = @line_width[:connected]
        view.drawing_color = @color[:connected]
        view.line_stipple = @line_stipple[:connected]
        @connected[0].each { |body|
          next if body.deleted?
          edges = []
          definition = body.respond_to?(:definition) ? body.definition : body.entities[0].parent
          bb = definition.bounds
          center = bb.center
          body_tra = body.transformation
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(body_tra)
            edges << pt
          }
          view.draw(GL_LINES, edges)
        }
        # Draw potentially connected bodies
        view.line_width = @line_width[:potential]
        view.drawing_color = @color[:potential]
        view.line_stipple = @line_stipple[:potential]
        @connected[1].each { |body|
          next if body.deleted?
          edges = []
          definition = body.respond_to?(:definition) ? body.definition : body.entities[0].parent
          bb = definition.bounds
          center = bb.center
          body_tra = body.transformation
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(body_tra)
            edges << pt
          }
          view.draw(GL_LINES, edges)
        }
      end
    end

    def onSetCursor
      UI.set_cursor(@cursor_id)
    end

    def onKeyDown(key, rpt, flags, view)
      return if rpt != 1
      if key == COPY_MODIFIER_KEY
        @ctrl_down = true
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_minus] : MSPhysics::CURSORS[:select_plus]
        refresh_viewport
      elsif key == CONSTRAIN_MODIFIER_KEY
        @shift_down = true
        @cursor_id = @ctrl_down ? MSPhysics::CURSORS[:select_minus] : MSPhysics::CURSORS[:select_plus_minus]
        refresh_viewport
      end
    end

    def onKeyUp(key, rpt, flags, view)
      if key == COPY_MODIFIER_KEY
        @ctrl_down = false
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_plus_minus] : MSPhysics::CURSORS[:select]
        refresh_viewport
      elsif key == CONSTRAIN_MODIFIER_KEY
        @shift_down = false
        @cursor_id = @ctrl_down ? MSPhysics::CURSORS[:select_plus] : MSPhysics::CURSORS[:select]
        refresh_viewport
      end
    end

    def onLButtonDown(flags, x, y, view)
      model = Sketchup.active_model
      ray = view.pickray(x,y)
      res = nil#model.raytest(ray)
      if res
        path = res[1]
      else
        ph = view.pick_helper
        ph.do_pick(x,y)
        path = ph.path_at(0)
      end
      if @ctrl_down || @shift_down
        return if path.nil? || @picked.nil?
        if !@picked.valid? || (@parent && !@parent.valid?)
          @parent = nil
          @picked = nil
          @picked_type = nil
          @identical_picked_joints.clear
          @connected.clear
          return
        end
        case @picked_type
        when 'Body'
          if (path[0] == @picked ||
              #(path[0].respond_to?(:definition) && @picked.respond_to?(:definition) && path[0].definition == @picked.definition) ||
              (MSPhysics.get_entity_type(path[0]) == 'Body' && path[0].get_attribute('MSPhysics Body', 'Ignore', false)))
            ::UI.beep
            return
          end
          to_connect = nil
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body'
            to_connect = path[1] if MSPhysics.get_entity_type(path[1]) == 'Joint'
          elsif type == 'Joint'
            to_connect = path[0]
          end
          if to_connect
            toggle_attach_joint(@picked, to_connect)
            @connected = JointConnectionTool.get_connected_joints(@picked)
          else
            ::UI.beep
          end
        when 'Joint'
          if (path[0] == @picked ||
              path[0] == @parent ||
              #(@picked.parent.is_a?(Sketchup::ComponentDefinition) && @picked.parent.instances.include?(path[0])) ||
              MSPhysics.get_entity_type(path[0]) != 'Body' ||
              path[0].get_attribute('MSPhysics Body', 'Ignore', false))
            ::UI.beep
            return
          end
          toggle_attach_joint(path[0], @picked)
          @connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
        end
      else
        @parent = nil
        @picked = nil
        @picked_type = nil
        @identical_picked_joints.clear
        @connected.clear
        if path && (path[0].is_a?(Sketchup::Group) || path[0].is_a?(Sketchup::ComponentInstance))
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body' && !path[0].get_attribute('MSPhysics Body', 'Ignore', false)
            if MSPhysics.get_entity_type(path[1]) == 'Joint'
              @parent = path[0]
              @picked = path[1]
              @picked_type = 'Joint'
              id = @picked.get_attribute('MSPhysics Joint', 'ID')
              if id.is_a?(Fixnum)
                @identical_picked_joints = JointConnectionTool.get_joints_by_id(id)
              end
              @connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
            else
              @parent = nil
              @picked = path[0]
              @picked_type = 'Body'
              @connected = JointConnectionTool.get_connected_joints(@picked)
            end
          elsif type == 'Joint'
            @parent = nil
            @picked = path[0]
            @picked_type = 'Joint'
            id = @picked.get_attribute('MSPhysics Joint', 'ID')
            if id.is_a?(Fixnum)
              @identical_picked_joints = JointConnectionTool.get_joints_by_id(id)
            end
            @connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
          else
            ::UI.beep
          end
        end
      end
      refresh_viewport
    end

    def onLButtonDoubleClick(flags, x, y, view)
      onLButtonDown(flags, x, y, view)
    end

  end # class JointConnectionTool
end # module MSPhysics
