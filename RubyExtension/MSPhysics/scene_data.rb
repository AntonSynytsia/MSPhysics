module MSPhysics

  # @since 1.0.0
  class SceneData < Entity

    # @overload initialize()
    #   Create scene data from current model.
    # @overload initialize(page)
    #   Create scene data from page.
    #   @param [Sketchup::Page] page
    def initialize(*args)
      @axes = Geom::Transformation.new
      @hidden_entities = []
      @hidden_layers = []
      @rendering_options = {}
      @shadow_info = {}
      @style = nil
      @camera = nil
      @use_axes = true
      @use_hidden_entities = true
      @use_hidden_layers = true
      @use_rendering_options = true
      @use_shadow_info = true
      @use_style = true
      @use_camera = true
      model = Sketchup.active_model
      if args.size == 0
        if Sketchup.version.to_i >= 16
          @axes = model.axes.transformation
        end
        model.definitions.each { |d| d.instances.each { |i| @hidden_entities << i unless i.visible? } }
        model.layers.each { |layer| @hidden_layers << layer unless layer.visible? }
        model.rendering_options.each { |k, v| @rendering_options[k] = v }
        model.shadow_info.each { |k, v| @shadow_info[k] = v }
        @style = model.styles.selected_style
        @camera = MSPhysics.duplicate_camera(model.active_view.camera)
      elsif args.size == 1
        page = args[0]
        AMS.validate_type(page, Sketchup::Page)
        if Sketchup.version.to_i >= 16
          @axes = page.axes.transformation
        end
        if page.hidden_entities
          page.hidden_entities.each { |e| @hidden_entities << e }
        end
        if page.layers
          page.layers.each { |layer| @hidden_layers << layer }
        end
        if page.rendering_options
          page.rendering_options.each { |k, v| @rendering_options[k] = v }
        end
        if page.shadow_info
          page.shadow_info.each { |k, v| @shadow_info[k] = v }
        end
        @style = page.style
        @camera = MSPhysics.duplicate_camera(page.camera)
        @use_axes = page.use_axes?
        @use_hidden_entities = page.use_hidden?
        @use_hidden_layers = page.use_hidden_layers?
        @use_rendering_options = page.use_rendering_options?
        @use_shadow_info = page.use_shadow_info?
        @use_style = page.use_style?
        @use_camera = page.use_camera?
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 0..1 arguments but got #{args.size}.", caller)
      end
    end

    # Transition between this and the desired scene data.
    # @param [SceneData] scene_data Other scene data
    # @param [Numeric] ratio A value between 0.0 and 1.0.
    # @return [void]
    # @note Manually wrap the operation.
    def transition(scene_data, ratio)
      AMS.validate_type(scene_data, SceneData)
      ratio = AMS.clamp(ratio, 0, 1)
      model = Sketchup.active_model
      if scene_data.use_axes? && Sketchup.version.to_i >= 16
        tra = AMS::Geometry.transition_transformation(@axes, scene_data.axes, ratio)
        model.axes.set(tra.origin, tra.xaxis, tra.yaxis, tra.zaxis)
      end
      if scene_data.use_hidden_entities?
        if ratio == 0
          (scene_data.hidden_entities - @hidden_entities).each { |e| e.visible = true if e.valid? && !e.visible? }
          @hidden_entities.each { |e| e.visible = false if e.valid? && e.visible? }
        else
          (@hidden_entities - scene_data.hidden_entities).each { |e| e.visible = true if e.valid? && !e.visible? }
          scene_data.hidden_entities.each { |e| e.visible = false if e.valid? && e.visible? }
        end
      end
      if scene_data.use_hidden_layers?
        if ratio == 0
          (scene_data.hidden_layers - @hidden_layers).each { |e| e.visible = true if e.valid? && !e.visible? }
          @hidden_layers.each { |e| e.visible = false if e.valid? && e.visible? }
        else
          (@hidden_layers - scene_data.hidden_layers).each { |e| e.visible = true if e.valid? && !e.visible? }
          scene_data.hidden_layers.each { |e| e.visible = false if e.valid? && e.visible? }
        end
      end
      if scene_data.use_style?
        desired_style = ratio == 0 ? @style : scene_data.style
        model.styles.selected_style = desired_style if desired_style && model.styles.selected_style != desired_style
      end
      if scene_data.use_rendering_options?
        model.rendering_options.each { |mk, mv|
          s1v = @rendering_options[mk]
          s2v = scene_data.rendering_options[mk]
          if s1v.class == mv.class && s2v.class == mv.class
            if mv.is_a?(Sketchup::Color)
              model.rendering_options[mk] = AMS::Geometry.transition_color(s1v, s2v, ratio)
            elsif mv.is_a?(Numeric)
              v = AMS::Geometry.transition_number(s1v, s2v, ratio)
              model.rendering_options[mk] = mv.is_a?(Fixnum) ? v.round : v
            elsif mv.is_a?(TrueClass) || mv.is_a?(FalseClass) || mv.is_a?(String) || mv.is_a?(Symbol)
              model.rendering_options[mk] = ratio == 0 ? s1v : s2v
            end
          end
        }
      end
      if scene_data.use_shadow_info?
        model.shadow_info.each { |mk, mv|
          s1v = @shadow_info[mk]
          s2v = scene_data.shadow_info[mk]
          if s1v.class == mv.class && s2v.class == mv.class
            if mv.is_a?(Numeric) && mk !~ /time_t/i
              v = AMS::Geometry.transition_number(s1v, s2v, ratio)
              model.shadow_info[mk] = mv.is_a?(Fixnum) ? v.round : v
            elsif mv.is_a?(Time)
              s1v = s1v.getutc
              s2v = s2v.getutc
              #t1d = Time.new(s1v.year, s1v.month, s1v.day)
              t1d = Time.utc(s1v.year, s1v.month, s1v.day)
              #t2d = Time.new(s2v.year, s2v.month, s2v.day)
              t2d = Time.utc(s2v.year, s2v.month, s2v.day)
              tdd = Time.at(t1d + ((t2d - t1d) * ratio).to_i)
              t1s = s1v.hour * 3600 + s1v.min * 60 + s1v.sec
              t2s = s2v.hour * 3600 + s2v.min * 60 + s2v.sec
              tds = t2s - t1s
              if (tds < -43200)
                tds += 86400
              elsif (tds > 43200)
                tds -= 86400
              end
              sec = t1s + (tds * ratio).to_i
              hour, sec = sec.divmod(3600)
              min, sec = sec.divmod(60)
              #model.shadow_info[mk] = Time.new(tdd.year, tdd.month, tdd.day, hour % 24, min, sec, ratio == 0 ? s1v.utc_offset : s2v.utc_offset)
              model.shadow_info[mk] = Time.utc(tdd.year, tdd.month, tdd.day, hour % 24, min, sec, 1)
            elsif mv.is_a?(Geom::Vector3d)
              model.rendering_options[mk] = AMS::Geometry.transition_vector(s1v, s2v, ratio)
            elsif mv.is_a?(TrueClass) || mv.is_a?(FalseClass) || mv.is_a?(String) || mv.is_a?(Symbol)
              model.shadow_info[mk] = ratio == 0 ? s1v : s2v
            end
          end
        }
      end
      if scene_data.use_camera?
        #~ model.active_view.camera = AMS::Geometry.transition_camera(@camera, scene_data.camera, ratio)
        c1 = @camera
        c2 = scene_data.camera
        c3 = model.active_view.camera
        m = c1.eye.distance(c2.eye) * 0.5
        p0 = c1.eye
        p1 = c1.eye + AMS::Geometry.scale_vector(c1.direction.reverse, m)
        p2 = c2.eye + AMS::Geometry.scale_vector(c2.direction.reverse, m)
        p3 = c2.eye
        eye = AMS::Geometry.calc_cubic_bezier_point(ratio, p0, p1, p2, p3)
        dir = AMS::Geometry.transition_vector(c1.direction, c2.direction, ratio)
        up = AMS::Geometry.transition_vector(c1.up, c2.up, ratio)
        c3.set(eye, eye + dir, up)
        t = c1.perspective?
        c1.perspective = c2.perspective?
        c3.perspective = c2.perspective?
        if c2.perspective?
          c3.focal_length = c1.focal_length + (c2.focal_length - c1.focal_length) * ratio
          c3.fov = c1.fov + (c2.fov - c1.fov) * ratio
          c3.image_width = c1.image_width + (c2.image_width - c1.image_width) * ratio
        else
          c3.height = c1.height + (c2.height - c1.height) * ratio
        end
        c1.perspective = t
        c3.description = ratio < 0.5 ? c1.description : c2.description
      end
    end

    # Get scene axes.
    # @return [Geom::Transformation]
    def axes
      @axes
    end

    # Get scene hidden entities.
    # @return [Array<Sketchup::Entity>]
    def hidden_entities
      @hidden_entities
    end

    # Get scene hidden layers.
    # @return [Array<Sketchup::Layer>]
    def hidden_layers
      @hidden_layers
    end

    # Get scene rendering options.
    # @return [Hash]
    def rendering_options
      @rendering_options
    end

    # Get scene shadow info.
    # @return [Hash]
    def shadow_info
      @shadow_info
    end

    # Get scene style.
    # @return [Sketchup::Style]
    def style
      @style
    end

    # Get scene camera.
    # @return [Sketchup::Camera]
    def camera
      @camera
    end

    # Enable/disable axes transitioning for this scene.
    # @param [Boolean] state
    def use_axes=(state)
      @use_axes = state ? true : false
    end

    # Determine if axes transitioning is enabled.
    # @return [Boolean]
    def use_axes?
      @use_axes
    end

    # Enable/disable hidden entities transitioning for this scene.
    # @param [Boolean] state
    def use_hidden_entities=(state)
      @use_hidden_entities = state ? true : false
    end

    # Determine if hidden entities transitioning is enabled.
    # @return [Boolean]
    def use_hidden_entities?
      @use_hidden_entities
    end

    # Enable/disable hidden layers transitioning for this scene.
    # @param [Boolean] state
    def use_hidden_layers=(state)
      @use_hidden_layers = state ? true : false
    end

    # Determine if hidden layers transitioning is enabled.
    # @return [Boolean]
    def use_hidden_layers?
      @use_hidden_layers
    end

    # Enable/disable rendering options transitioning for this scene.
    # @param [Boolean] state
    def use_rendering_options=(state)
      @use_rendering_options = state ? true : false
    end

    # Determine if rendering options transitioning is enabled.
    # @return [Boolean]
    def use_rendering_options?
      @use_rendering_options
    end

    # Enable/disable shadow transitioning for this scene.
    # @param [Boolean] state
    def use_shadow_info=(state)
      @use_shadow_info = state ? true : false
    end

    # Determine if shadow transitioning is enabled.
    # @return [Boolean]
    def use_shadow_info?
      @use_shadow_info
    end

    # Enable/disable style transitioning for this scene.
    # @param [Boolean] state
    def use_style=(state)
      @use_style = state ? true : false
    end

    # Determine if style transitioning is enabled.
    # @return [Boolean]
    def use_style?
      @use_style
    end

    # Enable/disable camera transitioning for this scene.
    # @param [Boolean] state
    def use_camera=(state)
      @use_camera = state ? true : false
    end

    # Determine if camera transitioning is enabled.
    # @return [Boolean]
    def use_camera?
      @use_camera
    end

  end # class SceneData
end # module MSPhysics
