module MSPhysics
  module Dialog

    module_function

    # @!visibility private
    @dlg = nil
    # @!visibility private
    @hwnd = nil
    # @!visibility private
    @editor_size = [520,520]
    # @!visibility private
    @init_called = false
    # @!visibility private
    @first_time = true
    # @!visibility private
    @selected_body = nil
    # @!visibility private
    @selected_joint = nil
    # @!visibility private
    @active_tab = 1
    # @!visibility private
    @last_active_tab = 1
    # @!visibility private
    @last_active_body_tab = 2
    # @!visibility private
    @cleared = false
    # @!visibility private
    @material = Material.new('Temp', 700, 0.50, 0.25, 0.40, 0.01)


    # @!visibility private
    def update_state
      return unless visible?
      model = Sketchup.active_model
      sel = model.selection.to_a
      bodies = []
      joints = []
      sel.each { |ent|
        next unless [Sketchup::Group, Sketchup::ComponentInstance].include?(ent.class)
        type = MSPhysics.get_entity_type(ent)
        if type == 'Body'
          bodies << ent
        elsif type == 'Joint'
          joints << ent
        end
      }
      cmd = ''
      # Simulation dialog.
      update_simulation_state
      # Body dialog.
      if bodies.size == 1
        @selected_body = bodies[0]
        active = Sketchup.version.to_i > 6 ? model.active_path : nil
        # Top level entities have access to the full body properties.
        # Only top level entities may contain scripts.
        # Child entities have access to the shape property only, and only if
        # its parent body is a compound.
        default = MSPhysics::DEFAULT_BODY_SETTINGS
        if active.nil?
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
          # Display shape
          shape = @selected_body.get_attribute('MSPhysics Body', 'Shape', default[:shape])
          if shape
            choice = shape.downcase.gsub(' ', '_')
            cmd << "$('#body-shape-#{choice}').prop('checked', true);"
          end
          # Display state and other check-box properties.
          ['Ignore', 'Not Collidable', 'Static', 'Frozen', 'Enable Friction', 'Magnetic', 'Enable Script'].each { |option|
            property = option.downcase.gsub(' ', '_')
            default_state = default[property.to_sym]
            state = @selected_body.get_attribute('MSPhysics Body', option, default_state) ? true : false
            cmd << "$('#body-#{ property }').prop('checked', #{ state });"
          }
          # Display numeric properties.
          ['Density', 'Static Friction', 'Kinetic Friction', 'Dynamic Friction', 'Elasticity', 'Softness', 'Magnet Force', 'Magnet Range'].each { |option|
            property = option.downcase.gsub(' ', '_')
            attr = @selected_body.get_attribute('MSPhysics Body', option, default[property.to_sym])
            value = attr.to_f rescue default[property.to_sym].to_f
            cmd << "$('#body-#{ property }').val('#{ sprintf('%.2f', value) }');"
          }
        else
          # Display tabs
          cmd << "$('#tab3-none').css('display', 'block');"
          cmd << "$('#tab3-content').css('display', 'none');"
          shape = active.last.get_attribute('MSPhysics Body', 'Shape')
          if shape == 'Compound'
            cmd << "$('#tab2-none').css('display', 'none');"
            cmd << "$('#tab2-content1').css('display', 'none');"
            cmd << "$('#tab2-content2').css('display', 'block');"
          end
          # Display shape
          shape = @selected_body.get_attribute('MSPhysics Body', 'Shape', default[:shape])
          if shape
            choice = shape.downcase.gsub(' ', '_')
            cmd << "$('#internal-body-shape-#{choice}').prop('checked', true);"
          end
          # Display state
          ['Ignore', 'Not Collidable'].each { |option|
            id = "#internal-body-#{option.downcase.gsub(' ', '_')}"
            state = @selected_body.get_attribute('MSPhysics Body', option, nil) ? true : false
            cmd << "$('#{id}').prop('checked', #{state});"
          }
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
      # Joint dialog.
      if joints.size == 1
        @selected_joint = joints[0]
        stype = @selected_joint.get_attribute('MSPhysics Joint', 'Type')
        found = false
        MSPhysics::Joint::TYPES.each { |type|
          if type.to_s == stype
            display = 'block'
            found = true
          else
            display = 'none'
          end
          cmd << "$('#tab4-#{type}').css('display', '#{display}');"
        }
        cmd << "$('#tab4-none').css('display', '#{found ? 'none' : 'block'}');"
        cmd << "activate_tab(4);"
      else
        @selected_joint = nil
        cmd << "$('#tab4-none').css('display', 'block');"
        MSPhysics::Joint::TYPES.each { |type|
          cmd << "$('#tab4-#{type}').css('display', 'none');"
        }
      end
      cmd << "update_size();"
      @dlg.execute_script(cmd)
    end

    def update_simulation_state
      settings = MSPhysics::Settings
      cmd = ''
      cmd << "$('#simulation-continuous_collision').prop('checked', #{settings.continuous_collision_mode_enabled?});"
      cmd << "$('#simulation-record').prop('checked', #{settings.record_animation_enabled?});"
      cmd << "$('#simulation-solver_model-#{settings.solver_model}').prop('checked', true);"
      cmd << "$('#simulation-speed-#{(1/settings.update_timestep).round}').prop('checked', true);"
      cmd << "$('#simulation-collision').prop('checked', #{settings.collision_visible?});"
      cmd << "$('#simulation-axis').prop('checked', #{settings.axis_visible?});"
      cmd << "$('#simulation-bounding_box').prop('checked', #{settings.bounding_box_visible?});"
      cmd << "$('#simulation-contact_points').prop('checked', #{settings.contact_points_visible?});"
      cmd << "$('#simulation-contact_forces').prop('checked', #{settings.contact_forces_visible?});"
      cmd << "$('#simulation-bodies').prop('checked', #{settings.bodies_visible?});"
      cmd << "$('#simulation-gravity').val('#{ sprintf('%.2f', settings.gravity) }');"
      cmd << "$('#simulation-material_thickness').val('#{ sprintf('%.2f', settings.material_thickness) }');"
      @dlg.execute_script(cmd)
    end

    # Open/Close MSPhysics UI.
    # @param [Boolean] state
    # @return [Boolean] true if state changed.
    def visible=(state)
      state = state == true
      return false if state == visible?
      if state
        title = 'MSPhysics UI'
        @dlg = UI::WebDialog.new(title, false, 'MSPhysics UI', 520, 520, 800, 600, false)
        # Callbacks
        @dlg.add_action_callback('init'){ |dlg, params|
          update_state
          next if @init_called
          @init_called = true
          if @first_time
            Sketchup.active_model.selection.add_observer(self)
            Sketchup.add_observer(self)
            @first_time = false
          end
        }
        @dlg.add_action_callback('editor_changed'){ |dlg, params|
          next unless @selected_body
          code = dlg.get_element_value('temp-area')
          @selected_body.set_attribute('MSPhysics Script', 'Value', code)
        }
        @dlg.add_action_callback('cursor_changed'){ |dlg, params|
          next unless @selected_body
          @selected_body.set_attribute('MSPhysics Script', 'Cursor', params)
        }
        @dlg.add_action_callback('open_link'){ |dlg, params|
          UI.openURL(params)
        }
        @dlg.add_action_callback('open_ruby_core'){ |dlg, params|
          UI.openURL("http://ruby-doc.org/core-#{RUBY_VERSION}/")
        }
        @dlg.add_action_callback('check_input_changed'){ |dlg, params|
          settings = MSPhysics::Settings
          data = eval(params)
          if data[0] =~ /simulation-solver_model-/
            value = data[0].split('simulation-solver_model-')[1].to_i
            settings.solver_model = value
          elsif data[0] =~ /simulation-speed-/
            value = data[0].split('simulation-speed-')[1].to_i
            settings.update_timestep = 1.0/value
          elsif data[0] =~ /simulation-continuous_collision/
            settings.continuous_collision_mode_enabled = data[1]
          elsif data[0] =~ /simulation-record/
            settings.record_animation_enabled = data[1]
          elsif data[0] =~ /simulation-/
            choice = data[0].split('simulation-')[1]+'_visible='
            if settings.respond_to?(choice)
              settings.method(choice).call(data[1])
            end
          elsif data[0] =~ /body-shape-/
            choice = data[0].split('body-shape-')[1]
            words = choice.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            @selected_body.set_attribute('MSPhysics Body', 'Shape', option)
          elsif data[0] =~ /body-/
            choice = data[0].split('body-')[1]
            words = choice.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            checked = data[1]
            @selected_body.set_attribute('MSPhysics Body', option, checked)
          end
        }
        @dlg.add_action_callback('numeric_input_changed'){ |dlg, params|
          data = eval(params)
          if data[0] =~ /simulation-/
            settings = MSPhysics::Settings
            choice = data[0].split('simulation-')[1]
            if settings.respond_to?(choice+'=')
              settings.method(choice+'=').call(data[1])
              value = settings.method(choice).call
              if value != data[1]
                dlg.execute_script("$('##{data[0]}').val('#{ sprintf('%.2f', value) }')")
              end
            end
          elsif data[0] =~ /body-/
            choice = data[0].split('body-')[1]
            words = choice.split('_')
            for i in 0...words.size
              words[i].capitalize!
            end
            option = words.join(' ')
            if @material.respond_to?(choice+'=')
              @material.method(choice+'=').call(data[1])
              value = @material.method(choice).call
              if value != data[1]
                dlg.execute_script("$('##{data[0]}').val('#{ sprintf('%.2f', value) }')")
              end
              @selected_body.set_attribute('MSPhysics Body', option, value)
              @selected_body.set_attribute('MSPhysics Body', 'Material', 'Custom')
            elsif choice =~ /magnet/
              value = data[1]
              value = MSPhysics.clamp(value, 0, nil) if choice =~ /range/
              if value != data[1]
                dlg.execute_script("$('##{data[0]}').val('#{ sprintf('%.2f', value) }')")
              end
              @selected_body.set_attribute('MSPhysics Body', option, value)
            end
          end
        }
        @dlg.add_action_callback('tab_changed'){ |dlg, params|
          num = params.to_i
          @last_active_body_tab = num if num.between?(2,3)
          @last_active_tab = @active_tab
          @active_tab = num
        }
        @dlg.add_action_callback('size_changed'){ |dlg, params|
          @editor_size = AMS::Window.size(@hwnd) if AMS::Window.resizeable?(@hwnd)
          tw,th = eval(params)
          if @active_tab == 3 and @selected_body
            AMS::Window.set_resizeable(@hwnd, true, false)
            w,h = @editor_size
            AMS::Window.set_size(@hwnd, w, h, false)
            next
          end
          changed = AMS::Window.set_resizeable(@hwnd, false, false)
          @editor_size = AMS::Window.size(@hwnd) if changed
          w,h = AMS::Window.size(@hwnd)
          cw,ch = AMS::Window.client_size(@hwnd)
          bx = w-cw
          by = h-ch
          AMS::Window.set_size(@hwnd, tw+bx, th+by, false)
        }
        @dlg.add_action_callback('update_simulation_state'){ |dlg, params|
          update_simulation_state
        }
        @dlg.set_on_close(){
          @dlg = nil
          @hwnd = nil
          @init_called = false
        }
        # Set content
        dir = File.dirname(__FILE__)
        url = File.join(dir, 'index.html')
        @dlg.set_file(url)
        # Limit size
        if Sketchup.version.to_i > 6
          @dlg.min_width = 500
          @dlg.min_height = 200
        end
        # Show dialog
        @dlg.show
        # Find dialog window handle
        @hwnd = AMS::Sketchup.window_by_caption(title)
      else
        @dlg.close
      end
      true
    end

    # Determine whether MSPhysics UI dialog is open.
    # @return [Boolean]
    def visible?
      @dlg ? true : false
    end

    # @!visibility private
    def lead_to_error(data)
      return unless data.is_a?(Array)
      ent = MSPhysics.get_entity_by_id(data[0])
      line = data[1]
      return unless ent
      sel = Sketchup.active_model.selection
      sel.clear
      sel.add(ent)
      if self.visible?
        @dlg.execute_script('activate_tab(3);')
        @last_active_body_tab = 3
        return unless line
        @dlg.execute_script("editor_set_cursor(#{line}, 0); editor_select_current_line();")
      else
        self.visible = true
        UI.start_timer(0.5, false){
          @dlg.execute_script('activate_tab(3);')
          next unless line
          @dlg.execute_script("editor_set_cursor(#{line}, 0); editor_select_current_line();")
        }
      end
    end

    # @!visibility private
    def onSelectionBulkChange(sel)
      @cleared = false
      update_state
    end

    # @!visibility private
    def onSelectionCleared(sel)
      @cleared = true
      UI.start_timer(0.1, false){
        update_state if @cleared
      }
    end

    # @!visibility private
    def onSelectionAdded(sel, element)
      update_state
    end

    # @!visibility private
    def onSelectionRemoved(sel, element)
      update_state
    end

    # @!visibility private
    def onNewModel(model)
      model.selection.add_observer(self) if visible?
    end

    # @!visibility private
    def onOpenModel(model)
      model.selection.add_observer(self) if visible?
    end

  end # module Dialog
end # module MSPhysics
