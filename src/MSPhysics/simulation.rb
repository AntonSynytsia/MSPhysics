module MSPhysics
  class Simulation

    def initialize
      @frame = 0
      @draw_queue = []
      @points_queue = []
      @points_queue2 = []
      @world_ptr = nil
      @solver_model = 1
      @friction_model = 0
      @gravity = -9.8
      @mat_id = 0
      @material = Material.new('wood', 700, 0.50, 0.25, 0.40, 0.15, 0.00)
      @bb = Geom::BoundingBox.new
      @mlt = nil
      @bodies = {}
      @emitted = {}
      @contact_data = []
      @ray_data = []
      @ray_continue = 0
      @error = nil
      @start_called = false
      @reset_called = false
      @destroy_all = false
      @show_bodies = true
      @animation = Animation.new
      @record_animation = false
      @collision = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :active         => Sketchup::Color.new(255,0,100),
        :sleeping       => Sketchup::Color.new(100,150,255),
        :kinematic      => Sketchup::Color.new(255,0,0),
        :deformable     => Sketchup::Color.new(0,0,255),
        :contact_face   => Sketchup::Color.new(255,255,0),
        :data           => {}
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
        :show_points    => false,
        :show_forces    => false,
        :force_ratio    => 30,
        :line_width     => 1,
        :line_stipple   => '',
        :point_size     => 5,
        :point_style    => 2,
        :point_color    => Sketchup::Color.new(255,0,0),
        :line_color     => Sketchup::Color.new(0,80,255)
      }
      @aabb = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :color          => Sketchup::Color.new(0,0,255)
      }
      # The smaller the time step, the more accurate the simulation is!
      # Min: 1/1024.0; Max: 1/32.0; Normal: 1/64.
      @update_step = 1/64.0
      # Update bodies' transformation every n frames.
      @update_rate = 1
      # Callbacks
      @destructor_callback = Proc.new { |body_ptr|
        next if @reset_called
        body = get_body_by_body_ptr(body_ptr)
        next unless body
        @collision[:data].delete(body_ptr.address)
        body.destroy(true, false)
      }
      @force_callback = Proc.new { |body_ptr, time_step, thread_index|
        body = get_body_by_body_ptr(body_ptr)
        next unless body
        if @gravity != 0
          mass = 0.chr*4
          ixx  = 0.chr*4
          iyy  = 0.chr*4
          izz  = 0.chr*4
          Newton.bodyGetMassMatrix(body_ptr, mass, ixx, iyy, izz)
          mass = mass.unpack('F')[0]
          force = [0,0, @gravity*mass]
          Newton.bodySetForce(body_ptr, force.pack('F*'))
        end
        data = body._applied_forces
        Newton.bodySetForce(body_ptr, data[:set_force]) if data[:set_force]
        Newton.bodySetTorque(body_ptr, data[:set_torque]) if data[:set_torque]
        Newton.bodyAddForce(body_ptr, data[:add_force].pack('F*')) if data[:add_force]
        Newton.bodyAddTorque(body_ptr, data[:add_torque].pack('F*')) if data[:add_torque]
        data.clear
      }
      @aabb_overlap_callback = Proc.new { |material, body_ptr0, body_ptr1, thread_index|
        body0 = get_body_by_body_ptr(body_ptr0)
        body1 = get_body_by_body_ptr(body_ptr1)
        # Verify the existence of both Body objects.
        next 1 unless body0 and body1
        next 1 if !body0.proc_assigned?(:onTouching) and !body1.proc_assigned?(:onTouching)
        found_data = false
        @contact_data.each{ |data|
          if data.include?(body0) and data.include?(body1)
            found_data = data
            break
          end
        }
        next 1 unless found_data
        found_data[2] = @frame
        begin
          body0.call_event(:onTouching, body1)
          body1.call_event(:onTouching, body0)
        rescue Exception => e
          @error = e
          next 1
        end
        1
      }
      @contacts_callback = Proc.new { |contact_joint, time_step, thread_index|
        body_ptr0 = Newton.jointGetBody0(contact_joint)
        body_ptr1 = Newton.jointGetBody1(contact_joint)
        body0 = get_body_by_body_ptr(body_ptr0)
        body1 = get_body_by_body_ptr(body_ptr1)
        # Verify the existence of both Body objects.
        next unless body0
        next unless body1
        # Unfreeze bodies unless both of them are frozen.
        next if body0.frozen? and body1.frozen?
        body0.frozen = false
        body1.frozen = false
        # Update contacts.
        contact = Newton.contactJointGetFirstContact(contact_joint)
        while !contact.null?
          mat = Newton.contactGetMaterial(contact)
          if body0.friction_enabled? and body1.friction_enabled?
            sfc = (body0.get_static_friction + body1.get_static_friction)/2.0
            kfc = (body0.get_kinetic_friction + body1.get_kinetic_friction)/2.0
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
        next if !body0.proc_assigned?(:onTouch) and !body1.proc_assigned?(:onTouch)
        # Do the rest for the onTouch event.
        found = false
        @contact_data.each{ |data|
          if data.include?(body0) and data.include?(body1)
            found = true
            break
          end
        }
        next if found
        # Get first contact material.
        contact = Newton.contactJointGetFirstContact(contact_joint)
        next if contact.null?
        @contact_data.push [body0, body1, @frame]
        material = Newton.contactGetMaterial(contact)
        point = 0.chr*12
        normal = 0.chr*12
        Newton.materialGetContactPositionAndNormal(material, body_ptr0, point, normal)
        point = Conversion.convert_point(point.unpack('F*'), :m, :in)
        normal = Geom::Vector3d.new(normal.unpack('F*'))
        speed = Newton.materialGetContactNormalSpeed(material)
        # Trigger the events.
        begin
          body0.call_event(:onTouch, body1, point, normal.reverse, speed)
          body1.call_event(:onTouch, body0, point, normal, speed)
        rescue Exception => e
          @error = e
          next
        end
      }
      @tree_collision_callback = Proc.new { |static_body_ptr, body_ptr, face_id, count, vertices, stride|
        next unless @collision[:show]
        face = []
        cloud = vertices.get_array_of_float(0, count*3)
        for i in 0...count
          x = cloud[i*3+0].m
          y = cloud[i*3+1].m
          z = cloud[i*3+2].m
          face << Geom::Point3d.new(x,y,z)
        end
        draw3d(face, @collision[:contact_face], :line_loop, @collision[:line_width], @collision[:line_stipple])
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
      BodyObserver.add_observer(self)
    end

    # @!attribute [r] world_ptr
    #   @return [AMS::FFI::Pointer, NilClass]

    # @!attribute [r] animation
    #   @return [Animation]


    attr_reader :world_ptr, :animation

    # @!visibility private
    attr_reader :bb

    # @!visibility private
    def on_create(body)
      return if body._world_ptr != @world_ptr
      return unless body.is_a?(BodyContext)
      @bodies[body._body_ptr.address] = body
      Newton.bodySetDestructorCallback(body._body_ptr, @destructor_callback)
      Newton.bodySetForceAndTorqueCallback(body._body_ptr, @force_callback)
    end

    # @!visibility private
    def on_destroy(body)
      return unless body.is_a?(BodyContext)
      begin
        body.call_event(:onDestroy)
      rescue Exception => e
        @error = e
      end
      @bodies.delete(body._body_ptr.address)
    end

    # @!visibility private
    def call_event(evt, *args)
      return false if @destroy_all and evt != :onDestroy
      @bodies.values.each{ |body|
        body.call_event(evt, *args)
      }
      true
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
      @draw_queue.each{ |points, color, type, width, stipple, mode|
        @bb.add(points)
        view.drawing_color = color
        view.line_width = width
        view.line_stipple = stipple
        if mode == 1
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
            sleeping = body.get_sleep_state
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
      return unless @contacts[:show_points] or @contacts[:show_forces]
      points = []
      lines = []
      @bodies.values.each{ |body|
        next if body.static?
        body.get_contacts.each { |contact|
          points.push(contact.position.to_a) if @contacts[:show_points]
          if @contacts[:show_forces]
            pos = contact.position
            force = contact.force
            next if force.length == 0
            force.length *= @contacts[:force_ratio]/body.get_mass.to_f
            lines << pos
            lines << pos + force
          end
        }
      }
      points.uniq!
      view.draw_points(points, @contacts[:point_size], @contacts[:point_style], @contacts[:point_color]) unless points.empty?
      return if lines.empty?
      view.drawing_color = @contacts[:line_color]
      view.line_width = @contacts[:line_width]
      view.line_stipple = @contacts[:line_stipple]
      view.draw(GL_LINES, lines)
    end

    def draw_bounding_box(view)
      return unless @aabb[:show]
      faces = []
      lines = []
      @bodies.values.each { |body|
        min, max = body.get_bounding_box
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

    def clear_drawing_queues
      @draw_queue.clear
      @points_queue.clear
      @points_queue2.clear
    end

    public

    # Add a group/component to the simulation.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Body, NilClass] A body object (if successful).
    def add_entity(entity)
      return unless [Sketchup::Group, Sketchup::ComponentInstance].include?(entity.class)
      return if get_body_by_entity(entity)
      handle = MSPhysics::ATTR_NAME
      return if entity.get_attribute(handle, 'Ignore')
      type = :dynamic
      shape = entity.get_attribute(handle, 'Shape', 'Convex Hull')
      begin
        body = BodyContext.new(@world_ptr, entity, type, shape, @material)
      rescue Exception => e
        puts "#{e}\n#{$@[0]}"
        index = Sketchup.active_model.entities.to_a.index(entity)
        puts "Entities[#{index}] has invalid collision shape! It was not added to simulation."
        return
      end
      if body.get_shape == :static_mesh
        Newton.staticCollisionSetDebugCallback(body._collision_ptr, @tree_collision_callback)
      end
      if entity.get_attribute(handle, 'Static')
        body.static = true
      end
      if entity.get_attribute(handle, 'Frozen')
        body.frozen = true
      end
      if entity.get_attribute(handle, 'No Collision')
        body.collidable = false
      end
      body
    end

    # Remove a group/component from the simulation.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Boolean] Whether the entity was removed.
    def remove_entity(entity)
      body = get_body_by_entity(entity)
      return false unless body
      body.destroy
      true
    end

    # @!visibility private
    def do_on_start
      return if @start_called
      @start_called = true
      model = Sketchup.active_model
      @mlt = AMS::MultiLineText.new(10,10)
      @mlt.limit = 10
      @mlt.display_lines = false
      mat = model.materials.add('MultiLineText')
      mat.color = [0,0,220]
      @mlt.ent.material = mat
      @world_ptr = Newton.create
      Newton.invalidateCache(@world_ptr)
      Newton.setSolverModel(@world_ptr, @solver_model)
      Newton.setFrictionModel(@world_ptr, @friction_model)
      @mat_id = Newton.materialGetDefaultGroupID(@world_ptr)
      Newton.materialSetCollisionCallback(@world_ptr, @mat_id, @mat_id, nil, @aabb_overlap_callback, @contacts_callback)
      Newton.materialSetSurfaceThickness(@world_ptr, @mat_id, @mat_id, @material.thickness)
      Newton.materialSetDefaultFriction(@world_ptr, @mat_id, @mat_id, @material.static_friction, @material.kinetic_friction)
      Newton.materialSetDefaultElasticity(@world_ptr, @mat_id, @mat_id, @material.elasticity)
      Newton.materialSetDefaultSoftness(@world_ptr, @mat_id, @mat_id, @material.softness)
      # Create Bodies
      count = 0
      model.entities.each { |ent|
        begin
          body = add_entity(ent)
          next unless body
          if count == 0
            script =%q`
class Vehicle

  # @param [Numeric] wheelbase distance between the axles of front and rear wheels.
  # @param [Numeric] track distance between the steering axles of two front wheels.
  def initialize(wheelbase, track)
    @wheelbase = wheelbase.to_f.abs
    @track = track.to_f.abs
    @theta = 0
    @front_center_radius = 0
    @rear_center_radius = 0
  end

  attr_reader :front_center_radius, :rear_center_radius

  def get_steering_angle
    @theta.radians
  end

  # Set steering angle of the center between two front wheels in degrees.
  # @param [Numeric] angle
  def set_steering_angle(angle)
    @theta = angle.degrees
    return if @theta.zero?
    @front_center_radius = @wheelbase / Math.sin(@theta)
    @rear_center_radius = Math.tan(Math::PI/2 - @theta)*@wheelbase
  end

  # Get steering angle of the front left wheel in degrees.
  # @return [Numeric]
  def steer1
    return 0 if @theta.zero?
    Math.atan(@wheelbase.to_f/(@rear_center_radius - @track/2)).radians
  end

  # Get steering angle of the front right wheel in degrees.
  # @return [Numeric]
  def steer2
    return 0 if @theta.zero?
    Math.atan(@wheelbase.to_f/(@rear_center_radius + @track/2)).radians
  end

  # Get rotational coefficient of the front left wheel.
  # @return [Numeric]
  def wheel1
    return 1 if @theta.zero?
    r = (@wheelbase**2 + (@rear_center_radius - @track/2)**2)**0.5
    (r.to_f / @front_center_radius).abs
  end

  # Get rotational coefficient of the front right wheel.
  # @return [Numeric]
  def wheel2
    return 1 if @theta.zero?
    r = (@wheelbase**2 + (@rear_center_radius + @track/2)**2)**0.5
    (r.to_f / @front_center_radius).abs
  end

  # Get rotational coefficient of the rear left wheel.
  # @return [Numeric]
  def wheel3
    return 1 if @theta.zero?
    r = @rear_center_radius - @track/2
    (r.to_f / @front_center_radius).abs
  end

  # Get rotational coefficient of the rear right wheel.
  # @return [Numeric]
  def wheel4
    return 1 if @theta.zero?
    r = @rear_center_radius + @track/2
    (r.to_f / @front_center_radius).abs
  end

end

onStart {
ents = Sketchup.active_model.entities
@bodies = {}
@joints = {}
# get bodies
ents.to_a.each { |e|
  next unless [Sketchup::Group, Sketchup::ComponentInstance].include?(e.class)
  next if e.name.size.zero?
  body = simulation.get_body_by_entity(e)
  @bodies[e.name.to_sym] = body
  if e.name == 'ground'
    #body.set_static_friction(1.4)
    #body.set_kinetic_friction(0.8)
  elsif e.name.include?('wheel')
    body.set_static_friction(1.16)
    body.set_kinetic_friction(1.16)
    body.set_softness(1.00)
  end
  #body.set_auto_sleep(false)
  #body.set_mass(1)
}
# connect
@joints[:steer1] = Servo.new(@bodies[:box1].entity.bounds.center, @bodies[:box1].entity.transformation.zaxis, @bodies[:body], @bodies[:box1], -45, 45, 200, 50000)
@joints[:steer2] = Servo.new(@bodies[:box2].entity.bounds.center, @bodies[:box2].entity.transformation.zaxis, @bodies[:body], @bodies[:box2], -45, 45, 200, 50000)
=begin
accel = 1
damp = 0.5
@joints[:motor1] = Motor.new(@bodies[:wheel1].entity.bounds.center, @bodies[:wheel1].entity.transformation.xaxis, @bodies[:box1], @bodies[:wheel1], accel, damp)
@joints[:motor2] = Motor.new(@bodies[:wheel2].entity.bounds.center, @bodies[:wheel2].entity.transformation.xaxis, @bodies[:box2], @bodies[:wheel2], accel, damp)
@joints[:motor3] = Motor.new(@bodies[:wheel3].entity.bounds.center, @bodies[:wheel3].entity.transformation.xaxis, @bodies[:body], @bodies[:wheel3], accel, damp)
@joints[:motor4] = Motor.new(@bodies[:wheel4].entity.bounds.center, @bodies[:wheel4].entity.transformation.xaxis, @bodies[:body], @bodies[:wheel4], accel, damp)
=end
@joints[:motor1] = Hinge.new(@bodies[:wheel1].entity.bounds.center, @bodies[:wheel1].entity.transformation.xaxis, @bodies[:box1], @bodies[:wheel1])
@joints[:motor1].limits_enabled = false
@joints[:motor2] = Hinge.new(@bodies[:wheel2].entity.bounds.center, @bodies[:wheel2].entity.transformation.xaxis, @bodies[:box2], @bodies[:wheel2])
@joints[:motor2].limits_enabled = false
@joints[:motor3] = Hinge.new(@bodies[:wheel3].entity.bounds.center, @bodies[:wheel3].entity.transformation.xaxis, @bodies[:body], @bodies[:wheel3])
@joints[:motor3].limits_enabled = false
@joints[:motor4] = Hinge.new(@bodies[:wheel4].entity.bounds.center, @bodies[:wheel4].entity.transformation.xaxis, @bodies[:body], @bodies[:wheel4])
@joints[:motor4].limits_enabled = false
@joints[:gear_box] = Fixed.new(@bodies[:gear_box].entity.bounds.center, @bodies[:body], @bodies[:gear_box], 0)

@vehicle = Vehicle.new(60, 40)
@note = Sketchup.active_model.add_note('note', 0.01, 0.02)
@mass = 0
@bodies.each { |name, body|
  next if name == :ground
  @mass += body.get_mass
}
simulation.set_solver_model(0)
simulation.set_update_timestep(1/64.0)
}

onUpdate {
v = key('a')-key('d')
s1 = 0
s2 = 0
if v == 1
  s1 = 16.3
  s2 = 20
elsif v == -1
  s1 = -20
  s2 = -16.3
end
@vehicle.set_steering_angle((key('a')-key('d'))*20)
@joints[:steer1].target_angle = @vehicle.steer1
@joints[:steer2].target_angle = @vehicle.steer2
f = (key('s') - key('w'))*1
=begin
@joints[:motor1].controller = f*@vehicle.wheel1
@joints[:motor2].controller = f*@vehicle.wheel2
@joints[:motor3].controller = f*@vehicle.wheel3
@joints[:motor4].controller = f*@vehicle.wheel4
=end
force = @bodies[:gear_box].entity.transformation.xaxis
force = MSPhysics.scale_vector(force, (key('w') - key('s'))*5000)
@bodies[:gear_box].add_force(force)
vc = @bodies[:gear_box].get_velocity.length.to_f

@note.text = "Vehicle Properties
steer1 : #{@vehicle.steer1.round(2)}
steer2 : #{@vehicle.steer2.round(2)}
wheel1 : #{@vehicle.wheel1.round(2)}
wheel2 : #{@vehicle.wheel2.round(2)}
wheel3 : #{@vehicle.wheel3.round(2)}
wheel4 : #{@vehicle.wheel4.round(2)}
front center radius  : #{@vehicle.front_center_radius.round(2)}
rear center radius   : #{@vehicle.rear_center_radius.round(2)}
average distribution : #{((@vehicle.wheel1 + @vehicle.wheel2 + @vehicle.wheel3 + @vehicle.wheel4)/4.0).round(2)}

Mass: #{@mass.round(2)}
Static friction: #{@bodies[:wheel1].get_static_friction}
Kinetic friction: #{@bodies[:wheel1].get_kinetic_friction}
Current Ratios

center : #{vc.round(2)}
wheel1 : #{(@bodies[:wheel1].get_velocity.length / vc).round(2)}
wheel2 : #{(@bodies[:wheel2].get_velocity.length / vc).round(2)}
wheel3 : #{(@bodies[:wheel3].get_velocity.length / vc).round(2)}
wheel4 : #{(@bodies[:wheel4].get_velocity.length / vc).round(2)}
"
}`

script2 = %q`
onStart {
ents = Sketchup.active_model.entities
@joints = []
# get bodies
ents.to_a.each { |e|
  next unless [Sketchup::Group, Sketchup::ComponentInstance].include?(e.class)
  next if e.name != 'ball'
  body = simulation.get_body_by_entity(e)
  body.set_density(7870)
  body.set_elasticity(1.05)
  body.set_softness(0.00)
  body.friction_enabled = false
  body.set_auto_sleep(false)
  #body.set_continuous_collision_mode(false)
  pos = e.bounds.center
  pos.z += 60
  jnt = Hinge.new(pos, [1,0,0], nil, body, 0, 0, 20)
  jnt.limits_enabled = false
  @joints << jnt
}
simulation.set_solver_model(0)
simulation.set_update_timestep(1/128.0)
}`
script3 = %q`
onStart{
simulation.get_bodies.each { |body|
  next if body.entity.name != 'brick'
  body.set_density(1500)
  body.set_elasticity(0.1)
  body.set_softness(0.00)
}
simulation.set_solver_model(1)
simulation.set_friction_model(1)
simulation.set_update_timestep(1/256.0)
}
`

            #body.set_script(script3)
          end
          count += 1
        rescue Exception => e
          raise "An error occurred while starting simulation!\nError:\n  #{e}\nLocation:\n  #{$@[0]}\n"
        end
      }
      call_event(:onStart)
    end

    # @!visibility private
    def do_on_end
      return if @reset_called
      @reset_called = true
      UI.start_timer(0.1, false){
        Newton.destroy(@world_ptr)
        @world_ptr = nil
      }
      show_collision(false)
      call_event(:onEnd)
    end

    # @!visibility private
    def do_on_update(frame)
      @frame = frame
      clear_drawing_queues
      Newton.update(@world_ptr, @update_step)
      raise @error if @error
      # Update particular joints
      Fixed::TO_DISCONNECT.each { |joint|
        joint.disconnect
      }
      Fixed::TO_DISCONNECT.clear
      # Update up vector joints
      @bodies.values.each { |body|
        if body.invalid?
          body.destroy(true)
          @emitted.delete(body)
          next
        end
        next unless body._up_vector
        dir = body.get_position(1).vector_to(body._up_vector[1]).normalize
        Newton.upVectorSetPin(body._up_vector[0], dir.to_a.pack('FFF'))
      }
      # Check whether the emitted body must be destroyed
      @emitted.reject! { |body, life_end|
        next false if frame < life_end
        body.destroy(true)
        true
      }
      # Update entities
      if frame % @update_rate == 0
        @bodies.values.each{ |body|
          next if body.get_sleep_state
          body.entity.transformation = body.get_matrix
          @animation.push_record(body.entity, @frame) if @record_animation
        }
      end
      call_event(:onUpdate)
      # Trigger the onUntouch events for the untouched bodies.
      @contact_data.reject! { |body0, body1, last_frame|
        next false if frame == last_frame
        body0.call_event(:onUntouch, body1)
        body1.call_event(:onUntouch, body0)
        true
      }
      # Remove all bodies if destroy all called
      if @destroy_all
        Newton.destroyAllBodies(@world_ptr)
        @destroy_all = false
      end
      get_collisions
    end

    # @!visibility private
    def do_on_draw(view)
      draw_queues(view)
      draw_collision(view)
      draw_axis(view)
      draw_bounding_box(view)
      draw_contacts(view)
    end

    # Draw with OpenGL.
    # @param [Array<Array[Numeric]>, Array<Geom::Point3d>] points An array of
    #   points.
    # @param [Array, String, Sketchup::Color] color
    # @param [String, Symbol] type Drawing type. The valid types include <i>
    #   line, lines, line_strip, line_loop, triangle, triangles, triangle_strip,
    #   triangle_fan, quad, quads, quad_strip, convex_polygon, and polygon</i>
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    # @param [Boolean] mode Drawing mode: +1+ : 3d, +0+ : 2d.
    def draw(points, color, type = :line, width = 1, stipple = '', mode = 1)
      raise ArgumentError, 'Expected an array of points.' unless [Array, Geom::Point3d].include?(points.class)
      points = [points] if points[0].is_a?(Numeric)
      s = points.size
      raise 'Not enough points: At least one required!' if s == 0
      type = case type.to_s.downcase.strip.gsub(/\s/i, '_').to_sym
        when :point, :points
          GL_POINTS
        when :line, :lines
          raise 'A pair of points is required for each line!' if (s % 2) != 0
          GL_LINES
        when :line_strip
          raise 'Not enough points: At least two required!' if s < 2
          GL_LINE_STRIP
        when :line_loop
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
      end
      @draw_queue.push([points, color, type, width, stipple, mode.to_i])
    end

    # Draw 2d.
    # @param [Array<Array[Numeric]>, Array<Geom::Point3d>] points An array of
    #   points.
    # @param [Array, String, Sketchup::Color] color
    # @param [Fixnum, String, Symbol] type Drawing type. See source for details.
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    def draw2d(points, color, type = :line, width = 1, stipple = '')
      draw(points, color, type, width, stipple, 0)
    end

    # Draw 3d.
    # @param [Array<Array[Numeric]>, Array<Geom::Point3d>] points An array of
    #   points.
    # @param [Array, String, Sketchup::Color] color
    # @param [Fixnum, String, Symbol] type Drawing type. See source for details.
    # @param [Fixnum] width Width of a line in pixels.
    # @param [String] stipple Line stipple: '.' (Dotted Line), '-' (Short Dashes
    #   Line), '_' (Long Dashes Line), '-.-' (Dash Dot Dash Line), '' (Solid
    #   Line).
    def draw3d(points, color, type = :line, width = 1, stipple = '')
      draw(points, color, type, width, stipple, 1)
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
    def draw_points(points, size = 1, style = 0, color = [0,0,0], width = 1, stipple = '')
      raise ArgumentError, 'Expected an array of points.' unless [Array, Geom::Point3d].include?(points.class)
      points = [points] if points[0].is_a?(Numeric)
      raise 'Not enough points: At least one required!' if points.empty?
      @points_queue.push([points, size, style, color, width, stipple])
    end

    # Draws 3d points width style. Unlike the {#draw_points}, this function
    # avoids drawing points behind camera and behind object.
    # @param (see #draw_points)
    def draw_points2(points, size = 1, style = 0, color = [0,0,0], width = 1, stipple = '')
      raise ArgumentError, 'Expected an array of points.' unless [Array, Geom::Point3d].include?(points.class)
      points = [points] if points[0].is_a?(Numeric)
      raise 'Not enough points: At least one required!' if points.empty?
      @points_queue2.push([points, size, style, color, width, stipple])
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
      if body_ptr.is_a?(AMS::FFI::Pointer)
        key = body_ptr.address
      else
        key = body_ptr
      end
      @bodies[key]
    end

    # Get all bodies in simulation.
    # @return [Array<Body>]
    def get_bodies
      @bodies.values
    end

    # Create a copy of the body and apply force to it.
    # @param [Array<Numeric>, Geom::Vector3d] force
    # @param [Fixnum] life_time Body life time in frames. A life of 0 will cause
    #   the new body to live for ever.
    # @return [Body] New body.
    def emit_body(body, force, life_time)
      life_time = life_time.to_i.abs
      life_end = @frame + life_time.to_i.abs
      new_body = body.copy
      new_body.add_force(force)
      new_body.collidable = true
      @emitted[new_body] = life_end if life_time != 0
      new_body
    end

    # Destroy all bodies in simulation.
    def destroy_all_bodies
      @destroy_all = true
    end

    # Show/hide bodies' collision.
    # @param [Boolean] state
    def show_collision(state = true)
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
      end
    end

    # Determine whether the bodies' collision is visible.
    # @return [Boolean]
    def collision_visible?
      @collision[:show]
    end

    # Show/hide bodies' centre of mass.
    # @param [Boolean] state
    def show_axis(state = true)
      @axis[:show] = state ? true : false
    end

    # Determine whether the bodies' centre of mass axis is visible.
    # @return [Boolean]
    def axis_visible?
      @axis[:show]
    end

    # Show/hide body contact points.
    # @param [Boolean] state
    def show_contact_points(state = true)
      @contacts[:show_points] = state ? true : false
    end

    # Determine whether the body contact points are visible.
    # @return [Boolean]
    def contact_points_visible?
      @contacts[:show_points]
    end

    # Show/hide body contact forces.
    # @param [Boolean] state
    def show_contact_forces(state = true)
      @contacts[:show_forces] = state ? true : false
    end

    # Determine whether the body contact forces are visible.
    # @return [Boolean]
    def contact_forces_visible?
      @contacts[:show_forces]
    end

    # Show/hide bodies' bounding box (AABB).
    # @param [Boolean] state
    def show_bounding_box(state = true)
      @aabb[:show] = state ? true : false
    end

    # Determine whether the bodies' bonding box (AABB) is visible.
    # @return [Boolean]
    def bounding_box_visible?
      @aabb[:show]
    end

    # Show/hide all bodies.
    # @param [Boolean] state
    def show_bodies(state = true)
      state = state ? true : false
      @bodies.values.each { |body|
        body.entity.visible = state
      }
      @show_bodies = state
    end

    # Determine whether all bodies are set visible.
    # @return [Boolean]
    def bodies_visible?
      @show_bodies
    end

    # Get Newton solver model.
    # @return [Fixnum] +0+ - exact, +n+ - interactive.
    def get_solver_model
      @solver_model
    end

    # Set Newton solver model.
    # @param [Fixnum] model +0+ - exact, +n+ - interactive.
    #   Use exact solver model when precision is more important than speed.
    #   Use interactive solver model when good degree of stability is important,
    #   but not as important as speed.
    def set_solver_model(model)
      @solver_model = model.to_i.abs
      Newton.setSolverModel(@world_ptr, @solver_model)
    end

    # Get Newton friction model.
    # @return [Fixnum] +0+ - exact, +1+ - adaptable.
    def get_friction_model
      @friction_model
    end

    # Set coulomb model of friction.
    # @param [Fixnum] model +0+ - exact coulomb, +1+ - adaptive coulomb.
    #   Adaptive coulomb is about 10% faster than exact coulomb.
    def set_friction_model(model)
      @friction_model = (model == 1 ? 1 : 0)
      Newton.setFrictionModel(@world_ptr, @friction_model)
    end

    # Get simulation update time-step for every second.
    # @return [Numeric]
    def get_update_timestep
      @update_step
    end

    # Set simulation update time-step in seconds. The smaller the time step, the
    # more accurate simulation is.
    # @param [Numeric] step Min: +1/1024.0+; Max: +1/32.0+; Normal: +1/64.0+.
    def set_update_timestep(step)
      @update_step = MSPhysics.clamp(step, 1/1024.0, 1/32.0)
    end

    # Get gravity.
    # @return [Numeric] in m/s/s.
    def get_gravity
      @gravity
    end

    # Set gravity.
    # @param [Numeric] gravity in m/s/s.
    def set_gravity(gravity)
      @gravity = gravity.to_f
    end

    # Get material thickness in meters.
    # @return [Numeric]
    def get_material_thickness(thickness)
      @material.thickness
    end

    # Set material thickness in meters.
    # @param [Numeric] thickness
    #   This value is clamped between +0.00+ and +0.125+ meters.
    def set_material_thickness(thickness)
      @material.thickness = thickness
      Newton.materialSetSurfaceThickness(@world_ptr, @mat_id, @mat_id, @material.thickness)
    end

    # Shoot a ray from +point1+ to +point2+ and get all ray intersections.
    # @param [Array<Numeric>, Geom::Point3d] point1
    # @param [Array<Numeric>, Geom::Point3d] point2
    # @return [Array<Hit>]
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
    # @param [Boolean]
    def record_animation=(state)
      @record_animation = state ? true : false
    end

    # Determine whether animation is recording.
    # @return [Boolean]
    def animation_recording?
      @record_animation
    end

  end # class Simulation
end # module MSPhysics
