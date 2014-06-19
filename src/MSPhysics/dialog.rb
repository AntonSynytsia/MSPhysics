module MSPhysics
  module Dialog

    module_function

    # @!visibility private
    @dlg = nil
    # @!visibility private
    @init_called = false
    # @!visibility private
    @first_time = true
    # @!visibility private
    @selected_body = nil
    # @!visibility private
    @selected_joint = nil
    # @!visibility private
    @last_active_body_tab = 2
    # @!visibility private
    @min_size = nil

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
      if bodies.size == 1
        @selected_body = bodies[0]
        active = Sketchup.version.to_i > 6 ? model.active_path : nil
        # Top level entities have access to the full body properties.
        # Only top level entities may contain scripts.
        # Child entities have access to the shape property only, and only if
        # its parent body is a compound.
        if active.nil?
          cmd << "$('#tab2-none').css('display', 'none');"
          cmd << "$('#tab2-content1').css('display', 'block');"
          cmd << "$('#tab2-content2').css('display', 'none');"
          cmd << "$('#tab3-none').css('display', 'none');"
          cmd << "$('#tab3-content').css('display', 'block');"
          script = @selected_body.get_attribute('MSPhysics Script', 'Value', '').inspect
          cmd << "editor_set_script(#{script});"
          line = @selected_body.get_attribute('MSPhysics Script', 'Line', 0).to_i
          cmd << "editor_set_line(#{line});"
        else
          cmd << "$('#tab3-none').css('display', 'block');"
          cmd << "$('#tab3-content').css('display', 'none');"
          shape = active.last.get_attribute('MSPhysics Body', 'Shape')
          if shape == 'Compound'
            cmd << "$('#tab2-none').css('display', 'none');"
            cmd << "$('#tab2-content1').css('display', 'none');"
            cmd << "$('#tab2-content2').css('display', 'block');"
          end
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
        width = 500
        height = 500
        @dlg = UI::WebDialog.new(title, false, 'MSPhysics UI', width, height, 800, 600, true)
        @dlg.set_size(width, height) unless @min_size
        # Callbacks
        @dlg.add_action_callback('init'){ |dlg, params|
          update_state
          next if @init_called
          unless @min_size
            w,h = eval(params)
            if Sketchup.version.to_i > 6
              @min_size = [447 + width - w, 200 + height - h]
              @dlg.min_width = @min_size[0]
              @dlg.min_height = @min_size[1]
            end
          end
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
          @selected_body.set_attribute('MSPhysics Script', 'Line', params)
        }
        @dlg.add_action_callback('open_link'){ |dlg, params|
          UI.openURL(params)
        }
        @dlg.add_action_callback('tab_changed'){ |dlg, params|
          num = params[-1].to_i
          @last_active_body_tab = num if (num == 2 or num == 3)
        }
        @dlg.set_on_close(){
          @dlg = nil
          @init_called = false
        }
        # Set content
        dir = File.dirname(__FILE__)
        url = File.join(dir, 'index.html')
        @dlg.set_file(url)
        # Show dialog
        @dlg.show
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
        return unless line
        @dlg.execute_script("editor_set_line(#{line}); editor_select_current_line();")
      else
        self.visible = true
        UI.start_timer(0.5, false){
          @dlg.execute_script('activate_tab(3);')
          next unless line
          @dlg.execute_script("editor_set_line(#{line}); editor_select_current_line();")
        }
      end
    end

    # @!visibility private
    def onSelectionBulkChange(sel)
      update_state
    end

    # @!visibility private
    def onSelectionCleared(sel)
      update_state
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
