module MSPhysics

  # @since 1.0.0
  class JointTool

    class << self

      def generate_uniq_id
        ids = []
        Sketchup.active_model.definitions.each { |cd|
          cd.instances.each { |inst|
            type = inst.get_attribute('MSPhysics', 'Type', 'Body')
            if type == 'Joint'
              id = inst.get_attribute('MSPhysics Joint', 'ID', nil)
              ids << id if id.is_a?(Fixnum)
            end
          }
        }
        while true
          id = rand(900000) + 100000
          return id unless ids.include?(id)
        end
      end

    end # class << self

    # @param [Symbol, String] joint_type See {MSPhysics::JOINT_TYPES} for valid
    #  joint types.
    def initialize(joint_type)
      @joint_type = MSPhysics::Joint.optimize_joint_name(joint_type)
      raise(TypeError, 'Given joint type is invalid!', caller) unless @joint_type
      dir = File.dirname(__FILE__)
      dir.force_encoding("UTF-8") if RUBY_VERSION !~ /1.8/
      @path = File.join(dir, 'models')
      @full_path = File.join(@path, @joint_type.to_s + '.skp')
      raise(TypeError, 'Given joint type is invalid!', caller) unless File.exists?(@full_path)
      words = @joint_type.to_s.split('_')
      for i in 0...words.size
        words[i].capitalize!
      end
      @joint_name = words.join() #words.join(' ')
      @ip1 = Sketchup::InputPoint.new
      @ip2 = Sketchup::InputPoint.new
      @ip = Sketchup::InputPoint.new
      @drawn = false
      @state = 0
      @xdown = 0
      @ydown = 0
      @shift_down_time = nil
      @layer_color = Sketchup::Color.new(44, 44, 164)
      @active = false
      Sketchup.active_model.select_tool(self)
    end

    # @!visibility private


    def is_active?
      return @active
    end

    def activate
      Sketchup::set_status_text('Length', SB_VCB_LABEL)
      reset(nil)
      @active = true
    end

    def deactivate(view)
      view.invalidate if @drawn
      @active = false
    end

    def onCancel(flag, view)
      reset(view)
    end

    def reset(view)
      @state = 0
      @ip1.clear
      @ip2.clear
      if view
        view.tooltip = nil
        view.invalidate if @drawn
      end
      @drawn = false
      @dragging = false
      Sketchup.set_status_text('Select first point', SB_PROMPT)
    end

    def create_geometry(pt1, pt2, view)
      model = Sketchup.active_model
      ents = model.active_entities
      model.start_operation('Create Joint')
      begin
        if ents.parent.is_a?(Sketchup::ComponentDefinition)
          type = ents.parent.instances.first.get_attribute('MSPhysics', 'Type', nil)
          raise(TypeError, 'Cannot create a recursively defined joint!', caller) if type == 'Joint'
        end
        cd = model.definitions.load(@full_path)
        tra = Geom::Transformation.new(pt1, pt1.vector_to(pt2))
        ts = Geom::Transformation.scaling(MSPhysics.get_joint_scale)
        layer = model.layers['MSPhysics Joints']
        unless layer
          layer = model.layers.add('MSPhysics Joints')
          layer.color = @layer_color if Sketchup.version.to_i > 13
        end
        if @joint_type == :curvy_slider || @joint_type == :curvy_piston
          curve_path = File.join(@path, 'curve.skp')
          curve_cd = model.definitions.load(curve_path)
          inst = ents.add_group
          inst2 = inst.entities.add_instance(cd, ts)
          inst2.layer = layer
          inst3 = inst.entities.add_instance(curve_cd, Geom::Transformation.new())
          inst3.explode
          inst.move!(tra)
        else
          inst = ents.add_instance(cd, tra * ts)
        end
        inst.layer = layer
        assign_attributes(inst)
        model.selection.clear
        model.selection.add(inst)
      rescue Exception => e
        model.abort_operation
        ::UI.messagebox(e)
        return
      end
      model.commit_operation
    end

    def assign_attributes(ent)
      ent.set_attribute('MSPhysics', 'Type', 'Joint')
      ent.set_attribute('MSPhysics Joint', 'Type', @joint_name.to_s)
      ent.set_attribute('MSPhysics Joint', 'ID', JointTool.generate_uniq_id)
    end

    def draw_geometry(pt1, pt2, view)
      view.draw_line(pt1, pt2)
    end

    def draw(view)
      return unless @ip1.valid?
      if @ip1.display?
        @ip1.draw(view)
        @drawn = true
      end
      if @ip2.valid?
        @ip2.draw(view) if @ip2.display?
        view.set_color_from_line(@ip1, @ip2)
        view.line_width = 2
        draw_geometry(@ip1.position, @ip2.position, view)
        @drawn = true
      end
    end

    def onMouseMove(flags, x, y, view)
      if @state == 0
        @ip.pick(view, x, y)
        if @ip != @ip1
          view.invalidate if @ip.display? or @ip1.display?
          @ip1.copy! @ip
          view.tooltip = @ip1.tooltip
        end
      else
        @ip2.pick(view, x, y, @ip1)
        view.tooltip = @ip2.tooltip if @ip2.valid?
        view.invalidate
        if @ip2.valid?
          dist = @ip1.position.distance(@ip2.position)
          Sketchup.set_status_text(dist.to_s, SB_VCB_VALUE)
        end
        @dragging = true if ( (x-@xdown).abs > 10 || (y-@ydown).abs > 10 )
      end
    end

    def onLButtonDown(flags, x, y, view)
      if @state == 0
        @ip1.pick(view, x, y)
        if @ip1.valid?
          @state = 1
          Sketchup.set_status_text('Select second point', SB_PROMPT)
          @xdown = x
          @ydown = y
        end
      else
        if @ip2.valid?
          create_geometry(@ip1.position, @ip2.position, view)
          reset(view)
        end
      end
      view.lock_inference
    end

    def onLButtonUp(flags, x, y, view)
      if @dragging and @ip2.valid?
        create_geometry(@ip1.position, @ip2.position, view)
        reset(view)
      end
    end

    def onKeyDown(key, repeat, flags, view)
      if key == CONSTRAIN_MODIFIER_KEY and repeat == 1
        @shift_down_time = Time.now
        if view.inference_locked?
          view.lock_inference
        elsif @state == 0 and @ip1.valid?
          view.lock_inference @ip1
        elsif @state == 1 and @ip2.valid?
          view.lock_inference @ip2, @ip1
        end
      end
    end

    def onKeyUp(key, repeat, flags, view)
      if key == CONSTRAIN_MODIFIER_KEY and view.inference_locked? and (Time.now - @shift_down_time) > 0.5
        view.lock_inference
      end
    end

    def onUserText(text, view)
      return if @state != 1
      return unless @ip2.valid?
      begin
        value = text.to_l
      rescue Exception => e
        ::UI.beep
        puts "Cannot convert '#{text}' to length."
        value = nil
        Sketchup.set_status_text('', SB_VCB_VALUE)
      end
      return unless value
      pt1 = @ip1.position
      vec = @ip2.position - pt1
      if vec.length.zero?
        ::UI.beep
        return
      end
      vec.length = value
      pt2 = pt1 + vec
      create_geometry(pt1, pt2, view)
      reset(view)
    end

    def getExtents
      bb = Sketchup.active_model.bounds
      bb.add(@ip1.position) if @ip1.valid?
      bb.add(@ip2.position) if @ip2.valid?
      bb.add(@ip.position) if @ip.valid?
      bb
    end

  end # class JointTool
end # module MSPhysics
