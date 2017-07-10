module MSPhysics

  # @since 1.0.0
  class JointConnectionTool < Entity

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
      def active?
        @@instance ? true : false
      end

      # Get all connection data.
      # @param [Boolean] consider_world Whether to return
      #   <tt>MSPhysics::Body</tt> instances or <tt>Sketchup::Group</tt>/
      #   <tt>Sketchup::ComponentInstance</tt> instances in place of bodies.
      # @return [Array<(Hash, Hash)>] An array of two values.
      #   * The first element is a Hash of body data:
      #     <tt>{ body => [centre, connected_ids], ... }</tt>
      #   * The second element is a Hash of joint data:
      #     <tt>{ joint_id => [[joint_ent, joint_tra, parent_body], ...], ... }</tt>
      def get_connection_data(consider_world)
        if consider_world
          sim_inst = MSPhysics::Simulation.instance
          unless sim_inst
            raise(StandardError, "Simulation must be active in order to obtain body connection data!", caller)
          end
        end
        # Gather all bodies, joints, and their data.
        fbdata = {} # { body => [centre, connected_ids], ... }
        fjdata = {} # { joint_id => [[joint_ent, joint_tra, parent_body], ...], ... }
        Sketchup.active_model.entities.each { |ent|
          next if !ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)
          ent_type = ent.get_attribute('MSPhysics', 'Type', 'Body')
          if ent_type == 'Body'
            next if ent.get_attribute('MSPhysics Body', 'Ignore', false)
            if consider_world
              body = sim_inst.find_body_by_group(ent)
              next unless body
            else
              body = ent
            end
            connected_ids = ent.get_attribute('MSPhysics Body', 'Connected Joints')
            connected_ids = connected_ids.is_a?(Array) ? connected_ids.grep(Fixnum).uniq : []
            ptra = ent.transformation
            bb = AMS::Group.get_bounding_box_from_faces(ent, true, ptra, &MSPhysics::Collision::ENTITY_VALIDATION_PROC)
            fbdata[body] = [bb.center, connected_ids]
            cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
            cents.each { |cent|
              next if !cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)
              next if cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint'
              jtype = cent.get_attribute('MSPhysics Joint', 'Type')
              next if !jtype.is_a?(String) || !MSPhysics::JOINT_NAMES.include?(jtype)
              jid = cent.get_attribute('MSPhysics Joint', 'ID', nil)
              next unless jid.is_a?(Fixnum)
              jtra = ptra * cent.transformation
              next unless AMS::Geometry.is_matrix_uniform?(jtra)
              xaxis = jtra.xaxis
              xaxis.reverse! if AMS::Geometry.is_matrix_flipped?(jtra)
              jtra = Geom::Transformation.new(xaxis, jtra.yaxis, jtra.zaxis, jtra.origin)
              jdata = [cent, jtra, body]
              if fjdata.has_key?(jid)
                fjdata[jid] << jdata
              else
                fjdata[jid] = [jdata]
              end
            }
          elsif ent_type == 'Joint'
            jtype = ent.get_attribute('MSPhysics Joint', 'Type')
            next if !jtype.is_a?(String) || !MSPhysics::JOINT_NAMES.include?(jtype)
            jid = ent.get_attribute('MSPhysics Joint', 'ID', nil)
            next unless jid.is_a?(Fixnum)
            jtra = ent.transformation
            next unless AMS::Geometry.is_matrix_uniform?(jtra)
            xaxis = jtra.xaxis
            xaxis.reverse! if AMS::Geometry.is_matrix_flipped?(jtra)
            jtra = Geom::Transformation.new(xaxis, jtra.yaxis, jtra.zaxis, jtra.origin)
            jdata = [ent, jtra, nil]
            if fjdata.has_key?(jid)
              fjdata[jid] << jdata
            else
              fjdata[jid] = [jdata]
            end
          end
        }
        return [fbdata, fjdata]
      end

      # Get all joint connections from connection data.
      # @param [Hash] fbdata A hash of body data:
      #   <tt>{ body => [centre, connected_ids], ... }</tt>
      # @param [Hash] fjdata A hash of joint data:
      #   <tt>{ joint_id => [[joint_ent, joint_tra, parent_body], ...], ... }</tt>
      def get_conections_from_data(fbdata, fjdata)
        # Map all bodies with their connected joint IDs
        jid_to_connected_bodies = {} # { joint_id => { child_body => [centre, flag], ... }, ... }
        fbdata.each { |body, bdata|
          bdata[1].each { |jid|
            next unless fjdata.has_key?(jid)
            data = jid_to_connected_bodies[jid]
            unless data
              data = {}
              jid_to_connected_bodies[jid] = data
            end
            data[body] = [bdata[0], false]
          }
        }
        # Make connections
        connections = []
        jid_to_connected_bodies.each { |jid, connected_bodies|
          fjdata[jid].each { |jdata|
            closest_body = nil
            closest_dist = nil
            connected_bodies.each { |body, bdata|
              next if body == jdata[2]
              dist = bdata[0].distance(jdata[1].origin).to_f
              if closest_body.nil? || dist < closest_dist
                closest_body = body
                closest_dist = dist
              end
            }
            next unless closest_body
            connections << [jdata[0], jdata[1], closest_body, jdata[2], jid]
            connected_bodies[closest_body][1] = true
            pbdata = connected_bodies[jdata[2]]
            pbdata[1] = true if pbdata
          }
          connected_bodies.each { |body, bdata|
            next if bdata[1]
            closest_jdata = nil
            closest_dist = nil
            fjdata[jid].each { |jdata|
              next if body == jdata[2]
              dist = bdata[0].distance(jdata[1].origin).to_f
              if closest_jdata.nil? || dist < closest_dist
                closest_jdata = jdata
                closest_dist = dist
              end
            }
            next unless closest_jdata
            connections << [closest_jdata[0], closest_jdata[1], body, closest_jdata[2], jid]
          }
=begin
          connected_bodies.each { |body, bdata|
            next if bdata[1]
            fjdata[jid].each { |jdata|
              next if body == jdata[2]
              connections << [jdata[0], jdata[1], body, jdata[2], jid]
            }
          }
=end
        }
        # Return all connections
        connections
      end

      # Get all joint connections.
      # @param [Boolean] consider_world Whether to return
      #   <tt>MSPhysics::Body</tt> instances or <tt>Sketchup::Group</tt>/
      #   <tt>Sketchup::ComponentInstance</tt> instances in place of bodies.
      # @return [Array<Array>]
      #   <tt>[ [joint_ent, joint_tra, child_body, parent_body, joint_id], ... ]</tt>
      def get_all_connections(consider_world)
        data = get_connection_data(consider_world)
        get_conections_from_data(data[0], data[1])
      end

      # Get all joints connected to a body.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @return [Array<Array>]
      #   <tt>[ [joint_ent, joint_tra, joint_parent], ... ]</tt>
      def get_connected_joints(body)
        data = []
        get_all_connections(false).each { |jdata|
          if jdata[2] == body
            data << [jdata[0], jdata[1], jdata[3]]
          end
        }
        data
      end

      # Get all bodies connected to a joint.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] joint_parent
      # @return [Array] <tt>[ connected_inst, ... ]</tt>
      def get_connected_bodies(joint, joint_parent)
        data = []
        get_all_connections(false).each { |jdata|
          if jdata[0] == joint && jdata[3] == joint_parent
            data << jdata[2]
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
      # @note Manually wrap the operation.
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
      # @note Manually wrap the operation.
      def toggle_connect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids.include?(id) ? ids.delete(id) : ids << id
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

      # Get all points on a CurvySlider or a CurvyPiston joint.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] joint_parent
      # @return [Array<Geom::Point3d>] An array of points in global space.
      def get_points_on_curve(joint, joint_parent)
        # Find the closest edge
        closest_dist = nil
        closest_edge = nil
        AMS::Group.get_entities(joint).each { |e|
          next unless e.is_a?(Sketchup::Edge)
          dist1 = e.start.position.distance(ORIGIN)
          dist2 = e.end.position.distance(ORIGIN)
          dist = dist1 < dist2 ? dist1 : dist2
          if closest_dist.nil? || dist < closest_dist
            closest_dist = dist
            closest_edge = e
            break if closest_dist < 1.0e-8
          end
        }
        # Verify
        return Array.new() unless closest_edge
        # Preset data
        edges = { closest_edge => 0 }
        edges[closest_edge] = 0
        vertices = {}
        # Get all preceding edge vertices
        edge = closest_edge
        v = edge.start
        vertices[0] = v
        count = -1
        while true
          next_edge = v.edges[0]
          if next_edge == edge
            if v.edges[1]
              next_edge = v.edges[1]
            else
              break
            end
          end
          break if edges.has_key?(next_edge)
          v = next_edge.other_vertex(v)
          vertices[count] = v
          edges[next_edge] = count
          edge = next_edge
          count -= 1
        end
        # Get all consequent edge vertices
        edge = closest_edge
        v = edge.end
        vertices[1] = v
        count = 2
        while true
          next_edge = v.edges[0]
          if next_edge == edge
            if v.edges[1]
              next_edge = v.edges[1]
            else
              break
            end
          end
          break if edges.has_key?(next_edge)
          v = next_edge.other_vertex(v)
          vertices[count] = v
          edges[next_edge] = count
          edge = next_edge
          count += 1
        end
        # Return sorted vertices
        tra = joint.transformation
        if joint_parent
          tra = joint_parent.transformation * tra
        end
        vertices.keys.sort.map { |i| vertices[i].position.transform(tra) }
      end

      # Get curve length of a CurvySlider or a CurvyPiston joint.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] joint_parent
      # @param [Boolean] loop
      # @return [Numeric] Curve length in inches.
      def get_curve_length(joint, joint_parent, loop = false)
        length = 0.0
        last_pt = nil
        pts = get_points_on_curve(joint, joint_parent)
        pts.each { |pt|
          if last_pt
            length += last_pt.distance(pt)
          end
          last_pt = pt
        }
        if loop && last_pt
          length += pts[0].distance(last_pt)
        end
        length
      end

    end # class << self

    def initialize
      # Initialize variables
      @color = {
        :picked     => Sketchup::Color.new(0, 0, 255),
        :identical  => Sketchup::Color.new(0, 0, 255),
        :connected  => Sketchup::Color.new(0, 225, 0),
        :potential  => Sketchup::Color.new(200, 40, 250),
        :curve      => Sketchup::Color.new(250, 250, 15)
      }
      @line_width = {
        :picked     => 2,
        :identical  => 2,
        :connected  => 2,
        :potential  => 2,
        :curve      => 3
      }
      @line_stipple = {
        :picked     => '',
        :identical  => '-',
        :connected  => '',
        :potential  => '_',
        :curve      => ''
      }
      @text_opts = {
        :font => 'Ariel',
        :size => 11,
        :bold => false,
        :align => TextAlignLeft,
        :color => Sketchup::Color.new(255, 255, 255),
        :hratio => 0.6,
        :vratio => 1.5,
        :padding => 10
      }
      @note = {
        :bg_color => Sketchup::Color.new(0, 180, 255, 230),
        :time => nil,
        :duration => 20,
        :text => nil,
        :pos => Geom::Point3d.new(0,0,0),
        :min => Geom::Point3d.new(0,0,0),
        :max => Geom::Point3d.new(0,0,0),
      }
      @warn = {
        :time => nil,
        :duration => 3,
        :bg_color => Sketchup::Color.new(255, 10, 10, 200),
        :text => nil,
        :pos => Geom::Point3d.new(0,0,0),
        :min => Geom::Point3d.new(0,0,0),
        :max => Geom::Point3d.new(0,0,0),
      }
      @cursor_pos = Geom::Point3d.new(0,0,0)
      @pins = [0,1,1,3,3,2,2,0, 4,5,5,7,7,6,6,4, 0,4,1,5,2,6,3,7]
      @scale = 1.03
      @control_down = false
      @shift_down = false
      @cursor_id = MSPhysics::CURSORS[:select]
      @parent = nil
      @picked = nil
      @picked_type = nil
      @identical = []
      @connected = []
      @potential = []
      @fbdata = {}
      @fjdata = {}
      @all_connections = nil
    end

    private

    def reset_variables
      @parent = nil
      @picked = nil
      @picked_type = nil
      @identical.clear
      @connected.clear
      @potential.clear
    end

    def update_status_text
      msg = 'Click to select body or joint. Use CTRL, SHIFT, or both to connect/disconnect.    BLUE = Selected    GREEN = Connected    DASHED-BLUE = Identical Joints   DASHED-MAGENTA = Potentially Connected'
      Sketchup.set_status_text(msg, SB_PROMPT)
    end

    def refresh_view(view)
      onSetCursor
      update_status_text
      view.invalidate
    end

    def note(view, message)
      return if Sketchup.version.to_i < 14
      @note[:time] = Time.now
      @note[:text] = message
      sx = message.length * @text_opts[:size] * @text_opts[:hratio] + @text_opts[:padding] * 2
      sy = @text_opts[:size] * @text_opts[:vratio] + @text_opts[:padding] * 2
      @note[:min].x = (view.vpwidth - sx) / 2
      @note[:min].y = 0
      @note[:max].x = @note[:min].x + sx
      @note[:max].y = @note[:min].y + sy
      @note[:pos].x = @note[:min].x + @text_opts[:padding]
      @note[:pos].y = @note[:min].y + @text_opts[:padding]
    end

    def warn(view, message)
      if Sketchup.version.to_i < 14
        ::UI.beep
        return
      end
      @warn[:time] = Time.now
      @warn[:text] = message
      sx = message.length * @text_opts[:size] * @text_opts[:hratio] + @text_opts[:padding] * 2
      sy = @text_opts[:size] * @text_opts[:vratio] + @text_opts[:padding] * 2
      px = view.vpwidth - sx
      py = @cursor_pos.y - sy
      @warn[:min].x = px < @cursor_pos.x ? px : @cursor_pos.x
      @warn[:min].y = py < 0 ? 0 : py
      @warn[:max].x = @warn[:min].x + sx
      @warn[:max].y = @warn[:min].y + sy
      @warn[:pos].x = @warn[:min].x + @text_opts[:padding]
      @warn[:pos].y = @warn[:min].y + @text_opts[:padding]
      ::UI.beep
      view.invalidate
    end

    def update_cursor
      @control_down = AMS::Keyboard.control_down?
      @shift_down = AMS::Keyboard.shift_down?
      if @control_down && @shift_down
        @cursor_id = MSPhysics::CURSORS[:select_plus_minus]
      elsif @control_down
        @cursor_id = MSPhysics::CURSORS[:select_plus]
      elsif @shift_down
        @cursor_id = MSPhysics::CURSORS[:select_minus]
      else
        @cursor_id = MSPhysics::CURSORS[:select]
      end
    end

    def process_connection(body, joint)
      op = 'MSPhysics - Connecting Joint'
      model = Sketchup.active_model
      Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
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

    def draw_connections(view)
      return unless @picked
      if @picked.deleted? || (@parent && @parent.deleted?)
        reset_variables
        return
      end
      # Draw picked
      edges = []
      bcurvy_joint = false
      curvy_obj = nil
      if @picked_type == 'Joint'
        jtype = @picked.get_attribute('MSPhysics Joint', 'Type')
        if jtype == 'CurvySlider' || jtype == 'CurvyPiston'
          bcurvy_joint = true
          AMS::Group.get_entities(@picked).each { |e|
            if e.is_a?(Sketchup::ComponentInstance) && (e.definition.name == 'curvy_slider' || e.definition.name == 'curvy_piston')
              curvy_obj = e
              break
            end
          }
        end
      end
      if curvy_obj
        definition = curvy_obj.respond_to?(:definition) ? curvy_obj.definition : curvy_obj.entities[0].parent
      else
        definition = @picked.respond_to?(:definition) ? @picked.definition : @picked.entities[0].parent
      end
      bb = definition.bounds
      center = bb.center
      if curvy_obj
        picked_tra = @picked.transformation * curvy_obj.transformation
      else
        picked_tra = @picked.transformation
      end
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
      view.line_width = @line_width[:picked]
      view.drawing_color = @color[:picked]
      view.line_stipple = @line_stipple[:picked]
      view.draw(GL_LINES, edges)
      if bcurvy_joint
        view.line_width = @line_width[:curve]
        view.drawing_color = @color[:curve]
        view.line_stipple = @line_stipple[:curve]
        curve_pts = JointConnectionTool.get_points_on_curve(@picked, @parent)
        view.draw(GL_LINE_STRIP, curve_pts) if curve_pts.size > 1
      end
      # Draw identical picked joints
      view.line_width = @line_width[:identical]
      view.drawing_color = @color[:identical]
      view.line_stipple = @line_stipple[:identical]
      @identical.each { |joint, jparent|
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
        @connected.each { |joint, jparent|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          bcurvy_joint = false
          curvy_obj = nil
          jtype = joint.get_attribute('MSPhysics Joint', 'Type')
          if jtype == 'CurvySlider' || jtype == 'CurvyPiston'
            bcurvy_joint = true
            AMS::Group.get_entities(joint).each { |e|
              if e.is_a?(Sketchup::ComponentInstance) && (e.definition.name == 'curvy_slider' || e.definition.name == 'curvy_piston')
                curvy_obj = e
                break
              end
            }
          end
          if curvy_obj
            definition = curvy_obj.respond_to?(:definition) ? curvy_obj.definition : curvy_obj.entities[0].parent
          else
            definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          end
          bb = definition.bounds
          center = bb.center
          if curvy_obj
            joint_tra = joint.transformation * curvy_obj.transformation
          else
            joint_tra = joint.transformation
          end
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
          view.line_width = @line_width[:connected]
          view.drawing_color = @color[:connected]
          view.line_stipple = @line_stipple[:connected]
          view.draw(GL_LINES, edges)
          if bcurvy_joint
            view.line_width = @line_width[:curve]
            view.drawing_color = @color[:curve]
            view.line_stipple = @line_stipple[:curve]
            curve_pts = JointConnectionTool.get_points_on_curve(joint, jparent)
            view.draw(GL_LINE_STRIP, curve_pts) if curve_pts.size > 1
          end
        }
        # Draw potentially connected joints
        @potential.each { |joint, jparent, jtra|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          bcurvy_joint = false
          curvy_obj = nil
          type = joint.get_attribute('MSPhysics', 'Type', 'Body')
          if type == 'Joint'
            jtype = joint.get_attribute('MSPhysics Joint', 'Type')
            if jtype == 'CurvySlider' || jtype == 'CurvyPiston'
              bcurvy_joint = true
              AMS::Group.get_entities(joint).each { |e|
                if e.is_a?(Sketchup::ComponentInstance) && (e.definition.name == 'curvy_slider' || e.definition.name == 'curvy_piston')
                  curvy_obj = e
                  break
                end
              }
            end
          end
          if curvy_obj
            definition = curvy_obj.respond_to?(:definition) ? curvy_obj.definition : curvy_obj.entities[0].parent
          else
            definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          end
          bb = definition.bounds
          center = bb.center
          if curvy_obj
            joint_tra = joint.transformation * curvy_obj.transformation
          else
            joint_tra = joint.transformation
          end
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
          view.line_width = @line_width[:potential]
          view.drawing_color = @color[:potential]
          view.line_stipple = @line_stipple[:potential]
          view.draw(GL_LINES, edges)
          if bcurvy_joint
            view.line_width = @line_width[:curve]
            view.drawing_color = @color[:curve]
            view.line_stipple = @line_stipple[:curve]
            curve_pts = JointConnectionTool.get_points_on_curve(joint, jparent)
            view.draw(GL_LINE_STRIP, curve_pts) if curve_pts.size > 1
          end
        }
      else
        # Draw connected bodies
        view.line_width = @line_width[:connected]
        view.drawing_color = @color[:connected]
        view.line_stipple = @line_stipple[:connected]
        @connected.each { |body|
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
        @potential.each { |body|
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

    def draw_note(view)
      return unless @note[:text]
      if Time.now > @note[:time] + @note[:duration]
        @note[:text] = nil
        return
      end
      view.drawing_color = @note[:bg_color]
      pts = [@note[:min], Geom::Point3d.new(@note[:min].x, @note[:max].y, 0), @note[:max], Geom::Point3d.new(@note[:max].x, @note[:min].y, 0)]
      view.draw2d(GL_POLYGON, pts)
      view.draw_text(@note[:pos], @note[:text], @text_opts)
    end

    def draw_warn(view)
      return unless @warn[:text]
      if Time.now > @warn[:time] + @warn[:duration]
        @warn[:text] = nil
        return
      end
      view.drawing_color = @warn[:bg_color]
      pts = [@warn[:min], Geom::Point3d.new(@warn[:min].x, @warn[:max].y, 0), @warn[:max], Geom::Point3d.new(@warn[:max].x, @warn[:min].y, 0)]
      view.draw2d(GL_POLYGON, pts)
      view.draw_text(@warn[:pos], @warn[:text], @text_opts)
    end

    public
    # @!visibility private


    def activate
      model = Sketchup.active_model
      # Close active path
      if model.active_entities != model.entities
        state = true
        while state
          state = model.close_active
        end
      end
      # Clear selection
      model.selection.clear
      # Gather connection data
      data = JointConnectionTool.get_connection_data(false)
      @fbdata = data[0]
      @fjdata = data[1]
      @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
      @@instance = self
      update_status_text
    end

    def deactivate(view)
      view.invalidate
      @@instance = nil
      @fbdata.clear
      @fjdata.clear
      @all_connections.clear
      reset_variables
    end

    def onCancel(reason, view)
      reset_variables
      data = JointConnectionTool.get_connection_data(false)
      @fbdata = data[0]
      @fjdata = data[1]
      @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
    end

    def onMouseEnter(view)
      update_cursor
      refresh_view(view)
      unless @note[:text]
        note(view, "Select a group or a component instance, representing a body or a joint.")
      end
      view.model.selection.clear
    end

    def resume(view)
      @control_down = false
      @shift_down = false
      update_cursor
      refresh_view(view)
    end

    def onSetCursor
      ::UI.set_cursor(@cursor_id)
    end

    def onKeyDown(key, rpt, flags, view)
      return if rpt != 1
      if key == COPY_MODIFIER_KEY
        @control_down = true
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_minus] : MSPhysics::CURSORS[:select_plus]
        refresh_view(view)
      elsif key == CONSTRAIN_MODIFIER_KEY
        @shift_down = true
        @cursor_id = @control_down ? MSPhysics::CURSORS[:select_minus] : MSPhysics::CURSORS[:select_plus_minus]
        refresh_view(view)
      end
    end

    def onKeyUp(key, rpt, flags, view)
      if key == COPY_MODIFIER_KEY
        @control_down = false
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_plus_minus] : MSPhysics::CURSORS[:select]
        refresh_view(view)
      elsif key == CONSTRAIN_MODIFIER_KEY
        @shift_down = false
        @cursor_id = @control_down ? MSPhysics::CURSORS[:select_plus] : MSPhysics::CURSORS[:select]
        refresh_view(view)
      end
    end

    def onLButtonDown(flags, x, y, view)
      ph = view.pick_helper
      ph.do_pick(x,y)
      path = ph.path_at(0)
      if @picked && (@picked.deleted? || (@parent && @parent.deleted?))
        reset_variables
      end
      if @picked && (@control_down || @shift_down)
        if path && (path[0].is_a?(Sketchup::Group) || path[0].is_a?(Sketchup::ComponentInstance))
          if path[0] == @picked
            return warn(view, "Connecting to self is not allowed!")
          end
          ctype = path[0].get_attribute('MSPhysics', 'Type', 'Body')
          if @picked_type == 'Body'
            if ctype == 'Body'
              if path[0].get_attribute('MSPhysics Body', 'Ignore')
                return warn(view, "Connecting to an ignored body or to a joint within an ignored body is not allowed!")
              elsif path[1].get_attribute('MSPhysics', 'Type', 'Body') != 'Joint'
                return warn(view, "Interconnecting between bodies is not allowed!")
              end
              process_connection(@picked, path[1])
            elsif ctype == 'Joint'
              process_connection(@picked, path[0])
            else
              return warn(view, "Connecting to an instance of \"#{ctype}\" type is not allowed!")
            end
            connected_ids = JointConnectionTool.get_connected_joint_ids(@picked)
            if @fbdata.has_key?(@picked)
              @fbdata[@picked][1] = connected_ids
            end
            @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
            @connected.clear
            @potential.clear
            @all_connections.each { |jdata|
              if jdata[2] == @picked
                @connected << [jdata[0], jdata[3]]
              elsif connected_ids.include?(jdata[4]) && jdata[3] != @picked
                @potential << [jdata[0], jdata[3], jdata[1]]
              end
            }
          else
            if path[0] == @parent
              return warn(view, "Connecting to the parent body is not allowed!")
            elsif ctype == 'Joint'
              return warn(view, "Interconnecting between joints is not allowed!")
            elsif ctype != 'Body'
              return warn(view, "Connecting to an instance of \"#{ctype}\" type is not allowed!")
            end
            process_connection(path[0], @picked)
            connected_ids = JointConnectionTool.get_connected_joint_ids(path[0])
            id = @picked.get_attribute('MSPhysics Joint', 'ID')
            if @fbdata.has_key?(path[0])
              @fbdata[path[0]][1] = connected_ids
            end
            @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
            @connected.clear
            @potential.clear
            @all_connections.each { |jdata|
              if jdata[0] == @picked && jdata[3] == @parent
                @connected << jdata[2]
              elsif jdata[4] == id && jdata[2] != @parent
                @potential << jdata[2]
              end
            }
          end
        end
      else
        reset_variables
        if path && (path[0].is_a?(Sketchup::Group) || path[0].is_a?(Sketchup::ComponentInstance))
          type = path[0].get_attribute('MSPhysics', 'Type', 'Body')
          if type == 'Body'
            if path[0].get_attribute('MSPhysics Body', 'Ignore')
              return warn(view, "The body is ignored. Making connections with ignored bodies is not allowed!")
            elsif (path[1].is_a?(Sketchup::Group) || path[1].is_a?(Sketchup::ComponentInstance)) && path[1].get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
              @warn[:text] = nil
              note(view, "Hold CTRL / SHIFT or both and select a body to connect to / disconnect from.")
              @parent = path[0]
              @picked = path[1]
              @picked_type = 'Joint'
              id = @picked.get_attribute('MSPhysics Joint', 'ID')
              if id.is_a?(Fixnum)
                @all_connections.each { |jdata|
                  if jdata[0] == @picked && jdata[3] == @parent
                    @connected << jdata[2]
                  elsif jdata[4] == id
                    @identical << [jdata[0], jdata[3]]
                    @potential << jdata[2] if jdata[2] != @parent
                  end
                }
              end
            else
              @warn[:text] = nil
              note(view, "Hold CTRL / SHIFT or both and select a joint to connect to / disconnect from.")
              @parent = nil
              @picked = path[0]
              @picked_type = 'Body'
              connected_ids = JointConnectionTool.get_connected_joint_ids(@picked)
              @all_connections.each { |jdata|
                if jdata[2] == @picked
                  @connected << [jdata[0], jdata[3]]
                elsif connected_ids.include?(jdata[4]) && jdata[3] != @picked
                  @potential << [jdata[0], jdata[3], jdata[1]]
                end
              }
            end
          elsif type == 'Joint'
            @warn[:text] = nil
            note(view, "Hold CTRL / SHIFT or both and select a body to connect to / disconnect from.")
            @parent = nil
            @picked = path[0]
            @picked_type = 'Joint'
            id = @picked.get_attribute('MSPhysics Joint', 'ID')
            if id.is_a?(Fixnum)
              @all_connections.each { |jdata|
                if jdata[0] == @picked && jdata[3] == @parent
                  @connected << jdata[2]
                elsif jdata[4] == id
                  @identical << [jdata[0], jdata[3]]
                  @potential << jdata[2] if jdata[2] != @parent
                end
              }
            end
          end
        else
          note(view, "Select a group or a component instance, representing a body or a joint.")
        end
      end
      refresh_view(view)
    end

    def onLButtonDoubleClick(flags, x, y, view)
      onLButtonDown(flags, x, y, view)
    end

    def onMouseMove(flags, x, y, view)
      @cursor_pos.x = x
      @cursor_pos.y = y
      ref_view = false
      if @note[:text] && Time.now > @note[:time] + @note[:duration]
        @note[:text] = nil
        ref_view = true
      end
      if @warn[:text] && Time.now > @warn[:time] + @warn[:duration]
        @warn[:text] = nil
        ref_view = true
      end
      view.invalidate if ref_view
    end

    def getMenu(menu)
      model = Sketchup.active_model
      view = model.active_view
      ph = view.pick_helper
      ph.do_pick(@cursor_pos.x, @cursor_pos.y)
      path = ph.path_at(0)
      reset_variables
      @warn[:text] = nil
      if path && (path[0].is_a?(Sketchup::Group) || path[0].is_a?(Sketchup::ComponentInstance))
        type = path[0].get_attribute('MSPhysics', 'Type', 'Body')
        if type == 'Body'
          if path[0].get_attribute('MSPhysics Body', 'Ignore')
            model.selection.add(path[0])
            menu.add_item('Clear Ignore Flag') {
              op = 'MSPhysics - Clear Ignore Flag'
              Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
              path[0].delete_attribute('MSPhysics Body', 'Ignore')
              model.commit_operation
              data = JointConnectionTool.get_connection_data(false)
              @fbdata = data[0]
              @fjdata = data[1]
              @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
            }
          elsif (path[1].is_a?(Sketchup::Group) || path[1].is_a?(Sketchup::ComponentInstance)) && path[1].get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
            model.selection.add(path[1])
            id = path[1].get_attribute('MSPhysics Joint', 'ID')
            if id.is_a?(Fixnum)
              menu.add_item('Disconnect all Bodies') {
                op = 'MSPhysics - Disconnect all Bodies'
                Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
                @fbdata.each { |body, data|
                  if data[1].include?(id)
                    data[1].delete(id)
                    body.set_attribute('MSPhysics Body', 'Connected Joints', data[1])
                  end
                }
                model.commit_operation
                @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
              }
            end
          else
            model.selection.add(path[0])
            menu.add_item('Disconnect all Joints') {
              op = 'MSPhysics - Disconnect all Joints'
              Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
              path[0].delete_attribute('MSPhysics Body', 'Connected Joints')
              model.commit_operation
              if @fbdata.has_key?(path[0])
                @fbdata[path[0]][1].clear
                @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
              end
            }
          end
        elsif type == 'Joint'
          model.selection.add(path[0])
          id = path[0].get_attribute('MSPhysics Joint', 'ID')
          if id.is_a?(Fixnum)
            menu.add_item('Disconnect all Bodies') {
              op = 'MSPhysics - Disconnect all Bodies'
              Sketchup.version.to_i > 6 ? model.start_operation(op, true, false, false) : model.start_operation(op)
              @fbdata.each { |body, data|
                if data[1].include?(id)
                  data[1].delete(id)
                  body.set_attribute('MSPhysics Body', 'Connected Joints', data[1])
                end
              }
              model.commit_operation
              @all_connections = JointConnectionTool.get_conections_from_data(@fbdata, @fjdata)
            }
          end
        end
      end
      menu.add_item('Exit') {
        model.selection.clear
        model.select_tool(nil)
      }
    end

    def draw(view)
      draw_connections(view)
      draw_note(view)
      draw_warn(view)
    end

  end # class JointConnectionTool
end # module MSPhysics
