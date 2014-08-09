module MSPhysics
  class Simulation

    def initialize
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS
      @frame = 0
      @draw_queue = []
      @points_queue = []
      @points_queue2 = []
      @world_ptr = nil
      @solver_model = default[:solver_model]
      @friction_model = default[:friction_model]
      @gravity = default[:gravity]
      @mat_id = 0
      @thickness = default[:material_thickness]
      @bb = Geom::BoundingBox.new
      @mlt = nil
      @bodies = {}
      @emitted = {}
      @added_entities = {}
      @contact_data = {}
      @ray_data = []
      @ray_continue = 0
      @start_called = false
      @reset_called = false
      @show_bodies = true
      @animation = Animation.new
      @record_animation = false
      @ccm = false
      @collision = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :active         => Sketchup::Color.new(255,0,100),
        :sleeping       => Sketchup::Color.new(100,150,255),
        :kinematic      => Sketchup::Color.new(255,0,0),
        :deformable     => Sketchup::Color.new(0,0,255),
        :contact_face   => Sketchup::Color.new(255,255,0),
        :data           => {},
        :tree_data      => {}
      }
      @axis = {
        :show           => false,
        :line_width     => 2,
        :line_stipple   => '',
        :size           => 20,
        :xaxis          => Sketchup::Color.new(255,0,0),
        :yaxis          => Sketchup::Color.new(0,255,0),
        :zaxis          => Sketchup::Color.new(0,0,255)
      }
      @contacts = {
        :show           => false,
        :point_size     => 3,
        :point_style    => 2,
        :point_color    => Sketchup::Color.new(255,0,0),
        :line_size      => 5,
        :line_width     => 1,
        :line_stipple   => '',
        :line_color     => Sketchup::Color.new(0,0,255)
      }
      @forces = {
        :show           => false,
        :ratio          => 5,
        :line_color     => Sketchup::Color.new(0,80,255),
        :line_width     => 1,
        :line_stipple   => ''
      }
      @aabb = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :color          => Sketchup::Color.new(0,0,255)
      }
      @record = {
        :color          => Sketchup::Color.new(50,240,50,200),
        :radius         => 10,
        :origin         => [50,30],
        :flash_time     => 0.5,
        :flash          => true,
        :last_time      => Time.now
      }
      # The smaller the time step, the more accurate the simulation is!
      # Min: 1/1200.0; Max: 1/30.0; Normal: 1/60.
      @update_step = default[:update_timestep]
      @last_update_time = Time.now
      @change = 0
      # Update bodies' transformation every n frames.
      @update_rate = 1
      # Callbacks
      @gravity_callback = Proc.new { |body_ptr, time_step, thread_index|
        if @gravity != 0
          mass = 0.chr*4
          ixx  = 0.chr*4
          iyy  = 0.chr*4
          izz  = 0.chr*4
          Newton.bodyGetMassMatrix(body_ptr, mass, ixx, iyy, izz)
          mass = mass.unpack('F')[0]
          force = [0, 0, @gravity*mass]
          Newton.bodySetForce(body_ptr, force.pack('F*'))
        end
      }
      @force_callback = Proc.new { |body_ptr, time_step, thread_index|
        @gravity_callback.call(body_ptr, time_step, thread_index)
        body = get_body_by_body_ptr(body_ptr)
        next unless body
        # Process applied forces.
        data = body._applied_forces
        Newton.bodySetForce(body_ptr, data[:set_force]) if data[:set_force]
        Newton.bodySetTorque(body_ptr, data[:set_torque]) if data[:set_torque]
        Newton.bodyAddForce(body_ptr, data[:add_force].pack('F*')) if data[:add_force]
        Newton.bodyAddTorque(body_ptr, data[:add_torque].pack('F*')) if data[:add_torque]
        Newton.bodySetSleepState(body_ptr, data[:sleep]) if data[:sleep]
        data.clear
      }
      @aabb_overlap_callback = Proc.new { |material, body_ptr0, body_ptr1, thread_index|
        body0 = get_body_by_body_ptr(body_ptr0)
        body1 = get_body_by_body_ptr(body_ptr1)
        # Verify the existence of both Body objects.
        next 1 unless body0 and body1
        # Skip collision if one of the bodies is not collidable.
        next 0 unless body0.collidable?
        next 0 unless body1.collidable?
        # Skip collision if both bodies are frozen.
        next 0 if body0.frozen? and body1.frozen?
        # Skip collision if one of the bodies is set non-collidable with another.
        next 0 unless body0.collidable_with?(body1)
        next 0 unless body1.collidable_with?(body0)
        1
      }
      @contacts_callback = Proc.new { |contact_joint, time_step, thread_index|
        body_ptr0 = Newton.jointGetBody0(contact_joint)
        body_ptr1 = Newton.jointGetBody1(contact_joint)
        body0 = get_body_by_body_ptr(body_ptr0)
        body1 = get_body_by_body_ptr(body_ptr1)
        # Verify the existence of both Body objects.
        next unless body0 and body1
        # Unfreeze bodies unless both of them are frozen.
        next if body0.frozen? and body1.frozen?
        body0.frozen = false if body0.frozen?
        body1.frozen = false if body1.frozen?
        # Update contacts.
        contact = Newton.contactJointGetFirstContact(contact_joint)
        while !contact.null?
          mat = Newton.contactGetMaterial(contact)
          if body0.friction_enabled? and body1.friction_enabled?
            sfc = (body0.get_static_friction + body1.get_static_friction)/2.0
            kfc = (body0.get_dynamic_friction + body1.get_dynamic_friction)/2.0
            Newton.materialSetContactFrictionCoef(mat, sfc, kfc, 0)
            Newton.materialSetContactFrictionCoef(mat, sfc, kfc, 1)
          else
            Newton.materialSetContactFrictionState(mat, 0, 0)
            Newton.materialSetContactFrictionState(mat, 0, 1)
          end
          cor = (body0.get_elasticity + body1.get_elasticity)/2.0
          sft = (body0.get_softness + body1.get_softness)/2.0
          Newton.materialSetContactElasticity(mat, cor)
          Newton.materialSetContactSoftness(mat, sft)
          contact = Newton.contactJointGetNextContact(contact_joint, contact)
        end
      }
      @tree_collision_callback = Proc.new { |static_body_ptr, body_ptr, face_id, count, vertices, stride|
        next unless @collision[:show]
        cloud = vertices.get_array_of_float(0, count*3)
        loc = static_body_ptr.address
        @collision[:tree_data][loc] ||= []
        @collision[:tree_data][loc] << cloud unless @collision[:tree_data][loc].include?(cloud)
      }
      @ray_prefilter_callback = Proc.new { |body_ptr, col_ptr, user_data|
      }
      @ray_filter_callback = Proc.new { |body_ptr, col_ptr, point, normal, col_id, user_data, intersect_param|
        body = get_body_by_body_ptr(body_ptr)
        next unless body
        point = Conversion.convert_point(point.get_array_of_float(0,3), :m, :in)
        normal = Geom::Vector3d.new(normal.get_array_of_float(0,3))
        @ray_data << Hit.new(body, point, normal)
        @ray_continue
      }
    end

    # @!attribute [r] world_ptr
    #   @return [AMS::FFI::Pointer, NilClass]

    # @!attribute [r] animation
    #   @return [Animation]

    # @!attribute [r] bb
    #   @return [Geom::BoundingBox]

    # @!attribute [r] gravity_callback
    #   @return [Proc]

    # @!attribute [r] frame
    #   @return [Fixnum]

    # @!attribute [r] change
    #   Get simulation nextFrame update change in milliseconds.
    #   @return [Fixnum]


    attr_reader :world_ptr, :animation, :bb, :gravity_callback, :frame, :change

    # @!visibility private
    def on_body_added(body)
      return if body._world_ptr != @world_ptr
      return unless body.is_a?(BodyContext)
      @bodies[body._body_ptr.address] = body
      Newton.bodySetForceAndTorqueCallback(body._body_ptr, @force_callback)
    end

    # @!visibility private
    def on_body_removed(body)
      return if body._world_ptr != @world_ptr
      return unless body.is_a?(BodyContext)
      @bodies.delete(body._body_ptr.address)
      @emitted.delete(body)
      @collision[:data].delete(body._body_ptr.address)
    end

    # @!visibility private
    def call_event(evt, *args)
      @bodies.values.each{ |body|
        next unless body.valid?
        body.call_event(evt, *args)
        return if (@reset_called and evt != :onEnd)
      }
    end

    private

    def get_collisions
      return false unless @collision[:show]
      @bodies.each{ |address, body|
        next if body.get_shape == :static_mesh
        data = @collision[:data][address]
        tra = body.entity.transformation
        next if data and tra.to_a == data[0].to_a
        data = [tra, body.get_collision_faces]
        @collision[:data][address] = data
      }
      true
    end

    def draw_queues(view)
      @draw_queue.each{ |type, points, color, width, stipple, mode|
        view.drawing_color = color
        view.line_width = width
        view.line_stipple = stipple
        if mode == 1
          @bb.add(points)
          view.draw(type, points)
        else
          view.draw2d(type, points)
        end
      }
      @points_queue.each{ |points, size, style, color, width, stipple|
        @bb.add(points)
        view.line_width = width
        view.line_stipple = stipple
        view.draw_points(points, size, style, color)
      }
      @points_queue2.each{ |points, size, style, color, width, stipple|
        points.reject! { |pt|
          vec = view.camera.eye.vector_to(pt)
          next false if vec.length.zero?
          next true if vec.angle_between(view.camera.direction) >= 90.degrees
          result = AMS::RayUtil.raytest_t3(view.camera.eye, vec)
          next false unless result
          view.camera.eye.distance(result[0]) < vec.length
        }
        next if points.empty?
        view.line_width = width
        view.line_stipple = stipple
        @bb.add(points)
        view.draw_points(points, size, style, color)
      }
    end

    def draw_collision(view)
      return unless @collision[:show]
      view.line_width = @collision[:line_width]
      view.line_stipple = @collision[:line_stipple]
      @collision[:data].each{ |address, data|
        body = get_body_by_body_ptr(address)
        next unless body
        color = case body.get_type
          when :dynamic
            sleeping = body.get_sleep_mode
            sleeping ? @collision[:sleeping] : @collision[:active]
          when :kinematic
            @collision[:kinematic]
          when :deformable
            @collision[:deformable]
          else
            @collision[:active]
        end
        view.drawing_color = color
        data[1].each { |face|
          view.draw(GL_LINE_LOOP, face)
        }
      }
      view.drawing_color = @collision[:contact_face]
      @collision[:tree_data].each { |address, clouds|
        clouds.each { |cloud|
          face = []
          for i in 0...cloud.size/3
            x = cloud[i*3+0].m
            y = cloud[i*3+1].m
            z = cloud[i*3+2].m
            face << [x,y,z]
          end
          view.draw(GL_LINE_LOOP, face)
        }
      }
    end

    def draw_axis(view)
      return unless @axis[:show]
      view.line_width = @axis[:line_width]
      view.line_stipple = @axis[:line_stipple]
      @bodies.values.each { |body|
        c = body.get_centre_of_mass
        tra = body.get_matrix
        c_glob = c.transform(tra)
        s = @axis[:size]
        # xaxis
        pt = [c.x+s, c.y, c.z].transform!(tra)
        view.drawing_color = @axis[:xaxis]
        view.draw_line(c_glob, pt)
        # yaxis
        pt = [c.x, c.y+s, c.z].transform!(tra)
        view.drawing_color = @axis[:yaxis]
        view.draw_line(c_glob, pt)
        # zaxis
        pt = [c.x, c.y, c.z+s].transform!(tra)
        view.drawing_color = @axis[:zaxis]
        view.draw_line(c_glob, pt)
      }
    end

    def draw_contacts(view)
      return unless @contacts[:show]
      points = []
      #lines = []
      @bodies.values.each{ |body|
        next if body.static?
        body.get_contacts.each { |contact|
          points << contact.position.to_a
          #v = contact.normal
          #v.length = @contacts[:line_size]
          #lines << contact.position
          #lines << contact.position + v
        }
      }
      points.uniq!
      return if points.empty?
      view.draw_points(points, @contacts[:point_size], @contacts[:point_style], @contacts[:point_color])
      #view.drawing_color = @contacts[:line_color]
      #view.line_width = @contacts[:line_width]
      #view.line_stipple = @contacts[:line_stipple]
      #view.draw(GL_LINES, lines)
    end

    def draw_forces(view)
      return unless @forces[:show]
      lines = []
      @bodies.values.each{ |body|
        next if body.static?
        mass = body.get_mass.to_f
        body.get_contacts.each { |contact|
          pos = contact.position
          force = contact.force
          next if force.length == 0
          force.length *= @forces[:ratio]/mass
          lines << pos
          lines << pos + force
        }
      }
      return if lines.empty?
      view.drawing_color = @forces[:line_color]
      view.line_width = @forces[:line_width]
      view.line_stipple = @forces[:line_stipple]
      view.draw(GL_LINES, lines)
    end

    def draw_bounding_box(view)
      return unless @aabb[:show]
      faces = []
      lines = []
      @bodies.values.each { |body|
        aabb = body.get_bounding_box
        min = aabb.min
        max = aabb.max
        # Top and bottom faces
        faces << [min, [min.x, max.y, min.z], [max.x, max.y, min.z], [max.x, min.y, min.z]]
        faces << [[min.x, min.y, max.z], [min.x, max.y, max.z], max, [max.x, min.y, max.z]]
        # Four vertical lines
        lines.concat [min, [min.x, min.y, max.z]]
        lines.concat [[min.x, max.y, min.z], [min.x, max.y, max.z]]
        lines.concat [[max.x, max.y, min.z], max]
        lines.concat [[max.x, min.y, min.z], [max.x, min.y, max.z]]
      }
      return if faces.empty?
      view.drawing_color = @aabb[:color]
      view.line_width = @aabb[:line_width]
      view.line_stipple = ''
      faces.each { |face|
        view.draw(GL_LINE_LOOP, face)
      }
      view.draw(GL_LINES, lines)
    end

    def draw_record(view)
      return unless @record_animation
      if Time.now-@record[:last_time] > @record[:flash_time]
        @record[:flash] = !@record[:flash]
        @record[:last_time] = Time.now
      end
      return unless @record[:flash]
      x = view.vpwidth - @record[:origin][0]
      y = @record[:origin][1]
      pts = MSPhysics.points_on_circle2d([x,y], @record[:radius], 12, 0)
      view.drawing_color = @record[:color]
      view.draw2d(GL_POLYGON, pts)
    end

    def clear_drawing_queues
      @draw_queue.clear
      @points_queue.clear
      @points_queue2.clear
    end

    def handle_operation(message, &block)
      begin
        block.call
      rescue Exception => e
        puts "#{message.to_s}\n#{e}\n#{e.backtrace.first}"
      end
    end

    public

    # Add a group/component to the simulation.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Body, NilClass] A body object (if successful).
    def add_entity(entity)
      return unless entity.is_a?(Sketchup::Group) or entity.is_a?(Sketchup::ComponentInstance)
      return if entity.deleted?
      return if get_body_by_entity(entity)
      handle = 'MSPhysics Body'
      return if entity.get_attribute(handle, 'Ignore')
      default = MSPhysics::DEFAULT_BODY_SETTINGS
      type = :dynamic
      shape = entity.get_attribute(handle, 'Shape', default[:shape])
      name = entity.get_attribute(handle, 'Material', default[:material_name])
      density = entity.get_attribute(handle, 'Density', default[:density])
      static_friction = entity.get_attribute(handle, 'Static Friction', default[:static_friction])
      dynamic_friction = entity.get_attribute(handle, 'Dynamic Friction', default[:dynamic_friction])
      elasticity = entity.get_attribute(handle, 'Elasticity', default[:elasticity])
      softness = entity.get_attribute(handle, 'Softness', default[:softness])
      mat = Material.new(name, density, static_friction, dynamic_friction, elasticity, softness)
      begin
        body = BodyContext.new(@world_ptr, entity, type, shape, mat)
      rescue Exception => e
        index = Sketchup.active_model.entities.to_a.index(entity)
        puts "Entity at index [#{index}] has an invalid collision shape! It was not added to simulation."
        return
      end
      body.friction_enabled = entity.get_attribute(handle, 'Enable Friction', default[:enable_friction])
      body.set_magnet_force( entity.get_attribute(handle, 'Magnet Force', default[:magnet_force]) )
      body.set_magnet_range( entity.get_attribute(handle, 'Magnet Range', default[:magnet_range]) )
      if body.get_shape == :static_mesh
        Newton.staticCollisionSetDebugCallback(body._collision_ptr, @tree_collision_callback)
      end
      if entity.get_attribute(handle, 'Static')
        body.static = true
      end
      if entity.get_attribute(handle, 'Frozen')
        body.frozen = true
      end
      if entity.get_attribute(handle, 'Magnetic', default[:magnetic])
        body.magnetic = true
      end
      if entity.get_attribute(handle, 'Not Collidable')
        body.collidable = false
      end
      if entity.get_attribute(handle, 'Enable Script', true)
        script = entity.get_attribute('MSPhysics Script', 'Value', '')
        body.set_script(script)
      end
      return unless body.valid?
      @added_entities[entity.entityID] = entity.transformation
      body
    end

    # Remove a group/component from the simulation.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @param [Boolean] erase_ent Whether to erase the entity.
    # @return [Boolean] Whether the entity was removed.
    def remove_entity(entity, erase_ent = true)
      body = get_body_by_entity(entity)
      return false unless body
      body.destroy(erase_ent)
      true
    end

    # @!visibility private
    def do_on_start
      return false if @start_called
      @start_called = true
      model = Sketchup.active_model
      # Initialize multi line text
      @mlt = AMS::MultiLineText.new(10,10)
      @mlt.limit = 10
      @mlt.display_lines = false
      mat = model.materials.add('MultiLineText')
      mat.color = [0,0,220]
      @mlt.ent.material = mat
      # Add body observer
      BodyObserver.add_observer(self)
      # Initialize Newton
      @world_ptr = Newton.create
      Newton.invalidateCache(@world_ptr)
      Newton.setSolverModel(@world_ptr, @solver_model)
      Newton.setFrictionModel(@world_ptr, @friction_model)
      # Set default material
      default = MSPhysics::DEFAULT_BODY_SETTINGS
      @mat_id = Newton.materialGetDefaultGroupID(@world_ptr)
      Newton.materialSetCollisionCallback(@world_ptr, @mat_id, @mat_id, nil, @aabb_overlap_callback, @contacts_callback)
      Newton.materialSetSurfaceThickness(@world_ptr, @mat_id, @mat_id, @thickness)
      Newton.materialSetDefaultFriction(@world_ptr, @mat_id, @mat_id, default[:static_friction], default[:dynamic_friction])
      Newton.materialSetDefaultElasticity(@world_ptr, @mat_id, @mat_id, default[:elasticity])
      Newton.materialSetDefaultSoftness(@world_ptr, @mat_id, @mat_id, default[:softness])
      # Create Bodies
      ents = []
      #~ Create from selection (if any)
      #~ model.selection.each { |e|
      #~  ents << e if MSPhysics.get_entity_type(e) == 'Body'
      #~ }
      ents = model.entities if ents.empty?
      ents.to_a.dup.each { |ent|
        next if MSPhysics.get_entity_type(ent) != 'Body'
        begin
          add_entity(ent)
        rescue Exception => e
          raise "An error occurred while starting simulation!\n#{e}"
        end
        return false if @reset_called
      }
      MSPhysics::Settings.apply_settings
      call_event(:onStart)
      true
    end

    # @!visibility private
    def do_on_end
      return false if @reset_called
      @reset_called = true
      # Call onEnd procedure.
      begin
        call_event(:onEnd)
        on_end_error = nil
      rescue Exception => on_end_error
        # Wait till all data is reset.
      end
      # Reset data.
      message = "An error occurred while resetting simulation data:"
      handle_operation(message){ ControllerContext.clear_variables }
      handle_operation(message){ Collision.reset_data }
      handle_operation(message){ Joint.destroy_all }
      handle_operation(message){ CustomCloth.destroy_all }
      handle_operation(message){ Particle.destroy_all }
      # Destroy all emitted bodies.
      @emitted.keys.each { |body| body.destroy(true) }
      # Destroy world.
      Newton.destroy(@world_ptr)
      @world_ptr = nil
      # Remove body observer.
      BodyObserver.remove_observer(self)
      # Erase multi-line text.
      @mlt.remove
      # Reset rendering options if debug collision was enabled.
      self.collision_visible = false
      # Clear variables just to ensure that garbage collection is working.
      @draw_queue.clear
      @points_queue.clear
      @points_queue2.clear
      @bb.clear
      @bodies.clear
      @emitted.clear
      @contact_data.clear
      @ray_data.clear
      # Now, safely throw the on_end_error if there was any.
      raise on_end_error if on_end_error
      true
    end

    # @!visibility private
    def do_on_end_post_operation
      # Reposition entities.
      @added_entities.each { |id, tra|
        e = MSPhysics.get_entity_by_id(id)
        next unless e
        e.move! tra if e.valid?
      }
      @added_entities.clear
    end

    # @!visibility private
    def do_on_update(frame)
      @frame = frame
      # Clear drawing queues.
      @collision[:tree_data].clear
      clear_drawing_queues
      # Trigger onPreUpdate event.
      call_event(:onPreUpdate)
      return if @reset_called
      # Update Newton world.
      Newton.update(@world_ptr, @update_step)
      @change = ((Time.now - @last_update_time)*1000).round
      #if @update_step > 1/120.0 and @change < 20
      #  2.times { Newton.update(@world_ptr, @update_step*0.5) }
      #else
      #  Newton.update(@world_ptr, @update_step)
      #end
      @last_update_time = Time.now
      # Update particular joints
      Fixed::TO_DISCONNECT.each { |joint|
        joint.disconnect
      }
      Fixed::TO_DISCONNECT.clear
      # Update up vector joints
      @bodies.values.each { |body|
        next unless body._up_vector
        dir = body.get_position(1).vector_to(body._up_vector[1]).normalize
        Newton.upVectorSetPin(body._up_vector[0], dir.to_a.pack('FFF'))
      }
      # Process emitted bodies.
      @emitted.reject! { |body, life_end|
        next false if @frame < life_end
        body.destroy(true)
        true
      }
      # Process magnets.
      @bodies.values.each { |body|
        if body.get_magnet_force != 0 and body.get_magnet_range > 0
          pos = body.get_position(1)
          @bodies.values.each { |other_body|
            next unless other_body.magnetic?
            next if other_body == body
            dir = other_body.get_position(1).vector_to(pos)
            mag = dir.length.to_m
            next if mag.zero? or mag >= body.get_magnet_range
            dir.length = (body.get_magnet_range - mag) * body.get_magnet_force / body.get_magnet_range.to_f
            other_body.add_force(dir)
            # For every action there is an equal and opposite reaction!
            body.add_force(dir.reverse)
          }
        end
      }
      # Update entities' transformation and record animation.
      if @frame % @update_rate == 0
        @bodies.values.each{ |body|
          next if body.get_sleep_mode
          body.entity.move! body.get_matrix
          @animation.push_record(body.entity, @frame) if @record_animation
        }
        # Update cloth
        CustomCloth.update_all
        # Update particles
        Particle.update_all
      end
      # Trigger onTouch, onTouching, and onUntouch events.
      @bodies.values.each { |body|
        next unless body.valid?
        next unless body.proc_assigned?(:onTouch) || body.proc_assigned?(:onTouching) || body.proc_assigned?(:onUntouch)
        @contact_data[body] ||= {}
        # In Newton 3.x contacts are not generated for the non-collidable
        # bodies. To determine whether the body is touching with another body we
        # have to check collision intersections manually using
        # NewtonCollisionCollide function.
        if (!body.collidable?) or body.get_noncollidable_bodies.size > 0
          colA = body._collision_ptr
          matA = 0.chr*64
          Newton.bodyGetMatrix(body._body_ptr, matA)
          @bodies.values.each { |other_body|
            next unless body.valid?
            next if other_body == body
            next unless Body.bodies_aabb_overlap?(body, other_body)
            colB = other_body._collision_ptr
            matB = 0.chr*64
            Newton.bodyGetMatrix(other_body._body_ptr, matB)
            buf1 = 0.chr*12
            buf2 = 0.chr*12
            buf3 = 0.chr*12
            attrA = 0.chr*4
            attrB = 0.chr*4
            count = Newton.collisionCollide(@world_ptr, 1, colA, matA, colB, matB, buf1, buf2, buf3, attrA, attrB, 0)
            next if count == 0
            origin = Conversion.convert_point(buf1.unpack('F*'), :m, :in)
            normal = Geom::Vector3d.new(buf2.unpack('F*'))
            force = Geom::Vector3d.new(0,0,0)
            speed = 0
            evt = @contact_data[body][other_body] ? :onTouching : :onTouch
            body.call_event(evt, other_body, origin, normal, force, speed)
            @contact_data[body][other_body] = @frame
          }
          next
        end
        # Otherwise, if body is collidable we irritate trough all the body
        # contact joints to get touch data.
        joint = Newton.bodyGetFirstContactJoint(body._body_ptr)
        while !joint.null?
          toucher_ptr = Newton.jointGetBody0(joint)
          if body._body_ptr.address == toucher_ptr.address
            toucher_ptr = Newton.jointGetBody1(joint)
          end
          toucher = get_body_by_body_ptr(toucher_ptr)
          if toucher
            contact = Newton.contactJointGetFirstContact(joint)
            mat = Newton.contactGetMaterial(contact)
            buf1 = 0.chr*12
            buf2 = 0.chr*12
            Newton.materialGetContactPositionAndNormal(mat, toucher_ptr, buf1, buf2)
            origin = Conversion.convert_point(buf1.unpack('F*'), :m, :in)
            normal = Geom::Vector3d.new(buf2.unpack('F*'))
            buf3 = 0.chr*12
            Newton.materialGetContactForce(mat, toucher_ptr, buf3)
            force = Geom::Vector3d.new(buf3.unpack('F*'))
            speed = Newton.materialGetContactNormalSpeed(mat)
            evt = @contact_data[body][toucher] ? :onTouching : :onTouch
            body.call_event(evt, toucher, origin, normal, force, speed)
            @contact_data[body][toucher] = @frame
          end
          joint = Newton.bodyGetNextContactJoint(body._body_ptr, joint)
        end
      }
      @contact_data.dup.each { |body, touchers|
        unless body.valid?
          @contact_data.delete(body)
          next
        end
        touchers.each { |toucher, tframe|
          unless toucher.valid?
            touchers.delete(toucher)
            next
          end
          if @frame > tframe
            body.call_event(:onUntouch, toucher)
            touchers.delete(toucher)
          end
        }
      }
      # Trigger onUpdate and onPostUpdate events.
      call_event(:onUpdate)
      return if @reset_called
      call_event(:onPostUpdate)
      return if @reset_called
      # Record collision wire-frame.
      get_collisions
    end

    # @!visibility private
    def do_on_draw(view)
      draw_queues(view)
      draw_collision(view)
      draw_axis(view)
      draw_bounding_box(view)
      draw_contacts(view)
      draw_forces(view)
      draw_record(view)
      Particle.draw_all(view)
      view.drawing_color = 'black'
      view.line_width = 1
      view.line_stipple = ''
      call_event(:onDraw, view, @bb)
    end

    # Draw with OpenGL.
    # @param [Fixnum, String, Symbol] type Drawing type. Valid types are <i>
    #   line, lines, line_strip, line_loop, triangle, triangles, triangle_strip,
    #   triangle_fan, quad, quads, quad_strip, convex_polygon, and polygon</i>.
    # @param [Array<Array[Numeric]>, Array<Geom::Point3d>] points An array of
    #   points.
    # @param [Array, String, Sketchup::Color] color
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    # @param [Boolean] mode Drawing mode: +0+ : 2d, +1+ : 3d.
    def draw(type, points, color = 'black', width = 1, stipple = '', mode = 1)
      raise ArgumentError, 'Expected an array of points.' unless points.is_a?(Array) or points.is_a?(Geom::Point3d)
      points = [points] if points[0].is_a?(Numeric)
      s = points.size
      raise 'Not enough points: At least one required!' if s == 0
      type = case type.to_s.downcase.strip.gsub(/\s/i, '_').to_sym
        when :point, :points
          GL_POINTS
        when :line, :lines
          raise 'A pair of points is required for each line!' if (s % 2) != 0
          GL_LINES
        when :line_strip, :strip
          raise 'Not enough points: At least two required!' if s < 2
          GL_LINE_STRIP
        when :line_loop, :loop
          raise 'Not enough points: At least two required!' if s < 2
          GL_LINE_LOOP
        when :triangle, :triangles
          raise 'Not enough points: At least three required!' if s < 3
          GL_TRIANGLES
        when :triangle_strip
          raise 'Not enough points: At least three required!' if s < 3
          GL_TRIANGLE_STRIP
        when :triangle_fan
          raise 'Not enough points: At least three required!' if s < 3
          GL_TRIANGLE_FAN
        when :quad, :quads
          raise 'Not enough points: At least four required!' if s < 4
          GL_QUADS
        when :quad_strip
          raise 'Not enough points: At least four required!' if s < 4
          GL_QUAD_STRIP
        when :convex_polygon, :polygon
          raise 'Not enough points: At least three required!' if s < 3
          GL_POLYGON
        else
          raise 'Invalid type.'
      end unless type.is_a?(Fixnum)
      @draw_queue << [type, points, color, width, stipple, mode.to_i]
    end

    # Draw 2D.
    # @param [Fixnum, String, Symbol] type Drawing type. See source for details.
    # @param [Array<Array[Numeric]>, Array<Geom::Point3d>] points An array of
    #   points.
    # @param [Array, String, Sketchup::Color] color
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    def draw2d(type, points, color = 'black', width = 1, stipple = '')
      draw(type, points, color, width, stipple, 0)
    end

    # Draw 3D.
    # @param [Fixnum, String, Symbol] type Drawing type. See source for details.
    # @param [Array<Array[Numeric]>, Array<Geom::Point3d>] points An array of
    #   points.
    # @param [Array, String, Sketchup::Color] color
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    def draw3d(type, points, color = 'black', width = 1, stipple = '')
      draw(type, points, color, width, stipple, 1)
    end

    # Draw 3d points with style.
    # @param [Array<Geom::Point3d>] points An array of points.
    # @param [Fixnum] size Size of the point in pixels.
    # @param [Fixnum] style Styles: 0 - none, 1 - open square, 2 - filled
    #   square, 3 - "+", 4 - "X", 5 - "*", 6 - open triangle, 7 - filled
    #   triangle.
    # @param [Array, Sketchup::Color, String] color
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    def draw_points(points, size = 1, style = 0, color = 'black', width = 1, stipple = '')
      raise ArgumentError, 'Expected an array of points.' unless points.is_a?(Array) or points.is_a?(Geom::Point3d)
      points = [points] if points[0].is_a?(Numeric)
      raise 'Not enough points: At least one required!' if points.empty?
      @points_queue << [points, size, style, color, width, stipple]
    end

    # Draws 3d points with style. Unlike the {#draw_points}, this function
    # avoids drawing points behind camera and behind object.
    # @param (see #draw_points)
    def draw_points2(points, size = 1, style = 0, color = [0,0,0], width = 1, stipple = '')
      raise ArgumentError, 'Expected an array of points.' unless points.is_a?(Array) or points.is_a?(Geom::Point3d)
      points = [points] if points[0].is_a?(Numeric)
      raise 'Not enough points: At least one required!' if points.empty?
      @points_queue2 << [points, size, style, color, width, stipple]
    end

    # Add text to the log line.
    # @param [String] text
    def log_line(text)
      @mlt.puts(text)
    end

    # Get body by group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Body, NilClass]
    def get_body_by_entity(entity)
      @bodies.values.each{ |body|
        return body if body.entity == entity
      }
      nil
    end

    # Get body by body pointer.
    # @param [AMS::FFI::Pointer, Fixnum] body_ptr
    # @return [Body, NilClass]
    def get_body_by_body_ptr(body_ptr)
      key = body_ptr.is_a?(AMS::FFI::Pointer) ? body_ptr.address : body_ptr.to_i
      @bodies[key]
    end

    # Get all bodies in simulation.
    # @return [Array<Body>]
    def bodies
      @bodies.values
    end

    # @overload emit_body(body, force, life_time)
    #   Create a copy of the body, and apply force to it.
    #   @param [Body] body The body to emit.
    #   @param [Geom::Vector3d, Array<Numeric>] force in Newtons.
    #   @param [Fixnum] life_time Body life time in frames. A life of 0 will
    #     give the body an endless life.
    #   @return [Body] A new body object if successful.
    # @overload emit_body(body, tra, force, life_time)
    #   Create a copy of the body at the specified transformation, and apply
    #   force to it.
    #   @param [Body] body The body to emit.
    #   @param [Geom::Vector3d, Array<Numeric>] force in Newtons.
    #   @param [Geom::Transformation, Geom::Point3d, Array<Numeric>] tra
    #   @param [Fixnum] life_time Body life time in frames. A life of 0 will
    #     give the body an endless life.
    #   @return [Body] A new body object if successful.
    # @example
    #   onUpdate {
    #     # Emit body every 5 frames if key 'space' is down.
    #     if key('space') == 1 && (frame % 5 == 0)
    #       dir = this.entity.transformation.yaxis
    #       simulation.emit_body(this, dir, 100)
    #     end
    #   }
    def emit_body(*args)
      if args.size == 3
        body, force, life_time = args
      elsif args.size == 4
        body, tra, force, life_time = args
        if tra.to_a.size == 3
          m = body.get_matrix.to_a
          m[12..14] = tra.to_a
          tra = m
        end
      else
        raise ArgumentError, "Expected 3 or 4 parameters, but got #{args.size}."
      end
      MSPhysics.validate_type(body, MSPhysics::Body)
      life_time = life_time.to_i.abs
      life_end = @frame + life_time.to_i.abs
      new_body = (args.size == 3) ? body.copy : body.copy(tra)
      new_body.collidable = true
      new_body.set_continuous_collision_mode(true)
      new_body.add_force(force)
      @emitted[new_body] = life_end if life_time != 0
      new_body
    end

    # Destroy all bodies in simulation.
    # @param [Boolean] delete_ents Whether to erase all entities belonging to
    #   the bodies.
    # @return [Fixnum] The number of bodies destroyed.
    def destroy_all_bodies(delete_ents = false)
      count = 0
      @bodies.values.each { |body|
        body.destroy(delete_ents)
        count += 1
      }
      # Newton.destroyAllBodies(@world_ptr)
      count
    end

    # Destroy all emitted bodies.
    # @return [Fixnum] The number of bodies destroyed.
    def destroy_all_emitted_bodies
      count = 0
      @emitted.keys.each { |body|
        body.destroy(true)
        count += 1
      }
      count
    end

    # Show/hide body collision wireframe.
    # @param [Boolean] state
    def collision_visible=(state)
      state = state ? true : false
      return if (@collision[:show] == state)
      @collision[:show] = state
      ro = Sketchup.active_model.rendering_options
      if @collision[:show]
        @collision[:display_edges] = ro['EdgeDisplayMode']
        @collision[:display_profiles] = ro['DrawSilhouettes']
        ro['EdgeDisplayMode'] = false
        ro['DrawSilhouettes'] = false
        get_collisions
      else
        ro['EdgeDisplayMode'] = @collision[:display_edges]
        ro['DrawSilhouettes'] = @collision[:display_profiles]
        @collision[:data].clear
        @collision[:tree_data].clear
      end
    end

    # Determine whether body collision wireframe is visible.
    # @return [Boolean]
    def collision_visible?
      @collision[:show]
    end

    # Show/hide body centre of mass axis.
    # @param [Boolean] state
    def axis_visible=(state)
      @axis[:show] = state ? true : false
    end

    # Determine whether body centre of mass axis is visible.
    # @return [Boolean]
    def axis_visible?
      @axis[:show]
    end

    # Show/hide body contact points.
    # @param [Boolean] state
    def contact_points_visible=(state)
      @contacts[:show] = state ? true : false
    end

    # Determine whether body contact points are visible.
    # @return [Boolean]
    def contact_points_visible?
      @contacts[:show]
    end

    # Show/hide body contact forces.
    # @param [Boolean] state
    def contact_forces_visible=(state)
      @forces[:show] = state ? true : false
    end

    # Determine whether body contact forces are visible.
    # @return [Boolean]
    def contact_forces_visible?
      @forces[:show]
    end

    # Show/hide body bounding box (AABB).
    # @param [Boolean] state
    def bounding_box_visible=(state)
      @aabb[:show] = state ? true : false
    end

    # Determine whether body bonding box (AABB) is visible.
    # @return [Boolean]
    def bounding_box_visible?
      @aabb[:show]
    end

    # Show/hide all bodies.
    # @param [Boolean] state
    def bodies_visible=(state = true)
      state = state ? true : false
      @bodies.values.each { |body|
        body.entity.visible = state
      }
      @show_bodies = state
    end

    # Determine whether bodies are set visible.
    # @return [Boolean]
    def bodies_visible?
      @show_bodies
    end

    # Get Newton solver model.
    # @return [Fixnum] +0+ - exact, +n+ - interactive.
    def solver_model
      @solver_model
    end

    # Set Newton solver model.
    # @param [Fixnum] model +0+ - exact, +n+ - interactive.
    #   Use exact solver model when precision is more important than speed.
    #   Use interactive solver model when good degree of stability is important,
    #   but not as important as speed.
    def solver_model=(model)
      @solver_model = model.to_i.abs
      Newton.setSolverModel(@world_ptr, @solver_model)
    end

    # Get Newton friction model.
    # @return [Fixnum] +0+ - exact, +1+ - adaptable.
    def friction_model
      @friction_model
    end

    # Set coulomb model of friction.
    # @param [Fixnum] model +0+ - exact coulomb, +1+ - adaptive coulomb.
    #   Adaptive coulomb is about 10% faster than exact coulomb.
    def friction_model=(model)
      @friction_model = (model == 1 ? 1 : 0)
      Newton.setFrictionModel(@world_ptr, @friction_model)
    end

    # Get simulation update time-step for every second.
    # @return [Numeric]
    def update_timestep
      @update_step
    end

    # Set simulation update time-step in seconds. The smaller the time step, the
    # more accurate simulation is.
    # @param [Numeric] step Min: +1/1200.0+; Max: +1/30.0+; Normal: +1/60.0+.
    def update_timestep=(step)
      @update_step = MSPhysics.clamp(step, 1/1200.0, 1/30.0)
    end

    # Get gravity.
    # @return [Numeric] in m/s/s.
    def gravity
      @gravity
    end

    # Set gravity.
    # @param [Numeric] accel in m/s/s.
    def gravity=(accel)
      @gravity = accel.to_f
    end

    # Get material thickness in meters.
    # @return [Numeric]
    def material_thickness
      @thickness
    end

    # Set material thickness in meters.
    # @param [Numeric] thickness
    #   This value is clamped between +0.00+ and +1/32.0+ meters.
    def material_thickness=(thickness)
      @thickness = MSPhysics.clamp(thickness, 0, 1/32.0)
      Newton.materialSetSurfaceThickness(@world_ptr, @mat_id, @mat_id, @thickness)
    end

    # Shoot a ray from +point1+ to +point2+ and get all ray intersections.
    # @param [Array<Numeric>, Geom::Point3d] point1
    # @param [Array<Numeric>, Geom::Point3d] point2
    # @return [Array<Hit>]
    # @example
    #   onKeyDown { |vk|
    #     next if vk != 'space'
    #     pt1 = this.get_position(1)
    #     v = this.get_matrix.yaxis
    #     v.length = 10000
    #     pt2 = pt1 + v
    #     hits = simulation.ray_cast(pt1, pt2)
    #     if hits.size != 0
    #       hits[0].body.destroy
    #     end
    #   }
    def ray_cast(point1, point2)
      point1 = Conversion.convert_point(point1, :in, :m).to_a.pack('F*')
      point2 = Conversion.convert_point(point2, :in, :m).to_a.pack('F*')
      @ray_data.clear
      @ray_continue = 1
      Newton.worldRayCast(@world_ptr, point1, point2, @ray_filter_callback, nil, nil, 0)
      @ray_data.clone
    end

    # Shoot a ray from +point1+ to +point2+ and get the first hit.
    # @param [Array<Numeric>, Geom::Point3d] point1
    # @param [Array<Numeric>, Geom::Point3d] point2
    # @return [Hit, NilClass]
    def ray_cast_first(point1, point2)
      point1 = Conversion.convert_point(point1, :in, :m).to_a.pack('F*')
      point2 = Conversion.convert_point(point2, :in, :m).to_a.pack('F*')
      @ray_data.clear
      @ray_continue = 0
      Newton.worldRayCast(@world_ptr, point1, point2, @ray_filter_callback, nil, nil, 0)
      @ray_data[0]
    end

    # Enable/Disable animation recording.
    # @param [Boolean] state
    def record_animation=(state)
      @record_animation = state ? true : false
    end

    # Determine whether animation is recording.
    # @return [Boolean]
    def animation_recording?
      @record_animation
    end

    # Enable/Disable continuous collision mode for all bodies at once.
    # Continuous collision check prevents bodies from penetrating into each
    # other and prevents them from passing each other at high speeds.
    # @param [Boolean] state
    def continuous_collision_mode_enabled=(state)
      @ccm = state ? true : false
      self.bodies.each { |body|
        body.set_continuous_collision_mode(@ccm)
      }
    end

    # Determine whether continuous collision mode is enabled.
    # @return [Boolean]
    def continuous_collision_mode_enabled?
      @ccm
    end

    # Apply an explosion force to all bodies surrounding the blast radius.
    # @param [Geom::Point3d] center in global space.
    # @param [Numeric] blast_radius in inches.
    # @param [Numeric] blast_force in Newtons.
    def explosion(center, blast_radius, blast_force)
      center = Geom::Point3d.new(center)
      @bodies.values.each { |body|
        next if body.static?
        hit = simulation.ray_cast_first(center, body.get_position(1))
        next if hit.nil? or hit.body != body
        dist = center.distance(hit.position.distance)
        next if dist.zero? or dist > blast_radius
        force = (blast_radius - dist)*blast_force/blast_radius.to_f
        vector.length = force
        body.add_force(vector)
      }
    end

  end # class Simulation
end # module MSPhysics
