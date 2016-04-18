module MSPhysics

  # @since 1.0.0
  module Dialog

    DEFAULT_THEME = 'tomorrow_night'
    DEFAULT_FONT = 12
    DEFAULT_WRAP = 'free'
    DEFAULT_PRINT_MARGIN = true

    @dialog = nil
    @handle = nil
    @title = 'MSPhysics UI'
    @editor_size = [800,600]
    @init_called = false
    @selected_body = nil
    @selected_joint = nil
    @active_tab = 1
    @last_active_body_tab = 2
    @cleared = false
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
    @observer_temp_disabled = false

    class << self

      # @api private
      # Update state of all UI.
      # @return [void]
      def update_state
        return unless is_visible?
        # Simulation dialog
        update_simulation_state
        # Sound dialog
        update_sound_state
        # Body dialog
        update_body_state
        # Joint dialog
        update_joint_state
        # Update size
        @dialog.execute_script("update_size();")
      end

      # @api private
      # Update dialog style.
      # @return [void]
      def update_dialog_style
        return unless @handle
        style = AMS::Window.get_long(@handle, -16)
        new_style = @active_tab == 3 && @selected_body && @selected_body.parent.is_a?(Sketchup::Model) ? style | 0x00050000 : style & ~0x01050000
        AMS::Window.lock_update(@handle)
        AMS::Window.set_long(@handle, -16, new_style)
        AMS::Window.lock_update(nil)
        AMS::Window.set_pos(@handle, 0, 0, 0, 0, 0, 0x0237)
      end

      # @api private
      # Update UI simulation tab.
      # @return [void]
      def update_simulation_state
        return unless is_visible?
        settings = MSPhysics::Settings
        cmd = ''
        cmd << "$('#simulation-solver_model').val('#{settings.get_solver_model}');"
        cmd << "$('#simulation-solver_model').trigger('chosen:updated');"
        cmd << "$('#simulation-friction_model').val('#{settings.get_friction_model}');"
        cmd << "$('#simulation-friction_model').trigger('chosen:updated');"
        cmd << "$('#simulation-update_timestep').val('#{(1.0/settings.get_update_timestep).round}');"
        cmd << "$('#simulation-update_timestep').trigger('chosen:updated');"
        cmd << "$('#simulation-continuous_collision').prop('checked', #{settings.get_continuous_collision_state});"
        cmd << "$('#simulation-collision_wireframe').prop('checked', #{settings.collision_wireframe_visible?});"
        cmd << "$('#simulation-axes').prop('checked', #{settings.axes_visible?});"
        cmd << "$('#simulation-aabb').prop('checked', #{settings.aabb_visible?});"
        cmd << "$('#simulation-contact_points').prop('checked', #{settings.contact_points_visible?});"
        cmd << "$('#simulation-contact_forces').prop('checked', #{settings.contact_forces_visible?});"
        cmd << "$('#simulation-bodies').prop('checked', #{settings.bodies_visible?});"
        cmd << "$('#simulation-gravity').val('#{ format_value(settings.get_gravity, @precision) }');"
        cmd << "$('#simulation-material_thickness').val('#{ format_value(settings.get_material_thickness * 32.0, @precision) }');"
        cmd << "$('#simulation-update_rate').val('#{settings.get_update_rate}');"
        cmd << "$('#simulation-world_scale').val('#{ format_value(settings.get_world_scale, @precision) }');"
        @dialog.execute_script(cmd)
      end

      # @api private
      # Update UI properties tab.
      # @return [void]
      def update_body_state
        return unless is_visible?
        model = Sketchup.active_model
        sel = model.selection.to_a
        bodies = []
        sel.each { |ent|
          next unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
          bodies << ent if ent.get_attribute('MSPhysics', 'Type', 'Body') == 'Body'
        }
        cmd = ''
        @body_internal_joints.clear
        @body_connected_joints.clear
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
            cursor = ( eval(@selected_body.get_attribute('MSPhysics Script', 'Cursor', '[1,0]')) rescue [1,0] )
            cmd << "editor_set_cursor(#{cursor[0]}, #{cursor[1]});"
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
              property = option.downcase.gsub(' ', '_')
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
            materials = MSPhysics::Materials.sort { |a, b| a.get_name <=> b.get_name }
            materials.each { |material|
              cmd << "$('#body-material').append('<option value=\"#{material.get_name}\">#{material.get_name}</option>');"
            }
            material = @selected_body.get_attribute('MSPhysics Body', 'Material', default[:material_name]).to_s
            cmd << "$('#body-material').val('#{material}');"
            cmd << "$('#body-material').trigger('chosen:updated');"
            # Display state and other check-box properties
            ['Ignore', 'Collidable', 'Static', 'Frozen', 'Auto Sleep', 'Enable Friction', 'Magnetic', 'Enable Script', 'Continuous Collision', 'Enable Gravity', 'Thruster Lock Axis', 'Emitter Lock Axis', 'Enable Thruster', 'Enable Emitter', 'Connect Closest Joints'].each { |option|
              property = option.downcase.gsub(' ', '_')
              default_state = default[property.to_sym]
              state = @selected_body.get_attribute('MSPhysics Body', option, default_state) ? true : false
              cmd << "$('#body-#{ property }').prop('checked', #{ state });"
            }
            # Display numeric properties
            ['Density', 'Mass', 'Static Friction', 'Kinetic Friction', 'Dynamic Friction', 'Elasticity', 'Softness', 'Magnet Force', 'Magnet Range', 'Linear Damping', 'Angular Damping'].each { |option|
              property = option.downcase.gsub(' ', '_')
              attr = @selected_body.get_attribute('MSPhysics Body', option, default[property.to_sym])
              value = attr.to_f rescue default[property.to_sym].to_f
              cmd << "$('#body-#{ property }').val('#{ format_value(value, @precision) }');"
            }
            # Display fixnum properties
            ['Emitter Rate', 'Emitter Lifetime'].each { |option|
              property = option.downcase.gsub(' ', '_')
              attr = @selected_body.get_attribute('MSPhysics Body', option, default[property.to_sym])
              value = attr.to_f rescue default[property.to_sym].to_f
              cmd << "$('#body-#{ property }').val('#{ value.round }');"
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
            cmd << "$('#body-internal_joints_table').empty();"
            identical_joints = {}
            MSPhysics::Group.get_entities(@selected_body).each { |e|
              if (e.is_a?(Sketchup::ComponentInstance) || e.is_a?(Sketchup::Group)) && e.get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
                type = e.get_attribute('MSPhysics Joint', 'Type')
                name = e.get_attribute('MSPhysics Joint', 'Name').to_s
                name = e.get_attribute('MSPhysics Joint', 'ID').to_s if name.empty?
                fname = type + '-' + name
                if @body_internal_joints[fname]
                  if identical_joints[fname]
                    identical_joints[fname] += 1
                  else
                    identical_joints[fname] = 1
                  end
                  fname2 = fname + ' (' + identical_joints[fname].to_s + ')'
                  @body_internal_joints[fname2] = e
                else
                  @body_internal_joints[fname] = e
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
              @body_internal_joints.keys.sort.each { |fname|
                if count == 3
                  cmd << "</tr><tr>"
                  count = 0
                end
                cmd << "<td><label class=\\\"joint-label\\\" id=\\\"Internal::#{fname}\\\">#{fname}</label></td>"
                count += 1
              }
              cmd << "</tr>"
              cmd << "\");"
            end
            # Display connected joints
            cmd << "$('#body-connected_joints_table').empty();"
            identical_joints.clear
            connected_joints = MSPhysics::JointConnectionTool.get_connected_joints(@selected_body, false)[0]
            connected_joints.each { |joint, jparent, jtra|
              type = joint.get_attribute('MSPhysics Joint', 'Type')
              name = joint.get_attribute('MSPhysics Joint', 'Name').to_s
              name = joint.get_attribute('MSPhysics Joint', 'ID').to_s if name.empty?
              fname = type + '-' + name
              if @body_connected_joints[fname]
                if identical_joints[fname]
                  identical_joints[fname] += 1
                else
                  identical_joints[fname] = 1
                end
                fname2 = fname + ' (' + identical_joints[fname].to_s + ')'
                @body_connected_joints[fname2] = joint
              else
                @body_connected_joints[fname] = joint
              end
            }
            if @body_connected_joints.empty?
              cmd << "$('#body-connected_joints_div').css('display', 'none');"
            else
              cmd << "$('#body-connected_joints_div').css('display', 'block');"
              cmd << "$('#body-connected_joints_table').append(\""
              count = 0
              cmd << "<tr>"
              @body_connected_joints.keys.sort.each { |fname|
                if count == 3
                  cmd << "</tr><tr>"
                  count = 0
                end
                cmd << "<td><label class=\\\"joint-label\\\" id=\\\"Connected::#{fname}\\\">#{fname}</label></td>"
                count += 1
              }
              cmd << "</tr>"
              cmd << "\");"
            end
            cmd << "assign_joint_click_event();"
            identical_joints.clear
          else
            # Display tabs
            cmd << "$('#tab3-none').css('display', 'block');"
            cmd << "$('#tab3-content').css('display', 'none');"
            cmd << "$('#tab2-none').css('display', 'none');"
            cmd << "$('#tab2-content1').css('display', 'none');"
            cmd << "$('#tab2-content2').css('display', 'block');"
            # Display Ignore state
            state = @selected_body.get_attribute('MSPhysics Body', 'Ignore', nil) ? true : false
            cmd << "$('#internal-body-ignore').prop('checked', #{state});"
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
        @dialog.execute_script(cmd)
      end

      # @api private
      # Update UI joint tab.
      # @return [void]
      def update_joint_state
        return unless is_visible?
        model = Sketchup.active_model
        sel = model.selection.to_a
        joints = []
        sel.each { |ent|
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
          cmd << "$('#joint_constraint-#{attr == 0 ? 'standard' : (attr == 1 ? 'flexible' : 'robust')}').prop('checked', true);"
          attr = @selected_joint.get_attribute(jdict, 'Name').to_s
          attr = @selected_joint.get_attribute(jdict, 'ID') if attr.empty?
          cmd << "$('#joint-name').val(#{attr.inspect});"
          attr = @selected_joint.get_attribute(jdict, 'Stiffness', MSPhysics::Joint::DEFAULT_STIFFNESS)
          cmd << "$('#joint-stiffness').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
          attr = @selected_joint.get_attribute(jdict, 'Bodies Collidable', MSPhysics::Joint::DEFAULT_BODIES_COLLIDABLE)
          cmd << "$('#joint-bodies_collidable').prop('checked', #{attr ? true : false});"
          attr = @selected_joint.get_attribute(jdict, 'Breaking Force', MSPhysics::Joint::DEFAULT_BREAKING_FORCE)
          cmd << "$('#joint-breaking_force').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"

          joint_type = @selected_joint.get_attribute(jdict, 'Type')
          case joint_type
          when 'Fixed'
            cmd << "$('#tab4-fixed').css('display', 'block');"
          when 'Hinge'
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
            attr = @selected_joint.get_attribute(jdict, 'Accel', fix_numeric_value(MSPhysics::Servo::DEFAULT_ACCEL))
            cmd << "$('#servo-accel').val('#{ format_value(attr.to_f, @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Damp', fix_numeric_value(MSPhysics::Servo::DEFAULT_DAMP))
            cmd << "$('#servo-damp').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Reduction Ratio', fix_numeric_value(MSPhysics::Servo::DEFAULT_REDUCTION_RATIO))
            cmd << "$('#servo-reduction_ratio').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Enable Sp Mode', MSPhysics::Servo::DEFAULT_SP_MODE_ENABLED)
            cmd << "$('#servo-enable_sp_mode').prop('checked', #{attr ? true : false});"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Servo::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#servo-controller').val(#{attr.inspect});"
          when 'Slider'
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
            attr = @selected_joint.get_attribute(jdict, 'Linear Rate', fix_numeric_value(MSPhysics::Piston::DEFAULT_LINEAR_RATE * ipos_ratio))
            cmd << "$('#piston-linear_rate').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Strength', fix_numeric_value(MSPhysics::Piston::DEFAULT_STRENGTH))
            cmd << "$('#piston-strength').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, nil), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Reduction Ratio', fix_numeric_value(MSPhysics::Piston::DEFAULT_REDUCTION_RATIO))
            cmd << "$('#piston-reduction_ratio').val('#{ format_value(AMS.clamp(attr.to_f, 0.0, 1.0), @precision) }');"
            attr = @selected_joint.get_attribute(jdict, 'Controller', MSPhysics::Piston::DEFAULT_CONTROLLER.to_s)
            cmd << "$('#piston-controller').val(#{attr.inspect});"
          when 'Spring'
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
            cmd << "$('#tab4-universal').css('display', 'block');"
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
          end
          cmd << "activate_tab(4);" unless @selected_body
        else
          @selected_joint = nil
        end
        @dialog.execute_script(cmd)
      end

      # @api private
      # Update UI sound tab.
      # @return [void]
      def update_sound_state
        return unless is_visible?
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
        @dialog.execute_script(cmd)
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
        joint_type = @selected_joint.get_attribute(jdict, 'Type')
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
              when 'Accel'; MSPhysics::Servo::DEFAULT_ACCEL
              when 'Damp'; MSPhysics::Servo::DEFAULT_DAMP
              when 'Reduction Ratio'; MSPhysics::Servo::DEFAULT_REDUCTION_RATIO
              when 'Enable Sp Mode'; MSPhysics::Servo::DEFAULT_SP_MODE_ENABLED
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
              when 'Linear Rate'; MSPhysics::Piston::DEFAULT_LINEAR_RATE * ipos_ratio
              when 'Strength'; MSPhysics::Piston::DEFAULT_STRENGTH
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
        v = sprintf("%.#{precision}f", value.to_f)
        (v.to_f == value.to_f ? '' : '~ ') + v
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
      # @return [Boolean] true if state changed.
      def show(state)
        return false if (state ? true : false) == is_visible?
        if state
          dbs = MSPhysics::DEFAULT_BODY_SETTINGS
          @material = MSPhysics::Material.new('Temp', dbs[:density], dbs[:static_friction], dbs[:dynamic_friction], dbs[:elasticity], dbs[:softness])
          @dialog = ::UI::WebDialog.new(@title, false, @title, 520, 520, 800, 600, false)
          # Callbacks
          @dialog.add_action_callback('init'){ |dlg, params|
            update_state
            next if @init_called
            @init_called = true
            Sketchup.active_model.selection.add_observer(self)
            Sketchup.add_observer(self)
          }
          @dialog.add_action_callback('editor_changed'){ |dlg, params|
            next unless @selected_body
            code = dlg.get_element_value('temp_script_area')
            @selected_body.set_attribute('MSPhysics Script', 'Value', code)
          }
          @dialog.add_action_callback('cursor_changed'){ |dlg, params|
            next unless @selected_body
            @selected_body.set_attribute('MSPhysics Script', 'Cursor', params)
          }
          @dialog.add_action_callback('open_link'){ |dlg, params|
            ::UI.openURL(params)
          }
          @dialog.add_action_callback('open_ruby_core'){ |dlg, params|
            ::UI.openURL("http://ruby-doc.org/core-#{RUBY_VERSION}/")
          }
          @dialog.add_action_callback('check_input_changed'){ |dlg, params|
            settings = MSPhysics::Settings
            id, value = data = eval(params)
            dict, attr = id.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              settings = MSPhysics::Settings
              case attr
              when 'continuous_collision'
                settings.set_continuous_collision_state(value)
              when 'collision_wireframe'
                settings.show_collision_wireframe(value)
              when 'axes'
                settings.show_axes(value)
              when 'aabb'
                settings.show_aabb(value)
              when 'contact_points'
                settings.show_contact_points(value)
              when 'contact_forces'
                settings.show_contact_forces(value)
              when 'bodies'
                settings.show_bodies(value)
              end
            when 'body', 'internal_body'
              if @selected_body
                @selected_body.set_attribute('MSPhysics Body', option, value)
              end
            when 'editor'
              if attr == 'print_margin'
                @editor_print_margin = value
              elsif attr == 'read_only'
                @editor_read_only = value
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint
                @selected_joint.set_attribute('MSPhysics Joint', option, value)
              end
            when 'joint_constraint'
              if value
                ctype = (attr == 'standard' ? 0 : (attr == 'flexible' ? 1 : 2))
                @selected_joint.set_attribute('MSPhysics Joint', 'Constraint Type', ctype)
              end
            end
          }
          @dialog.add_action_callback('button_clicked'){ |dlg, params|
            case params
            when 'sound-add'
              path = ::UI.openpanel('Add Sound File', @default_sound_path)
              if path
                @default_sound_path = path
                begin
                  add_sound(path)
                  update_sound_state
                rescue TypeError => e
                  ::UI.messagebox(e.message)
                end
              end
            when 'sound-remove'
              if @selected_sound
                remove_sound(@selected_sound)
                @selected_sound = nil
                update_sound_state
              end
            when 'sound-remove_all'
              if get_num_sounds < 2 || ::UI.messagebox('Are you sure you want to remove all sounds?', MB_YESNO) == IDYES
                remove_all_sounds
                @selected_sound = nil
                update_sound_state
              end
            when 'sound-play'
              if @selected_sound
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
              if MSPhysics::Music.is_paused?
                MSPhysics::Music.resume
              else
                MSPhysics::Music.pause
              end
            when 'sound-stop'
              MSPhysics::Music.stop
            when 'motor-generate_slider'
              if @selected_joint
                jdict = 'MSPhysics Joint'
                name = @selected_joint.get_attribute(jdict, 'Name').to_s
                name = 'Motor-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                controller = "slider(#{name.inspect}, 0.0, -1.0, 1.0)"
                @selected_joint.set_attribute(jdict, 'Controller', controller)
                dlg.execute_script("$('#motor-controller').val(#{controller.inspect});")
              end
            when 'servo-generate_slider'
              if @selected_joint
                jdict = 'MSPhysics Joint'
                attr = @selected_joint.get_attribute(jdict, 'Angle Units', MSPhysics::DEFAULT_ANGLE_UNITS)
                ang_ratio = attr == 'deg' ? 1.degrees : 1
                iang_ratio = 1.0 / ang_ratio
                name = @selected_joint.get_attribute(jdict, 'Name').to_s
                name = 'Servo-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                min = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Servo::DEFAULT_MIN * iang_ratio)).to_f
                max = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Servo::DEFAULT_MAX * iang_ratio)).to_f
                sp_mode = @selected_joint.get_attribute(jdict, 'Enable Sp Mode', MSPhysics::Servo::DEFAULT_SP_MODE_ENABLED)
                if sp_mode
                  controller = "slider(#{name.inspect}, 0.5, 0.0, 1.0)"
                else
                  controller = "slider(#{name.inspect}, 0.0, #{min}, #{max})"
                end
                @selected_joint.set_attribute(jdict, 'Controller', controller)
                dlg.execute_script("$('#servo-controller').val(#{controller.inspect});")
              end
            when 'piston-generate_slider'
              if @selected_joint
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
                name = @selected_joint.get_attribute(jdict, 'Name').to_s
                name = 'Piston-' + @selected_joint.get_attribute(jdict, 'ID').to_s if name.empty?
                min = @selected_joint.get_attribute(jdict, 'Min', fix_numeric_value(MSPhysics::Piston::DEFAULT_MIN * ipos_ratio)).to_f
                max = @selected_joint.get_attribute(jdict, 'Max', fix_numeric_value(MSPhysics::Piston::DEFAULT_MAX * ipos_ratio)).to_f
                controller = "slider(#{name.inspect}, 0.0, #{min}, #{max})"
                @selected_joint.set_attribute(jdict, 'Controller', controller)
                dlg.execute_script("$('#piston-controller').val(#{controller.inspect});")
              end
            end
          }
          @dialog.add_action_callback('numeric_input_changed'){ |dlg, params|
            id, value = eval(params)
            dict, attr = id.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              case attr
              when 'gravity'
                value = MSPhysics::Settings.set_gravity(value)
              when 'material_thickness'
                value = AMS.clamp(value, 0.0, 1.0)
                MSPhysics::Settings.set_material_thickness(value/32.0)
              when 'world_scale'
                value = MSPhysics::Settings.set_world_scale(value)
              end
            when 'body'
              if @selected_body
                method = "set_#{attr}"
                if @material.respond_to?(method)
                  value = @material.method(method).call(value)
                  if value != @activated_value
                    dlg.execute_script("$('#body-material').val('Custom'); $('#body-material').trigger('chosen:updated');")
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
              if @selected_joint
                if %w(stiffness reduction_ratio).include?(attr)
                  value = AMS.clamp(value, 0.0, 1.0)
                elsif %w(accel damp breaking_force linear_rate strength friction linear_friction angular_friction).include?(attr)
                  value = AMS.clamp(value, 0.0, nil)
                end
                @selected_joint.set_attribute('MSPhysics Joint', option, value.to_i.is_a?(Bignum) ? value.to_s : value)
              end
            end
            dlg.execute_script("$('##{id}').val('#{ format_value(value, @precision) }')")
            @activated_value = nil
          }
          @dialog.add_action_callback('fixnum_input_changed'){ |dlg, params|
            id, value = eval(params)
            dict, attr = id.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              if attr == 'update_rate'
                value = MSPhysics::Settings.set_update_rate(value)
              end
            when 'body'
              if @selected_body
                if attr == 'emitter_rate'
                  value = AMS.clamp(value.round, 1, nil)
                elsif attr == 'emitter_lifetime'
                  value = AMS.clamp(value.round, 0, nil)
                end
                @selected_body.set_attribute('MSPhysics Body', option, value.to_i.is_a?(Bignum) ? value.to_s : value)
              end
            when 'joint', *MSPhysics::JOINT_NAMES
            end
            dlg.execute_script("$('##{id}').val('#{ value.to_i }')")
          }
          @dialog.add_action_callback('numeric_input_focused'){ |dlg, params|
            dict, attr = params.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            value = case dict
            when 'simulation'
              case attr
              when 'gravity'
                MSPhysics::Settings.get_gravity
              when 'material_thickness'
                MSPhysics::Settings.get_material_thickness * 32.0
              when 'world_scale'
                MSPhysics::Settings.get_world_scale
              end
            when 'body'
              if @selected_body
                default = MSPhysics::DEFAULT_BODY_SETTINGS
                @selected_body.get_attribute('MSPhysics Body', option, default[attr.to_sym])
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint
                begin
                  #default = eval("MSPhysics::#{dict.split('_').map { |w| w.capitalize }.join}::DEFAULT_#{attr.upcase}")
                  default = get_joint_default_value(@selected_joint, option)
                rescue Exception => e
                  default = 0
                end
                @selected_joint.get_attribute('MSPhysics Joint', option, default)
              end
            end
            if value.is_a?(Numeric)
              dlg.execute_script("$('##{params}').val(#{value});")
              @activated_value = value
            end
          }
          @dialog.add_action_callback('controller_input_changed'){ |dlg, id|
            dict, attr = id.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
            when 'body'
              if @selected_body
                code = dlg.get_element_value(id)
                @selected_body.set_attribute('MSPhysics Body', option, code)
              end
            when 'joint', *MSPhysics::JOINT_NAMES
              if @selected_joint
                code = dlg.get_element_value(id)
                @selected_joint.set_attribute('MSPhysics Joint', option, code)
              end
            end
          }
          @dialog.add_action_callback('text_input_changed'){ |dlg, id|
            dict, attr = id.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'joint'
              if @selected_joint
                code = dlg.get_element_value(id)
                @selected_joint.set_attribute('MSPhysics Joint', option, code)
              end
            end
          }
          @dialog.add_action_callback('select_input_changed'){ |dlg, params|
            id, value = eval(params)
            dict, attr = id.split('-', 2)
            words = attr.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            case dict
            when 'simulation'
              method = "set_#{attr}"
              if MSPhysics::Settings.respond_to?(method)
                value = 1.0/value.to_f if attr == 'update_timestep'
                MSPhysics::Settings.method(method).call(value.to_f)
              end
            when 'body'
              if @selected_body
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
                    material = MSPhysics::Materials.get_by_name(value)
                    if material
                      @selected_body.set_attribute('MSPhysics Body', 'Density', material.get_density)
                      @selected_body.set_attribute('MSPhysics Body', 'Static Friction', material.get_static_friction)
                      @selected_body.set_attribute('MSPhysics Body', 'Dynamic Friction', material.get_dynamic_friction)
                      @selected_body.set_attribute('MSPhysics Body', 'Enable Friction', true)
                      @selected_body.set_attribute('MSPhysics Body', 'Elasticity', material.get_elasticity)
                      @selected_body.set_attribute('MSPhysics Body', 'Softness', material.get_softness)
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
              @selected_joint.set_attribute('MSPhysics Joint', option, value)
              update_joint_state
            end
          }
          @dialog.add_action_callback('tab_changed'){ |dlg, params|
            num = params.to_i
            if num != 3 && @active_tab == 3 && @selected_body && @selected_body.parent.is_a?(Sketchup::Model) && AMS::Window.is_restored?(@handle)
              @editor_size = AMS::Window.get_size(@handle)
            end
            @last_active_body_tab = num if num == 2 || num == 3
            @active_tab = num
          }
          @dialog.add_action_callback('editor_size_changed'){ |dlg, params|
            @editor_size = AMS::Window.get_size(@handle)
          }
          @dialog.add_action_callback('update_size'){ |dlg, params|
            update_dialog_style
            if @active_tab == 3 && @selected_body && @selected_body.parent.is_a?(Sketchup::Model)
              AMS::Window.set_size(@handle, @editor_size.x, @editor_size.y, false)
            else
              tw,th = eval(params)
              w,h = AMS::Window.get_size(@handle)
              crect = AMS::Window.get_client_rect(@handle)
              bx = w - crect[2] + crect[0]
              by = h - crect[3] + crect[1]
              AMS::Window.set_size(@handle, tw+bx, th+by, false)
            end
          }
          @dialog.add_action_callback('update_simulation_state'){ |dlg, params|
            update_simulation_state
          }
          @dialog.add_action_callback('sound_select_changed'){ |dlg, params|
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
            dlg.execute_script(cmd)
          }
          @dialog.add_action_callback('joint_label_selected'){ |dlg, params|
            if (params =~ /Internal::/) == 0
              fname = params.split("Internal::", 2)[1]
              joint = @body_internal_joints[fname]
              if joint && joint.valid?
                @observer_temp_disabled = true
                @selected_joint = joint
                Sketchup.active_model.selection.clear
                Sketchup.active_model.selection.add(@selected_body)
                Sketchup.active_model.selection.add(@selected_joint)
                @observer_temp_disabled = false
                update_joint_state
                dlg.execute_script("activate_tab(4); update_size();")
              end
            elsif (params =~ /Connected::/) == 0
              fname = params.split("Connected::", 2)[1]
              joint = @body_connected_joints[fname]
              if joint && joint.valid?
                @observer_temp_disabled = true
                @selected_joint = joint
                Sketchup.active_model.selection.clear
                Sketchup.active_model.selection.add(@selected_body)
                Sketchup.active_model.selection.add(@selected_joint)
                @observer_temp_disabled = false
                update_joint_state
                dlg.execute_script("activate_tab(4); update_size();")
              end
            end
          }
          @dialog.add_action_callback('debug'){ |dlg, params|
            puts params
          }
          @dialog.set_on_close(){
            if @active_tab == 3 && @selected_body && @selected_body.parent.is_a?(Sketchup::Model) && AMS::Window.is_restored?(@handle)
              @editor_size = AMS::Window.get_size(@handle)
            end
            @dialog = nil
            @handle = nil
            @init_called = false
            @selected_body = nil
            @selected_joint = nil
            @active_tab = 1
            @cleared = false
            @activated_value = nil
            @selected_sound = nil
            Sketchup.active_model.selection.remove_observer(self)
            Sketchup.remove_observer(self)
          }
          # Set content
          dir = File.dirname(__FILE__)
          url = File.join(dir, 'html/dialog.html')
          @dialog.set_file(url)
          # Show dialog
          @dialog.show
          # Find dialog window handle
          @handle = AMS::Sketchup.find_window_by_caption(@title)
          # Set dialog style
          update_dialog_style
        else
          @dialog.close
        end
        true
      end

      # Determine if MSPhysics UI is open.
      # @return [Boolean]
      def is_visible?
        @dialog ? true : false
      end

      # Open MSPhysics UI and set pointer to the location of an error.
      # @param [MSPhysics::ScriptException] error
      # @return [Boolean] success
      def locate_error(error)
        AMS.validate_type(error, MSPhysics::ScriptException)
        return false if error.entity.deleted?
        model = Sketchup.active_model
        model.selection.clear
        model.selection.add(error.entity)
        # Display_mesage
        msg = "activate_tab(3); update_size();"
        msg << "editor_set_cursor(#{error.line}, 0); editor_select_current_line();" if error.line
        if is_visible?
          @dialog.execute_script(msg)
        else
          show(true)
          ::UI.start_timer(0.25, false) {
            @dialog.execute_script(msg) if @dialog
          }
        end
        true
      end

      # Add sound to UI.
      # @param [String] path
      # @return [String] Name of added file.
      # @raise [TypeError] if file path is invalid.
      # @raise [TypeError] if file is over 100 megabytes.
      def add_sound(path)
        unless File.exist?(path)
          raise(TypeError, "Invalid path!", caller)
        end
        name = File.basename(path, File.extname(path)).gsub("'", " ")
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
        music = MSPhysics::Music.get_by_name(name)
        MSPhysics::Music.destroy(music) if music
        dict1 = Sketchup.active_model.attribute_dictionary('MSPhysics Sounds', false)
        dict1.delete_key(name.to_s) if dict1
        dict2 = Sketchup.active_model.attribute_dictionary('MSPhysics Sound Types', false)
        dict2.delete_key(name.to_s) if dict2
      end

      # Remove all sounds from UI.
      def remove_all_sounds
        dict = Sketchup.active_model.attribute_dictionary('MSPhysics Sounds', false)
        if dict
          dict.each_key { |name|
            music = MSPhysics::Music.get_by_name(name)
            MSPhysics::Music.destroy(music) if music
          }
        end
        Sketchup.active_model.attribute_dictionaries.delete('MSPhysics Sounds')
        Sketchup.active_model.attribute_dictionaries.delete('MSPhysics Sound Types')
      end

      # Get number of sounds in UI.
      # @return [Fixnum]
      def get_num_sounds
        dict = Sketchup.active_model.attribute_dictionary('MSPhysics Sounds', false)
        dict ? dict.length : 0
      end

      # Load editor settings from registry.
      def load_editor_settings
        theme = Sketchup.read_default('MSPhysics', 'Editor Theme', DEFAULT_THEME).to_s
        font = Sketchup.read_default('MSPhysics', 'Editor Font', DEFAULT_FONT).to_i
        wrap = Sketchup.read_default('MSPhysics', 'Editor Wrap', DEFAULT_WRAP).to_s
        print_margin = Sketchup.read_default('MSPhysics', 'Editor Wrap', DEFAULT_PRINT_MARGIN) ? true : false
        @editor_theme = theme
        @editor_font = font
        @editor_wrap = wrap
        @editor_print_margin = print_margin
      end

      # Save editor settings into registry.
      def save_editor_settings
        Sketchup.write_default('MSPhysics', 'Editor Theme', @editor_theme.to_s)
        Sketchup.write_default('MSPhysics', 'Editor Font', @editor_font.to_i)
        Sketchup.write_default('MSPhysics', 'Editor Wrap', @editor_wrap.to_s)
        Sketchup.write_default('MSPhysics', 'Editor Print Margin', @editor_print_margin)
      end

      # SU Selection Observers
      # @!visibility private


      def onSelectionBulkChange(sel)
        return if @observer_temp_disabled
        @cleared = false
        update_state
      end

      def onSelectionCleared(sel)
        return if @observer_temp_disabled
        @cleared = true
        ::UI.start_timer(0.1, false){
          update_state if @cleared
        }
      end

      def onSelectionAdded(sel, element)
        return if @observer_temp_disabled
        update_state
      end

      def onSelectionRemoved(sel, element)
        return if @observer_temp_disabled
        update_state
      end

      def onNewModel(model)
        return if @observer_temp_disabled
        model.selection.add_observer(self) if is_visible?
        update_state
      end

      def onOpenModel(model)
        return if @observer_temp_disabled
        model.selection.add_observer(self) if is_visible?
        update_state
      end

    end # class << self

    # @!visibility private
    class AppObserver < ::Sketchup::AppObserver

      def onQuit
        MSPhysics::Dialog.save_editor_settings
      end

    end # class AppObserver
  end # module Dialog
end # module MSPhysics
