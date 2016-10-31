module MSPhysics

  # @since 1.0.0
  module Replay

    DEFAULT_RECORD_GROUPS = true
    DEFAULT_RECORD_MATERIALS = true
    DEFAULT_RECORD_LAYERS = false
    DEFAULT_RECORD_CAMERA = true
    DEFAULT_RECORD_RENDER = false
    DEFAULT_RECORD_SHADOW = false

    DEFAULT_REPLAY_GROUPS = true
    DEFAULT_REPLAY_MATERIALS = true
    DEFAULT_REPLAY_LAYERS = false
    DEFAULT_REPLAY_CAMERA = true
    DEFAULT_REPLAY_RENDER = false
    DEFAULT_REPLAY_SHADOW = false

    @groups_data = {}
    @materials_data = {}
    @layers_data = {}
    @camera_data = {}
    @render_data = {}
    @shadow_data = {}
    @start_frame = nil
    @end_frame = nil

    @tgroups_data = {}
    @tmaterials_data = {}
    @tlayers_data = {}
    @tcamera_data = {}
    @trender_data = {}
    @tshadow_data = {}
    @tstart_frame = nil
    @tend_frame = nil

    @active = false
    @paused = false
    @reversed = false
    @frame = 0
    @speed = 1
    @animation = nil
    @selected_page = nil

    @record = false

    @record_groups = DEFAULT_RECORD_GROUPS
    @record_materials = DEFAULT_RECORD_MATERIALS
    @record_layers = DEFAULT_RECORD_LAYERS
    @record_camera = DEFAULT_RECORD_CAMERA
    @record_render = DEFAULT_RECORD_RENDER
    @record_shadow = DEFAULT_RECORD_SHADOW

    @replay_groups = DEFAULT_REPLAY_GROUPS
    @replay_materials = DEFAULT_REPLAY_MATERIALS
    @replay_layers = DEFAULT_REPLAY_LAYERS
    @replay_camera = DEFAULT_REPLAY_CAMERA
    @replay_render = DEFAULT_REPLAY_RENDER
    @replay_shadow = DEFAULT_REPLAY_SHADOW

    @image_defaults = {
      :speed        => '1',
      :reversed     => 'No',
      :image_type   => 'png',
      :resolution   => 'Model-Inherited',
      :antialias    => 'Yes',
      :compression  => '0.9',
      :tbackground  => 'No'
    }

    @skp_defaults = {
      :speed        => '1',
      :reversed     => 'No'
    }

    class << self

      # Start replay animation.
      # @param [Boolean] activate_animation
      # @return [Boolean] success
      def start(activate_animation = true)
        return false if @active
        model = Sketchup.active_model
        MSPhysics::Simulation.reset
        @active = true
        # Close active path
        state = true
        while state
          state = model.close_active
        end
        # Wrap operations
        if Sketchup.version.to_i > 6
          model.start_operation('MSPhysics Replay', true)
        else
          model.start_operation('MSPhysics Replay')
        end
        # Record original group data.
        @groups_data.each { |group, data|
          if (group.is_a?(Sketchup::Group) || group.is_a?(Sketchup::ComponentInstance)) && group.valid?
            data[:original] = {
              :transformation => group.transformation,
              :visible        => group.visible?,
              :material       => group.material,
              :layer          => group.layer
            }
            group.transformation = group.transformation # Do this once just to be able to undo.
          elsif data[:definition] && data[:definition].valid? && (data[:instance].nil? || data[:instance].deleted?)
            data[:instance] = model.entities.add_instance(data[:definition], Geom::Transformation.new())
            data[:instance].visible = false
          end
        }
        # Record original material data.
        count = 0
        @materials_data.each { |material, data|
          if material.is_a?(Sketchup::Material) && material.valid?
            t = material.texture
            data[:original] = {
              :color   => material.color,
              :alpha   => material.alpha,
              :texture => t ? t.filename : nil,
              :width   => t ? t.width : nil,
              :height  => t ? t.height : nil
            }
            if Sketchup.version.to_i >= 15
              data[:original][:colorize_type] = material.colorize_type
            end
          elsif (data[:instance].nil? || data[:instance].deleted?)
            data[:instance] = model.materials.add("MSPReplay#{sprintf("%05d", count)}")
            count += 1
          end
        }
        # Record original layer data.
        @layers_data.each { |layer, data|
          if layer.is_a?(Sketchup::Layer) && layer.valid?
            data[:original] = { :visible => layer.visible? }
            data[:original][:color] = layer.color if Sketchup.version.to_i > 13
          end
        }
        # Record original camera data.
        camera = model.active_view.camera
        data = {
          :eye          => camera.eye,
          :target       => camera.target,
          :up           => camera.up,
          :perspective  => camera.perspective?,
          :aspect_ratio => camera.aspect_ratio,
          :xaxis        => camera.xaxis,
          :yaxis        => camera.yaxis,
          :zaxis        => camera.zaxis
        }
        if camera.perspective?
          data[:focal_length] = camera.focal_length
          data[:fov] = camera.fov
          data[:image_width] = camera.image_width
        else
          data[:height] = camera.height
        end
        @camera_data[:original] = data
        # Record original render data.
        data = {}
        Sketchup.active_model.rendering_options.each { |k, v| data[k] = v }
        @render_data[:original] = data
        # Record original shadow data.
        data = {}
        Sketchup.active_model.shadow_info.each { |k, v| data[k] = v }
        @shadow_data[:original] = data
        # Set starting frame
        @frame = @reversed ? @end_frame : @start_frame
        # Save selected page
        @selected_page = model.pages.selected_page
        # Activate animation
        @animation = MSPhysics::Replay::Animation.new
        @animation.activate_tool if activate_animation
        true
      end

      # Stop replay animation, but avoid reseting entity positions.
      # @return [Boolean] success
      def stop
        return false unless @active
        # Reset Variables
        @active = false
        @paused = false
        @reversed = false
        @frame = 0
        # Stop animation
        @animation.deactivate_tool
        # Commit operation
        Sketchup.active_model.commit_operation
        true
      end

      # Stop replay animation and reset entity positions.
      # @return [Boolean] success
      def reset
        return false unless @active
        # Reset Variables
        @active = false
        @paused = false
        @reversed = false
        @frame = 0
        # Shortcuts
        model = Sketchup.active_model
        view = model.active_view
        camera = view.camera
        # Stop animation
        @animation.deactivate_tool
        # Reset selected page
        if @selected_page && @selected_page.valid?
          tt = @selected_page.transition_time
          @selected_page.transition_time = 0
          model.pages.selected_page = @selected_page
          @selected_page.transition_time = tt
          @selected_page = nil
        end
        # Reset group data
        @groups_data.each { |group, data|
          instance = data[:instance]
          data.delete(:instance)
          if (group.is_a?(Sketchup::Group) || group.is_a?(Sketchup::ComponentInstance)) && group.valid?
            orig_data = data[:original]
            next if orig_data.nil?
            group.move!(orig_data[:transformation])
            #~ group.transformation = orig_data[:transformation]
            group.visible = orig_data[:visible] if group.visible? != orig_data[:visible]
            if orig_data[:material].nil?
              group.material = nil if group.material
            elsif orig_data[:material].valid?
              group.material = orig_data[:material] if group.material != orig_data[:material]
            end
            group.layer = orig_data[:layer] if orig_data[:layer].valid? && group.layer != orig_data[:layer]
            data.delete(:original)
          end
          if instance && instance.valid?
            instance.material = nil if instance.material
            instance.erase!
          end
        }
        # Reset material data
        @materials_data.each { |material, data|
          instance = data[:instance]
          data.delete(:instance)
          if material.is_a?(Sketchup::Material) && material.valid?
            orig_data = data[:original]
            next if orig_data.nil?
            material.color = orig_data[:color] if material.color.to_i != orig_data[:color].to_i
            material.alpha = orig_data[:alpha] if material.alpha != orig_data[:alpha]
            if material.texture
              material.texture = orig_data[:texture] if material.texture.filename != orig_data[:texture]
            else
              material.texture = orig_data[:texture] if orig_data[:texture]
            end
            if material.texture && (material.texture.width != orig_data[:width] || material.texture.height != orig_data[:height])
              material.texture.size = [orig_data[:width], orig_data[:height]]
            end
            if Sketchup.version.to_i >= 15 && material.colorize_type != orig_data[:colorize_type]
              material.colorize_type = orig_data[:colorize_type]
            end
            data.delete(:original)
          end
          if instance && instance.valid? && Sketchup.version.to_i >= 14
            model.materials.remove(instance)
          end
        }
        if Sketchup.version.to_i < 14
          model.materials.purge_unused
        end
        # Reset layer data
        @layers_data.each { |layer, data|
          next if !(layer.is_a?(Sketchup::Layer) && layer.valid?)
          orig_data = data[:original]
          next if orig_data.nil?
          layer.visible = orig_data[:visible] if layer.visible? != orig_data[:visible]
          if Sketchup.version.to_i > 13 && layer.color.to_i != orig_data[:color].to_i
            layer.color = orig_data[:color]
          end
          data.delete(:original)
        }
        # Reset camera data
        data = @camera_data[:original]
        if data
          camera.set(data[:eye], data[:target], data[:up])
          camera.perspective = data[:perspective]
          camera.aspect_ratio = data[:aspect_ratio]
          if camera.perspective?
            camera.focal_length = data[:focal_length]
            camera.fov = data[:fov]
            camera.image_width = data[:image_width]
          else
            camera.height = data[:height]
          end
        end
        @camera_data.delete(:original)
        # Reset render data
        if @render_data[:original]
          @render_data[:original].each { |k, v| model.rendering_options[k] = v if model.rendering_options[k] != v }
        end
        @render_data.delete(:original)
        # Reset shadow data
        if @shadow_data[:original]
          @shadow_data[:original].each { |k, v| model.shadow_info[k] = v if model.shadow_info[k] != v }
        end
        @shadow_data.delete(:original)
        # Commit operation
        model.commit_operation
        true
      end

      # Play replay animation.
      # @return [Boolean] success
      def play
        return false unless @active
        @paused = false
        @animation.activate_tool
        @animation.activate_anim
        true
      end

      # Pause replay animation.
      # @return [Boolean] success
      def pause
        return false unless @active
        @paused = true
        true
      end

      # Pause/Resume replay animation.
      # @return [Boolean] success
      def toggle_play
        return false unless @active
        @paused = !@paused
        unless @paused
          @animation.activate_tool
          @animation.activate_anim
        end
        true
      end

      # Determine whether replay animation is playing.
      # @return [Boolean]
      def playing?
        @active && !@paused
      end

      # Determine whether replay animation is paused.
      # @return [Boolean]
      def paused?
        @active && @paused
      end

      # Reverse replay animation.
      # @param [Boolean] state
      def reversed=(state)
        @reversed = state ? true : false
      end

      # Determine whether replay animation is reversed.
      # @return [Boolean]
      def reversed?
        @reversed
      end

      # Determine whether replay animation is in operation.
      # @return [Boolean]
      def active?
        @active
      end

      # Determine whether recorded data is not empty.
      # @return [Boolean]
      def recorded_data_valid?
        @tgroups_data.size > 0 || @tmaterials_data.size > 0 || @tlayers_data.size > 0 || @tcamera_data.size > 0 || @trender_data.size > 0 || @tshadow_data.size > 0
      end

      # Determine whether active data is not empty.
      # @return [Boolean]
      def active_data_valid?
        @groups_data.size > 0 || @materials_data.size > 0 || @layers_data.size > 0 || @camera_data.size > 0 || @render_data.size > 0 || @shadow_data.size > 0
      end

      # Enable/disable simulation recording.
      # @param [Boolean] state
      def record_enabled=(state)
        @record = state ? true : false
      end

      # Determine whether simulation recording is enabled.
      # @return [Boolean]
      def record_enabled?
        @record
      end

      # Enable/disable groups recording.
      # @param [Boolean] state
      def groups_record_enabled=(state)
        @record_groups = state ? true : false
      end

      # Determine whether groups recording is enabled.
      # @return [Boolean]
      def groups_record_enabled?
        @record_groups
      end

      # Enable/disable materials recording.
      # @param [Boolean] state
      def materials_record_enabled=(state)
        @record_materials = state ? true : false
      end

      # Determine whether materials recording is enabled.
      # @return [Boolean]
      def materials_record_enabled?
        @record_materials
      end

      # Enable/disable layers recording.
      # @param [Boolean] state
      def layers_record_enabled=(state)
        @record_layers = state ? true : false
      end

      # Determine whether layers recording is enabled.
      # @return [Boolean]
      def layers_record_enabled?
        @record_layers
      end

      # Enable/disable camera recording.
      # @param [Boolean] state
      def camera_record_enabled=(state)
        @record_camera = state ? true : false
      end

      # Determine whether camera recording is enabled.
      # @return [Boolean]
      def camera_record_enabled?
        @record_camera
      end

      # Enable/disable render recording.
      # @param [Boolean] state
      def render_record_enabled=(state)
        @record_render = state ? true : false
      end

      # Determine whether render recording is enabled.
      # @return [Boolean]
      def render_record_enabled?
        @record_render
      end

      # Enable/disable shadow recording.
      # @param [Boolean] state
      def shadow_record_enabled=(state)
        @record_shadow = state ? true : false
      end

      # Determine whether shadow recording is enabled.
      # @return [Boolean]
      def shadow_record_enabled?
        @record_shadow
      end

      # Enable/disable groups replay.
      # @param [Boolean] state
      def groups_replay_enabled=(state)
        @replay_groups = state ? true : false
      end

      # Determine whether groups replay is enabled.
      # @return [Boolean]
      def groups_replay_enabled?
        @replay_groups
      end

      # Enable/disable materials replay.
      # @param [Boolean] state
      def materials_replay_enabled=(state)
        @replay_materials = state ? true : false
      end

      # Determine whether materials replay is enabled.
      # @return [Boolean]
      def materials_replay_enabled?
        @replay_materials
      end

      # Enable/disable layers replay.
      # @param [Boolean] state
      def layers_replay_enabled=(state)
        @replay_layers = state ? true : false
      end

      # Determine whether layers replay is enabled.
      # @return [Boolean]
      def layers_replay_enabled?
        @replay_layers
      end

      # Enable/disable camera replay.
      # @param [Boolean] state
      def camera_replay_enabled=(state)
        @replay_camera = state ? true : false
      end

      # Determine whether camera replay is enabled.
      # @return [Boolean]
      def camera_replay_enabled?
        @replay_camera
      end

      # Enable/disable replay of rendering options.
      # @param [Boolean] state
      def render_replay_enabled=(state)
        @replay_render = state ? true : false
      end

      # Determine whether replay of rendering options is enabled.
      # @return [Boolean]
      def render_replay_enabled?
        @replay_render
      end

      # Enable/disable replay of shadow info.
      # @param [Boolean] state
      def shadow_replay_enabled=(state)
        @replay_shadow = state ? true : false
      end

      # Determine whether relay of shadow info is enabled.
      # @return [Boolean]
      def shadow_replay_enabled?
        @replay_shadow
      end

      # Get replay animation frame.
      # @return [Numeric]
      def frame
        @frame
      end

      # Set replay animation frame.
      # @param [Numeric] value
      def frame=(value)
        @frame = value.to_f
      end

      # Get starting frame of all data recorded.
      # @return [Fixnum, nil]
      def start_frame
        @start_frame
      end

      # Get ending frame of all data recorded.
      # @return [Fixnum, nil]
      def end_frame
        @end_frame
      end

      # Get replay animation speed.
      # @return [Numeric]
      def speed
        @speed
      end

      # Set replay animation speed.
      # @param [Numeric] value A value between 0.01 and 10000.
      def speed=(value)
        @speed = sprintf("%.3f", AMS.clamp(value.to_f, 0.01, 10000)).to_f
      end

      # @!visibility private
      def update_data_frame_limits(data, pframe)
        data[:start_frame] = pframe if data[:start_frame].nil? || pframe < data[:start_frame]
        data[:end_frame] = pframe if data[:end_frame].nil? || pframe > data[:end_frame]
        @tstart_frame = pframe if @tstart_frame.nil? || pframe < @tstart_frame
        @tend_frame = pframe if @tend_frame.nil? || pframe > @tend_frame
      end

      # Record group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] group
      # @param [Fixnum] pframe
      def record_group(group, pframe)
        data = @tgroups_data[group]
        unless data
          @tgroups_data[group] = {}
          data = @tgroups_data[group]
          data[:id] = group.entityID
          data[:definition] = AMS::Group.get_definition(group)
        end
        data[pframe] = {
          :transformation => group.transformation,
          :visible        => group.visible?,
          :material       => group.material ? group.material : 0,
          :layer          => group.layer ? group.layer : 0
        }
        update_data_frame_limits(data, pframe)
      end

      # Record all groups.
      # @param [Fixnum] pframe
      def record_groups(pframe)
        Sketchup.active_model.definitions.each { |d|
          d.instances.each { |i|
            next if i.get_attribute('MSPhysics Body', 'Ignore') || i.get_attribute('MSPhysics Joint', 'Type')
            record_group(i, pframe)
          }
        }
      end

      # Record material.
      # @param [Sketchup::Material] material
      # @param [Fixnum] pframe
      def record_material(material, pframe)
        data = @tmaterials_data[material]
        unless data
          @tmaterials_data[material] = {}
          data = @tmaterials_data[material]
          data[:id] = material.entityID
        end
        t = material.texture
        data[pframe] = {
          :color   => material.color,
          :alpha   => material.alpha,
          :texture => t ? t.filename : nil,
          :width   => t ? t.width : nil,
          :height  => t ? t.height : nil
        }
        if Sketchup.version.to_i >= 15
          data[pframe][:colorize_type] = material.colorize_type
        end
        update_data_frame_limits(data, pframe)
      end

      # Record all materials.
      # @param [Fixnum] pframe
      def record_materials(pframe)
        Sketchup.active_model.materials.each { |m|
          record_material(m, pframe)
        }
      end

      # Record layer.
      # @param [Sketchup::Layer] layer
      # @param [Fixnum] pframe
      def record_layer(layer, pframe)
        data = @tlayers_data[layer]
        unless data
          @tlayers_data[layer] = {}
          data = @tlayers_data[layer]
          data[:id] = layer.entityID
        end
        data[pframe] = { :visible => layer.visible? }
        data[pframe][:color] = layer.color if Sketchup.version.to_i > 13
        update_data_frame_limits(data, pframe)
      end

      # Record all layers.
      # @param [Fixnum] pframe
      def record_layers(pframe)
        Sketchup.active_model.layers.each { |l|
          record_layer(l, pframe)
        }
      end

      # Record camera.
      # @param [Fixnum] pframe
      def record_camera(pframe)
        camera = Sketchup.active_model.active_view.camera
        data = {
          :eye          => camera.eye,
          :target       => camera.target,
          :up           => camera.up,
          :perspective  => camera.perspective?,
          :aspect_ratio => camera.aspect_ratio,
          :xaxis        => camera.xaxis,
          :yaxis        => camera.yaxis,
          :zaxis        => camera.zaxis
        }
        if camera.perspective?
          data[:focal_length] = camera.focal_length
          data[:fov] = camera.fov
          data[:image_width] = camera.image_width
        else
          data[:height] = camera.height
        end
        @tcamera_data[pframe] = data
        update_data_frame_limits(@tcamera_data, pframe)
      end

      # Record rendering options.
      # @param [Fixnum] pframe
      def record_render(pframe)
        data = {}
        Sketchup.active_model.rendering_options.each { |k, v| data[k] = v }
        @trender_data[pframe] = data
        update_data_frame_limits(@trender_data, pframe)
      end

      # Record shadow info.
      # @param [Fixnum] pframe
      def record_shadow(pframe)
        data = {}
        Sketchup.active_model.shadow_info.each { |k, v| data[k] = v }
        @tshadow_data[pframe] = data
        update_data_frame_limits(@tshadow_data, pframe)
      end

      # Record groups, camera, materials, layers, render, and shadow based on
      # whether their recording is enabled.
      # @param [Fixnum] pframe
      def record_all(pframe)
        record_groups(pframe) if @record_groups
        record_materials(pframe) if @record_materials
        record_layers(pframe) if @record_layers
        record_camera(pframe) if @record_camera
        record_render(pframe) if @record_render
        record_shadow(pframe) if @record_shadow
      end

      # Activate recorded data.
      def save_recorded_data
        @groups_data.clear
        @materials_data.clear
        @layers_data.clear
        @camera_data.clear
        @render_data.clear
        @shadow_data.clear
        @start_frame = @tstart_frame
        @end_frame = @tend_frame
        # Save groups data
        @tgroups_data.each { |group, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:definition] = data[:definition] if data[:definition]
          gdata[:start_frame] = data[:start_frame]
          gdata[:end_frame] = data[:end_frame]
          data.keys.grep(Fixnum).sort.each { |pframe|
            gdata[pframe] = data[pframe]
          }
          @groups_data[group] = gdata
        }
        # Save materials data
        @tmaterials_data.each { |material, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:definition] = data[:definition] if data[:definition]
          gdata[:start_frame] = data[:start_frame]
          gdata[:end_frame] = data[:end_frame]
          data.keys.grep(Fixnum).sort.each { |pframe|
            gdata[pframe] = data[pframe]
          }
          @materials_data[material] = gdata
        }
        # Save layers data
        @tlayers_data.each { |layer, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:definition] = data[:definition] if data[:definition]
          gdata[:start_frame] = data[:start_frame]
          gdata[:end_frame] = data[:end_frame]
          data.keys.grep(Fixnum).sort.each { |pframe|
            gdata[pframe] = data[pframe]
          }
          @layers_data[layer] = gdata
        }
        # Save camera data
        @camera_data[:start_frame] = @tcamera_data[:start_frame]
        @camera_data[:end_frame] = @tcamera_data[:end_frame]
        @tcamera_data.keys.grep(Fixnum).sort.each { |pframe|
          @camera_data[pframe] = @tcamera_data[pframe]
        }
        # Save render data
        @render_data[:start_frame] = @trender_data[:start_frame]
        @render_data[:end_frame] = @trender_data[:end_frame]
        @trender_data.keys.grep(Fixnum).sort.each { |pframe|
          @render_data[pframe] = @trender_data[pframe]
        }
        # Save shadow data
        @shadow_data[:start_frame] = @tshadow_data[:start_frame]
        @shadow_data[:end_frame] = @tshadow_data[:end_frame]
        @tshadow_data.keys.grep(Fixnum).sort.each { |pframe|
          @shadow_data[pframe] = @tshadow_data[pframe]
        }
        # Flatten all data
        flatten_active_data
      end

      # Clear recorded data.
      def clear_recorded_data
        @tgroups_data.clear
        @tmaterials_data.clear
        @tlayers_data.clear
        @tcamera_data.clear
        @trender_data.clear
        @tshadow_data.clear
        @tstart_frame = nil
        @tend_frame = nil
      end

      # Clear active data.
      def clear_active_data
        @groups_data.clear
        @materials_data.clear
        @layers_data.clear
        @camera_data.clear
        @render_data.clear
        @shadow_data.clear
        @start_frame = nil
        @end_frame = nil
      end

      # Fill in the gaps within all the recorded information.
      def flatten_active_data
        # Flatten groups data
        @groups_data.each { |group, data|
          next if !data[:start_frame] || !data[:end_frame]
          last = {}
          for pframe in data[:start_frame]..data[:end_frame]
            fdata = data[pframe]
            unless fdata
              fdata = {}
              data[pframe] = fdata
            end
            last.each { |k,v|
              fdata[k] = v unless fdata.has_key?(k)
            }
            fdata.each { |k,v| last[k] = v }
          end
        }
        # Flatten materials data
        @materials_data.each { |material, data|
          next if !data[:start_frame] || !data[:end_frame]
          last = {}
          for pframe in data[:start_frame]..data[:end_frame]
            fdata = data[pframe]
            unless fdata
              fdata = {}
              data[pframe] = fdata
            end
            last.each { |k,v|
              fdata[k] = v unless fdata.has_key?(k)
            }
            fdata.each { |k,v| last[k] = v }
          end
        }
        # Flatten layers data
        @layers_data.each { |layer, data|
          next if !data[:start_frame] || !data[:end_frame]
          last = {}
          for pframe in data[:start_frame]..data[:end_frame]
            fdata = data[pframe]
            unless fdata
              fdata = {}
              data[pframe] = fdata
            end
            last.each { |k,v|
              fdata[k] = v unless fdata.has_key?(k)
            }
            fdata.each { |k,v| last[k] = v }
          end
        }
        # Flatten camera data
        if @camera_data[:start_frame] && @camera_data[:end_frame]
          last = {}
          for pframe in @camera_data[:start_frame]..@camera_data[:end_frame]
            fdata = @camera_data[pframe]
            unless fdata
              fdata = {}
              @camera_data[pframe] = fdata
            end
            last.each { |k,v|
              fdata[k] = v unless fdata.has_key?(k)
            }
            fdata.each { |k,v| last[k] = v }
          end
        end
        # Flatten render data
        if @render_data[:start_frame] && @render_data[:end_frame]
          last = {}
          for pframe in @render_data[:start_frame]..@render_data[:end_frame]
            fdata = @render_data[pframe]
            unless fdata
              fdata = {}
              @render_data[pframe] = fdata
            end
            last.each { |k,v|
              fdata[k] = v unless fdata.has_key?(k)
            }
            fdata.each { |k,v| last[k] = v }
          end
        end
        # Flatten shadow data
        if @shadow_data[:start_frame] && @shadow_data[:end_frame]
          last = {}
          for pframe in @shadow_data[:start_frame]..@shadow_data[:end_frame]
            fdata = @shadow_data[pframe]
            unless fdata
              fdata = {}
              @shadow_data[pframe] = fdata
            end
            last.each { |k,v|
              fdata[k] = v unless fdata.has_key?(k)
            }
            fdata.each { |k,v| last[k] = v }
          end
        end
      end

      # Save active data into model.
      # @param [Boolean] wrap_in_op Whether to wrap in operation.
      # @return [Boolean] success
      def save_data_to_model(wrap_in_op = true)
        op_started = false
        return false unless active_data_valid?
        model = Sketchup.active_model
        dict = 'MSPhysics Replay'
        # Notify
        Sketchup.status_text = "Saving Replay data to model. This might take a while..."
        # Start operation
        if wrap_in_op
          if Sketchup.version.to_i > 6
            model.start_operation('Saving MSPhysics Replay', true)
          else
            model.start_operation('Saving MSPhysics Replay')
          end
          op_started = true
        end
        # Create temporary variables to store unique information.
        fgroups_data = {}
        fmaterials_data = {}
        flayers_data = {}
        fcamera_data = {}
        frender_data = {}
        fshadow_data = {}
        # Filter groups data
        @groups_data.each { |group, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:definition] = data[:definition] if data[:definition]
          gdata[:start_frame] = data[:start_frame] if data[:start_frame]
          gdata[:end_frame] = data[:end_frame] if data[:end_frame]
          last = {}
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            sdata = {}
            fdata.each { |k, v|
              if v.is_a?(Sketchup::Material) || v.is_a?(Sketchup::Layer)
                v2 = v.__id__
              elsif v.is_a?(Geom::Transformation)
                v2 = v.to_a
              else
               v2 = v
              end
              next if last.has_key?(k) && last[k] == v2
              sdata[k] = v
              last[k] = v2
            }
            gdata[pframe] = sdata unless sdata.empty?
          }
          fgroups_data[group] = gdata
        }
        # Filter materials data
        @materials_data.each { |material, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:start_frame] = data[:start_frame] if data[:start_frame]
          gdata[:end_frame] = data[:end_frame] if data[:end_frame]
          last = {}
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            sdata = {}
            fdata.each { |k, v|
              v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
              next if last.has_key?(k) && last[k] == v2
              sdata[k] = v
              last[k] = v2
            }
            gdata[pframe] = sdata unless sdata.empty?
          }
          fmaterials_data[material] = gdata
        }
        # Filter layers data
        @layers_data.each { |layer, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:start_frame] = data[:start_frame] if data[:start_frame]
          gdata[:end_frame] = data[:end_frame] if data[:end_frame]
          last = {}
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            sdata = {}
            fdata.each { |k, v|
              v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
              next if last.has_key?(k) && last[k] == v2
              sdata[k] = v
              last[k] = v2
            }
            gdata[pframe] = sdata unless sdata.empty?
          }
          flayers_data[layer] = gdata
        }
        # Filter camera data
        fcamera_data[:start_frame] = @camera_data[:start_frame] if @camera_data[:start_frame]
        fcamera_data[:end_frame] = @camera_data[:end_frame] if @camera_data[:end_frame]
        last = {}
        @camera_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          sdata = {}
          fdata.each { |k, v|
            next if last.has_key?(k) && last[k] == v
            sdata[k] = v
            last[k] = v
          }
          fcamera_data[pframe] = sdata unless sdata.empty?
        }
        # Filter render data
        frender_data[:start_frame] = @render_data[:start_frame] if @render_data[:start_frame]
        frender_data[:end_frame] = @render_data[:end_frame] if @render_data[:end_frame]
        last = {}
        @render_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          sdata = {}
          fdata.each { |k, v|
            v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
            next if last.has_key?(k) && last[k] == v2
            sdata[k] = v
            last[k] = v2
          }
          frender_data[pframe] = sdata unless sdata.empty?
        }
        # Filter shadow data
        fshadow_data[:start_frame] = @shadow_data[:start_frame] if @shadow_data[:start_frame]
        fshadow_data[:end_frame] = @shadow_data[:end_frame] if @shadow_data[:end_frame]
        last = {}
        @shadow_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          sdata = {}
          fdata.each { |k, v|
            v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
            next if last.has_key?(k) && last[k] == v2
            sdata[k] = v
            last[k] = v2
          }
          fshadow_data[pframe] = sdata unless sdata.empty?
        }
        # Save main info
        model.set_attribute(dict, 'Start Frame', @start_frame)
        model.set_attribute(dict, 'End Frame', @end_frame)
        # Save groups
        gz = Zlib::Deflate.new(Zlib::DEFAULT_COMPRESSION, Zlib::MAX_WBITS, Zlib::MAX_MEM_LEVEL, Zlib::FILTERED)
        gz << "{"
        fgroups_data.each { |group, data|
          next if !(((group.is_a?(Sketchup::Group) || group.is_a?(Sketchup::ComponentInstance)) && group.valid?) || (data[:definition] && data[:definition].valid?))
          gz << "#{data[:id]}=>{"
          gz << ":start_frame=>#{data[:start_frame]}," if data[:start_frame]
          gz << ":end_frame=>#{data[:end_frame]}," if data[:end_frame]
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            gz << "#{pframe}=>{"
            fdata.each { |k, v|
              res = nil
              if v.is_a?(Length)
                res = v.to_f.to_s
              elsif v.is_a?(Numeric)
                res = v.to_s
              elsif v.is_a?(Sketchup::Layer)
                if flayers_data[v]
                  res = flayers_data[v][:id].to_s
                elsif v.valid?
                  res = v.entityID.to_s
                else
                  next
                end
              elsif v.is_a?(Sketchup::Material)
                if fmaterials_data[v]
                  res = fmaterials_data[v][:id].to_s
                elsif v.valid?
                  res = v.entityID.to_s
                else
                  next
                end
              elsif v.is_a?(Sketchup::Color)
                res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
              elsif v.is_a?(Geom::Transformation)
                res = "Geom::Transformation.new(#{v.to_a.inspect})"
              elsif v.is_a?(Geom::Point3d)
                res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
              elsif v.is_a?(Geom::Vector3d)
                res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
              else
                res = v.inspect
              end
              #res.gsub!(/\s/, '')
              res.gsub!('Infinity', '1.0/0.0')
              res.gsub!('NaN', '0.0/0.0')
              gz << "#{k.inspect}=>#{res},"
            }
            gz << "},"
          }
          gz << "},"
          if (group.is_a?(Sketchup::Group) || group.is_a?(Sketchup::ComponentInstance)) && group.valid?
            group.set_attribute(dict, 'ID', data[:id])
          else
            ids = data[:definition].get_attribute(dict, 'IDs')
            if ids.is_a?(Array)
              ids << data[:id]
            else
              ids = [data[:id]]
            end
            data[:definition].set_attribute(dict, 'IDs', ids)
          end
        }
        gz << "}"
        res = gz.flush(Zlib::FINISH)
        chop_count = 0
        while(res.size % 4 != 0)
          res << "\0"
          chop_count += 1
        end
        model.set_attribute(dict, 'Groups Data', res.unpack('l*'))
        model.set_attribute(dict, 'Groups Data Chop Count', chop_count)
        gz.close
        # Save materials
        saved_mats = {}
        gz = Zlib::Deflate.new(Zlib::DEFAULT_COMPRESSION, Zlib::MAX_WBITS, Zlib::MAX_MEM_LEVEL, Zlib::FILTERED)
        gz << "{"
        fmaterials_data.each { |material, data|
          if material.is_a?(Sketchup::Material) && material.valid?
            material.set_attribute(dict, 'ID', data[:id])
            saved_mats[material] = true
          end
          gz << "#{data[:id]}=>{"
          gz << ":start_frame=>#{data[:start_frame]}," if data[:start_frame]
          gz << ":end_frame=>#{data[:end_frame]}," if data[:end_frame]
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            gz << "#{pframe}=>{"
            fdata.each { |k, v|
              res = nil
              if v.is_a?(Length)
                res = v.to_f.to_s
              elsif v.is_a?(Numeric)
                res = v.to_s
              elsif v.is_a?(Sketchup::Color)
                res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
              else
                res = v.inspect
              end
              #res.gsub!(/\s/, '')
              res.gsub!('Infinity', '1.0/0.0')
              res.gsub!('NaN', '0.0/0.0')
              gz << "#{k.inspect}=>#{res},"
            }
            gz << "},"
          }
          gz << "},"
        }
        gz << "}"
        res = gz.flush(Zlib::FINISH)
        chop_count = 0
        while(res.size % 4 != 0)
          res << "\0"
          chop_count += 1
        end
        model.set_attribute(dict, 'Materials Data', res.unpack('l*'))
        model.set_attribute(dict, 'Materials Data Chop Count', chop_count)
        gz.close
        # Save layers
        saved_lays = {}
        gz = Zlib::Deflate.new(Zlib::DEFAULT_COMPRESSION, Zlib::MAX_WBITS, Zlib::MAX_MEM_LEVEL, Zlib::FILTERED)
        gz << "{"
        flayers_data.each { |layer, data|
          if layer.is_a?(Sketchup::Layer) && layer.valid?
            layer.set_attribute(dict, 'ID', data[:id])
            saved_lays[layer] = true
          end
          gz << "#{data[:id]}=>{"
          gz << ":start_frame=>#{data[:start_frame]}," if data[:start_frame]
          gz << ":end_frame=>#{data[:end_frame]}," if data[:end_frame]
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            gz << "#{pframe}=>{"
            fdata.each { |k, v|
              res = nil
              if v.is_a?(Length)
                res = v.to_f.to_s
              elsif v.is_a?(Numeric)
                res = v.to_s
              elsif v.is_a?(Sketchup::Color)
                res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
              else
                res = v.inspect
              end
              #res.gsub!(/\s/, '')
              res.gsub!('Infinity', '1.0/0.0')
              res.gsub!('NaN', '0.0/0.0')
              gz << "#{k.inspect}=>#{res},"
            }
            gz << "},"
          }
          gz << "},"
        }
        gz << "}"
        res = gz.flush(Zlib::FINISH)
        chop_count = 0
        while(res.size % 4 != 0)
          res << "\0"
          chop_count += 1
        end
        model.set_attribute(dict, 'Layers Data', res.unpack('l*'))
        model.set_attribute(dict, 'Layers Data Chop Count', chop_count)
        gz.close
        # Save camera
        gz = Zlib::Deflate.new(Zlib::DEFAULT_COMPRESSION, Zlib::MAX_WBITS, Zlib::MAX_MEM_LEVEL, Zlib::FILTERED)
        gz << "{"
        gz << ":start_frame=>#{fcamera_data[:start_frame]}," if fcamera_data[:start_frame]
        gz << ":end_frame=>#{fcamera_data[:end_frame]}," if fcamera_data[:end_frame]
        fcamera_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          gz << "#{pframe}=>{"
          fdata.each { |k, v|
            res = nil
            if v.is_a?(Length)
              res = v.to_f.to_s
            elsif v.is_a?(Numeric)
              res = v.to_s
            elsif v.is_a?(Geom::Point3d)
              res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            elsif v.is_a?(Geom::Vector3d)
              res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            else
              res = v.inspect
            end
            #res.gsub!(/\s/, '')
            res.gsub!('Infinity', '1.0/0.0')
            res.gsub!('NaN', '0.0/0.0')
            gz << "#{k.inspect}=>#{res},"
          }
          gz << "},"
        }
        gz << "}"
        res = gz.flush(Zlib::FINISH)
        chop_count = 0
        while(res.size % 4 != 0)
          res << "\0"
          chop_count += 1
        end
        model.set_attribute(dict, 'Camera Data', res.unpack('l*'))
        model.set_attribute(dict, 'Camera Data Chop Count', chop_count)
        gz.close
        # Save render
        gz = Zlib::Deflate.new(Zlib::DEFAULT_COMPRESSION, Zlib::MAX_WBITS, Zlib::MAX_MEM_LEVEL, Zlib::FILTERED)
        gz << "{"
        gz << ":start_frame=>#{frender_data[:start_frame]}," if frender_data[:start_frame]
        gz << ":end_frame=>#{frender_data[:end_frame]}," if frender_data[:end_frame]
        frender_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          gz << "#{pframe}=>{"
          fdata.each { |k, v|
            res = nil
            if v.is_a?(Length)
              res = v.to_f.to_s
            elsif v.is_a?(Numeric)
              res = v.to_s
            elsif v.is_a?(Sketchup::Color)
              res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
            elsif v.is_a?(Geom::Point3d)
              res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            elsif v.is_a?(Geom::Vector3d)
              res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            else
              res = v.inspect
            end
            #res.gsub!(/\s/, '')
            res.gsub!('Infinity', '1.0/0.0')
            res.gsub!('NaN', '0.0/0.0')
            gz << "#{k.inspect}=>#{res},"
          }
          gz << "},"
        }
        gz << "}"
        res = gz.flush(Zlib::FINISH)
        chop_count = 0
        while(res.size % 4 != 0)
          res << "\0"
          chop_count += 1
        end
        model.set_attribute(dict, 'Render Data', res.unpack('l*'))
        model.set_attribute(dict, 'Render Data Chop Count', chop_count)
        gz.close
        # Save shadow
        gz = Zlib::Deflate.new(Zlib::DEFAULT_COMPRESSION, Zlib::MAX_WBITS, Zlib::MAX_MEM_LEVEL, Zlib::FILTERED)
        gz << "{"
        gz << ":start_frame=>#{fshadow_data[:start_frame]}," if fshadow_data[:start_frame]
        gz << ":end_frame=>#{fshadow_data[:end_frame]}," if fshadow_data[:end_frame]
        fshadow_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          gz << "#{pframe}=>{"
          fdata.each { |k, v|
            res = nil
            if v.is_a?(Length)
              res = v.to_f.to_s
            elsif v.is_a?(Numeric)
              res = v.to_s
            elsif v.is_a?(Time)
              next
            elsif v.is_a?(Sketchup::Color)
              res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
            elsif v.is_a?(Geom::Point3d)
              res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            elsif v.is_a?(Geom::Vector3d)
              res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            else
              res = v.inspect
            end
            #res.gsub!(/\s/, '')
            res.gsub!('Infinity', '1.0/0.0')
            res.gsub!('NaN', '0.0/0.0')
            gz << "#{k.inspect}=>#{res},"
          }
          gz << "},"
        }
        gz << "}"
        res = gz.flush(Zlib::FINISH)
        chop_count = 0
        while(res.size % 4 != 0)
          res << "\0"
          chop_count += 1
        end
        model.set_attribute(dict, 'Shadow Data', res.unpack('l*'))
        model.set_attribute(dict, 'Shadow Data Chop Count', chop_count)
        gz.close
        # Return success
        return true
      ensure
        # End operation
        model.commit_operation if op_started
      end

      # Load saved data from model.
      # @return [Boolean] success
      def load_data_from_model
        model = Sketchup.active_model
        dict = 'MSPhysics Replay'
        # Notify
        Sketchup.status_text = "Loading Replay data from model. This might take a while..."
        # Clear active data
        @groups_data.clear
        @materials_data.clear
        @layers_data.clear
        @camera_data.clear
        @render_data.clear
        @shadow_data.clear
        # Load general info
        sframe = model.get_attribute(dict, 'Start Frame')
        eframe = model.get_attribute(dict, 'End Frame')
        @start_frame = sframe.is_a?(Fixnum) ? sframe : nil
        @end_frame = eframe.is_a?(Fixnum) ? eframe : nil
        # Load IDs
        id_to_grp = {}
        id_to_def = {}
        id_to_mat = {}
        id_to_lay = {}
        model.entities.each { |e|
          if e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)
            id = e.get_attribute(dict, 'ID')
            id_to_grp[id] = e if id.is_a?(Fixnum)
          end
        }
        model.definitions.each { |d|
          ids = d.get_attribute(dict, 'IDs')
          if ids.is_a?(Array)
            ids.each { |id|
              id_to_def[id] = d if id.is_a?(Fixnum)
            }
          end
        }
        model.materials.each { |mat|
          id = mat.get_attribute(dict, 'ID')
          id_to_mat[id] = mat if id.is_a?(Fixnum)
        }
        model.layers.each { |lay|
          id = lay.get_attribute(dict, 'ID')
          id_to_lay[id] = lay if id.is_a?(Fixnum)
        }
        # Load groups
        data = model.get_attribute(dict, 'Groups Data')
        if data.is_a?(Array)
          chop_count = AMS.clamp(model.get_attribute(dict, 'Groups Data Chop Count', 0).to_i, 0, 3)
          res = data.pack('l*')
          chop_count.times { res.chop! }
          data = eval(Zlib::Inflate.inflate(res))
          data.each { |id, gdata|
            g = id_to_grp[id]
            d = id_to_def[id]
            gdata[:id] = id
            gdata[:definition] = d if d
            gdata.each { |pframe, fdata|
              next unless pframe.is_a?(Fixnum)
              if fdata[:layer].is_a?(Fixnum)
                lay = id_to_lay[fdata[:layer]]
                if lay
                  fdata[:layer] = lay
                else
                  fdata.delete(:layer)
                end
              end
              if fdata[:material].is_a?(Fixnum)
                mat = id_to_mat[fdata[:material]]
                fdata[:material] = mat if mat
              end
            }
            @groups_data[g ? g : id] = gdata
          }
        end
        # Load materials
        data = model.get_attribute(dict, 'Materials Data')
        if data.is_a?(Array)
          chop_count = AMS.clamp(model.get_attribute(dict, 'Materials Data Chop Count', 0).to_i, 0, 3)
          res = data.pack('l*')
          chop_count.times { res.chop! }
          data = eval(Zlib::Inflate.inflate(res))
          data.each { |id, gdata|
            g = id_to_mat[id]
            gdata[:id] = id
            @materials_data[g ? g : id] = gdata
          }
        end
        # Load layers
        data = model.get_attribute(dict, 'Layers Data')
        if data.is_a?(Array)
          chop_count = AMS.clamp(model.get_attribute(dict, 'Layers Data Chop Count', 0).to_i, 0, 3)
          res = data.pack('l*')
          chop_count.times { res.chop! }
          data = eval(Zlib::Inflate.inflate(res))
          data.each { |id, gdata|
            g = id_to_lay[id]
            gdata[:id] = id
            @layers_data[g ? g : id] = gdata
          }
        end
        # Load camera
        data = model.get_attribute(dict, 'Camera Data')
        if data.is_a?(Array)
          chop_count = AMS.clamp(model.get_attribute(dict, 'Camera Data Chop Count', 0).to_i, 0, 3)
          res = data.pack('l*')
          chop_count.times { res.chop! }
          @camera_data = eval(Zlib::Inflate.inflate(res))
        end
        # Load render
        data = model.get_attribute(dict, 'Render Data')
        if data.is_a?(Array)
          chop_count = AMS.clamp(model.get_attribute(dict, 'Render Data Chop Count', 0).to_i, 0, 3)
          res = data.pack('l*')
          chop_count.times { res.chop! }
          @render_data = eval(Zlib::Inflate.inflate(res))
        end
        # Load shadow
        data = model.get_attribute(dict, 'Shadow Data')
        if data.is_a?(Array)
          chop_count = AMS.clamp(model.get_attribute(dict, 'Shadow Data Chop Count', 0).to_i, 0, 3)
          res = data.pack('l*')
          chop_count.times { res.chop! }
          @shadow_data = eval(Zlib::Inflate.inflate(res))
        end
        # Flatten all data
        flatten_active_data
        # Return success
        true
      end

      # Delete saved data from model.
      # @param [Boolean] wrap_in_op Whether to wrap in operation.
      def clear_data_from_model(wrap_in_op = true)
        model = Sketchup.active_model
        # Start operation
        if wrap_in_op
          if Sketchup.version.to_i > 6
            model.start_operation('Clearing MSPhysics Replay', true)
          else
            model.start_operation('Clearing MSPhysics Replay')
          end
        end
        dict = model.attribute_dictionaries['MSPhysics Replay']
        if dict
          dict.delete_key('Groups Data')
          dict.delete_key('Materials Data')
          dict.delete_key('Layers Data')
          dict.delete_key('Camera Data')
          dict.delete_key('Render Data')
          dict.delete_key('Shadow Data')
          dict.delete_key('Start Frame')
          dict.delete_key('End Frame')
          dict.delete_key('Groups Data Chop Count')
          dict.delete_key('Materials Data Chop Count')
          dict.delete_key('Layers Data Chop Count')
          dict.delete_key('Camera Data Chop Count')
          dict.delete_key('Render Data Chop Count')
          dict.delete_key('Shadow Data Chop Count')
        end
        model.definitions.each { |d|
          d.attribute_dictionaries.delete('MSPhysics Replay') if d.attribute_dictionaries
          d.instances.each { |i|
            i.attribute_dictionaries.delete('MSPhysics Replay') if i.attribute_dictionaries
          }
        }
        model.materials.each { |m|
          m.attribute_dictionaries.delete('MSPhysics Replay') if m.attribute_dictionaries
        }
        model.layers.each { |l|
          l.attribute_dictionaries.delete('MSPhysics Replay') if l.attribute_dictionaries
        }
        # End operation
        model.commit_operation if wrap_in_op
      end


      # Smoothen the camera transitioning.
      # @param [Fixnum] interval
      # @return [Boolean] success
      def smooth_camera_data(interval = 40)
        return false if @camera_data.empty?
        pframe = @camera_data[:start_frame]
        return false unless pframe
        oxaxis = X_AXIS
        oyaxis = Y_AXIS
        ozaxis = Z_AXIS
        oeye = ORIGIN
        while true
          sframe = pframe
          sdata = get_frame_data(@camera_data, sframe)
          pframe += interval
          pframe = @camera_data[:end_frame] if pframe > @camera_data[:end_frame]
          eframe = pframe
          edata = get_frame_data(@camera_data, eframe)
          return if eframe - sframe < 2
          oxaxis = sdata[:xaxis] if sdata[:xaxis]
          oyaxis = sdata[:yaxis] if sdata[:yaxis]
          ozaxis = sdata[:zaxis] if sdata[:zaxis]
          oeye = sdata[:eye] if sdata[:eye]
          tra1 = Geom::Transformation.new(oxaxis, ozaxis, oyaxis, oeye)
          tra2 = Geom::Transformation.new(
            edata[:xaxis] ? edata[:xaxis] : oxaxis,
            edata[:zaxis] ? edata[:zaxis] : ozaxis,
            edata[:yaxis] ? edata[:yaxis] : oyaxis,
            edata[:eye] ? edata[:eye] : oeye)
          for n in 0..(eframe - sframe).to_i
            ratio = AMS.clamp(n/(eframe - sframe).to_f, 0.0, 1.0)
            #ratio = -0.5 * Math.cos(ratio * Math::PI) + 0.5
            tra = Geom::Transformation.interpolate(tra1, tra2, ratio)
            data = @camera_data[sframe + n]
            unless data
              data = {}
              @camera_data[sframe + n] = data
            end
            data[:eye] = tra.origin
            data[:target] = tra.origin + tra.yaxis
            data[:up] = tra.zaxis
            data[:xaxis] = tra.xaxis
            data[:yaxis] = tra.zaxis
            data[:zaxis] = tra.yaxis
          end
          break if pframe >= @camera_data[:end_frame]
        end
        true
      end

      # Save active data into file.
      # @param [Boolean] wrap_in_op Whether to wrap in operation.
      # @return [Boolean] success
      def save_data_to_file(wrap_in_op = true)
        op_started = false
        model = Sketchup.active_model
        return false if model.path.empty?
        dict = 'MSPhysics Replay'
        # Parse path to the current model.
        model_path = model.path.gsub(/\\/, '/')
        mspr_path = File.dirname(model_path)
        mspr_name = File.basename(model_path, '.skp') + '.mspreplay'
        mspr_fpath = File.join(mspr_path, mspr_name)
        # Start operation
        if wrap_in_op
          if Sketchup.version.to_i > 6
            model.start_operation('Saving MSPhysics Replay', true)
          else
            model.start_operation('Saving MSPhysics Replay')
          end
          op_started = true
        end
        # Notify
        Sketchup.status_text = "Saving Replay data to file. This might take a while..."
        # Create temporary variables to store unique information.
        fgroups_data = {}
        fmaterials_data = {}
        flayers_data = {}
        fcamera_data = {}
        frender_data = {}
        fshadow_data = {}
        # Filter groups data
        @groups_data.each { |group, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:definition] = data[:definition] if data[:definition]
          gdata[:start_frame] = data[:start_frame] if data[:start_frame]
          gdata[:end_frame] = data[:end_frame] if data[:end_frame]
          last = {}
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            sdata = {}
            fdata.each { |k, v|
              if v.is_a?(Sketchup::Material) || v.is_a?(Sketchup::Layer)
                v2 = v.__id__
              elsif v.is_a?(Geom::Transformation)
                v2 = v.to_a
              else
               v2 = v
              end
              next if last.has_key?(k) && last[k] == v2
              sdata[k] = v
              last[k] = v2
            }
            gdata[pframe] = sdata unless sdata.empty?
          }
          fgroups_data[group] = gdata
        }
        # Filter materials data
        @materials_data.each { |material, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:start_frame] = data[:start_frame] if data[:start_frame]
          gdata[:end_frame] = data[:end_frame] if data[:end_frame]
          last = {}
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            sdata = {}
            fdata.each { |k, v|
              v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
              next if last.has_key?(k) && last[k] == v2
              sdata[k] = v
              last[k] = v2
            }
            gdata[pframe] = sdata unless sdata.empty?
          }
          fmaterials_data[material] = gdata
        }
        # Filter layers data
        @layers_data.each { |layer, data|
          gdata = {}
          gdata[:id] = data[:id]
          gdata[:start_frame] = data[:start_frame] if data[:start_frame]
          gdata[:end_frame] = data[:end_frame] if data[:end_frame]
          last = {}
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            sdata = {}
            fdata.each { |k, v|
              v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
              next if last.has_key?(k) && last[k] == v2
              sdata[k] = v
              last[k] = v2
            }
            gdata[pframe] = sdata unless sdata.empty?
          }
          flayers_data[layer] = gdata
        }
        # Filter camera data
        fcamera_data[:start_frame] = @camera_data[:start_frame] if @camera_data[:start_frame]
        fcamera_data[:end_frame] = @camera_data[:end_frame] if @camera_data[:end_frame]
        last = {}
        @camera_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          sdata = {}
          fdata.each { |k, v|
            next if last.has_key?(k) && last[k] == v
            sdata[k] = v
            last[k] = v
          }
          fcamera_data[pframe] = sdata unless sdata.empty?
        }
        # Filter render data
        frender_data[:start_frame] = @render_data[:start_frame] if @render_data[:start_frame]
        frender_data[:end_frame] = @render_data[:end_frame] if @render_data[:end_frame]
        last = {}
        @render_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          sdata = {}
          fdata.each { |k, v|
            v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
            next if last.has_key?(k) && last[k] == v2
            sdata[k] = v
            last[k] = v2
          }
          frender_data[pframe] = sdata unless sdata.empty?
        }
        # Filter shadow data
        fshadow_data[:start_frame] = @shadow_data[:start_frame] if @shadow_data[:start_frame]
        fshadow_data[:end_frame] = @shadow_data[:end_frame] if @shadow_data[:end_frame]
        last = {}
        @shadow_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          sdata = {}
          fdata.each { |k, v|
            v2 = v.is_a?(Sketchup::Color) ? v.to_a : v
            next if last.has_key?(k) && last[k] == v2
            sdata[k] = v
            last[k] = v2
          }
          fshadow_data[pframe] = sdata unless sdata.empty?
        }
        # Create a Zlib writer instance
        #gz = Zlib::GzipWriter.open(mspr_fpath, Zlib::BEST_SPEED, Zlib::FILTERED)
        gz = File.open(mspr_fpath, 'w')
        # Save main info
        gz.puts "{:start_frame=>#{@start_frame}}" if @start_frame
        gz.puts "{:end_frame=>#{@end_frame}}" if @end_frame
        # Save groups
        fgroups_data.each { |group, data|
          next if !(((group.is_a?(Sketchup::Group) || group.is_a?(Sketchup::ComponentInstance)) && group.valid?) || (data[:definition] && data[:definition].valid?))
          str = "{:groups_data=>{"
          str << "#{data[:id]}=>{"
          str << ":start_frame=>#{data[:start_frame]}," if data[:start_frame]
          str << ":end_frame=>#{data[:end_frame]}," if data[:end_frame]
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            str << "#{pframe}=>{"
            fdata.each { |k, v|
              res = nil
              if v.is_a?(Length)
                res = v.to_f.to_s
              elsif v.is_a?(Numeric)
                res = v.to_s
              elsif v.is_a?(Sketchup::Layer)
                if flayers_data[v]
                  res = flayers_data[v][:id].to_s
                elsif v.valid?
                  res = v.entityID.to_s
                else
                  next
                end
              elsif v.is_a?(Sketchup::Material)
                if fmaterials_data[v]
                  res = fmaterials_data[v][:id].to_s
                elsif v.valid?
                  res = v.entityID.to_s
                else
                  next
                end
              elsif v.is_a?(Sketchup::Color)
                res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
              elsif v.is_a?(Geom::Transformation)
                res = "Geom::Transformation.new(#{v.to_a.inspect})"
              elsif v.is_a?(Geom::Point3d)
                res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
              elsif v.is_a?(Geom::Vector3d)
                res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
              else
                res = v.inspect
              end
              #res.gsub!(/\s/, '')
              res.gsub!('Infinity', '1.0/0.0')
              res.gsub!('NaN', '0.0/0.0')
              str << "#{k.inspect}=>#{res},"
            }
            str << "},"
          }
          str << "}}}"
          gz.puts str
          if (group.is_a?(Sketchup::Group) || group.is_a?(Sketchup::ComponentInstance)) && group.valid?
            group.set_attribute(dict, 'ID', data[:id])
          else
            ids = data[:definition].get_attribute(dict, 'IDs')
            if ids.is_a?(Array)
              ids << data[:id]
            else
              ids = [data[:id]]
            end
            data[:definition].set_attribute(dict, 'IDs', ids)
          end
        }
        # Save materials
        saved_mats = {}
        str = "{:materials_data=>{"
        fmaterials_data.each { |material, data|
          if material.is_a?(Sketchup::Material) && material.valid?
            material.set_attribute(dict, 'ID', data[:id])
            saved_mats[material] = true
          end
          str << "#{data[:id]}=>{"
          str << ":start_frame=>#{data[:start_frame]}," if data[:start_frame]
          str << ":end_frame=>#{data[:end_frame]}," if data[:end_frame]
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            str << "#{pframe}=>{"
            fdata.each { |k, v|
              res = nil
              if v.is_a?(Length)
                res = v.to_f.to_s
              elsif v.is_a?(Numeric)
                res = v.to_s
              elsif v.is_a?(Sketchup::Color)
                res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
              else
                res = v.inspect
              end
              #res.gsub!(/\s/, '')
              res.gsub!('Infinity', '1.0/0.0')
              res.gsub!('NaN', '0.0/0.0')
              str << "#{k.inspect}=>#{res},"
            }
            str << "},"
          }
          str << "},"
        }
        str << "}}"
        gz.puts str
        # Save layers
        saved_lays = {}
        str = "{:layers_data=>{"
        flayers_data.each { |layer, data|
          if layer.is_a?(Sketchup::Layer) && layer.valid?
            layer.set_attribute(dict, 'ID', data[:id])
            saved_lays[layer] = true
          end
          str << "#{data[:id]}=>{"
          str << ":start_frame=>#{data[:start_frame]}," if data[:start_frame]
          str << ":end_frame=>#{data[:end_frame]}," if data[:end_frame]
          data.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            str << "#{pframe}=>{"
            fdata.each { |k, v|
              res = nil
              if v.is_a?(Length)
                res = v.to_f.to_s
              elsif v.is_a?(Numeric)
                res = v.to_s
              elsif v.is_a?(Sketchup::Color)
                res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
              else
                res = v.inspect
              end
              #res.gsub!(/\s/, '')
              res.gsub!('Infinity', '1.0/0.0')
              res.gsub!('NaN', '0.0/0.0')
              str << "#{k.inspect}=>#{res},"
            }
            str << "},"
          }
          str << "},"
        }
        str << "}}"
        gz.puts str
        # Save camera
        str = "{:camera_data=>{"
        str << ":start_frame=>#{fcamera_data[:start_frame]}," if fcamera_data[:start_frame]
        str << ":end_frame=>#{fcamera_data[:end_frame]}," if fcamera_data[:end_frame]
        fcamera_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          str << "#{pframe}=>{"
          fdata.each { |k, v|
            res = nil
            if v.is_a?(Length)
              res = v.to_f.to_s
            elsif v.is_a?(Numeric)
              res = v.to_s
            elsif v.is_a?(Geom::Point3d)
              res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            elsif v.is_a?(Geom::Vector3d)
              res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            else
              res = v.inspect
            end
            #res.gsub!(/\s/, '')
            res.gsub!('Infinity', '1.0/0.0')
            res.gsub!('NaN', '0.0/0.0')
            str << "#{k.inspect}=>#{res},"
          }
          str << "},"
        }
        str << "}}"
        gz.puts str
        # Save render
        str = "{:render_data=>{"
        str << ":start_frame=>#{frender_data[:start_frame]}," if frender_data[:start_frame]
        str << ":end_frame=>#{frender_data[:end_frame]}," if frender_data[:end_frame]
        frender_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          str << "#{pframe}=>{"
          fdata.each { |k, v|
            res = nil
            if v.is_a?(Length)
              res = v.to_f.to_s
            elsif v.is_a?(Numeric)
              res = v.to_s
            elsif v.is_a?(Sketchup::Color)
              res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
            elsif v.is_a?(Geom::Point3d)
              res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            elsif v.is_a?(Geom::Vector3d)
              res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            else
              res = v.inspect
            end
            #res.gsub!(/\s/, '')
            res.gsub!('Infinity', '1.0/0.0')
            res.gsub!('NaN', '0.0/0.0')
            str << "#{k.inspect}=>#{res},"
          }
          str << "},"
        }
        str << "}}"
        gz.puts str
        # Save shadow
        str = "{:shadow_data=>{"
        str << ":start_frame=>#{fshadow_data[:start_frame]}," if fshadow_data[:start_frame]
        str << ":end_frame=>#{fshadow_data[:end_frame]}," if fshadow_data[:end_frame]
        fshadow_data.each { |pframe, fdata|
          next unless pframe.is_a?(Fixnum)
          str << "#{pframe}=>{"
          fdata.each { |k, v|
            res = nil
            if v.is_a?(Length)
              res = v.to_f.to_s
            elsif v.is_a?(Numeric)
              res = v.to_s
            elsif v.is_a?(Time)
              next
            elsif v.is_a?(Sketchup::Color)
              res = "Sketchup::Color.new(#{v.red},#{v.green},#{v.blue},#{v.alpha})"
            elsif v.is_a?(Geom::Point3d)
              res = "Geom::Point3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            elsif v.is_a?(Geom::Vector3d)
              res = "Geom::Vector3d.new(#{v.x.to_f},#{v.y.to_f},#{v.z.to_f})"
            else
              res = v.inspect
            end
            #res.gsub!(/\s/, '')
            res.gsub!('Infinity', '1.0/0.0')
            res.gsub!('NaN', '0.0/0.0')
            str << "#{k.inspect}=>#{res},"
          }
          str << "},"
        }
        str << "}}"
        gz.puts str
        # Close the Zlib writer instance
        gz.close
        # Return success
        return true
      ensure
        # End operation
        model.commit_operation if op_started
      end

      # Load saved data from file.
      # @return [Boolean] success
      def load_data_from_file
        model = Sketchup.active_model
        return false if model.path.empty?
        dict = 'MSPhysics Replay'
        # Parse path to the current model.
        model_path = model.path.gsub(/\\/, '/')
        mspr_path = File.dirname(model_path)
        mspr_name = File.basename(model_path, '.skp') + '.mspreplay'
        mspr_fpath = File.join(mspr_path, mspr_name)
        # Return if file doesn't exist.
        return false unless File.exists?(mspr_fpath)
        # Notify
        Sketchup.status_text = "Loading Replay data from file. This might take a while..."
        # Clear active data
        @groups_data.clear
        @materials_data.clear
        @layers_data.clear
        @camera_data.clear
        @render_data.clear
        @shadow_data.clear
        # Create a Zlib reader instance
        #gz = Zlib::GzipReader.open(mspr_fpath)
        #begin
          #mspr_data = eval(gz.read)
        #ensure
          #gz.close
        #end
        mspr_data = {}
        gz = File.open(mspr_fpath, 'r')
        begin
          gz.each_line { |line|
            res = eval(line)
            res.each { |k, v|
              if mspr_data[k]
                v.each { |sk, sv| mspr_data[k][sk] = sv }
              else
                mspr_data[k] = v
              end
            }
          }
        ensure
          gz.close
        end
        # Load general info
        @start_frame = mspr_data[:start_frame].is_a?(Fixnum) ? mspr_data[:start_frame] : nil
        @end_frame = mspr_data[:end_frame].is_a?(Fixnum) ? mspr_data[:end_frame] : nil
        # Load IDs
        id_to_grp = {}
        id_to_def = {}
        id_to_mat = {}
        id_to_lay = {}
        model.entities.each { |e|
          if e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)
            id = e.get_attribute(dict, 'ID')
            id_to_grp[id] = e if id.is_a?(Fixnum)
          end
        }
        model.definitions.each { |d|
          ids = d.get_attribute(dict, 'IDs')
          if ids.is_a?(Array)
            ids.each { |id|
              id_to_def[id] = d if id.is_a?(Fixnum)
            }
          end
        }
        model.materials.each { |mat|
          id = mat.get_attribute(dict, 'ID')
          id_to_mat[id] = mat if id.is_a?(Fixnum)
        }
        model.layers.each { |lay|
          id = lay.get_attribute(dict, 'ID')
          id_to_lay[id] = lay if id.is_a?(Fixnum)
        }
        # Load groups
        mspr_data[:groups_data].each { |id, gdata|
          g = id_to_grp[id]
          d = id_to_def[id]
          gdata[:id] = id
          gdata[:definition] = d if d
          gdata.each { |pframe, fdata|
            next unless pframe.is_a?(Fixnum)
            if fdata[:layer].is_a?(Fixnum)
              lay = id_to_lay[fdata[:layer]]
              if lay
                fdata[:layer] = lay
              else
                fdata.delete(:layer)
              end
            end
            if fdata[:material].is_a?(Fixnum)
              mat = id_to_mat[fdata[:material]]
              fdata[:material] = mat if mat
            end
          }
          @groups_data[g ? g : id] = gdata
        }
        # Load materials
        mspr_data[:materials_data].each { |id, gdata|
          g = id_to_mat[id]
          gdata[:id] = id
          @materials_data[g ? g : id] = gdata
        }
        # Load layers
        mspr_data[:layers_data].each { |id, gdata|
          g = id_to_lay[id]
          gdata[:id] = id
          @layers_data[g ? g : id] = gdata
        }
        # Load camera
        @camera_data = mspr_data[:camera_data]
        # Load render
        @render_data = mspr_data[:render_data]
        # Load shadow
        @shadow_data = mspr_data[:shadow_data]
        # Flatten all data
        flatten_active_data
        # Return success
        true
      end

      # Clear saved data from file.
      # @note This attempts to delete the file if permissions are granted.
      # @return [Boolean] success
      def clear_data_from_file
        model = Sketchup.active_model
        return false if model.path.empty?
        dict = 'MSPhysics Replay'
        # Parse path to the current model.
        model_path = model.path.gsub(/\\/, '/')
        mspr_path = File.dirname(model_path)
        mspr_name = File.basename(model_path, '.skp') + '.mspreplay'
        mspr_fpath = File.join(mspr_path, mspr_name)
        # Return if file doesn't exist.
        return false unless File.exists?(mspr_fpath)
        begin
          File.delete(mspr_fpath)
        rescue
          File.open(mspr_fpath, 'w') { |f| f.truncate(0) }
        rescue
          return false
        end
        # Return success
        true
      end

      # Get data at a particular frame.
      # @param [Hash] data
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_frame_data(data, pframe)
        pframe = pframe.to_i
        fdata = data[pframe]
        return fdata if fdata
        ld = nil
        data.each { |f, d|
          next if !f.is_a?(Fixnum)
          return d if pframe < f
          return ld if pframe == f
          ld = d
        }
        ld
      end

      # Get group/component data at a particular frame.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] group
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_group_data(group, pframe)
        data = @groups_data[group]
        return unless data
        get_frame_data(data, pframe)
      end

      # Get material data at a particular frame.
      # @param [Sketchup::Material] material
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_material_data(material, pframe)
        data = @materials_data[group]
        return unless data
        get_frame_data(data, pframe)
      end

      # Get layer data at a particular frame.
      # @param [Sketchup::Layer] layer
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_layer_data(layer, pframe)
        data = @layers_data[layer]
        return unless data
        get_frame_data(data, pframe)
      end

      # Get camera data at a particular frame.
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_camera_data(pframe)
        get_frame_data(@camera_data, pframe)
      end

      # Get render data at a particular frame.
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_render_data(pframe)
        get_frame_data(@render_data, pframe)
      end

      # Get shadow data at a particular frame.
      # @param [Fixnum] pframe
      # @return [Hash, nil]
      def get_shadow_data(pframe)
        get_frame_data(@shadow_data, pframe)
      end

      # Determine whether groups data is not empty.
      # @return [Boolean]
      def groups_data_valid?
        @groups_data.size > 0
      end

      # Determine whether materials data is not empty.
      # @return [Boolean]
      def materials_data_valid?
        @materials_data.size > 0
      end

      # Determine whether layers data is not empty.
      # @return [Boolean]
      def layers_data_valid?
        @layers_data.size > 0
      end

      # Determine whether camera data is not empty.
      # @return [Boolean]
      def camera_data_valid?
        @camera_data[:start_frame] && @camera_data[:end_frame] ? true : false
      end

      # Determine whether render data is not empty.
      # @return [Boolean]
      def render_data_valid?
        @render_data[:start_frame] && @render_data[:end_frame] ? true : false
      end

      # Determine whether shadow data is not empty.
      # @return [Boolean]
      def shadow_data_valid?
        @shadow_data[:start_frame] && @shadow_data[:end_frame] ? true : false
      end

      # Activate frame data.
      # @param [Fixnum] pframe
      # @return [Boolean] success
      def activate_frame(pframe)
        return false unless active_data_valid?
        model = Sketchup.active_model
        pframe = AMS.clamp(pframe.to_i, @start_frame, @end_frame)
        # Activate group data
        if @replay_groups
          @groups_data.each { |entity, data|
            instance = data[:instance]
            if pframe < data[:start_frame] || pframe > data[:end_frame]
              if instance && instance.valid? && instance.visible?
                instance.visible = false
              end
              next
            end
            frame_data = get_frame_data(data, pframe)
            next unless frame_data
            material = frame_data[:material]
            if (entity.is_a?(Sketchup::Group) || entity.is_a?(Sketchup::ComponentInstance)) && entity.valid?
              entity.move!(frame_data[:transformation]) if frame_data[:transformation]
              #~ entity.transformation = frame_data[:transformation]
              entity.visible = frame_data[:visible] if frame_data[:visible] != nil && entity.visible? != frame_data[:visible]
              if material
                if material == 0
                  entity.material = nil if entity.material
                elsif material.is_a?(Sketchup::Material) && material.valid?
                  entity.material = material if entity.material != material
                else
                  material_data = @materials_data[material]
                  if material_data && material_data[:instance] && material_data[:instance].valid?
                    entity.material = material_data[:instance] if entity.material != material_data[:instance]
                  end
                end
              end
              entity.layer = frame_data[:layer] if frame_data[:layer] && frame_data[:layer] != 0 && frame_data[:layer].valid? && entity.layer != frame_data[:layer]
            elsif instance && instance.valid? && pframe >= data[:start_frame] && pframe <= data[:end_frame]
              instance.move!(frame_data[:transformation])
              #~ instance.transformation = frame_data[:transformation]
              instance.visible = frame_data[:visible] if frame_data[:visible] != nil && instance.visible? != frame_data[:visible]
              if material
                if material == 0
                  instance.material = nil if instance.material
                elsif material.is_a?(Sketchup::Material) && material.valid?
                  instance.material = material if instance.material != material
                else
                  material_data = @materials_data[material]
                  if material_data && material_data[:instance] && material_data[:instance].valid?
                    instance.material = material_data[:instance] if instance.material != material_data[:instance]
                  end
                end
              end
              instance.layer = frame_data[:layer] if frame_data[:layer] && frame_data[:layer] != 0 && frame_data[:layer].valid? && instance.layer != frame_data[:layer]
            end
          }
        end
        # Activate material data
        if @replay_materials
          @materials_data.each { |material, data|
            frame_data = get_frame_data(data, pframe)
            next if frame_data.nil?
            mat = (material.is_a?(Sketchup::Material) && material.valid?) ? material : (data[:instance] && data[:instance].valid? ? data[:instance] : nil)
            next if mat.nil?
            mat.color = frame_data[:color] if frame_data[:color] && mat.color.to_i != frame_data[:color].to_i
            mat.alpha = frame_data[:alpha] if frame_data[:alpha] && mat.alpha != frame_data[:alpha]
            if mat.texture
              mat.texture = frame_data[:texture] if frame_data[:texture] && mat.texture.filename != frame_data[:texture]
            else
              mat.texture = frame_data[:texture] if frame_data[:texture]
            end
            if mat.texture && frame_data[:width] && frame_data[:height] && (mat.texture.width != frame_data[:width] || mat.texture.height != frame_data[:height])
              mat.texture.size = [frame_data[:width], frame_data[:height]]
            end
            if Sketchup.version.to_i >= 15 && frame_data[:colorize_type] && mat.colorize_type != frame_data[:colorize_type]
              mat.colorize_type = frame_data[:colorize_type]
            end
          }
        end
        # Activate layer data
        if @replay_layers
          @layers_data.each { |layer, data|
            next unless layer.valid?
            frame_data = get_frame_data(data, pframe)
            next if frame_data.nil?
            layer.visible = frame_data[:visible] if frame_data[:visible] != nil && layer.visible? != frame_data[:visible]
            if Sketchup.version.to_i > 13 && frame_data[:color] && layer.color.to_i != frame_data[:color].to_i
              layer.color = frame_data[:color]
            end
          }
        end
        # Activate camera data.
        if @replay_camera
          frame_data = get_frame_data(@camera_data, pframe)
          if frame_data
            camera = model.active_view.camera
            eye = frame_data[:eye] ? frame_data[:eye] : camera.eye
            target = frame_data[:target] ? frame_data[:target] : camera.target
            up = frame_data[:up] ? frame_data[:up] : camera.up
            camera.set(eye, target, up)
            camera.perspective = frame_data[:perspective] if frame_data[:perspective] != nil
            camera.aspect_ratio = frame_data[:aspect_ratio] if frame_data[:aspect_ratio]
            if camera.perspective?
              camera.focal_length = frame_data[:focal_length] if frame_data[:focal_length]
              camera.fov = frame_data[:fov] if frame_data[:fov]
              camera.image_width = frame_data[:image_width] if frame_data[:image_width]
            else
              camera.height = frame_data[:height] if frame_data[:height]
            end
          end
        end
        # Activate render data.
        if @replay_render
          frame_data = get_frame_data(@render_data, pframe)
          if frame_data
            frame_data.each { |k, v| model.rendering_options[k] = v if model.rendering_options[k] != nil && model.rendering_options[k] != v }
          end
        end
        # Activate shadow data.
        if @replay_shadow
          frame_data = get_frame_data(@shadow_data, pframe)
          if frame_data
            frame_data.each { |k, v| model.shadow_info[k] = v if model.shadow_info[k] != nil && model.shadow_info[k] != v }
          end
        end
        true
      end

      # Export animation into image files.
      # @return [Boolean] success
      def export_to_images
        model = Sketchup.active_model
        view = model.active_view
        # Check if animation record is not empty.
        unless active_data_valid?
          UI.messagebox("Nothing to export!")
          return false
        end
        # Display options input box.
        prompts = ['Start Frame', 'End Frame', 'Speed (0.01 - 10000)', 'Reverse', 'Image Type', 'Resolution', 'Anti-alias', 'Compression', 'Transparent Background   ', 'Replay Materials', 'Replay Layers', 'Replay Camera', 'Replay Render', 'Replay Shadow']
        yn = 'Yes|No'
        image_types = 'bmp|jpg|png|tif'
        res = 'Model-Inherited|Custom|320x240|640x480|768x576|800x600|1024x768|1280x720|1280x1024|1920x1080'
        compression = '0.0|0.1|0.2|0.3|0.4|0.5|0.6|0.7|0.8|0.9|1.0'
        drop_downs = ['', '', '', yn, image_types, res, yn, compression, yn, yn, yn, yn, yn, yn]
        #values = [@start_frame, @end_frame, @speed, @reversed ? 'Yes' : 'No', 'png', 'Model-Inherited', 'Yes', '0.9', 'No', @replay_materials ? 'Yes' : 'No', @replay_layers ? 'Yes' : 'No', @replay_camera ? 'Yes' : 'No', @replay_render ? 'Yes' : 'No', @replay_shadow ? 'Yes' : 'No']
        values = [@start_frame, @end_frame, @image_defaults[:speed], @image_defaults[:reversed], @image_defaults[:image_type], @image_defaults[:resolution], @image_defaults[:antialias], @image_defaults[:compression], @image_defaults[:tbackground], @replay_materials ? 'Yes' : 'No', @replay_layers ? 'Yes' : 'No', @replay_camera ? 'Yes' : 'No', @replay_render ? 'Yes' : 'No', @replay_shadow ? 'Yes' : 'No']
        results = UI.inputbox(prompts, values, drop_downs, 'Export Animation Options')
        return false unless results
        # Display Custom resolution input box if desired.
        if results[5] == 'Custom'
          results2 = UI.inputbox(['Width', 'Height'], [800, 600], 'Use Custom Resolution')
          return false unless results2
          w = AMS.clamp(results2[0].to_i, 1, 16000)
          h = AMS.clamp(results2[1].to_i, 1, 16000)
          results[5] = "#{w}x#{h}"
        end
        # Select export path
        model_fname = File.basename(model.path, '.skp')
        model_fname = "msp_anim" if model_fname.empty?
        script_file = UI.savepanel('Choose Export Directory and Name', nil, model_fname)
        return false unless script_file
        fpath = File.dirname(script_file)
        fpath.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
        fname = File.basename(script_file, '.skp')
        # Preset user data
        sframe = results[0].to_i
        eframe = results[1].to_i
        speed = AMS.clamp(results[2].to_f, 0.01, 10000)
        reversed = results[3] == 'Yes'
        opts = {}
        if results[5] != 'Model-Inherited'
          wh = results[5].split('x')
          opts[:width] = wh[0].to_i
          opts[:height] = wh[1].to_i
        end
        opts[:antialias] = results[6] == 'Yes'
        opts[:compression] = results[7].to_f
        opts[:transparent] = results[8] == 'Yes'
        orig_rep_grp = @replay_groups
        orig_rep_mat = @replay_materials
        orig_rep_lay = @replay_layers
        orig_rep_cam = @replay_camera
        orig_rep_ren = @replay_render
        orig_rep_sha = @replay_shadow
        @replay_groups = true
        @replay_materials = results[9] == 'Yes'
        @replay_layers = results[10] == 'Yes'
        @replay_camera = results[11] == 'Yes'
        @replay_render = results[12] == 'Yes'
        @replay_shadow = results[13] == 'Yes'
        # Save defaults
        @image_defaults[:speed] = speed.to_s
        @image_defaults[:reversed] = results[3]
        @image_defaults[:image_type] = results[4]
        @image_defaults[:resolution] = results[5]
        @image_defaults[:antialias] = results[6]
        @image_defaults[:compression] = results[7]
        @image_defaults[:tbackground] = results[8]
        # Export animation
        start_time = Time.now
        called_while_active = @active
        start(false) unless called_while_active
        pframe = reversed ? eframe : sframe
        last_frame = nil
        count = 1
        while(pframe.to_i >= sframe && pframe.to_i <= eframe)
          if pframe.to_i != last_frame
            activate_frame(pframe)
            last_frame = pframe.to_i
          end
          opts[:filename] = "#{fpath}/#{fname}#{sprintf("%04d", count)}.#{results[4]}"
          view.write_image(opts)
          progress = (pframe.to_i - sframe) * 100 / (eframe - sframe).to_f
          Sketchup.status_text = "Exporting MSPhysics Replay Animation    Progress: #{pframe.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
          pframe += reversed ? -speed : speed
          count += 1
        end
        reset unless called_while_active
        # Set original settings
        @replay_groups = orig_rep_grp
        @replay_materials = orig_rep_mat
        @replay_layers = orig_rep_lay
        @replay_camera = orig_rep_cam
        @replay_render = orig_rep_ren
        @replay_shadow = orig_rep_sha
        # Display results
        UI.messagebox("Finished exporting MSPhysics Replay Animation!\n\nExported #{count} frames in #{sprintf("%.2f", Time.now - start_time)} seconds.\n\nYou may use Movie Maker, MakeAVI, or a similar tool to combine all images into a video file.")
        # Return success
        true
      end

      # Export animation into sketchup files.
      # @return [Boolean] success
      def export_to_skp
        model = Sketchup.active_model
        view = model.active_view
        # Check if animation record is not empty.
        unless active_data_valid?
          UI.messagebox("Nothing to export!")
          return false
        end
        # Display options input box.
        prompts = ['Start Frame', 'End Frame', 'Speed (0.01 - 10000)   ', 'Reverse', 'Replay Materials', 'Replay Layers', 'Replay Camera', 'Replay Render', 'Replay Shadow']
        yn = 'Yes|No'
        drop_downs = ['', '', '', yn, yn, yn, yn, yn, yn]
        values = [@start_frame, @end_frame, @skp_defaults[:speed],  @skp_defaults[:reversed], @replay_materials ? 'Yes' : 'No', @replay_layers ? 'Yes' : 'No', @replay_camera ? 'Yes' : 'No', @replay_render ? 'Yes' : 'No', @replay_shadow ? 'Yes' : 'No']
        results = UI.inputbox(prompts, values, drop_downs, 'Export Animation Options')
        return false unless results
        # Select export path
        model_fname = File.basename(model.path, '.skp')
        model_fname = "msp_anim" if model_fname.empty?
        script_file = UI.savepanel('Choose Export Directory and Name', nil, model_fname)
        return false unless script_file
        fpath = File.dirname(script_file)
        fpath.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
        fname = File.basename(script_file, '.skp')
        # Preset user data
        sframe = results[0].to_i
        eframe = results[1].to_i
        speed = AMS.clamp(results[2].to_f, 0.01, 10000)
        reversed = results[3] == 'Yes'
        orig_rep_grp = @replay_groups
        orig_rep_mat = @replay_materials
        orig_rep_lay = @replay_layers
        orig_rep_cam = @replay_camera
        orig_rep_ren = @replay_render
        orig_rep_sha = @replay_shadow
        @replay_groups = true
        @replay_materials = results[4] == 'Yes'
        @replay_layers = results[5] == 'Yes'
        @replay_camera = results[6] == 'Yes'
        @replay_render = results[7] == 'Yes'
        @replay_shadow = results[8] == 'Yes'
        # Save skp defaults
        @skp_defaults[:speed] = speed.to_s
        @skp_defaults[:reversed] = results[3]
        # Export animation
        start_time = Time.now
        called_while_active = @active
        clear_data_from_model(true)
        start(false) unless called_while_active
        pframe = reversed ? eframe : sframe
        last_frame = nil
        count = 1
        while(pframe.to_i >= sframe && pframe.to_i <= eframe)
          if pframe.to_i != last_frame
            activate_frame(pframe)
            last_frame = pframe.to_i
          end
          full_path = "#{fpath}/#{fname}#{sprintf("%04d", count)}.skp"
          if Sketchup.version.to_i < 14
            model.save(full_path)
          else
            if model.path.empty?
              model.save(full_path)
            else
              model.save_copy(full_path)
            end
          end
          progress = (pframe.to_i - sframe) * 100 / (eframe - sframe).to_f
          Sketchup.status_text = "Exporting MSPhysics Replay Animation    Progress: #{pframe.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
          pframe += reversed ? -speed : speed
          count += 1
        end
        reset unless called_while_active
        begin
          save_data_to_file(true)
        rescue Exception => e
          puts 'Failed to save replay data to file!'
        end
        # Set original settings
        @replay_groups = orig_rep_grp
        @replay_materials = orig_rep_mat
        @replay_layers = orig_rep_lay
        @replay_camera = orig_rep_cam
        @replay_render = orig_rep_ren
        @replay_shadow = orig_rep_sha
        # Display results
        UI.messagebox("Finished exporting MSPhysics Replay Animation!\n\nExported #{count} frames in #{sprintf("%.2f", Time.now - start_time)} seconds.")
        # Return success
        true
      end

      # Export animation into Kerkythea files.
      # @return [Boolean] success
      def export_to_kerkythea
        unless defined?(::SU2KT)
          UI.messagebox 'Kerkythea (SU2KT) plugin is not installed.'
          return false
        end
        unless ::SU2KT.respond_to?(:export_msp_animation)
          add_in_script = %q{
def self.export_msp_animation
  model = Sketchup.active_model
  view = model.active_view
  replay = MSPhysics::Replay
  # Check if animation record is not empty.
  unless replay.active_data_valid?
    UI.messagebox("Nothing to export!")
    return false
  end
  SU2KT.reset_global_variables
  # Get first stage settings.
  return false unless SU2KT.export_options_window
  # Get second stage settings.
  render_set, rend_files = SU2KT.get_render_settings
  settings = SU2KT.get_stored_values
  # If file doesn't exist use the first render setting file that was found.
  settings[6] = File.exist?(settings[6]) ? File.basename(settings[6], '.xml') : render_set[0]
  prompts = ['Start Frame', 'End Frame', 'Speed (0.01 - 10000)', 'Replay Camera?', 'Reverse?', 'Animated Lights and Sun?', 'Resolution', 'Render Settings']
  res = %w[Model-Inherited Custom 320x240 640x480 768x576 800x600 1024x768 1280x720 1280x1024 1920x1080].join('|')
  drop_downs = ['', '', '', 'Yes|No', 'Yes|No', 'Yes|No', res, render_set.join('|')]
  values = [
    replay.start_frame,
    replay.end_frame,
    replay.speed,
    replay.camera_replay_enabled? ? 'Yes' : 'No',
    replay.reversed? ? 'Yes' : 'No',
    settings[2],
    settings[5],
    settings[6]
  ]
  results = UI.inputbox(prompts, values, drop_downs, 'Export Animation Options')
  return false unless results
  # Use custom resolution
  if results[6] == 'Custom'
    results2 = UI.inputbox(['Width', 'Height'], [800, 600], 'Use Custom Resolution')
    return false unless results2
    w = AMS.clamp(results2[0].to_i, 1, 16000)
    h = AMS.clamp(results2[1].to_i, 1, 16000)
    results[6] = "#{w}x#{h}"
  end
  # Replace rendering setting with full file path.
  results[7] = rend_files[render_set.index(results[7])]
  # Store new settings.
  settings[2] = results[5]
  settings[5] = results[6]
  settings[6] = results[7]
  SU2KT.store_values(settings)
  # Select export path and create export folder.
  #~ script_file = SU2KT.select_script_path_window
  #~ return false unless script_file
  model_filename = File.basename(model.path)
  if model_filename.empty?
    model_name = 'Untitled.kst'
  else
    model_name = model_filename.split(/\./)[0..-2].join('.') + '.kst'
  end
  script_file = UI.savepanel('Select Export Directory and Name', '', model_name)
  return false unless script_file
  script_file.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
  if script_file == script_file.split(/\./)[0..-2].join('.') # No file extension
    script_file << '.kst'
  end
  @model_name = File.basename(script_file)
  @model_name = @model_name.split(/\./)[0]
  @frames_path = File.dirname(script_file) + @ds + 'Anim_' + File.basename(script_file).split(/\./)[0..-2].join('.')
  Dir.mkdir(@frames_path) unless FileTest.exist?(@frames_path)
  @path_textures = File.dirname(script_file)
  # Optimize values.
  sframe = results[0].to_i
  eframe = results[1].to_i
  speed = AMS.clamp(results[2].to_f, 0.01, 10000)
  reversed = results[4] == 'Yes'
  orig_replay_camera = replay.camera_replay_enabled?
  replay.camera_replay_enabled = results[3] == 'Yes'
  @anim_sun = (results[5] == 'Yes')
  @export_full_frame = true
  @scene_export = true
  @resolution = (results[6] == 'Model-Inherited') ? '4x4' : results[6]
  @instanced = false
  # Create main XML file.
  out_file = script_file.split(/\./)[0..-2].join('.') + '.xml'
  out = File.new(out_file, 'w')
  # Export data to the main XML file.
  #SU2KT.export_global_settings(out)
  SU2KT.export_render_settings(out, results[7])
  SU2KT.find_lights(model.entities, Geom::Transformation.new)
  SU2KT.write_sky(out)
  if @instanced
    SU2KT.export_instanced(out, model.entities)
  else
    SU2KT.export_meshes(out, model.entities)
  end
  SU2KT.export_current_view(model.active_view, out)
  SU2KT.export_lights(out) if @export_lights
  SU2KT.write_sun(out)
  SU2KT.finish_close(out)
  # Update merge settings.
  SU2KT.set_merge_settings
  # Create script file.
  script = File.new(script_file, 'w')
  # Make sure it loads the main XML file.
  script.puts "message \"Load #{out_file}\""
  # Export animation
  start_time = Time.now
  called_while_active = replay.active?
  replay.start(false) unless called_while_active
  pframe = reversed ? eframe : sframe
  last_frame = nil
  count = 1
  while(pframe.to_i >= sframe && pframe.to_i <= eframe)
    if pframe.to_i != last_frame
      replay.activate_frame(pframe)
      last_frame = pframe.to_i
    end
    # Export data to the frame file
    frame_name = sprintf("%04d", count)
    full_path = @frames_path + @ds + frame_name + '.xml'
    script.puts("message \"Merge '#{full_path}' 5 5 4 0 0\"")
    script.puts("message \"Render\"")
    script.puts("message \"SaveImage " + @frames_path + @ds + frame_name + ".jpg\"")
    out = File.new(full_path, 'w')
    SU2KT.export_render_settings(out, settings[6])
    SU2KT.find_lights(model.entities, Geom::Transformation.new)
    SU2KT.write_sky(out)
    SU2KT.collect_faces(model.entities, Geom::Transformation.new)
    SU2KT.export_faces(out)
    SU2KT.export_fm_faces(out)
    SU2KT.export_current_view(model.active_view, out)
    SU2KT.export_lights(out) if @export_lights
    SU2KT.write_sun(out)
    SU2KT.finish_close(out)
    # Display progress
    progress = (pframe.to_i - sframe) * 100 / (eframe - sframe).to_f
    Sketchup.status_text = "Exporting MSPhysics Replay to KT    Progress: #{pframe.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
    # Increment frame and counter
    pframe += reversed ? -speed : speed
    count += 1
  end
  replay.reset unless called_while_active
  # Set original replay settings
  replay.camera_replay_enabled = orig_replay_camera
  # Finalize
  script.close
  # It is important that textures are exported last!
  SU2KT.write_textures
  msg = "Finished exporting MSPhysics Replay Animation!\n\nExported #{count} frames in #{sprintf("%.2f", Time.now - start_time)} seconds.\n\nNow you're left to adjust '#{File.basename(out_file)}' render settings, run '#{File.basename(script_file)}' render script, and combine rendered images using a software, like Windows Movie Maker.\n\nYou may skip adjusting render settings and get to the rendering right away. Would you like to start rendering right now?"
  result = UI.messagebox(msg, MB_YESNO)
  @export_file = script_file # Used by render_animation as the script path.
  # Render animation.
  if result == IDYES
    kt_path = SU2KT.get_kt_path
    return unless kt_path
    kt_path.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
    if AMS::IS_PLATFORM_WINDOWS
      #batch_file_path = File.join(File.dirname(kt_path), 'start.bat')
      batch_file_path = File.join(File.dirname(script_file), "#{File.basename(script_file, '.kst')}_start_render.bat")
      batch = File.new(batch_file_path, 'w')
      batch.puts "start \"\" \"#{kt_path}\" \"#{script_file}\""
      batch.close
      UI.openURL(batch_file_path)
    else # MAC solution
      Thread.new do
        script_file_path = File.join( script_file.split(@ds) )
        system(`#{kt_path} "#{script_file_path}"`)
      end
    end
  end
  SU2KT.reset_global_variables
  # Return success
  true
end}
          ::SU2KT.module_eval(add_in_script, __FILE__, 0)
        end
        ::SU2KT.export_msp_animation
      end

      # Export animation into SkIndigo files.
      # @return [Boolean] success
      def export_to_skindigo
        unless defined?(::SkIndigo)
          UI.messagebox 'SkIndigo plugin is not installed.'
          return false
        end
        unless ::SkIndigo.respond_to?(:export_msp_animation)
          add_in_script = %q{
def self.export_msp_animation
  model = Sketchup.active_model
  view = model.active_view
  replay = MSPhysics::Replay
  # Check if animation record is not empty.
  unless replay.active_data_valid?
    UI.messagebox("Nothing to export!")
    return false
  end
  # Verify the halt time
  if IndigoRenderSettings.new.halt.to_i == -1
    result = UI.messagebox("Warning: Halt time is set to -1. This means that each frame will render forever unless the halt time is set to a value such as 10 (s). Halt time can be set in Advanced tab of the SkIndigo render settings dialog. Press YES to proceed exporting or NO to adjust rendering settings first.", MB_YESNO)
    return false if result == IDNO
  end

  prompts = ['Start Frame', 'End Frame', 'Speed (0.01 - 10000)', 'Replay Camera?', 'Reverse?']
  drop_downs = ['', '', '', 'Yes|No', 'Yes|No']
  values = [
    replay.start_frame,
    replay.end_frame,
    replay.speed,
    replay.camera_replay_enabled? ? 'Yes' : 'No',
    replay.reversed? ? 'Yes' : 'No'
  ]
  results = UI.inputbox(prompts, values, drop_downs, 'Export Animation Options')
  return false unless results

  # Select export path and create export folder.
  model_filename = File.basename(model.path, ".*")
  model_filename.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
  if model_filename.empty?
    queue_filename = 'Untitled.igq'
  else
    queue_filename = model_filename + '.igq'
  end
  batch_file_path = UI.savepanel('Select Export Directory and Name', '', queue_filename)
  return false unless batch_file_path
  batch_file_path.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
  export_dir = File.join(File.dirname(batch_file_path), File.basename(batch_file_path, ".*"))
  Dir.mkdir(export_dir) unless FileTest.exist?(export_dir)
  export_path = File.join(export_dir, File.basename(batch_file_path, ".*")) + '.igs'
  export_path.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/

  ie = ::IndigoExporter.new(export_path)
  ie.settings.save_to_model()
  model_name = File.basename(ie.path, '.igs')
  return false if model_name.empty?
  tex_path = File.join(File.dirname(export_path), 'TX_' + model_name)
  ie.tex_path = 'TX_' + model_name

  # Optimize values.
  sframe = results[0].to_i
  eframe = results[1].to_i
  speed = AMS.clamp(results[2].to_f, 0.01, 10000)
  reversed = results[4] == 'Yes'
  orig_replay_camera = replay.camera_replay_enabled?
  replay.camera_replay_enabled = results[3] == 'Yes'

  print_timings = !SkIndigo.run_tests?

  # Open the shared IGS file to export to.
  shared_igs_path = File.join(File.dirname(ie.path), model_name + '-shared.igs')
  shared_igs_file = File.new(shared_igs_path, 'w')
  shared_igs_file.puts "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
  shared_igs_file.puts "<scenedata>"

  ie.export_default_mat(shared_igs_file)

  mesh_builder = IndigoMeshBuilder.new
  IndigoMeshBuilder.reset # Erases the stored meshes, instances, and spheres from last export

  # Process entities (iterate over scene, get list of components/groups etc.. to export)
  t = Time.now
  Sketchup.set_status_text("Collecting Instances...")
  entities_to_export = Sketchup.active_model.entities
  mesh_builder.process_entities(entities_to_export)
  puts "Traversed model and got instances in #{Time.now - t} seconds."

  # Build required (visible, referenced) meshes
  t = Time.now
  Sketchup.set_status_text("Building Meshes...")
  mesh_builder.build_meshes()
  puts "Built meshes in #{Time.now - t} seconds."

  # Write meshes
  t = Time.now
  Sketchup.set_status_text("Exporting Meshes...")
  ie.export_meshes(mesh_builder, shared_igs_file)
  puts "Exported meshes in #{Time.now - t} seconds."

  ie.get_used_layers()

  # Build a set of the materials that were actually referenced by a model, so that we can just export those materials, and not all materials in the scene.
  referenced_materials = ie.get_referenced_materials(mesh_builder)

  # Export all the materials
  t = Time.now
  Sketchup.set_status_text("Exporting Materials...")
  referenced_materials.each { |mat, generate_uvs|
    ie.export_material(IndigoMaterial.new(mat), shared_igs_file, generate_uvs)
  }
  puts "Exported materials in #{Time.now - t} seconds." if print_timings

  # Write scatters
  ie.export_scatters(mesh_builder, shared_igs_file)

  shared_igs_file.puts "</scenedata>"
  shared_igs_file.close

  # Export textures
  Sketchup.set_status_text("Exporting textures...")
  Dir.mkdir(tex_path) if !FileTest.exist?(tex_path)

  # Export all textures collected during the material export process
  ie.export_textures(ie.textures)

  # Export all collected nkata
  #~ for path in ie.nkdata
  #~   nk = NKdata.new(path)
  #~   nk.export(tex_path) # exports the nkdata to the textures directory
  #~ end

  # Export all collected ies profiles
  for path in ie.ies
    profile = IESProfile.new(path)
    profile.export(tex_path) if profile.valid? # export the IES profiles to the textures directory
  end

  if print_timings
    puts "------ SkIndigo MSPhysics Animation Export Run ------"
    puts "Instances: #{mesh_builder.instances.size}"
    puts "Meshes: #{mesh_builder.num_meshes()}"
    puts "Meshes exported: #{mesh_builder.num_new_meshes_exported}"
    puts "Spheres: #{mesh_builder.spheres.length}"
  end

  # Export animation
  start_time = Time.now
  called_while_active = replay.active?
  replay.start(false) unless called_while_active
  pframe = reversed ? eframe : sframe
  last_frame = nil
  count = 1
  while(pframe.to_i >= sframe && pframe.to_i <= eframe)
    if pframe.to_i != last_frame
      replay.activate_frame(pframe)
      last_frame = pframe.to_i
    end
    # Export data to the frame file
    ie.export_msp_frame(File.dirname(export_path), mesh_builder, shared_igs_path)
    # Display progress
    progress = (pframe.to_i - sframe) * 100 / (eframe - sframe).to_f
    Sketchup.status_text = "Exporting MSPhysics Replay to SkIndigo    Progress: #{pframe.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
    # Increment frame and counter
    pframe += reversed ? -speed : speed
    count += 1
  end
  replay.reset unless called_while_active
  # Set original replay settings
  replay.camera_replay_enabled = orig_replay_camera
  # Finalize
  ie.create_animation_igq_file(batch_file_path)

  msg = "Finished exporting MSPhysics Replay Animation!\n\nExported #{count} frames in #{sprintf("%.2f", Time.now - start_time)} seconds.\n\nNow you're left to adjust '#{File.basename(shared_igs_file)}' render settings, run '#{File.basename(batch_file_path)}' render script, and combine rendered images using a software, like Windows Movie Maker.\n\nYou may skip adjusting render settings and get to the rendering right away. Would you like to start rendering right now?"
  result = UI.messagebox(msg, MB_YESNO)
  # Render animation.
  if result == IDYES
    # Open Indigo for rendering
    is = IndigoRenderSettings.new
    indigo_path = SkIndigo.get_indigo_path
    if indigo_path.nil?
      UI.messagebox "Indigo application not found.  Rendering Aborted."
      return true
    end
    if FileTest.exist?(indigo_path)
      output_param = ""
      case is.network_mode
      when 1 # master
        output_param += "-n m " unless SkIndigo.on_mac?
      when 2 # working master
        output_param += "-n wm " unless SkIndigo.on_mac?
      end
      SkIndigo.launch_indigo(indigo_path, batch_file_path, output_param, is.low_priority?, "normal")
    end
  end
  # Return success
  true
end}
          add_in_script2 = %q{
def export_msp_frame(igs_path, mb, shared_igs_path)
  Dir.mkdir(igs_path) unless FileTest.exist?(igs_path)
  return 0 unless FileTest.exist?(igs_path) # failed to create path

  @next_uid += 100

  # put this in initialization method?
  igs_path = File.basename(self.path)
  model_name = File.basename(igs_path, ".*") # minus the file extension

  # Begin exporting the current frame
  ents = Sketchup.active_model.entities
  #mb = IndigoMeshBuilder.new
  mb.reset_instances
  #mb.build_active_mesh(ents)

  # exported_entity_set = Set.new() # Used to prevent infinite loops on recursive components.
  # mb.get_instances(ents, Geom::Transformation.new, exported_entity_set)
  entities_to_export = Sketchup.active_model.entities
  mb.process_entities(entities_to_export)
  #mb.build_meshes2()

  frame_num = "%04d" % @frame
  frame_path = File.join(File.dirname(self.path), model_name + "-#{frame_num}.igs")
  get_used_layers()
  out = File.new(frame_path, "w")
  out.puts "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
  out.puts "<scene>"
  export_metadata(out)
  export_render_settings(out)
  export_tonemapping(out)
  export_camera(self.model.active_view, out)
  export_environment(out)
  export_default_mat(out)
  mb.spheres.each { |sphere| export_sphere(sphere, out) }

  export_fog(out) if self.model.rendering_options["DisplayFog"]

  out.puts "  <include>"
  out.puts "      <offset_uids>false</offset_uids>"
  out.puts "      <pathname>#{File.basename(shared_igs_path)}</pathname>"
  out.puts "  </include>"
  out.puts ""

  export_instances(mb, out)
  export_light_layer_names(out)
  out.puts "</scene>"
  out.close
  @frame += 1
end}
          ::SkIndigo.module_eval(add_in_script, __FILE__, 0)
          ::IndigoExporter.class_eval(add_in_script2, __FILE__, 0)
        end
        ::SkIndigo.export_msp_animation
      end

      # Export animation into AVI video file.
      # @return [Boolean] success
      def export_to_avi
      end

      # Save replay settings into model dictionary.
      # @param [Boolean] wrap_in_op Whether to wrap in operation.
      def save_replay_settings(wrap_in_op = true)
        model = Sketchup.active_model
        dict = 'MSPhysics Replay'
        if wrap_in_op
          if Sketchup.version.to_i > 6
            model.start_operation('Saving Replay Settings', true)
          else
            model.start_operation('Saving Replay Settings')
          end
        end
        model.set_attribute(dict, 'Record Groups', @record_groups)
        model.set_attribute(dict, 'Record Materials', @record_materials)
        model.set_attribute(dict, 'Record Layers', @record_layers)
        model.set_attribute(dict, 'Record Camera', @record_camera)
        model.set_attribute(dict, 'Record Render', @record_render)
        model.set_attribute(dict, 'Record Shadow', @record_shadow)
        model.set_attribute(dict, 'Replay Groups', @replay_groups)
        model.set_attribute(dict, 'Replay Materials', @replay_materials)
        model.set_attribute(dict, 'Replay Layers', @replay_layers)
        model.set_attribute(dict, 'Replay Camera', @replay_camera)
        model.set_attribute(dict, 'Replay Render', @replay_render)
        model.set_attribute(dict, 'Replay Shadow', @replay_shadow)
        model.commit_operation if wrap_in_op
      end

      # Load replay settings from model dictionary.
      def load_replay_settings
        model = Sketchup.active_model
        dict = 'MSPhysics Replay'
        @record_groups = model.get_attribute(dict, 'Record Group', DEFAULT_RECORD_GROUPS) ? true : false
        @record_materials = model.get_attribute(dict, 'Record Materials', DEFAULT_RECORD_MATERIALS) ? true : false
        @record_layers = model.get_attribute(dict, 'Record Layers', DEFAULT_RECORD_LAYERS) ? true : false
        @record_camera = model.get_attribute(dict, 'Record Camera', DEFAULT_RECORD_CAMERA) ? true : false
        @record_render = model.get_attribute(dict, 'Record Render', DEFAULT_RECORD_RENDER) ? true : false
        @record_shadow = model.get_attribute(dict, 'Record Shadow', DEFAULT_RECORD_SHADOW) ? true : false
        @replay_groups = model.get_attribute(dict, 'Replay Group', DEFAULT_REPLAY_GROUPS) ? true : false
        @replay_materials = model.get_attribute(dict, 'Replay Materials', DEFAULT_REPLAY_MATERIALS) ? true : false
        @replay_layers = model.get_attribute(dict, 'Replay Layers', DEFAULT_REPLAY_LAYERS) ? true : false
        @replay_camera = model.get_attribute(dict, 'Replay Camera', DEFAULT_REPLAY_CAMERA) ? true : false
        @replay_render = model.get_attribute(dict, 'Replay Render', DEFAULT_REPLAY_RENDER) ? true : false
        @replay_shadow = model.get_attribute(dict, 'Replay Shadow', DEFAULT_REPLAY_SHADOW) ? true : false
      end

      # Reset replay settings.
      def reset_replay_settings
        @record_groups = DEFAULT_RECORD_GROUPS
        @record_materials = DEFAULT_RECORD_MATERIALS
        @record_layers = DEFAULT_RECORD_LAYERS
        @record_camera = DEFAULT_RECORD_CAMERA
        @record_render = DEFAULT_RECORD_RENDER
        @record_shadow = DEFAULT_RECORD_SHADOW
        @replay_groups = DEFAULT_REPLAY_GROUPS
        @replay_materials = DEFAULT_REPLAY_MATERIALS
        @replay_layers = DEFAULT_REPLAY_LAYERS
        @replay_camera = DEFAULT_REPLAY_CAMERA
        @replay_render = DEFAULT_REPLAY_RENDER
        @replay_shadow = DEFAULT_REPLAY_SHADOW
      end

      # Load settings and data procedure.
      def load_replay_proc
        MSPhysics::Replay.stop
        MSPhysics::Replay.clear_recorded_data
        MSPhysics::Replay.clear_active_data
        MSPhysics::Replay.reset_replay_settings
        begin
          MSPhysics::Replay.load_replay_settings
          MSPhysics::Replay.load_data_from_file
        rescue Exception => e
          MSPhysics::Replay.clear_recorded_data
          MSPhysics::Replay.clear_active_data
          MSPhysics::Replay.reset_replay_settings
          MSPhysics::Replay.clear_data_from_model(true)
          MSPhysics::Replay.clear_data_from_file
          err_message = e.message
          err_backtrace = e.backtrace
          if RUBY_VERSION !~ /1.8/
            err_message.force_encoding('UTF-8')
            err_backtrace.each { |i| i.force_encoding('UTF-8') }
          end
          msg = "An error occurred while loading MSPhysics Replay data. You might be getting this error because the saved Replay data is irrelevant to the current MSPhysics version.\n#{e.class}:\n#{err_message}\nBacktrace:\n#{err_backtrace.join("\n")}\n"
          ::UI.messagebox(msg)
          puts msg
        end
      end

      # @!visibility private
      def init
        Sketchup.add_observer(MSPhysics::Replay::AppObserver.new)
        MSPhysics::Replay.load_replay_proc
      end

    end # class << self

    # @!visibility private
    class AppObserver < ::Sketchup::AppObserver

      def onNewModel(model)
        MSPhysics::Replay.load_replay_proc if model
      end

      def onOpenModel(model)
        MSPhysics::Replay.load_replay_proc if model
      end

    end # class AppObserver

    # @!visibility private
    class Animation

      def initialize
        @last_frame = nil
        @tool_active = false
        @anim_active = false
      end

      def tool_active?
        @tool_active
      end

      def anim_active?
        @anim_active
      end

      def activate_tool
        Sketchup.active_model.select_tool(self) unless @tool_active
      end

      def deactivate_tool
        Sketchup.active_model.select_tool(nil) if @tool_active
      end

      def activate_anim
        Sketchup.active_model.active_view.animation = self unless @anim_active
        @anim_active = true
        Sketchup.active_model.active_view.show_frame
      end

      def deactivate_anim
        Sketchup.active_model.active_view.animation = nil if @anim_active
      end

      def activate
        @tool_active = true
        activate_anim
      end

      def deactivate(view)
        @tool_active = false
        deactivate_anim
      end

      def resume(view)
        activate_anim
      end

      def suspend(view)
      end

      def stop
        @anim_active = false
        MSPhysics::Replay.pause
      end

      def onLButtonDown(flags, x, y, view)
      end

      def onLButtonUp(flags, x, y, view)
      end

      def getExtents
        bb = Sketchup.active_model.bounds
        if Sketchup.version.to_i > 6
          Sketchup.active_model.entities.each { |e|
            next if e.is_a?(Sketchup::Text)
            bb.add(e.bounds) if e.visible?
          }
        end
        bb
      end

      def nextFrame(view)
        @anim_active = true unless @anim_active
        replay = MSPhysics::Replay
        return false unless replay.active_data_valid?
        Sketchup.status_text = "MSPhysics Replay    Frame: #{replay.frame.to_i}    Speed: #{sprintf("%.2f", replay.speed)}    Reversed: #{replay.reversed? ? 'Yes' : 'No'}    Start Frame: #{replay.start_frame}    End Frame: #{replay.end_frame}"
        if replay.paused? || (replay.frame.to_i <= replay.start_frame && replay.reversed?) || (replay.frame.to_i >= replay.end_frame && !replay.reversed?)
          view.show_frame if view
          return true
        end
        replay.frame += replay.reversed? ? -replay.speed : replay.speed
        if replay.frame.to_i != @last_frame
          replay.activate_frame(replay.frame)
          @last_frame = replay.frame.to_i
        end
        view.show_frame if view
        return true
      end

    end # class Animation
  end # module Replay
end # module MSPhysics
