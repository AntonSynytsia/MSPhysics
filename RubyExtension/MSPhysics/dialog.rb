module MSPhysics

  # @since 1.0.0
  module Dialog

    DEFAULT_THEME = 'tomorrow_night'
    DEFAULT_FONT = 12
    DEFAULT_WRAP = 'free'
    DEFAULT_PRINT_MARGIN = true
    DEFAULT_EDITOR_SIZE = [800,600]

    @dialog = nil
    @handle = nil
    @title = 'MSPhysics UI'
    @editor_size = DEFAULT_EDITOR_SIZE.dup
    @init_called = false
    @selected_body = nil
    @selected_joint = nil
    @active_tab = 1
    @last_active_body_tab = 2
    @material = nil
    @activated_value = nil
    @precision = 2
    @selected_sound = nil
    @default_sound_path = nil
    @editor_theme = DEFAULT_THEME
    @editor_font = DEFAULT_FONT
    @editor_wrap = DEFAULT_WRAP
    @editor_print_margin = DEFAULT_PRINT_MARGIN
    @editor_read_only = false
    @body_internal_joints = {}
    @body_connected_joints = {}
    @geared_joints = {}
    @selection_observer = nil
    @border_size = [0,0]

    class << self

      # @param [String] code
      # @return [Boolean] success
      def execute_js(code)
        return false unless @dialog
        @dialog.execute_script("try { #{code} } catch(err) { alert(err); }")
        #@dialog.execute_script(code)
      end

      # @api private
      # Update state of all UI.
      # @return [void]
      def update_state
        return unless self.visible?
        # Simulation dialog
        update_simulation_state
        # Sound dialog
        update_sound_state
        # Joint dialog
        update_joint_state
        # Body dialog
        update_body_state
        # Update size
        execute_js("setTimeout(function() { update_size(); }, 250);")
      end

      # @api private
      # Update dialog style.
      # @return [Boolean] success
      def update_dialog_style
        return false if !AMS::IS_PLATFORM_WINDOWS || @handle.nil?
        style = AMS::Window.get_long(@handle, -16)
        new_style = @active_tab == 3 && @selected_body && @selected_body.valid? && @selected_body.parent.is_a?(Sketchup::Model) ? style | 0x00050000 : style & ~0x01050000
        AMS::Window.lock_update(@handle)
        AMS::Window.set_long(@handle, -16, new_style)
        AMS::Window.lock_update(nil)
        AMS::Window.set_pos(@handle, 0, 0, 0, 0, 0, 0x0237)
        true
      end

      # @api private
      # Update UI simulation tab.
      # @return [void]
      def update_simulation_state
        return unless self.visible?
        settings = MSPhysics::Settings
        cmd = ''
        cmd << "$('#simulation-solver_model').val('#{settings.solver_model}');"
        cmd << "$('#simulation-solver_model').trigger('chosen:updated');"
        cmd << "$('#simulation-update_timestep').val('#{(1.0/settings.update_timestep).round}');"
        cmd << "$('#simulation-update_timestep').trigger('chosen:updated');"
        cmd << "$('#simulation-continuous_collision').prop('checked', #{settings.continuous_collision_check_enabled?});"
        cmd << "$('#simulation-collision_wireframe').prop('checked', #{settings.collision_wireframe_visible?});"
        cmd << "$('#simulation-axes').prop('checked', #{settings.axes_visible?});"
        cmd << "$('#simulation-aabb').prop('checked', #{settings.aabb_visible?});"
        cmd << "$('#simulation-contact_points').prop('checked', #{settings.contact_points_visible?});"
        cmd << "$('#simulation-contact_forces').prop('checked', #{settings.contact_forces_visible?});"
        cmd << "$('#simulation-bodies').prop('checked', #{settings.bodies_visible?});"
        cmd << "$('#simulation-gravity').val('#{ format_value(settings.gravity, @precision) }');"
        cmd << "$('#simulation-material_thickness').val('#{ format_value(settings.material_thickness * 32.0, @precision) }');"
        cmd << "$('#simulation-update_rate').val('#{settings.update_rate}');"
        cmd << "$('#simulation-world_scale').val('#{ format_value(settings.world_scale, @precision) }');"
        execute_js(cmd)
      end

      # @api private
      # Update UI properties tab.
      # @return [void]
      def update_body_state
        return unless self.visible?
        model = Sketchup.active_model
        bodies = []
        model.selection.each { |ent|
          next unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
          bodies << ent if ent.get_attribute('MSPhysics', 'Type', 'Body') == 'Body'
        }
        cmd = ""
        @body_internal_joints.clear
        @body_connected_joints.clear
        cmd << "$('#body-internal_joints_table').empty();"
        cmd << "$('#body-connected_joints_table').empty();"
        if bodies.size == 1
          @selected_body = bodies[0]
          # Top level entities have access to the full body properties.
          # Only top level entities may contain scripts.
          # Child entities have access to Ignore state only.
          default = MSPhysics::DEFAULT_BODY_SETTINGS
          if model.active_entities == model.entities
            # Display tabs
            cmd << "$('#tab2-none').css('display', 'none');"
            cmd << "$('#tab2-content1').css('display', 'block');"
            cmd << "$('#tab2-content2').css('display', 'none');"
            cmd << "$('#tab3-none').css('display', 'none');"
            cmd << "$('#tab3-content').css('display', 'block');"
            # Display script
            script = @selected_body.get_attribute('MSPhysics Script', 'Value', '').inspect
            cmd << "editor_set_script(#{script});"
            cursor = nil
            begin
              cursor = eval(@selected_body.get_attribute('MSPhysics Script', 'Cursor', '[1,0]'))
            rescue Exception => e
              cursor = [1,0]
            end
            cmd << "editor_set_cursor(#{cursor.x}, #{cursor.y});"
            cmd << "window.aceEditor.setTheme('ace/theme/#{@editor_theme}');"
            cmd << "document.getElementById('editor').style.fontSize='#{@editor_font}px';"
            cmd << "window.aceEditor.setOption('wrap', '#{@editor_wrap}');"
            cmd << "window.aceEditor.setShowPrintMargin(#{@editor_print_margin});"
            cmd << "window.aceEditor.setReadOnly(#{@editor_read_only});"
            cmd << "$('#editor-theme').val('#{@editor_theme}');"
            cmd << "$('#editor-theme').trigger('chosen:updated');"
            cmd << "$('#editor-font').val('#{@editor_font}');"
            cmd << "$('#editor-font').trigger('chosen:updated');"
            cmd << "$('#editor-wrap').val('#{@editor_wrap}');"
            cmd << "$('#editor-wrap').trigger('chosen:updated');"
            cmd << "$('#editor-print_margin').prop('checked', #{ @editor_print_margin });"
            cmd << "$('#editor-read_only').prop('checked', #{ @editor_read_only });"
            # Display entity id and index
            cmd << "$('#body-entity_id').val('#{ @selected_body.entityID }');"
            cmd << "$('#body-entity_index').val('#{ model.entities.to_a.index(@selected_body) }');"
            # Display Controllers
            ['Thruster Controller', 'Emitter Controller'].each { |option|
              property = option.downcase.gsub(/\s/, '_')
              script = @selected_body.get_attribute('MSPhysics Body', option)
              script = '0.0' if script.nil?
              cmd << "$('#body-#{ property }').val(#{script.inspect});"
            }
            # Display shape
            shape = @selected_body.get_attribute('MSPhysics Body', 'Shape', default[:shape]).to_s
            cmd << "$('#body-shape').val('#{shape}');"
            cmd << "$('#body-shape').trigger('chosen:updated');"
            # Display material
            cmd << "$('#body-material').empty();"
            cmd << "$('#body-material').append('<option value=\"#{default[:material_name]}\">#{default[:material_name]}</option>');"
            cmd << "$('#body-material').append('<option value=\"Custom\">Custom</option>');"
            materials = MSPhysics::Materials.sort { |a, b| a.name <=> b.name }
            materials.each { |material|
              cmd << "$('#body-material').append('<option value=\"#{material.name}\">#{material.name}</option>');"
            }
            material = @selected_body.get_attribute('MSPhysics Body', 'Material', default[:material_name]).to_s
            cmd << "$('#body-material').val('#{material}');"
            cmd << "$('#body-material').trigger('chosen:updated');"
            # Display state and other check-box properties
            ['Ignore', 'Collidable', 'Static', 'Frozen', 'Auto Sleep', 'Enable Friction', 'Magnetic', 'Enable Script', 'Continuous Collision', 'Enable Gravity', 'Thruster Lock Axis', 'Emitter Lock Axis', 'Enable Thruster', 'Enable Emitter', 'Connect Closest Joints'].each { |option|
              property = option.downcase.gsub(/\s/, '_')
              default_state = default[property.to_sym]
              state = @selected_body.get_attribute('MSPhysics Body', option, default_state) ? true : false
              cmd << "$('#body-#{ property }').prop('checked', #{ state });"
            }
            # Display name
            cmd << "$('#body-name').val(#{@selected_body.name.inspect});"
            # Display numeric properties
            ['Density', 'Mass', 'Static Friction', 'Dynamic Friction', 'Elasticity', 'Softness', 'Magnet Force', 'Magnet Range', 'Linear Damping', 'Angular Damping'].each { |option|
              property = option.downcase.gsub(/\s/, '_')
              attr = @selected_body.get_attribute('MSPhysics Body', option)
              if attr.is_a?(String)
                attr = attr.to_f
              elsif !attr.is_a?(Numeric)
                attr = default[property.to_sym]
              end
              cmd << "$('#body-#{ property }').val('#{ format_value(attr, @precision) }');"
            }
            # Display fixnum properties
            ['Emitter Rate', 'Emitter Lifetime'].each { |option|
              property = option.downcase.gsub(/\s/, '_')
              attr = @selected_body.get_attribute('MSPhysics Body', option)
              if attr.is_a?(String)
                attr = attr.to_f
              elsif !attr.is_a?(Numeric)
                attr = default[property.to_sym]
              end
              cmd << "$('#body-#{ property }').val('#{ attr.round }');"
            }
            # Display weight control
            weight_control = @selected_body.get_attribute('MSPhysics Body', 'Weight Control', 'Density').to_s
            cmd << "$('#body-weight_control').val('#{weight_control}');"
            cmd << "$('#body-weight_control').trigger('chosen:updated');"
            if weight_control == 'Mass'
              cmd << "$('#body-density_control').css('display', 'none');"
              cmd << "$('#body-mass_control').css('display', 'table-row');"
            else
              cmd << "$('#body-density_control').css('display', 'table-row');"
              cmd << "$('#body-mass_control').css('display', 'none');"
            end
            # Display internal joints
            identical_joints = {}
            AMS::Group.get_entities(@selected_body).each { |e|
              if (e.is_a?(Sketchup::ComponentInstance) || e.is_a?(Sketchup::Group)) && e.get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
                type = e.get_attribute('MSPhysics Joint', 'Type')
                id = e.get_attribute('MSPhysics Joint', 'ID')
                name = e.name.to_s
                name = id.to_s if name.empty?
                fname = type + '-' + name
                fid = type + '-' + id.to_s
                if @body_internal_joints[fid]
                  if identical_joints[fid]
                    identical_joints[fid] += 1
                  else
                    identical_joints[fid] = 1
                  end
                  fid2 = fid + '-' + identical_joints[fid].to_s
                  fname2 = fname + ' (' + identical_joints[fid].to_s + ')'
                  @body_internal_joints[fid2] = [e, fname2]
                else
                  @body_internal_joints[fid] = [e, fname]
                end
              end
            }
            if @body_internal_joints.empty?
              cmd << "$('#body-internal_joints_div').css('display', 'none');"
            else
              cmd << "$('#body-internal_joints_div').css('display', 'block');"
              cmd << "$('#body-internal_joints_table').append(\""
              count = 0
              cmd << "<tr>"
              @body_internal_joints.keys.sort.each { |fid|
                ent, fname = @body_internal_joints[fid]
                if count == 3
                  cmd << "</tr><tr>"
                  count = 0
                end
                cmd << "<td><label class=\\\"joint-label#{ent == @selected_joint ? "-selected" : ""}\\\" id=\\\"Internal::#{fid}\\\">#{fname.inspect[1...-1]}</label></td>"
                count += 1
              }
              cmd << "</tr>"
              cmd << "\");"
            end
            identical_joints.clear
            # Display connected joints
            connected_joints = MSPhysics::JointConnectionTool.get_connected_joints(@selected_body, false)[0]
            connected_joints.each { |joint, jparent, jtra|
              type = joint.get_attribute('MSPhysics Joint', 'Type')
              id = joint.get_attribute('MSPhysics Joint', 'ID')
              name = joint.name.to_s
              name = id.to_s if name.empty?
              fname = type + '-' + name
              fid = type + '-' + id.to_s
              if @body_connected_joints[fid]
                if identical_joints[fid]
                  identical_joints[fid] += 1
                else
                  identical_joints[fid] = 1
                end
                fid2 = fid + '-' + identical_joints[fid].to_s
                fname2 = fname + ' (' + identical_joints[fid].to_s + ')'
                @body_connected_joints[fid2] = [joint, fname2]
              else
                @body_connected_joints[fid] = [joint, fname]
              end
            }
            if @body_connected_joints.empty?
              cmd << "$('#body-connected_joints_div').css('display', 'none');"
            else
              cmd << "$('#body-connected_joints_div').css('display', 'block');"
              cmd << "$('#body-connected_joints_table').append(\""
              count = 0
              cmd << "<tr>"
              @body_connected_joints.keys.sort.each { |fid|
                ent, fname = @body_connected_joints[fid]
                if count == 3
                  cmd << "</tr><tr>"
                  count = 0
                end
                cmd << "<td><label class=\\\"joint-label#{ent == @selected_joint ? "-selected" : ""}\\\" id=\\\"Connected::#{fid}\\\">#{fname.inspect[1...-1]}</label></td>"
                count += 1
              }
              cmd << "</tr>"
              cmd << "\");"
            end
            identical_joints.clear
            # Assign click events to these joints.
            cmd << "assign_joint_click_event();"
          else
            # Display tabs
            cmd << "$('#tab3-none').css('display', 'block');"
            cmd << "$('#tab3-content').css('display', 'none');"
            cmd << "$('#tab2-none').css('display', 'none');"
            cmd << "$('#tab2-content1').css('display', 'none');"
            cmd << "$('#tab2-content2').css('display', 'block');"
            # Display Ignore state
            state = @selected_body.get_attribute('MSPhysics Body', 'Ignore', nil) ? true : false
            cmd << "$('#internal_body-ignore').prop('checked', #{state});"
          end
          cmd << "activate_tab(#{@last_active_body_tab});"
        else
          @selected_body = nil
          cmd << "$('#tab2-none').css('display', 'block');"
          cmd << "$('#tab2-content1').css('display', 'none');"
          cmd << "$('#tab2-content2').css('display', 'none');"
          cmd << "$('#tab3-none').css('display', 'block');"
          cmd << "$('#tab3-content').css('display', 'none');"
        end
        execute_js(cmd)
      end

      # @api private
      # Update UI joint tab.
      # @return [void]
      def update_joint_state
        return unless self.visible?
        model = Sketchup.active_model
        joints = []
        model.selection.each { |ent|
          next unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
          joints << ent if ent.get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
        }
        cmd = ''
        cmd << "$('#tab4-none').css('display', 'block');"
        cmd << "$('#tab4-general').css('display', 'none');"
        cmd << "$('#tab4-fixed').css('display', 'none');"
        cmd << "$('#tab4-hinge').css('display', 'none');"
        cmd << "$('#tab4-motor').css('display', 'none');"
        cmd << "$('#tab4-servo').css('display', 'none');"
        cmd << "$('#tab4-slider').css('display', 'none');"
        cmd << "$('#tab4-piston').css('display', 'none');"
        cmd << "$('#tab4-spring').css('display', 'none');"
        cmd << "$('#tab4-up_vector').css('display', 'none');"
        cmd << "$('#tab4-corkscrew').css('display', 'none');"
        cmd << "$('#tab4-ball_and_socket').css('display', 'none');"
        cmd << "$('#tab4-universal').css('display', 'none');"
        cmd << "$('#tab4-curvy_slider').css('display', 'none');"
        cmd << "$('#tab4-curvy_piston').css('display', 'none');"

        @geared_joints.clear
        MSPhysics::JOINT_NAMES.each { |jname|
          cmd << "$('##{jname}-geared_joints_field').empty();"
          cmd << "$('##{jname}-geared_joints_field').css('display', 'none');"
        }

        if joints.size == 1
          @selected_joint = joints[0]
          jdict = 'MSPhysics Joint'
          attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS)
          ang_ratio = attr == 'deg' ? 1.degrees : 1
          iang_ratio = 1.0 / ang_ratio
          attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS)
          pos_ratio = case attr
            when 'mm'
              0.001
            when 'cm'
              0.01
            when 'dm'
              0.1
            when 'm'
              1.0
            when 'in'
              0.0254
            when 'ft'
              0.3048
            when 'yd'
              0.9144
            else
              1.0
          end
          ipos_ratio = 1.0 / pos_ratio

          cmd << "$('#tab4-none').css('display', 'none');"
          cmd << "$('#tab4-general').css('display', 'block');"

          attr = @selected_joint.get_attribute(jdict, 'Constraint Type', MSPhysics::Joint::DEFAULT_CONSTRAINT_TYPE).to_i
          cmd << "$('#joint-constraint_type-#{attr == 0 ? 'standard' : (attr == 1 ? 'flexible' : 'robust')}').prop('checked', true);"
          attr = @selected_joint.name.to_s
          attr = @selected_joint.get_attribute(jdict, 'ID') if attr.empty?
          unless attr
            attr = JointTool.generate_uniq_id
            @selected_joint.set_attribute('MSPhysics Joint', 'ID', attr)
          end
          cmd << "$('#joint-name').val(#{attr.inspect});"
          attr = @selected_joint.get_attribute(jdict, 'Stiffness', MSPhysics::Joint::DEFAULT_STIFFNESS)
          cmd << "$('#joint-stiffness').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
          attr = @selected_joint.get_attribute(jdict, 'Bodies Collidable', MSPhysics::Joint::DEFAULT_BODIES_COLLIDABLE)
          cmd << "$('#joint-bodies_collidable').prop('checked', #{attr ? true : false});"
          attr = @selected_joint.get_attribute(jdict, 'Breaking Force', MSPhysics::Joint::DEFAULT_BREAKING_FORCE)
          cmd << "$('#joint-breaking_force').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"

          joint_type = @selected_joint.get_attribute(jdict, 'Type')
          joint_type2 = nil
          case joint_type
          when 'Fixed'
            joint_type2 = 'fixed'
            cmd << "$('#tab4-fixed').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Adjust To', 0)
            mode = attr == 2 ? 'parent' : (attr == 1 ? 'child' : 'none')
            cmd << "$('#fixed-adjust_to-#{mode}').prop('checked', true);"
          when 'Hinge'
            joint_type2 = 'hinge'
            cmd << "$('#tab4-hinge').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS).to_s
            cmd << "$('#hinge-angle_units').val('#{attr}');"
            cmd << "$('#hinge-angle_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Hinge::DEFAULT_MIN * iang_ratio))
            cmd << "$('#hinge-min').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Hinge::DEFAULT_MAX * iang_ratio))
            cmd << "$('#hinge-max').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits', MSPhysics::Hinge::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#hinge-enable_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Friction', fix_numeric_value(MSPhysics::Hinge::DEFAULT_FRICTION))
            cmd << "$('#hinge-friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Accel', fix_numeric_value(MSPhysics::Hinge::DEFAULT_ACCEL))
            cmd << "$('#hinge-accel').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Damp', fix_numeric_value(MSPhysics::Hinge::DEFAULT_DAMP))
            cmd << "$('#hinge-damp').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Rotate Back', MSPhysics::Hinge::DEFAULT_ROTATE_BACK_ENABLED)
            cmd << "$('#hinge-enable_rotate_back').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Enable Strong Mode', MSPhysics::Hinge::DEFAULT_STRONG_MODE_ENABLED)
            cmd << "$('#hinge-enable_strong_mode').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Start Angle', fix_numeric_value(MSPhysics::Hinge::DEFAULT_START_ANGLE * iang_ratio))
            cmd << "$('#hinge-start_angle').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Hinge::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#hinge-controller').val(#{attr.inspect});"
          when 'Motor'
            joint_type2 = 'motor'
            cmd << "$('#tab4-motor').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Accel', fix_numeric_value(MSPhysics::Motor::DEFAULT_ACCEL))
            cmd << "$('#motor-accel').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Damp', fix_numeric_value(MSPhysics::Motor::DEFAULT_DAMP))
            cmd << "$('#motor-damp').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Free Rotate', MSPhysics::Motor::DEFAULT_FREE_ROTATE_ENABLED)
            cmd << "$('#motor-enable_free_rotate').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Motor::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#motor-controller').val(#{attr.inspect});"
          when 'Servo'
            joint_type2 = 'servo'
            cmd << "$('#tab4-servo').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS).to_s
            cmd << "$('#servo-angle_units').val('#{attr}');"
            cmd << "$('#servo-angle_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Servo::DEFAULT_MIN * iang_ratio))
            cmd << "$('#servo-min').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Servo::DEFAULT_MAX * iang_ratio))
            cmd << "$('#servo-max').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits', MSPhysics::Servo::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#servo-enable_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Rate', fix_numeric_value(MSPhysics::Servo::DEFAULT_RATE * iang_ratio))
            cmd << "$('#servo-rate').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Power', fix_numeric_value(MSPhysics::Servo::DEFAULT_POWER))
            cmd << "$('#servo-power').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Reduction Ratio', fix_numeric_value(MSPhysics::Servo::DEFAULT_REDUCTION_RATIO))
            cmd << "$('#servo-reduction_ratio').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Servo::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#servo-controller').val(#{attr.inspect});"
          when 'Slider'
            joint_type2 = 'slider'
            cmd << "$('#tab4-slider').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS).to_s
            cmd << "$('#slider-position_units').val('#{attr}');"
            cmd << "$('#slider-position_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Slider::DEFAULT_MIN * ipos_ratio))
            cmd << "$('#slider-min').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Slider::DEFAULT_MAX * ipos_ratio))
            cmd << "$('#slider-max').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits', MSPhysics::Slider::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#slider-enable_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Friction', fix_numeric_value(MSPhysics::Slider::DEFAULT_FRICTION))
            cmd << "$('#slider-friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Slider::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#slider-controller').val(#{attr.inspect});"
          when 'Piston'
            joint_type2 = 'piston'
            cmd << "$('#tab4-piston').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS).to_s
            cmd << "$('#piston-position_units').val('#{attr}');"
            cmd << "$('#piston-position_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Piston::DEFAULT_MIN * ipos_ratio))
            cmd << "$('#piston-min').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Piston::DEFAULT_MAX * ipos_ratio))
            cmd << "$('#piston-max').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits', MSPhysics::Piston::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#piston-enable_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Rate', fix_numeric_value(MSPhysics::Piston::DEFAULT_RATE * ipos_ratio))
            cmd << "$('#piston-rate').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Power', fix_numeric_value(MSPhysics::Piston::DEFAULT_POWER))
            cmd << "$('#piston-power').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Reduction Ratio', fix_numeric_value(MSPhysics::Piston::DEFAULT_REDUCTION_RATIO))
            cmd << "$('#piston-reduction_ratio').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Piston::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#piston-controller').val(#{attr.inspect});"
          when 'Spring'
            joint_type2 = 'spring'
            cmd << "$('#tab4-spring').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS).to_s
            cmd << "$('#spring-position_units').val('#{attr}');"
            cmd << "$('#spring-position_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Spring::DEFAULT_MIN * ipos_ratio))
            cmd << "$('#spring-min').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Spring::DEFAULT_MAX * ipos_ratio))
            cmd << "$('#spring-max').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits', MSPhysics::Spring::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#spring-enable_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Accel', fix_numeric_value(MSPhysics::Spring::DEFAULT_ACCEL))
            cmd << "$('#spring-accel').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Damp', fix_numeric_value(MSPhysics::Spring::DEFAULT_DAMP))
            cmd << "$('#spring-damp').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Strong Mode', MSPhysics::Spring::DEFAULT_STRONG_MODE_ENABLED)
            cmd << "$('#spring-enable_strong_mode').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Start Position', fix_numeric_value(MSPhysics::Spring::DEFAULT_START_POSITION * ipos_ratio))
            cmd << "$('#spring-start_position').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Spring::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#spring-controller').val(#{attr.inspect});"
          when 'UpVector'
            joint_type2 = 'up_vector'
            cmd << "$('#tab4-up_vector').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Accel', fix_numeric_value(MSPhysics::UpVector::DEFAULT_ACCEL))
            cmd << "$('#up_vector-accel').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Damp', fix_numeric_value(MSPhysics::UpVector::DEFAULT_DAMP))
            cmd << "$('#up_vector-damp').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Damper', MSPhysics::UpVector::DEFAULT_DAMPER_ENABLED)
            cmd << "$('#up_vector-enable_damper').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::UpVector::DEFAULT_PIN_DIR.to_a.to_s)
            cmd << "$('#up_vector-controller').val(#{attr.inspect});"
          when 'Corkscrew'
            joint_type2 = 'corkscrew'
            cmd << "$('#tab4-corkscrew').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS).to_s
            cmd << "$('#corkscrew-position_units').val('#{attr}');"
            cmd << "$('#corkscrew-position_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min Position', fix_numeric_value(MSPhysics::Corkscrew::DEFAULT_MIN_POSITION * ipos_ratio))
            cmd << "$('#corkscrew-min_position').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max Position', fix_numeric_value(MSPhysics::Corkscrew::DEFAULT_MAX_POSITION * ipos_ratio))
            cmd << "$('#corkscrew-max_position').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Linear Limits', MSPhysics::Corkscrew::DEFAULT_LINEAR_LIMITS_ENABLED)
            cmd << "$('#corkscrew-enable_linear_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Linear Friction', fix_numeric_value(MSPhysics::Corkscrew::DEFAULT_LINEAR_FRICTION))
            cmd << "$('#corkscrew-linear_friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS).to_s
            cmd << "$('#corkscrew-angle_units').val('#{attr}');"
            cmd << "$('#corkscrew-angle_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min Angle', fix_numeric_value(MSPhysics::Corkscrew::DEFAULT_MIN_ANGLE * iang_ratio))
            cmd << "$('#corkscrew-min_angle').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max Angle', fix_numeric_value(MSPhysics::Corkscrew::DEFAULT_MAX_ANGLE * iang_ratio))
            cmd << "$('#corkscrew-max_angle').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Angular Limits', MSPhysics::Corkscrew::DEFAULT_ANGULAR_LIMITS_ENABLED)
            cmd << "$('#corkscrew-enable_angular_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Angular Friction', fix_numeric_value(MSPhysics::Corkscrew::DEFAULT_ANGULAR_FRICTION))
            cmd << "$('#corkscrew-angular_friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
          when 'BallAndSocket'
            joint_type2 = 'ball_and_socket'
            cmd << "$('#tab4-ball_and_socket').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS).to_s
            cmd << "$('#ball_and_socket-angle_units').val('#{attr}');"
            cmd << "$('#ball_and_socket-angle_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Max Cone Angle', fix_numeric_value(MSPhysics::BallAndSocket::DEFAULT_MAX_CONE_ANGLE * iang_ratio))
            cmd << "$('#ball_and_socket-max_cone_angle').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Cone Limits', MSPhysics::BallAndSocket::DEFAULT_CONE_LIMITS_ENABLED)
            cmd << "$('#ball_and_socket-enable_cone_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Min Twist Angle', fix_numeric_value(MSPhysics::BallAndSocket::DEFAULT_MIN_TWIST_ANGLE * iang_ratio))
            cmd << "$('#ball_and_socket-min_twist_angle').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max Twist Angle', fix_numeric_value(MSPhysics::BallAndSocket::DEFAULT_MAX_TWIST_ANGLE * iang_ratio))
            cmd << "$('#ball_and_socket-max_twist_angle').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Twist Limits', MSPhysics::BallAndSocket::DEFAULT_TWIST_LIMITS_ENABLED)
            cmd << "$('#ball_and_socket-enable_twist_limits').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Friction', fix_numeric_value(MSPhysics::BallAndSocket::DEFAULT_FRICTION))
            cmd << "$('#ball_and_socket-friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::BallAndSocket::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#ball_and_socket-controller').val(#{attr.inspect});"
          when 'Universal'
            joint_type2 = 'universal'
            cmd << "$('#tab4-universal').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS).to_s
            cmd << "$('#universal-angle_units').val('#{attr}');"
            cmd << "$('#universal-angle_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Min1', fix_numeric_value(MSPhysics::Universal::DEFAULT_MIN * iang_ratio))
            cmd << "$('#universal-min1').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max1', fix_numeric_value(MSPhysics::Universal::DEFAULT_MAX * iang_ratio))
            cmd << "$('#universal-max1').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits1', MSPhysics::Universal::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#universal-enable_limits1').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Min2', fix_numeric_value(MSPhysics::Universal::DEFAULT_MIN * iang_ratio))
            cmd << "$('#universal-min2').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Max2', fix_numeric_value(MSPhysics::Universal::DEFAULT_MAX * iang_ratio))
            cmd << "$('#universal-max2').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Limits2', MSPhysics::Universal::DEFAULT_LIMITS_ENABLED)
            cmd << "$('#universal-enable_limits2').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Friction', fix_numeric_value(MSPhysics::Universal::DEFAULT_FRICTION))
            cmd << "$('#universal-friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Universal::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#universal-controller').val(#{attr.inspect});"
          when 'CurvySlider'
            joint_type2 = 'curvy_slider'
            cmd << "$('#tab4-curvy_slider').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Alignment', MSPhysics::CurvySlider::DEFAULT_ALIGNMENT_ENABLED)
            cmd << "$('#curvy_slider-enable_alignment').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Enable Rotation', MSPhysics::CurvySlider::DEFAULT_ROTATION_ENABLED)
            cmd << "$('#curvy_slider-enable_rotation').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Enable Loop', MSPhysics::CurvySlider::DEFAULT_LOOP_ENABLED)
            cmd << "$('#curvy_slider-enable_loop').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Linear Friction', fix_numeric_value(MSPhysics::CurvySlider::DEFAULT_LINEAR_FRICTION))
            cmd << "$('#curvy_slider-linear_friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Angular Friction', fix_numeric_value(MSPhysics::CurvySlider::DEFAULT_ANGULAR_FRICTION))
            cmd << "$('#curvy_slider-angular_friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Alignment Power', fix_numeric_value(MSPhysics::CurvySlider::DEFAULT_ALIGNMENT_POWER))
            cmd << "$('#curvy_slider-alignment_power').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::CurvySlider::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#curvy_slider-controller').val(#{attr.inspect});"
          when 'CurvyPiston'
            joint_type2 = 'curvy_piston'
            cmd << "$('#tab4-curvy_piston').css('display', 'block');"
            attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS).to_s
            cmd << "$('#curvy_piston-position_units').val('#{attr}');"
            cmd << "$('#curvy_piston-position_units').trigger('chosen:updated');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Alignment', MSPhysics::CurvyPiston::DEFAULT_ALIGNMENT_ENABLED)
            cmd << "$('#curvy_piston-enable_alignment').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Enable Rotation', MSPhysics::CurvyPiston::DEFAULT_ROTATION_ENABLED)
            cmd << "$('#curvy_piston-enable_rotation').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Enable Loop', MSPhysics::CurvyPiston::DEFAULT_LOOP_ENABLED)
            cmd << "$('#curvy_piston-enable_loop').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Angular Friction', fix_numeric_value(MSPhysics::CurvyPiston::DEFAULT_ANGULAR_FRICTION))
            cmd << "$('#curvy_piston-angular_friction').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Rate', fix_numeric_value(MSPhysics::CurvyPiston::DEFAULT_RATE * ipos_ratio))
            cmd << "$('#curvy_piston-rate').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Power', fix_numeric_value(MSPhysics::CurvyPiston::DEFAULT_POWER))
            cmd << "$('#curvy_piston-power').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Alignment Power', fix_numeric_value(MSPhysics::CurvyPiston::DEFAULT_ALIGNMENT_POWER))
            cmd << "$('#curvy_piston-alignment_power').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Reduction Ratio', fix_numeric_value(MSPhysics::CurvyPiston::DEFAULT_REDUCTION_RATIO))
            cmd << "$('#curvy_piston-reduction_ratio').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::CurvyPiston::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#curvy_piston-controller').val(#{attr.inspect});"
            attr = @selected_joint.get_attribute(jdict, 'Controller Mode', MSPhysics::CurvyPiston::DEFAULT_CONTROLLER_MODE).to_i
            cmd << "$('#curvy_piston-controller_mode').val('#{attr == 1 ? 1 : 0}');"
            cmd << "$('#curvy_piston-controller_mode').trigger('chosen:updated');"
          end
          # Display geared joints
          if joint_type2
            identical_joints = {}
            geared_data = {}
            if Sketchup.version.to_i > 6
              sjparent = model.active_path ? model.active_path.last : nil
            else
              sjparent = model.active_entities.is_a?(Sketchup::Model) ? nil : model.active_entities.parent.instances.first
            end
            MSPhysics::JointConnectionTool.get_geared_joints(@selected_joint, sjparent, true, false).each { |joint, jparent, jtra, gear_id, gear_type, gear_ratio|
              type = joint.get_attribute('MSPhysics Joint', 'Type')
              name = joint.name.to_s
              name = gear_id.to_s if name.empty?
              fname = type + '-' + name
              fid = gear_type.to_s + '-' + gear_id.to_s
              geared_data[gear_type] = {} unless geared_data[gear_type]
              if @geared_joints[fid]
                if identical_joints[fid]
                  identical_joints[fid] += 1
                else
                  identical_joints[fid] = 1
                end
                fid2 = fid + '-' + identical_joints[fid].to_s
                fname2 = fname + ' (' + identical_joints[fid].to_s + ')'
                @geared_joints[fid2] = joint
                geared_data[gear_type][fid2] = [joint, fname2, gear_ratio]
              else
                @geared_joints[fid] = joint
                geared_data[gear_type][fid] = [joint, fname, gear_ratio]
              end
            }
            identical_joints.clear
            unless geared_data.empty?
              cmd << "$('##{joint_type2}-geared_joints_field').css('display', 'block');"
              cmd << "$('##{joint_type2}-geared_joints_field').append(\""
              cmd2 = ""
              geared_data.each { |gear_type, data|
                words = DOUBLE_JOINT_TYPES[gear_type].split(/\_/)
                for i in 0...words.size
                  words[i].capitalize!
                end
                gear_name = words.join(' ')
                cmd << "<label class=\\\"heading\\\">#{gear_name}</label>"
                cmd << "<div class=\\\"spacing_tab\\\">"
                cmd << "<table class=\\\"struct_table\\\">"
                data.keys.sort.each { |fid|
                  joint, fname, gear_ratio = data[fid]
                  cmd << "<tr>"
                  cmd << "<td colspan=\\\"1\\\"><label>With</label></td>"
                  cmd << "<td colspan=\\\"4\\\"><label class=\\\"joint-label#{joint == @selected_joint ? "-selected" : ""}\\\" id=\\\"Geared::#{fid}\\\">#{fname.inspect[1...-1]}</label></td>"
                  cmd << "<td colspan=\\\"1\\\"><label>Ratio</label></td>"
                  cmd << "<td colspan=\\\"4\\\"><input class=\\\"custom-text numeric-input\\\" id=\\\"gear-#{fid}\\\"></input></td>"
                  cmd << "</tr>"
                  cmd2 << "$('#gear-#{fid}').val('#{ format_value(gear_ratio.nil? ? 1.0 : gear_ratio, @precision) }');"
                }
                cmd << "</table>"
                cmd << "</div>"
              }
              cmd << "\");"
              cmd << "assign_joint_click_event(); update_input_events();"
              cmd << cmd2
              geared_data.clear
            end
          end
          # Activate Joint tab
          cmd << "activate_tab(4);" unless @selected_body
        else
          @selected_joint = nil
        end
        execute_js(cmd)
      end

      # @api private
      # Update UI sound tab.
      # @return [void]
      def update_sound_state
        return unless self.visible?
        model = Sketchup.active_model
        cmd = ""
        cmd << "$('#sound-list').empty();"
        dict = model.attribute_dictionary('MSPhysics Sounds', false)
        if dict
          dict.each_key { |name|
            ext = model.get_attribute('MSPhysics Sound Types', name)
            cmd << "$('#sound-list').append('<option value=#{name.inspect}>#{name}#{ext.is_a?(String) ? '.' + ext : ''}</option>');"
          }
        end
        if @selected_sound
          ext = model.get_attribute('MSPhysics Sound Types', @selected_sound)
          if MSPhysics::EMBEDDED_MUSIC_FORMATS.include?(ext)
            cmd << "$('#sound-command_music').val('simulation.play_music(#{@selected_sound.inspect})');"
          else
            cmd << "$('#sound-command_music').val('Not Supported');"
          end
          if MSPhysics::EMBEDDED_SOUND_FORMATS.include?(ext)
            cmd << "$('#sound-command_sound').val('simulation.play_sound(#{@selected_sound.inspect})');"
          else
            cmd << "$('#sound-command_sound').val('Not Supported');"
          end
        else
          cmd << "$('#sound-command_music').val('simulation.play_music(\"sound_name\")');"
          cmd << "$('#sound-command_sound').val('simulation.play_sound(\"sound_name\")');"
        end
        if MSPhysics.sdl_used?
          cmd << "$('#sound-no_sdl').css('display', 'none');"
        else
          cmd << "$('#sound-no_sdl').css('display', 'block');"
        end
        execute_js(cmd)
      end

      # @api private
      # Get default value of a particular attribute in joint.
      # @param [Sketchup::Group, Sketchup::ComonentInstance] joint
      # @param [String] attr_name
      # @return [Object, nil]
      def get_joint_default_value(joint, attr_name)
        jdict = 'MSPhysics Joint'
        attr = joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS)
        ang_ratio = attr == 'deg' ? 1.degrees : 1
        iang_ratio = 1.0 / ang_ratio
        attr = joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS)
        pos_ratio = case attr
          when 'mm'
            0.001
          when 'cm'
            0.01
          when 'dm'
            0.1
          when 'm'
            1.0
          when 'in'
            0.0254
          when 'ft'
            0.3048
          when 'yd'
            0.9144
        else
          1.0
        end
        ipos_ratio = 1.0 / pos_ratio
        joint_type = joint.get_attribute(jdict, 'Type')
        res = case joint_type
          when 'Hinge'
            case attr_name
              when 'Min'; MSPhysics::Hinge::DEFAULT_MIN * iang_ratio
              when 'Max'; MSPhysics::Hinge::DEFAULT_MAX * iang_ratio
              when 'Enable Limits'; MSPhysics::Hinge::DEFAULT_LIMITS_ENABLED
              when 'Friction'; MSPhysics::Hinge::DEFAULT_FRICTION
              when 'Accel'; MSPhysics::Hinge::DEFAULT_ACCEL
              when 'Damp'; MSPhysics::Hinge::DEFAULT_DAMP
              when 'Enable Rotate Back'; MSPhysics::Hinge::DEFAULT_ROTATE_BACK_ENABLED
              when 'Enable Strong Mode'; MSPhysics::Hinge::DEFAULT_STRONG_MODE_ENABLED
              when 'Start Angle'; MSPhysics::Hinge::DEFAULT_START_ANGLE * iang_ratio
              when 'Controller'; MSPhysics::Hinge::DEFAULT_CONTROLLER
            end
          when 'Motor'
            case attr_name
              when 'Accel'; MSPhysics::Motor::DEFAULT_ACCEL
              when 'Damp'; MSPhysics::Motor::DEFAULT_DAMP
              when 'Enable Free Rotate'; MSPhysics::Motor::DEFAULT_FREE_ROTATE_ENABLED
              when 'Controller'; MSPhysics::Motor::DEFAULT_CONTROLLER
            end
          when 'Servo'
            case attr_name
              when 'Min'; MSPhysics::Servo::DEFAULT_MIN * iang_ratio
              when 'Max'; MSPhysics::Servo::DEFAULT_MAX * iang_ratio
              when 'Enable Limits'; MSPhysics::Servo::DEFAULT_LIMITS_ENABLED
              when 'Rate'; MSPhysics::Servo::DEFAULT_RATE * iang_ratio
              when 'Power'; MSPhysics::Servo::DEFAULT_POWER
              when 'Reduction Ratio'; MSPhysics::Servo::DEFAULT_REDUCTION_RATIO
              when 'Controller'; MSPhysics::Servo::DEFAULT_CONTROLLER
            end
          when 'Slider'
            case attr_name
              when 'Min'; MSPhysics::Slider::DEFAULT_MIN * ipos_ratio
              when 'Max'; MSPhysics::Slider::DEFAULT_MAX * ipos_ratio
              when 'Enable Limits'; MSPhysics::Slider::DEFAULT_LIMITS_ENABLED
              when 'Friction'; MSPhysics::Slider::DEFAULT_FRICTION
              when 'Controller'; MSPhysics::Slider::DEFAULT_CONTROLLER
            end
          when 'Piston'
            case attr_name
              when 'Min'; MSPhysics::Piston::DEFAULT_MIN * ipos_ratio
              when 'Max'; MSPhysics::Piston::DEFAULT_MAX * ipos_ratio
              when 'Enable Limits'; MSPhysics::Piston::DEFAULT_LIMITS_ENABLED
              when 'Rate'; MSPhysics::Piston::DEFAULT_RATE * ipos_ratio
              when 'Power'; MSPhysics::Piston::DEFAULT_POWER
              when 'Reduction Ratio'; MSPhysics::Piston::DEFAULT_REDUCTION_RATIO
              when 'Controller'; MSPhysics::Piston::DEFAULT_CONTROLLER
            end
          when 'Spring'
            case attr_name
              when 'Min'; MSPhysics::Spring::DEFAULT_MIN * ipos_ratio
              when 'Max'; MSPhysics::Spring::DEFAULT_MAX * ipos_ratio
              when 'Enable Limits'; MSPhysics::Spring::DEFAULT_LIMITS_ENABLED
              when 'Accel'; MSPhysics::Spring::DEFAULT_ACCEL
              when 'Damp'; MSPhysics::Spring::DEFAULT_DAMP
              when 'Enable Strong Mode'; MSPhysics::Spring::DEFAULT_STRONG_MODE_ENABLED
              when 'Start Position'; MSPhysics::Spring::DEFAULT_START_POSITION * ipos_ratio
              when 'Controller'; MSPhysics::Spring::DEFAULT_CONTROLLER
            end
          when 'UpVector'
            case attr_name
              when 'Accel'; MSPhysics::UpVector::DEFAULT_ACCEL
              when 'Damp'; MSPhysics::UpVector::DEFAULT_DAMP
              when 'Enable Damper'; MSPhysics::UpVector::DEFAULT_DAMPER_ENABLED
              when 'Controller'; MSPhysics::UpVector::DEFAULT_PIN_DIR
            end
          when 'Corkscrew'
            case attr_name
              when 'Min Position'; MSPhysics::Corkscrew::DEFAULT_MIN_POSITION * ipos_ratio
              when 'Max Position'; MSPhysics::Corkscrew::DEFAULT_MAX_POSITION * ipos_ratio
              when 'Enable Linear Limits'; MSPhysics::Corkscrew::DEFAULT_LINEAR_LIMITS_ENABLED
              when 'Linear Friction'; MSPhysics::Corkscrew::DEFAULT_LINEAR_FRICTION
              when 'Min Angle'; MSPhysics::Corkscrew::DEFAULT_MIN_ANGLE * iang_ratio
              when 'Max Angle'; MSPhysics::Corkscrew::DEFAULT_MAX_ANGLE * iang_ratio
              when 'Enable Angular Limits'; MSPhysics::Corkscrew::DEFAULT_ANGULAR_LIMITS_ENABLED
              when 'Angular Friction'; MSPhysics::Corkscrew::DEFAULT_ANGULAR_FRICTION
            end
          when 'BallAndSocket'
            case attr_name
              when 'Accel'; MSPhysics::BallAndSocket::DEFAULT_ACCEL
              when 'Damp'; MSPhysics::BallAndSocket::DEFAULT_DAMP
              when 'Enable Damper'; MSPhysics::BallAndSocket::DEFAULT_DAMPER_ENABLED
              when 'Max Cone Angle'; MSPhysics::BallAndSocket::DEFAULT_MAX_CONE_ANGLE * iang_ratio
              when 'Enable Cone Limits'; MSPhysics::BallAndSocket::DEFAULT_CONE_LIMITS_ENABLED
              when 'Min Twist Angle'; MSPhysics::BallAndSocket::DEFAULT_MIN_TWIST_ANGLE * iang_ratio
              when 'Max Twist Angle'; MSPhysics::BallAndSocket::DEFAULT_MAX_TWIST_ANGLE * iang_ratio
              when 'Enable Twist Limits'; MSPhysics::BallAndSocket::DEFAULT_TWIST_LIMITS_ENABLED
            end
          when 'Universal'
            case attr_name
              when 'Min1'; MSPhysics::Universal::DEFAULT_MIN * iang_ratio
              when 'Max1'; MSPhysics::Universal::DEFAULT_MAX * iang_ratio
              when 'Enable Limits1'; MSPhysics::Universal::DEFAULT_LIMITS_ENABLED
              when 'Min2'; MSPhysics::Universal::DEFAULT_MIN * iang_ratio
              when 'Max2'; MSPhysics::Universal::DEFAULT_MAX * iang_ratio
              when 'Enable Limits2'; MSPhysics::Universal::DEFAULT_LIMITS_ENABLED
              when 'Friction'; MSPhysics::Universal::DEFAULT_FRICTION
              when 'Controller'; MSPhysics::Universal::DEFAULT_CONTROLLER
            end
          when 'CurvySlider'
            case attr_name
              when 'Enable Alignment'; MSPhysics::CurvySlider::DEFAULT_ALIGNMENT_ENABLED
              when 'Enable Rotation'; MSPhysics::CurvySlider::DEFAULT_ROTATION_ENABLED
              when 'Enable Loop'; MSPhysics::CurvySlider::DEFAULT_LOOP_ENABLED
              when 'Linear Friction'; MSPhysics::CurvySlider::DEFAULT_LINEAR_FRICTION
              when 'Angular Friction'; MSPhysics::CurvySlider::DEFAULT_ANGULAR_FRICTION
              when 'Alignment Power'; MSPhysics::CurvySlider::DEFAULT_ALIGNMENT_POWER
              when 'Controller'; MSPhysics::CurvySlider::DEFAULT_CONTROLLER
            end
          when 'CurvyPiston'
            case attr_name
              when 'Enable Alignment'; MSPhysics::CurvyPiston::DEFAULT_ALIGNMENT_ENABLED
              when 'Enable Rotation'; MSPhysics::CurvyPiston::DEFAULT_ROTATION_ENABLED
              when 'Enable Loop'; MSPhysics::CurvyPiston::DEFAULT_LOOP_ENABLED
              when 'Angular Friction'; MSPhysics::CurvyPiston::DEFAULT_ANGULAR_FRICTION
              when 'Rate'; MSPhysics::CurvyPiston::DEFAULT_RATE * ipos_ratio
              when 'Power'; MSPhysics::CurvyPiston::DEFAULT_POWER
              when 'Alignment Power'; MSPhysics::CurvyPiston::DEFAULT_ALIGNMENT_POWER
              when 'Reduction Ratio'; MSPhysics::CurvyPiston::DEFAULT_REDUCTION_RATIO
              when 'Controller'; MSPhysics::CurvyPiston::DEFAULT_CONTROLLER
              when 'Controller Mode'; MSPhysics::CurvyPiston::DEFAULT_CONTROLLER_MODE
            end
        end
        res = fix_numeric_value(res) if res.is_a?(Numeric)
        res
      end

      # @api private
      # Format value into string.
      # @param [Numeric] value
      # @param [Fixnum] precision
      # @return [String]
      def format_value(value, precision = 2)
        precision = AMS.clamp(precision.to_i, 0, 10)
        if precision == 0
          fv = value.to_i.to_s
        elsif value.to_f.zero?
          fv = '0.' + '0' * precision
        else
          fv = sprintf("%.#{precision}f", value.to_f)
        end
        return fv.to_f == value.to_f ? fv : '~ ' + fv
      end

      # @api private
      # Remove trailing 9s.
      # @param [Numeric] value
      # @return [String]
      def fix_numeric_value(value)
        (value * 1.0e10).round * 1.0e-10
      end

      # Open/Close MSPhysics UI.
      # @param [Boolean] state
      def visible=(state)
        return false if (state ? true : false) == self.visible?
        if state
          dbs = MSPhysics::DEFAULT_BODY_SETTINGS
          @material = MSPhysics::Material.new('Temp', dbs[:density], dbs[:static_friction], dbs[:dynamic_friction], dbs[:elasticity], dbs[:softness])
          iws = [520, 520]
          @dialog = ::UI::WebDialog.new(@title, false, @title, iws.x, iws.y, 800, 600, true)
          # Callbacks
          @dialog.add_action_callback('init') { |dlg, params|
            unless @init_called
              @init_called = true
              ds = eval(params)
              #@border_size = [iws.x - ds.x, iws.y - ds.y]
              if AMS::IS_PLATFORM_WINDOWS && @handle
                ws = AMS::Window.get_size(@handle)
                cr = AMS::Window.get_client_rect(@handle)
                cs = [cr[2] - cr[0], cr[3] - cr[1]]
                @border_size = [ws.x - cs.x, ws.y - cs.y]
              else
                @border_size = [2, 24]
              end
              Sketchup.active_model.selection.add_observer(@selection_observer)
            end
            update_state
          }
          @dialog.add_action_callback('editor_changed') { |dlg, params|
            if @selected_body && @selected_body.valid?
              code = dlg.get_element_value('temp_script_area')
              @selected_body.set_attribute('MSPhysics Script', 'Value', code)
            end
          }
          @dialog.add_action_callback('cursor_changed') { |dlg, params|
            if @selected_body && @selected_body.valid?
              @selected_body.set_attribute('MSPhysics Script', 'Cursor', params)
            end
          }
          @dialog.add_action_callback('open_link') { |dlg, params|
            ::UI.openURL(params)
          }
          @dialog.add_action_callback('open_ruby_core') { |dlg, params|
            ::UI.openURL("http://ruby-doc.org/core-#{RUBY_VERSION}/")
          }
          @dialog.add_action_callback('check_input_changed') { |dlg, params|
            settings = MSPhysics::Settings
            id, value = data = eval(params)
            dict, attr = id.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              settings = MSPhysics::Settings
              case attr
              when 'continuous_collision'
                settings.continuous_collision_check_enabled = value
              when 'full_screen_mode'
                settings.full_screen_mode_enabled = value
              when 'game_mode'
                settings.game_mode_enabled = value
              when 'hide_joint_layer'
                settings.hide_joint_layer_enabled = value
              when 'undo_on_end'
                settings.undo_on_end_enabled = value
              when 'collision_wireframe'
                settings.collision_wireframe_visible = value
              when 'axes'
                settings.axes_visible = value
              when 'aabb'
                settings.aabb_visible = value
              when 'contact_points'
                settings.contact_points_visible = value
              when 'contact_forces'
                settings.contact_forces_visible = value
              when 'bodies'
                settings.bodies_visible = value
              end
            when 'body', 'internal_body'
              if @selected_body && @selected_body.valid?
                @selected_body.set_attribute('MSPhysics Body', option, value)
              end
            when 'editor'
              if attr == 'print_margin'
                @editor_print_margin = value
              elsif attr == 'read_only'
                @editor_read_only = value
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint && @selected_joint.valid?
                if dict == 'joint' && attr =~ /constraint_type/i
                  if @selected_joint && @selected_joint.valid? && value
                    mode = attr.split(/\-/, 2)[1]
                    ctype = (mode == 'standard' ? 0 : (mode == 'flexible' ? 1 : 2))
                    @selected_joint.set_attribute('MSPhysics Joint', 'Constraint Type', ctype)
                  end
                elsif dict == 'fixed' && attr =~ /adjust_to/i
                  mode = attr.split(/\-/, 2)[1]
                  ctype = (mode == 'none' ? 0 : (mode == 'child' ? 1 : 2))
                  @selected_joint.set_attribute('MSPhysics Joint', 'Adjust To', ctype)
                else
                  @selected_joint.set_attribute('MSPhysics Joint', option, value)
                end
              end
            end
          }
          @dialog.add_action_callback('button_clicked') { |dlg, params|
            case params
            when 'sound-add'
              path = ::UI.openpanel('Add Sound File', @default_sound_path)
              if path
                path.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
                @default_sound_path = path
                begin
                  add_sound(path)
                  update_sound_state
                rescue TypeError => e
                  err_message = e.message
                  err_message.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
                  ::UI.messagebox(err_message)
                end
              end
            when 'sound-remove'
              if @selected_sound
                remove_sound(@selected_sound)
                @selected_sound = nil
                update_sound_state
              end
            when 'sound-remove_all'
              dict = Sketchup.active_model.attribute_dictionary('MSPhysics Sounds', false)
              num_sounds = dict ? dict.length : 0
              if num_sounds < 2 || ::UI.messagebox('Are you sure you want to remove all sounds?', MB_YESNO) == IDYES
                remove_all_sounds
                @selected_sound = nil
                update_sound_state
              end
            when 'sound-play'
              if MSPhysics.sdl_used? && @selected_sound
                music = MSPhysics::Music.get_by_name(@selected_sound)
                unless music
                  begin
                    buf = Sketchup.active_model.get_attribute('MSPhysics Sounds', @selected_sound).pack('l*')
                    music = MSPhysics::Music.create_from_buffer(buf, buf.size)
                    buf = nil
                    MSPhysics::Music.set_name(music, @selected_sound)
                  rescue Exception => e
                    ::UI.messagebox("Can't play selected sound, \"#{@selected_sound}\", as it seems to be invalid!")
                  end
                end
                MSPhysics::Music.play(music, 0) if music
              end
            when 'sound-toggle_pause'
              if MSPhysics.sdl_used?
                if MSPhysics::Music.is_paused?
                  MSPhysics::Music.resume
                else
                  MSPhysics::Music.pause
                end
              end
            when 'sound-stop'
              if MSPhysics.sdl_used?
                MSPhysics::Music.stop
              end
            when 'motor-generate_slider'
              if @selected_joint && @selected_joint.valid?
                jdict = 'MSPhysics Joint'
                name = @selected_joint.name.to_s
                name = 'Motor-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                controller = "slider(#{name.inspect}, 0.0, -1.0, 1.0)"
                @selected_joint.set_attribute(jdict, 'Controller', controller)
                execute_js("$('#motor-controller').val(#{controller.inspect});")
              end
            when 'servo-generate_slider'
              if @selected_joint && @selected_joint.valid?
                jdict = 'MSPhysics Joint'
                attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS)
                ang_ratio = attr == 'deg' ? 1.degrees : 1
                iang_ratio = 1.0 / ang_ratio
                name = @selected_joint.name.to_s
                name = 'Servo-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                min = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Servo::DEFAULT_MIN * iang_ratio)).to_f
                max = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Servo::DEFAULT_MAX * iang_ratio)).to_f
                controller = "slider(#{name.inspect}, 0.0, #{min}, #{max})"
                @selected_joint.set_attribute(jdict, 'Controller', controller)
                execute_js("$('#servo-controller').val(#{controller.inspect});")
              end
            when 'piston-generate_slider'
              if @selected_joint && @selected_joint.valid?
                jdict = 'MSPhysics Joint'
                attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS)
                pos_ratio = case attr
                  when 'mm'
                    0.001
                  when 'cm'
                    0.01
                  when 'dm'
                    0.1
                  when 'm'
                    1.0
                  when 'in'
                    0.0254
                  when 'ft'
                    0.3048
                  when 'yd'
                    0.9144
                else
                  1.0
                end
                ipos_ratio = 1.0 / pos_ratio
                name = @selected_joint.name.to_s
                name = 'Piston-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                min = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Piston::DEFAULT_MIN * ipos_ratio)).to_f
                max = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Piston::DEFAULT_MAX * ipos_ratio)).to_f
                controller = "slider(#{name.inspect}, 0.0, #{min}, #{max})"
                @selected_joint.set_attribute(jdict, 'Controller', controller)
                execute_js("$('#piston-controller').val(#{controller.inspect});")
              end
            when 'curvy_piston-generate_slider'
              if @selected_joint && @selected_joint.valid?
                jdict = 'MSPhysics Joint'
                attr = @selected_joint.get_attribute(jdict, 'Position Units', MSPhysics::DEFAULT_POSITION_UNITS)
                pos_ratio = case attr
                  when 'mm'
                    0.001
                  when 'cm'
                    0.01
                  when 'dm'
                    0.1
                  when 'm'
                    1.0
                  when 'in'
                    0.0254
                  when 'ft'
                    0.3048
                  when 'yd'
                    0.9144
                else
                  1.0
                end
                ipos_ratio = 1.0 / pos_ratio
                name = @selected_joint.name.to_s
                name = 'CurvyPiston-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                if @selected_joint.get_attribute(jdict, 'Controller Mode', MSPhysics::CurvyPiston::DEFAULT_CONTROLLER_MODE).to_i == 0
                  min = 0.0
                  max = MSPhysics::JointConnectionTool.get_curve_length(@selected_joint, nil).to_m * ipos_ratio
                  controller = "slider(#{name.inspect}, 0.0, #{min}, #{ sprintf("%0.3f", max) })"
                  @selected_joint.set_attribute(jdict, 'Controller', controller)
                  execute_js("$('#curvy_piston-controller').val(#{controller.inspect});")
                else
                  controller = "slider(#{name.inspect}, 0.0, -1.0, 1.0)"
                  @selected_joint.set_attribute(jdict, 'Controller', controller)
                  execute_js("$('#curvy_piston-controller').val(#{controller.inspect});")
                end
              end
            when 'body-assign_props_to_all'
              if @selected_body && @selected_body.valid? && (@selected_body.name.size > 0 || ::UI.messagebox("This body is unnamed. Would you like to assign identical properties to all the unnamed bodies?", MB_YESNO) == IDYES)
                dict = @selected_body.attribute_dictionary('MSPhysics Body')
                if dict
                  props = ['Ignore', 'Collidable', 'Static', 'Frozen', 'Auto Sleep', 'Enable Friction', 'Magnetic', 'Enable Script', 'Continuous Collision', 'Enable Gravity', 'Thruster Lock Axis', 'Emitter Lock Axis', 'Enable Thruster', 'Enable Emitter', 'Connect Closest Joints', 'Thruster Controller', 'Emitter Controller', 'Emitter Rate', 'Emitter Lifetime', 'Density', 'Mass', 'Static Friction', 'Dynamic Friction', 'Elasticity', 'Softness', 'Magnet Force', 'Magnet Range', 'Linear Damping', 'Angular Damping', 'Material', 'Shape', 'Weight Control']
                  model = Sketchup.active_model
                  op = 'MSPhysics Body - Assign Properties to All'
                  if Sketchup.version.to_i > 6
                    model.start_operation(op, true)
                  else
                    model.start_operation(op)
                  end
                  model.definitions.each { |cd|
                    cd.instances.each { |inst|
                      next if inst == @selected_body || inst.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' || inst.name != @selected_body.name
                      props.each { |prop|
                        value = dict[prop]
                        inst.set_attribute('MSPhysics Body', prop, value) if value != nil
                      }
                    }
                  }
                  model.commit_operation
                end
              end
            when 'body-assign_script_to_all'
              if @selected_body && @selected_body.valid? && (@selected_body.name.size > 0 || ::UI.messagebox("This body is unnamed. Would you like to assign identical script to all the unnamed bodies?", MB_YESNO) == IDYES)
                dict = @selected_body.attribute_dictionary('MSPhysics Script')
                if dict
                  props = ['Value', 'Cursor']
                  model = Sketchup.active_model
                  op = 'MSPhysics Body - Assign Script to All'
                  if Sketchup.version.to_i > 6
                    model.start_operation(op, true)
                  else
                    model.start_operation(op)
                  end
                  model.definitions.each { |cd|
                    cd.instances.each { |inst|
                      next if inst == @selected_body || inst.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' || inst.name != @selected_body.name
                      props.each { |prop|
                        value = dict[prop]
                        inst.set_attribute('MSPhysics Script', prop, value) if value != nil
                      }
                    }
                  }
                  model.commit_operation
                end
              end
            when 'joint-assign_to_all'
              if @selected_joint && @selected_joint.valid?
                dict = @selected_joint.attribute_dictionary('MSPhysics Joint')
                name = @selected_joint.name
                name = dict['ID'].to_s if name.empty?
                type = dict['Type']
                if dict && name.size > 0 && type
                  model = Sketchup.active_model
                  op = 'MSPhysics Joint - Assign to All'
                  if Sketchup.version.to_i > 6
                    model.start_operation(op, true)
                  else
                    model.start_operation(op)
                  end
                  msgbox_displayed = false
                  change_controllers = false
                  model.definitions.each { |cd|
                    cd.instances.each { |inst|
                      next if inst == @selected_joint || inst.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint' || inst.get_attribute('MSPhysics Joint', 'Type') != type
                      inst_name = inst.name.empty? ? inst.get_attribute('MSPhysics Joint', 'ID').to_s : inst.name
                      next if inst_name != name
                      dict.each { |key, value|
                        next if key == 'ID' || key == 'Type' || value == nil
                        if key == 'Controller'
                          if !msgbox_displayed
                            change_controllers = ::UI.messagebox("Should the controllers be changed as well?", MB_YESNO) == IDYES
                            msgbox_displayed = true
                          end
                          inst.set_attribute('MSPhysics Joint', key, value) if change_controllers
                        else
                          inst.set_attribute('MSPhysics Joint', key, value)
                        end
                      }
                    }
                  }
                  model.commit_operation
                end
              end
            end
          }
          @dialog.add_action_callback('numeric_input_changed') { |dlg, params|
            id, value = eval(params)
            dict, attr = id.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              case attr
              when 'gravity'
                MSPhysics::Settings.gravity = value
                value = MSPhysics::Settings.gravity
              when 'material_thickness'
                value = AMS.clamp(value, 0.0, 1.0)
                MSPhysics::Settings.material_thickness = value / 32.0
              when 'world_scale'
                MSPhysics::Settings.world_scale = value
                value = MSPhysics::Settings.world_scale
              end
            when 'body'
              if @selected_body && @selected_body.valid?
                method = "#{attr}="
                if @material.respond_to?(method)
                  value = @material.method(method).call(value)
                  if value != @activated_value
                    execute_js("$('#body-material').val('Custom'); $('#body-material').trigger('chosen:updated');")
                    @selected_body.set_attribute('MSPhysics Body', 'Material', 'Custom')
                  end
                elsif attr == 'magnet_range'
                  value = AMS.clamp(value, 0.0, nil)
                elsif attr == 'linear_damping' || attr == 'angular_damping'
                  value = AMS.clamp(value, 0.0, 1.0)
                end
                @selected_body.set_attribute('MSPhysics Body', option, value.to_i.is_a?(Bignum) ? value.to_s : value)
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint && @selected_joint.valid?
                if %w(stiffness reduction_ratio).include?(attr)
                  value = AMS.clamp(value, 0.0, 1.0)
                elsif %w(accel damp breaking_force rate power alignment_power friction linear_friction angular_friction).include?(attr)
                  value = AMS.clamp(value, 0.0, nil)
                end
                @selected_joint.set_attribute('MSPhysics Joint', option, value.to_i.is_a?(Bignum) ? value.to_s : value)
              end
            when 'gear'
              if @selected_joint && @selected_joint.valid?
                joint = @geared_joints[attr]
                if joint && joint.valid?
                  gear_type = attr.split(/\_/).first.to_i
                  MSPhysics::JointConnectionTool.set_gear_ratio(@selected_joint, joint, gear_type, value)
                end
              end
            end
            execute_js("$('##{id}').val('#{ format_value(value, @precision) }');")
            @activated_value = nil
          }
          @dialog.add_action_callback('fixnum_input_changed') { |dlg, params|
            id, value = eval(params)
            dict, attr = id.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              if attr == 'update_rate'
                MSPhysics::Settings.update_rate = value
                value = MSPhysics::Settings.update_rate
              end
            when 'body'
              if @selected_body && @selected_body.valid?
                if attr == 'emitter_rate'
                  value = AMS.clamp(value.round, 1, nil)
                elsif attr == 'emitter_lifetime'
                  value = AMS.clamp(value.round, 0, nil)
                end
                @selected_body.set_attribute('MSPhysics Body', option, value.to_i.is_a?(Bignum) ? value.to_s : value)
              end
            when 'joint', *MSPhysics::JOINT_NAMES
            end
            execute_js("$('##{id}').val('#{ value.to_i }')")
          }
          @dialog.add_action_callback('numeric_input_focused') { |dlg, params|
            dict, attr = params.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            value = case dict
            when 'simulation'
              case attr
              when 'gravity'
                MSPhysics::Settings.gravity
              when 'material_thickness'
                MSPhysics::Settings.material_thickness * 32.0
              when 'world_scale'
                MSPhysics::Settings.world_scale
              end
            when 'body'
              if @selected_body && @selected_body.valid?
                default = MSPhysics::DEFAULT_BODY_SETTINGS
                @selected_body.get_attribute('MSPhysics Body', option, default[attr.to_sym])
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint && @selected_joint.valid?
                begin
                  #default = eval("MSPhysics::#{dict.split(/\_/).map { |w| w.capitalize }.join}::DEFAULT_#{attr.upcase}")
                  default = get_joint_default_value(@selected_joint, option)
                rescue Exception => e
                  default = 0
                end
                @selected_joint.get_attribute('MSPhysics Joint', option, default)
              end
            when 'gear'
              if @selected_joint && @selected_joint.valid?
                joint = @geared_joints[attr]
                if joint && joint.valid?
                  gear_type = attr.split(/\_/).first.to_i
                  MSPhysics::JointConnectionTool.get_gear_ratio(@selected_joint, joint, gear_type)
                end
              end
            end
            if value.is_a?(Numeric)
              execute_js("$('##{params}').val(#{value});")
              @activated_value = value
            end
          }
          @dialog.add_action_callback('controller_input_changed') { |dlg, id|
            dict, attr = id.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
            when 'body'
              if @selected_body && @selected_body.valid?
                code = dlg.get_element_value(id)
                @selected_body.set_attribute('MSPhysics Body', option, code)
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint && @selected_joint.valid?
                code = dlg.get_element_value(id)
                @selected_joint.set_attribute('MSPhysics Joint', option, code)
              end
            end
          }
          @dialog.add_action_callback('text_input_changed') { |dlg, id|
            dict, attr = id.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'body'
              if @selected_body && @selected_body.valid?
                code = dlg.get_element_value(id)
                if attr == 'name'
                  @selected_body.name = code
                end
              end
            when 'joint'
              if @selected_joint && @selected_joint.valid?
                code = dlg.get_element_value(id)
                if attr == 'name'
                  @selected_joint.name = code
                else
                  @selected_joint.set_attribute('MSPhysics Joint', option, code)
                end
              end
            end
          }
          @dialog.add_action_callback('select_input_changed') { |dlg, params|
            id, value = eval(params)
            dict, attr = id.split(/\-/, 2)
            words = attr.split(/\_/)
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              case attr
              when 'update_timestep'
                MSPhysics::Settings.update_timestep = 1.0 / value.to_i
              when 'solver_model'
                MSPhysics::Settings.solver_model = value.to_i
              end
            when 'body'
              if @selected_body && @selected_body.valid?
                model = Sketchup.active_model
                if attr == 'material'
                  op = 'MSPhysics Body - Change Material'
                  if Sketchup.version.to_i > 6
                    model.start_operation(op, true)
                  else
                    model.start_operation(op)
                  end
                  @selected_body.set_attribute('MSPhysics Body', option, value)
                  if value == MSPhysics::DEFAULT_BODY_SETTINGS[:material_name]
                    ['Material', 'Density', 'Static Friction', 'Dynamic Friction', 'Enable Friction', 'Elasticity', 'Softness'].each { |opt|
                      @selected_body.delete_attribute('MSPhysics Body', opt)
                    }
                  else
                    material = MSPhysics::Materials.material_by_name(value)
                    if material
                      @selected_body.set_attribute('MSPhysics Body', 'Density', material.density)
                      @selected_body.set_attribute('MSPhysics Body', 'Static Friction', material.static_friction)
                      @selected_body.set_attribute('MSPhysics Body', 'Dynamic Friction', material.dynamic_friction)
                      @selected_body.set_attribute('MSPhysics Body', 'Enable Friction', true)
                      @selected_body.set_attribute('MSPhysics Body', 'Elasticity', material.elasticity)
                      @selected_body.set_attribute('MSPhysics Body', 'Softness', material.softness)
                    end
                  end
                  update_state
                  model.commit_operation
                else
                  @selected_body.set_attribute('MSPhysics Body', option, value)
                end
              end
            when 'editor'
              if id == 'editor-theme'
                @editor_theme = value.to_s
              elsif id == 'editor-font'
                @editor_font = value.to_i
              elsif id == 'editor-wrap'
                @editor_wrap = value.to_s
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint && @selected_joint.valid?
                @selected_joint.set_attribute('MSPhysics Joint', option, value)
                update_joint_state
              end
            end
          }
          @dialog.add_action_callback('tab_changed') { |dlg, params|
            tab = params.to_i
            @last_active_body_tab = tab if tab == 2 || tab == 3
            @active_tab = tab
          }
          @dialog.add_action_callback('editor_size_changed') { |dlg, params|
            if @active_tab == 3 && @selected_body && @selected_body.valid? && @selected_body.parent.is_a?(Sketchup::Model)
              @editor_size = eval(params) if !AMS::IS_PLATFORM_WINDOWS || AMS::Window.is_restored?(@handle)
            end
          }
          @dialog.add_action_callback('size_changed') { |dlg, params|
            update_dialog_style
            if @active_tab == 3 && @selected_body && @selected_body.valid? && @selected_body.parent.is_a?(Sketchup::Model)
              wsx = @border_size.x + @editor_size.x
              wsy = @border_size.y + @editor_size.y
              if AMS::IS_PLATFORM_WINDOWS && @handle
                AMS::Window.set_size(@handle, wsx, wsy, false)
              else
                dlg.set_size(wsx, wsy)
              end
            else
              ds = eval(params)
              wsx = @border_size.x + ds.x
              wsy = @border_size.y + ds.y
              if AMS::IS_PLATFORM_WINDOWS && @handle
                AMS::Window.set_size(@handle, wsx, wsy, false)
              else
                dlg.set_size(wsx, wsy)
              end
            end
          }
          @dialog.add_action_callback('mouse_enter') { |dlg, params|
            update_simulation_state
          }
          @dialog.add_action_callback('sound_select_changed') { |dlg, params|
            @selected_sound = params
            cmd = ''
            ext = Sketchup.active_model.get_attribute('MSPhysics Sound Types', @selected_sound)
            if MSPhysics::EMBEDDED_MUSIC_FORMATS.include?(ext)
              cmd << "$('#sound-command_music').val('simulation.play_music(#{@selected_sound.inspect})');"
            else
              cmd << "$('#sound-command_music').val('Not Supported');"
            end
            if MSPhysics::EMBEDDED_SOUND_FORMATS.include?(ext)
              cmd << "$('#sound-command_sound').val('simulation.play_sound(#{@selected_sound.inspect})');"
            else
              cmd << "$('#sound-command_sound').val('Not Supported');"
            end
            execute_js(cmd)
          }
          @dialog.add_action_callback('joint_label_selected') { |dlg, params|
            if (params =~ /Internal::/) == 0
              fid = params.split('Internal::', 2)[1]
              jdata = @body_internal_joints[fid]
              if jdata && jdata[0] && jdata[0].valid?
                @selected_joint = jdata[0]
                @selection_observer.enabled = false
                sel = Sketchup.active_model.selection
                sel.clear
                sel.add(@selected_body) if @selected_body && @selected_body.valid?
                sel.add(@selected_joint)
                @selection_observer.enabled = true
                update_joint_state
                execute_js("setTimeout(function() { activate_tab(4); update_size(); }, 250);")
              end
            elsif (params =~ /Connected::/) == 0
              fid = params.split('Connected::', 2)[1]
              jdata = @body_connected_joints[fid]
              if jdata && jdata[0] && jdata[0].valid?
                @selected_joint = jdata[0]
                @selection_observer.enabled = false
                sel = Sketchup.active_model.selection
                sel.clear
                sel.add(@selected_body) if @selected_body && @selected_body.valid?
                sel.add(@selected_joint)
                @selection_observer.enabled = true
                update_joint_state
                execute_js("setTimeout(function() { activate_tab(4); update_size(); }, 250);")
              end
            elsif (params =~ /Geared::/) == 0
              fid = params.split('Geared::', 2)[1]
              joint = @geared_joints[fid]
              if joint && joint.valid?
                @selected_joint = joint
                @selection_observer.enabled = false
                sel = Sketchup.active_model.selection
                sel.clear
                sel.add(@selected_body) if @selected_body && @selected_body.valid?
                sel.add(@selected_joint)
                @selection_observer.enabled = true
                update_joint_state
                execute_js("setTimeout(function() { activate_tab(4); update_size(); }, 250);")
              end
            end
          }
          @dialog.add_action_callback('debug') { |dlg, params|
            puts params
          }
          # Set content
          dir = File.dirname(__FILE__)
          dir.force_encoding('UTF-8') if RUBY_VERSION !~ /1.8/
          url = File.join(dir, 'html/dialog.html')
          @dialog.set_file(url)
          # Show dialog
          AMS::IS_PLATFORM_WINDOWS ? @dialog.show : @dialog.show_modal
          # Assign the on_close callback. Important: This must be called after
          # showing dialog in order to work on Mac OS X.
          @dialog.set_on_close() {
            execute_js("update_editor_size(); if (document.activeElement instanceof HTMLInputElement) document.activeElement.blur();")
            @dialog = nil
            @handle = nil
            @init_called = false
            @selected_body = nil
            @selected_joint = nil
            @active_tab = 1
            @activated_value = nil
            @selected_sound = nil
            Sketchup.active_model.selection.remove_observer(@selection_observer)
          }
          # Find dialog window handle
          @handle = AMS::IS_PLATFORM_WINDOWS ? AMS::Sketchup.find_window_by_caption(@title) : nil
          if @handle
            AMS::Sketchup.ignore_dialog(@handle)
          end
          # Set dialog style
          update_dialog_style
        else
          @dialog.close
        end
        true
      end

      # Determine if MSPhysics UI is open.
      # @return [Boolean]
      def visible?
        @dialog ? true : false
      end

      # Open MSPhysics UI and set pointer to the location of an error.
      # @param [MSPhysics::ScriptException] error
      # @return [Boolean] success
      def locate_error(error)
        AMS.validate_type(error, MSPhysics::ScriptException)
        return false unless error.entity.valid?
        model = Sketchup.active_model
        @selection_observer.enabled = false
        model.selection.clear
        model.selection.add(error.entity)
        @selection_observer.enabled = true
        @last_active_body_tab = 3
        msg = "setTimeout(function() { activate_tab(3); update_size(); }, 100);"
        msg << "setTimeout(function() { editor_set_cursor(#{error.line}, 0); editor_select_current_line(); }, 200);" if error.line
        if @dialog
          update_state
          execute_js(msg)
        else
          self.visible = true
          t = ::UI.start_timer(0.25, false) {
            ::UI.stop_timer(t)
            execute_js(msg)
          }
        end
        true
      end

      # Add sound to UI.
      # @param [String] path
      # @return [String] Name of added file.
      # @raise [TypeError] if file path is invalid.
      # @raise [TypeError] if file is over 100 megabytes.
      # @raise [TypeError] if SDL is not supported on your system.
      def add_sound(path)
        unless File.exist?(path)
          raise(TypeError, "Invalid path!", caller)
        end
        unless MSPhysics.sdl_used?
          raise(TypeError, "SDL is not supported on your system!", caller)
        end
        name = File.basename(path, File.extname(path)).gsub(/\'/, ' ')
        size = File.size(path) * 1.0e-6
        if size > 100
          raise(TypeError, "Selected file, \"#{File.basename(path)}\", is #{sprintf("%0.2f", size)} megabytes in size. File size must be no more than 100 megabytes!", caller)
        end
        ext = File.extname(path).downcase[1..-1]
        if MSPhysics::EMBEDDED_MUSIC_FORMATS.include?(ext) || MSPhysics::EMBEDDED_SOUND_FORMATS.include?(ext)
          file = File.new(path, 'rb')
          Sketchup.active_model.set_attribute('MSPhysics Sounds', name, file.read.unpack('l*'))
          Sketchup.active_model.set_attribute('MSPhysics Sound Types', name, ext)
          file.close
          music = MSPhysics::Music.get_by_name(name)
          MSPhysics::Music.destroy(music) if music
          name
        else
          raise(TypeError, "File format is not supported!", caller)
        end
      end

      # Remove sound from UI.
      # @param [String] name
      def remove_sound(name)
        if MSPhysics.sdl_used?
          music = MSPhysics::Music.get_by_name(name)
          MSPhysics::Music.destroy(music) if music
        end
        dict1 = Sketchup.active_model.attribute_dictionary('MSPhysics Sounds', false)
        dict1.delete_key(name.to_s) if dict1
        dict2 = Sketchup.active_model.attribute_dictionary('MSPhysics Sound Types', false)
        dict2.delete_key(name.to_s) if dict2
      end

      # Remove all sounds from UI.
      def remove_all_sounds
        dict = Sketchup.active_model.attribute_dictionary('MSPhysics Sounds', false)
        if MSPhysics.sdl_used? && dict
          dict.each_key { |name|
            music = MSPhysics::Music.get_by_name(name)
            MSPhysics::Music.destroy(music) if music
          }
        end
        Sketchup.active_model.attribute_dictionaries.delete('MSPhysics Sounds')
        Sketchup.active_model.attribute_dictionaries.delete('MSPhysics Sound Types')
      end

      # Load editor settings from registry.
      def load_editor_settings
        theme = Sketchup.read_default('MSPhysics', 'Editor Theme', DEFAULT_THEME).to_s
        font = Sketchup.read_default('MSPhysics', 'Editor Font', DEFAULT_FONT).to_i
        wrap = Sketchup.read_default('MSPhysics', 'Editor Wrap', DEFAULT_WRAP).to_s
        print_margin = Sketchup.read_default('MSPhysics', 'Editor Print Margin', DEFAULT_PRINT_MARGIN) ? true : false
        editor_size = Sketchup.read_default('MSPhysics', 'Editor Size', DEFAULT_EDITOR_SIZE.inspect)
        @editor_theme = theme
        @editor_font = font
        @editor_wrap = wrap
        @editor_print_margin = print_margin
        begin
          @editor_size = Kernel.eval(editor_size)
          if !@editor_size.is_a?(Array) || !@editor_size.x.is_a?(Fixnum) || !@editor_size.y.is_a?(Fixnum)
            @editor_size = DEFAULT_EDITOR_SIZE.dup
          end
        rescue Exception => e
          @editor_size = DEFAULT_EDITOR_SIZE.dup
        end
      end

      # Save editor settings into registry.
      def save_editor_settings
        Sketchup.write_default('MSPhysics', 'Editor Theme', @editor_theme.to_s)
        Sketchup.write_default('MSPhysics', 'Editor Font', @editor_font.to_i)
        Sketchup.write_default('MSPhysics', 'Editor Wrap', @editor_wrap.to_s)
        Sketchup.write_default('MSPhysics', 'Editor Print Margin', @editor_print_margin)
        Sketchup.write_default('MSPhysics', 'Editor Size', @editor_size.inspect)
      end

      # @!visibility private
      def selection_observer
        @selection_observer
      end

      # @!visibility private
      def init
        @selection_observer = MSPhysics::Dialog::SelectionObserver.new
        Sketchup.add_observer(MSPhysics::Dialog::AppObserver.new)
        MSPhysics::Dialog.load_editor_settings
      end

    end # class << self

    # @!visibility private
    class SelectionObserver < ::Sketchup::SelectionObserver

      def initialize
        @enabled = true
        @selection_changed = true
      end

      def enabled?
        @enabled
      end

      def enabled=(state)
        @enabled = state ? true : false
      end

      def selection_changed?
        @selection_changed
      end

      def selection_changed=(state)
        @selection_changed = state ? true : false
      end

      def onSelectionBulkChange(sel)
        return if !@enabled || MSPhysics::Simulation.active?
        @selection_changed = true
        MSPhysics::Dialog.update_state
      end

      def onSelectionAdded(sel, entity)
        return if !@enabled || MSPhysics::Simulation.active?
        @selection_changed = true
        MSPhysics::Dialog.update_state
      end

      def onSelectionCleared(sel)
        return if !@enabled || MSPhysics::Simulation.active?
        @selection_changed = false
        MSPhysics::Dialog.update_state
      end

    end # class SelectionObserver

    # @!visibility private
    class AppObserver < ::Sketchup::AppObserver

      def onNewModel(model)
        if MSPhysics::Dialog.visible?
          model.selection.add_observer(MSPhysics::Dialog.selection_observer)
          MSPhysics::Dialog.update_state
        end
      end

      def onOpenModel(model)
        if MSPhysics::Dialog.visible?
          model.selection.add_observer(MSPhysics::Dialog.selection_observer)
          MSPhysics::Dialog.update_state
        end
      end

      def onQuit
        MSPhysics::Dialog.save_editor_settings
      end

    end # class AppObserver

  end # module Dialog
end # module MSPhysics
