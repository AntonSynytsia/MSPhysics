module MSPhysics

  # @since 1.0.0
  module Replay

    @groups_data = {}
    @materials_data = {}
    @styles_data = {}
    @layers_data = {}
    @camera_data = {}
    @shadow_data = {}
    @start_frame = nil
    @end_frame = nil

    @tgroups_data = {}
    @tmaterials_data = {}
    @tstyles_data = {}
    @tlayers_data = {}
    @tcamera_data = {}
    @tshadow_data = {}
    @tstart_frame = nil
    @tend_frame = nil

    @active = false
    @paused = false
    @reversed = false
    @frame = 0
    @speed = 1
    @animation = nil

    @record = false
    @replay_groups = true
    @replay_materials = true
    @replay_styles = false
    @replay_layers = false
    @replay_camera = false
    @replay_shadow = false

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
          if group.valid?
            data[:original] = {
              :transformation => group.transformation,
              :visible        => group.visible?,
              :material       => group.material
            }
            group.transformation = group.transformation # Do this once just to be able to undo.
          elsif data[:definition] && data[:definition].valid?
            data[:instance] = model.entities.add_instance(data[:definition], Geom::Transformation.new())
            data[:instance].visible = false
          end
        }
        # Record original material data.
        @materials_data.each { |material, data|
          if material.valid?
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
          else
            data[:instance] = model.materials.add("MSPReplay#{1000 + rand(9000)}")
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
        # Set starting frame
        @frame = @reversed ? @end_frame : @start_frame
        # Activate animation
        @animation = MSPhysics::Replay::Animation.new
        if activate_animation
          model.active_view.animation = @animation
        end
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
        Sketchup.active_model.active_view.animation = nil
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
        view.animation = nil
        # Reset group data
        @groups_data.each { |group, data|
          instance = data[:instance]
          if group.valid?
            orig_data = data[:original]
            next if orig_data.nil?
            group.move!(orig_data[:transformation])
            group.visible = orig_data[:visible] if group.visible? != orig_data[:visible]
            if orig_data[:material].nil?
              group.material = nil if group.material
            elsif orig_data[:material].valid?
              group.material = orig_data[:material] if group.material != orig_data[:material]
            end
          end
          if instance && instance.valid?
            instance.material = nil if instance.material
            instance.erase!
          end
        }
        # Reset material data
        @materials_data.each { |material, data|
          instance = data[:instance]
          if material.valid?
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
          end
          if instance && instance.valid? && Sketchup.version.to_i >= 14
            model.materials.remove(instance)
          end
        }
        if Sketchup.version.to_i < 14
          model.materials.purge_unused
        end
        # Reset camera data
        data = @camera_data[:original]
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
        # Commit operation
        model.commit_operation
        true
      end

      # Play replay animation.
      # @return [Boolean] success
      def play
        return false unless @active
        @paused = false
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
        @tgroups_data.size > 0 || @tmaterials_data.size > 0 || @tstyles_data.size > 0 || @tlayers_data.size > 0 || @tcamera_data.size > 0 || @tshadow_data.size> 0
      end

      # Determine whether active data is not empty.
      # @return [Boolean]
      def active_data_valid?
        @groups_data.size > 0 || @materials_data.size > 0 || @styles_data.size > 0 || @layers_data.size > 0 || @camera_data.size > 0 || @shadow_data.size> 0
      end

      # Enable/Disable simulation recording.
      # @param [Boolean] state
      def record_enabled=(state)
        @record = state ? true : false
      end

      # Determine whether simulation recording is enabled.
      # @return [Boolean]
      def record_enabled?
        @record
      end

      # Enable/Disable groups replay.
      # @param [Boolean] state
      def groups_replay_enabled=(state)
        @replay_groups = state ? true : false
      end

      # Determine whether groups replay is enabled.
      # @return [Boolean]
      def groups_replay_enabled?
        @replay_groups
      end

      # Enable/Disable materials replay.
      # @param [Boolean] state
      def materials_replay_enabled=(state)
        @replay_materials = state ? true : false
      end

      # Determine whether materials replay is enabled.
      # @return [Boolean]
      def materials_replay_enabled?
        @replay_materials
      end

      # Enable/Disable styles replay.
      # @param [Boolean] state
      def styles_replay_enabled=(state)
        @replay_styles = state ? true : false
      end

      # Determine whether styles replay is enabled.
      # @return [Boolean]
      def styles_replay_enabled?
        @replay_styles
      end

      # Enable/Disable layers replay.
      # @param [Boolean] state
      def layers_replay_enabled=(state)
        @replay_layers = state ? true : false
      end

      # Determine whether layers replay is enabled.
      # @return [Boolean]
      def layers_replay_enabled?
        @replay_layers
      end

      # Enable/Disable camera replay.
      # @param [Boolean] state
      def camera_replay_enabled=(state)
        @replay_camera = state ? true : false
      end

      # Determine whether camera replay is enabled.
      # @return [Boolean]
      def camera_replay_enabled?
        @replay_camera
      end

      # Enable/Disable shadow replay.
      # @param [Boolean] state
      def shadow_replay_enabled=(state)
        @replay_shadow = state ? true : false
      end

      # Determine whether shadow replay is enabled.
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
      def update_data_frame_limits(data, frame)
        frame = frame.to_i
        data[:start_frame] = frame if data[:start_frame].nil? || frame < data[:start_frame]
        data[:end_frame] = frame if data[:end_frame].nil? || frame > data[:end_frame]
        @tstart_frame = frame if @tstart_frame.nil? || frame < @tstart_frame
        @tend_frame = frame if @tend_frame.nil? || frame > @tend_frame
      end

      # Record group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] group
      # @param [Fixnum] frame
      def record_group(group, frame)
        frame = frame.to_i
        data = @tgroups_data[group]
        unless data
          @tgroups_data[group] = {}
          data = @tgroups_data[group]
        end
        unless data[:definition]
          data[:definition] = MSPhysics::Group.get_definition(group)
        end
        data[frame] = {
          :transformation => group.transformation,
          :visible        => group.visible?,
          :material       => group.material,
        }
        update_data_frame_limits(data, frame)
      end

      # Record all groups.
      # @param [Fixnum] frame
      def record_groups(frame)
        Sketchup.active_model.definitions.each { |d|
          d.instances.each { |i| record_group(i, frame) }
        }
      end

      # Record camera.
      # @param [Fixnum] frame
      def record_camera(frame)
        frame = frame.to_i
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
        @tcamera_data[frame] = data
        update_data_frame_limits(data, frame)
      end

      # Record material.
      # @param [Sketchup::Material] material
      # @param [Fixnum] frame
      def record_material(material, frame)
        frame = frame.to_i
        data = @tmaterials_data[material]
        unless data
          @tmaterials_data[material] = {}
          data = @tmaterials_data[material]
        end
        t = material.texture
        data[frame] = {
          :color   => material.color,
          :alpha   => material.alpha,
          :texture => t ? t.filename : nil,
          :width   => t ? t.width : nil,
          :height  => t ? t.height : nil
        }
        if Sketchup.version.to_i >= 15
          data[frame][:colorize_type] = material.colorize_type
        end
        update_data_frame_limits(data, frame)
      end

      # Record all materials.
      # @param [Fixnum] frame
      def record_materials(frame)
        Sketchup.active_model.materials.each { |m|
          record_material(m, frame)
        }
      end

      # Record style.
      # @param [Sketchup::Style] style
      # @param [Fixnum] frame
      def record_style(style, frame)
      end

      # Record all styles.
      # @param [Fixnum] frame
      def record_styles(frame)
      end

      # Record layer.
      # @param [Sketchup::Layer] layer
      # @param [Fixnum] frame
      def record_layer(layer, frame)
      end

      # Record all layers.
      # @param [Fixnum] frame
      def record_layers(frame)
      end

      # Record shadow.
      # @param [Fixnum] frame
      def record_shadow(frame)
      end

      # Record groups, camera, materials, styles, layers, and shadow.
      # @param [Fixnum] frame
      def record_all(frame)
        record_groups(frame)
        record_materials(frame)
        record_styles(frame)
        record_layers(frame)
        record_camera(frame)
        record_shadow(frame)
      end

      # Activate recorded data.
      def save_recorded_data
        @groups_data = @tgroups_data
        @materials_data = @tmaterials_data
        @styles_data = @tstyles_data
        @layers_data = @tlayers_data
        @camera_data = @tcamera_data
        @shadow_data = @tshadow_data
        @start_frame = @tstart_frame
        @end_frame = @tend_frame
      end

      # Clear recorded data.
      def clear_recorded_data
        @tgroups_data = {}
        @tmaterials_data = {}
        @tstyles_data = {}
        @tlayers_data = {}
        @tcamera_data = {}
        @tshadow_data = {}
        @tstart_frame = nil
        @tend_frame = nil
      end

      # Save active data into attribute dictionary.
      def save_active_data
      end

      # Clear active data.
      def clear_active_data
        @groups_data = {}
        @materials_data = {}
        @styles_data = {}
        @layers_data = {}
        @camera_data = {}
        @shadow_data = {}
        @start_frame = nil
        @end_frame = nil
      end

      # Make camera data smooth.
      # @param [Fixnum] interval
      # @return [Boolean] success
      def smooth_camera_data1(interval = 40)
        return false if @camera_data.empty?
        frame = @start_frame
        while true
          sframe = frame
          sdata = get_camera_data(sframe)
          frame += interval
          frame = @end_frame if frame > @end_frame
          eframe = frame
          edata = get_camera_data(eframe)
          return if eframe - sframe < 2
          tra1 = Geom::Transformation.new(sdata[:xaxis], sdata[:zaxis], sdata[:yaxis], sdata[:eye])
          tra2 = Geom::Transformation.new(edata[:xaxis], edata[:zaxis], edata[:yaxis], edata[:eye])
          for n in 0..(eframe - sframe).to_i
            ratio = AMS.clamp(n/(eframe - sframe).to_f, 0.0, 1.0)
            #ratio = -0.5 * Math.cos(ratio * Math::PI) + 0.5
            tra = Geom::Transformation.interpolate(tra1, tra2, ratio)
            data = @camera_data[sframe + n]
            data[:eye] = tra.origin
            data[:target] = tra.origin + tra.yaxis
            data[:up] = tra.zaxis
            data[:xaxis] = tra.xaxis
            data[:yaxis] = tra.zaxis
            data[:zaxis] = tra.yaxis
          end
          break if frame >= @end_frame
        end
        true
      end

      # Make camera data smooth.
      # @param [Fixnum] interval
      # @return [Boolean] success
      def smooth_camera_data2(interval = 25)
        return false if @camera_data.empty?
        tcam_data = {}
        @camera_data.each { |frame, data|
          next unless frame.is_a?(Fixnum)
          s = AMS.clamp(frame - interval, @start_frame, @end_frame)
          e = AMS.clamp(frame + interval, @start_frame, @end_frame)
          count = 1
          ndata = {
            :eye    => Geom::Point3d.new(data[:eye]),
            :target => Geom::Point3d.new(data[:target]),
            :up     => Geom::Vector3d.new(data[:up])
          }
          for n in s..e
            next if n == frame
            data2 = @camera_data[n]
            next unless data2
            for i in 0..2
              ndata[:eye][i] += data2[:eye][i]
              ndata[:target][i] += data2[:target][i]
              ndata[:up][i] += data2[:up][i]
            end
            count += 1
          end
          ratio = 1.0/count
          for i in 0..2
            ndata[:eye][i] *= ratio
            ndata[:target][i] *= ratio
            ndata[:up][i] *= ratio
          end
          tcam_data[frame] = ndata
          frame += 1
        }
        tcam_data.each { |frame, ndata|
          data = @camera_data[frame]
          next unless data
          data[:eye] = ndata[:eye]
          data[:target] = ndata[:target]
          data[:up] = ndata[:up]
        }
        tcam_data.clear
        true
      end

      # Get group/component data at a particular frame.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] group
      # @param [Fixnum] frame
      # @return [Hash, nil]
      def get_group_data(group, frame)
        frame = frame.to_i
        data = @groups_data[group]
        return unless data
        return data[frame] if data[frame]
        last = nil
        data.each { |f, fdata|
          next unless f.is_a?(Fixnum)
          return last if last != nil && f > frame
          last = fdata
        }
        return last
      end

      # Get material data at a particular frame.
      # @param [Sketchup::Material] material
      # @param [Fixnum] frame
      # @return [Hash, nil]
      def get_material_data(material, frame)
        frame = frame.to_i
        data = @materials_data[material]
        return unless data
        return data[frame] if data[frame]
        last = nil
        data.each { |f, fdata|
          next unless f.is_a?(Fixnum)
          return last if last != nil && f > frame
          last = fdata
        }
        return last
      end

      # Get style data at a particular frame.
      # @param [Sketchup::Style] style
      # @param [Fixnum] frame
      # @return [Hash, nil]
      def get_style_data(style, frame)
        frame = frame.to_i
        data = @styles_data[style]
        return unless data
        return data[frame] if data[frame]
        last = nil
        data.each { |f, fdata|
          next unless f.is_a?(Fixnum)
          return last if last != nil && f > frame
          last = fdata
        }
        return last
      end

      # Get layer data at a particular frame.
      # @param [Sketchup::Layer] layer
      # @param [Fixnum] frame
      # @return [Hash, nil]
      def get_layer_data(layer, frame)
        frame = frame.to_i
        data = @layers_data[layer]
        return unless data
        return data[frame] if data[frame]
        last = nil
        data.each { |f, fdata|
          next unless f.is_a?(Fixnum)
          return last if last != nil && f > frame
          last = fdata
        }
        return last
      end

      # Get camera data at a particular frame.
      # @param [Fixnum] frame
      # @return [Hash, nil]
      def get_camera_data(frame)
        frame = frame.to_i
        return @camera_data[frame] if @camera_data[frame]
        last = nil
        @camera_data.each { |f, fdata|
          next unless f.is_a?(Fixnum)
          return last if last != nil && f > frame
          last = fdata
        }
        return last
      end

      # Get shadow data at a particular frame.
      # @param [Fixnum] frame
      # @return [Hash, nil]
      def get_shadow_data(frame)
        frame = frame.to_i
        return @shadow_data[frame] if @shadow_data[frame]
        last = nil
        @shadow_data.each { |f, fdata|
          next unless f.is_a?(Fixnum)
          return last if last != nil && f > frame
          last = fdata
        }
        return last
      end

      # Activate frame data.
      # @param [Fixnum] frame
      # @return [Boolean] success
      def activate_frame(frame)
        return false unless active_data_valid?
        frame = frame.to_i
        # Activate group data
        if @replay_groups
          @groups_data.each { |entity, data|
            frame_data = data[frame]
            instance = data[:instance]
            if frame_data.nil?
              if instance && instance.valid? && instance.visible? && (frame < data[:start_frame] || frame > data[:end_frame])
                instance.visible = false
              end
              next
            end
            material = frame_data[:material]
            if entity.valid?
              entity.move!(frame_data[:transformation])
              entity.visible = frame_data[:visible] if entity.visible? != frame_data[:visible]
              if material.nil?
                entity.material = nil if entity.material
              elsif material.valid?
                entity.material = material if entity.material != material
              else
                material_data = @materials_data[material]
                if material_data && material_data[:instance] && material_data[:instance].valid?
                  entity.material = material_data[:instance] if entity.material != material_data[:instance]
                end
              end
            elsif instance && instance.valid? && frame >= data[:start_frame] && frame <= data[:end_frame]
              instance.move!(frame_data[:transformation])
              instance.visible = frame_data[:visible] if instance.visible? != frame_data[:visible]
              if material.nil?
                instance.material = nil if instance.material
              elsif material.valid?
                instance.material = material if instance.material != material
              else
                material_data = @materials_data[material]
                if material_data && material_data[:instance] && material_data[:instance].valid?
                  instance.material = material_data[:instance] if instance.material != material_data[:instance]
                end
              end
            end
          }
        end
        # Activate material data
        if @replay_materials
          @materials_data.each { |material, data|
            frame_data = data[frame]
            next if frame_data.nil?
            mat = material.valid? ? material : (data[:instance].valid? ? data[:instance] : nil)
            next if mat.nil?
            mat.color = frame_data[:color] if mat.color.to_i != frame_data[:color].to_i
            mat.alpha = frame_data[:alpha] if mat.alpha != frame_data[:alpha]
            if mat.texture
              mat.texture = frame_data[:texture] if mat.texture.filename != frame_data[:texture]
            else
              mat.texture = frame_data[:texture] if frame_data[:texture]
            end
            if mat.texture && (mat.texture.width != frame_data[:width] || mat.texture.height != frame_data[:height])
              mat.texture.size = [frame_data[:width], frame_data[:height]]
            end
            if Sketchup.version.to_i >= 15 && mat.colorize_type != frame_data[:colorize_type]
              mat.colorize_type = frame_data[:colorize_type]
            end
          }
        end
        # Activate camera data.
        if @replay_camera && @camera_data[frame]
          data = @camera_data[frame]
          camera = Sketchup.active_model.active_view.camera
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
        prompts = ['Start Frame', 'End Frame', 'Speed (0.01 - 10000)', 'Replay Camera', 'Reverse', 'Image Type', 'Resolution', 'Anti-alias', 'Compression', 'Transparent Background   ']
        yes_no = 'Yes|No'
        image_types = 'bmp|jpg|png|tif'
        res = 'Model-Inherited|Custom|320x240|640x480|768x576|800x600|1024x768|1280x720|1280x1024|1920x1080'
        compression = '0.0|0.1|0.2|0.3|0.4|0.5|0.6|0.7|0.8|0.9|1.0'
        drop_downs = ['', '', '', yes_no, yes_no, image_types, res, yes_no, compression, yes_no]
        values = [@start_frame, @end_frame, @speed, @replay_camera ? 'Yes' : 'No', @reversed ? 'Yes' : 'No', 'png', 'Model-Inherited', 'Yes', '0.9', 'No']
        results = UI.inputbox(prompts, values, drop_downs, 'Export Animation Options')
        return false unless results
        # Display Custom resolution input box if desired.
        if results[6] == 'Custom'
          results2 = UI.inputbox(['Width', 'Height'], [800, 600], 'Use Custom Resolution')
          return false unless results2
          w = AMS.clamp(results2[0].to_i, 1, 16000)
          h = AMS.clamp(results2[1].to_i, 1, 16000)
          results[6] = "#{w}x#{h}"
        end
        # Select export path
        model_fname = File.basename(model.path, '.skp')
        model_fname = "msp_anim" if model_fname.empty?
        script_file = UI.savepanel('Choose Export Directory and Name', nil, model_fname)
        return false unless script_file
        fpath = File.dirname(script_file)
        fname = File.basename(script_file, '.skp')
        # Preset user data
        sframe = results[0].to_i
        eframe = results[1].to_i
        speed = AMS.clamp(results[2].to_f, 0.01, 10000)
        reversed = results[4] == 'Yes'
        orig_replay_camera = @replay_camera
        @replay_camera = results[3] == 'Yes'
        opts = {}
        if results[6] != 'Model-Inherited'
          wh = results[6].split('x')
          opts[:width] = wh[0].to_i
          opts[:height] = wh[1].to_i
        end
        opts[:antialias] = results[7] == 'Yes'
        opts[:compression] = results[8].to_f
        opts[:transparent] = results[9] == 'Yes'
        # Export animation
        start_time = Time.now
        called_while_active = @active
        start(false) unless called_while_active
        frame = reversed ? eframe : sframe
        last_frame = nil
        count = 1
        while(frame.to_i >= sframe && frame.to_i <= eframe)
          if frame.to_i != last_frame
            activate_frame(frame)
            last_frame = frame.to_i
          end
          opts[:filename] = "#{fpath}/#{fname}#{sprintf("%06d", count)}.#{results[5]}"
          view.write_image(opts)
          progress = (frame.to_i - sframe) * 100 / (eframe - sframe).to_f
          Sketchup.status_text = "Exporting MSPhysics Replay Animation    Progress: #{frame.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
          frame += reversed ? -speed : speed
          count += 1
        end
        reset unless called_while_active
        # Set original settings
        @replay_camera = orig_replay_camera
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
        prompts = ['Start Frame', 'End Frame', 'Speed (0.01 - 10000)   ', 'Replay Camera', 'Reverse']
        yes_no = 'Yes|No'
        drop_downs = ['', '', '', yes_no, yes_no]
        values = [@start_frame, @end_frame, @speed, @replay_camera ? 'Yes' : 'No', @reversed ? 'Yes' : 'No']
        results = UI.inputbox(prompts, values, drop_downs, 'Export Animation Options')
        return false unless results
        # Select export path
        model_fname = File.basename(model.path, '.skp')
        model_fname = "msp_anim" if model_fname.empty?
        script_file = UI.savepanel('Choose Export Directory and Name', nil, model_fname)
        return false unless script_file
        fpath = File.dirname(script_file)
        fname = File.basename(script_file, '.skp')
        # Preset user data
        sframe = results[0].to_i
        eframe = results[1].to_i
        speed = AMS.clamp(results[2].to_f, 0.01, 10000)
        reversed = results[4] == 'Yes'
        orig_replay_camera = @replay_camera
        @replay_camera = results[3] == 'Yes'
        # Export animation
        start_time = Time.now
        called_while_active = @active
        start(false) unless called_while_active
        frame = reversed ? eframe : sframe
        last_frame = nil
        count = 1
        while(frame.to_i >= sframe && frame.to_i <= eframe)
          if frame.to_i != last_frame
            activate_frame(frame)
            last_frame = frame.to_i
          end
          full_path = "#{fpath}/#{fname}#{sprintf("%06d", count)}.skp"
          if Sketchup.version.to_i < 14
            model.save(full_path)
          else
            if model.path.empty?
              model.save(full_path)
            else
              model.save_copy(full_path)
            end
          end
          progress = (frame.to_i - sframe) * 100 / (eframe - sframe).to_f
          Sketchup.status_text = "Exporting MSPhysics Replay Animation    Progress: #{frame.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
          frame += reversed ? -speed : speed
          count += 1
        end
        reset unless called_while_active
        # Set original settings
        @replay_camera = orig_replay_camera
        # Display results
        UI.messagebox("Finished exporting MSPhysics Replay Animation!\n\nExported #{count} frames in #{sprintf("%.2f", Time.now - start_time)} seconds.")
        # Return success
        true
      end

      # Export animation into kerkythea files.
      # @return [Boolean] success
      def export_to_kt
        unless defined?(SU2KT)
          UI.messagebox 'Kerkythea (SU2KT) plugin is not installed.'
          return false
        end
        def SU2KT.export_msp_animation
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
            model_name = model_filename.split('.')[0..-2].join('.') + '.kst'
          end
          script_file = UI.savepanel('Select Export Directory and Name', '', model_name)
          return false unless script_file
          if script_file == script_file.split('.')[0..-2].join('.') # No file extension
            script_file << '.kst'
          end
          @model_name = File.basename(script_file)
          @model_name = @model_name.split('.')[0]
          @frames_path = File.dirname(script_file) + @ds + 'Anim_' + File.basename(script_file).split('.')[0..-2].join('.')
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
          out_file = script_file.split('.')[0..-2].join('.') + '.xml'
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
          frame = reversed ? eframe : sframe
          last_frame = nil
          count = 1
          while(frame.to_i >= sframe && frame.to_i <= eframe)
            if frame.to_i != last_frame
              replay.activate_frame(frame)
              last_frame = frame.to_i
            end
            # Export data to the frame file
            frame_name = sprintf("%06d", count)
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
            progress = (frame.to_i - sframe) * 100 / (eframe - sframe).to_f
            Sketchup.status_text = "Exporting MSPhysics Replay to KT    Progress: #{frame.to_i - sframe} / #{eframe - sframe} -- #{sprintf("%.2f", progress)}%"
            # Increment frame and counter
            frame += reversed ? -speed : speed
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
            if RUBY_PLATFORM =~ /mswin|mingw/i
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
        end unless SU2KT.respond_to?(:export_msp_animation)
        SU2KT.export_msp_animation
      end

      # Export animation into AVI video file.
      # @return [Boolean] success
      def export_to_avi
      end

    end # class << self

    # @!visibility private
    class AppObserver

      def onNewModel(model)
        MSPhysics::Replay.stop
        MSPhysics::Replay.clear_recorded_data
        MSPhysics::Replay.clear_active_data
      end

      def onOpenModel(model)
        onNewModel(model)
      end

    end # class AppObserver

    # @!visibility private
    class Animation

      def initialize
        @last_frame = nil
      end

      def nextFrame(view)
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
        true
      end

    end # class Animation
  end # module Replay
end # module MSPhysics
