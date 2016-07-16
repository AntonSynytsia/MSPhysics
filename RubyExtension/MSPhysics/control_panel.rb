module MSPhysics

  # @since 1.0.0
  module ControlPanel

    @dialog = nil
    @handle = nil
    @title = 'MSPhysics Control Panel'
    @size = [0,0]
    @border_size = [0,0]
    @sliders = {}
    @mouse_over = false
    @init_called = false

    class << self

      # Acquire handle to the control panel.
      # @return [Fixnum, nil]
      def handle
        @handle
      end

      # Open/close MSPhysics control panel.
      # @param [Boolean] state
      def visible=(state)
        return false if (state ? true : false) == self.visible?
        if state
          iws = [300, 300]
          @dialog = ::UI::WebDialog.new(@title, false, @title, iws.x, iws.y, 800, 600, false)
          # Callbacks
          @dialog.add_action_callback('init') { |dlg, params|
            unless @init_called
              @init_called = true
              ds = eval(params)
              #@border_size = [iws.x - ds.x, iws.y - ds.y]
              if RUBY_PLATFORM =~ /mswin|mingw/i && @handle
                ws = AMS::Window.get_size(@handle)
                cr = AMS::Window.get_client_rect(@handle)
                cs = [cr[2] - cr[0], cr[3] - cr[1]]
                @border_size = [ws.x - cs.x, ws.y - cs.y]
              else
                @border_size = [2, 24]
              end
            end
            @sliders.each { |name, data|
              generate_slider_html(name, false)
            }
            dlg.execute_script("update_size();")
          }
          @dialog.add_action_callback('size_changed') { |dlg, params|
            @size = eval(params)
            update_placement
          }
          @dialog.add_action_callback('mouse_enter') { |dlg, params|
            @mouse_over = true
          }
          @dialog.add_action_callback('mouse_leave') { |dlg, params|
            @mouse_over = false
            AMS::Sketchup.activate if RUBY_PLATFORM =~ /mswin|mingw/i
          }
          @dialog.add_action_callback('update_note') { |dlg, params|
            next if RUBY_PLATFORM !~ /mswin|mingw/i
            cmd = ""
            if AMS::Sketchup.is_main_window_active?
              cmd << "$('#note1').css('display', 'none');"
              cmd << "$('#note2').css('display', 'none');"
              cmd << "$('#note3').fadeIn(750);"
            elsif AMS::Window.is_active?(@handle)
              cmd << "$('#note1').css('display', 'none');"
              cmd << "$('#note3').css('display', 'none');"
              cmd << "$('#note2').fadeIn(750);"
            else
              cmd << "$('#note2').css('display', 'none');"
              cmd << "$('#note3').css('display', 'none');"
              cmd << "$('#note1').fadeIn(750);"
            end
            dlg.execute_script(cmd)
          }
          # Set content
          dir = File.dirname(__FILE__)
          dir.force_encoding("UTF-8") if RUBY_VERSION !~ /1.8/
          url = File.join(dir, 'html/control_panel.html')
          @dialog.set_file(url)
          # Show dialog
          RUBY_PLATFORM =~ /mswin|mingw/i ? @dialog.show : @dialog.show_modal
          # Assign the on_close callback. Important: This must be called after
          # showing dialog in order to work on Mac OS X.
          @dialog.set_on_close {
            if RUBY_PLATFORM =~ /mswin|mingw/i
              AMS::Sketchup.include_dialog(@handle)
              AMS::Sketchup.remove_observer(self)
            end
            @dialog.execute_script('uninit();')
            @dialog = nil
            @handle = nil
            @mouse_over = false
            @init_called = false
          }
          # Find dialog window handle
          @handle = RUBY_PLATFORM =~ /mswin|mingw/i ? AMS::Sketchup.find_window_by_caption(@title) : nil
          if @handle
            # Add observer
            AMS::Sketchup.add_observer(self)
            # Remove dialog caption and borders
            layered = AMS::System.get_windows_version < 6.0 ? 0 : 0x00080000
            style_ex = 0x00010000 | layered # WS_EX_CONTROLPARENT | WS_EX_LAYERED
            #style = 0x54000000 # WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
            style = 0x94000000 # WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS
            AMS::Window.lock_update(@handle)
            AMS::Window.set_long(@handle, -20, style_ex)
            AMS::Window.set_long(@handle, -16, style)
            AMS::Window.lock_update(nil)
            AMS::Window.set_pos(@handle, 0, 0, 0, 0, 0, 0x0267)
            AMS::Window.set_layered_attributes(@handle, 0, 200, 2)
            AMS::Sketchup.ignore_dialog(@handle)
            AMS::Sketchup.activate
          end
        else
          @dialog.close
        end
        true
      end

      # Determine whether control panel is visible.
      # @return [Boolean]
      def visible?
        @dialog ? true : false
      end

      # Create a range slider.
      # @param [String] name Slider name.
      # @param [Numeric] default_value Starting value.
      # @param [Numeric] min Minimum value.
      # @param [Numeric] max Maximum value.
      # @param [Numeric] step Snap step.
      # @return [Boolean] success
      def add_slider(name, default_value = 0, min = 0, max = 1, step = 0.01)
        cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
        return false if @sliders[cname] != nil
        @sliders[cname] = {
          :default_value => default_value.to_f,
          :min => min.to_f,
          :max => max.to_f,
          :step => step.to_f < 1.0e-3 ? 1.0e-3 : step.to_f
        }
        generate_slider_html(name)
        true
      end

      # Destroy a range slider.
      # @param [String] name Slider name.
      # @return [Boolean] success
      def remove_slider(name)
        return false if @sliders[name.to_s].nil?
        degenerate_slider_html(name.to_s)
        @sliders.delete(name.to_s)
        true
      end

      # Destroy all range sliders.
      # @return [Fixnum] Number of sliders removed.
      def remove_sliders
        size = @sliders.size
        @sliders.clear
        @dialog.execute_script("remove_sliders(); update_size();") if @dialog
        size
      end

      # Determine whether a range slider with a particular name already exists.
      # @param [String] name
      # @return [Boolean]
      def slider_exists?(name)
        @sliders[name] ? true : false
      end

      # Get slider value.
      # @param [String] name Slider name.
      # @return [Numeric, nil] Slider value or nil if slider with the given name
      #   doesn't exist.
      def get_slider_value(name)
        return unless @dialog
        cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
        data = @sliders[cname]
        return unless data
        res = @dialog.get_element_value("lcrs-" + cname)
        res.empty? ? data[:default_value] : res.to_f
      end

      # Set slider value.
      # @param [String] name Slider name.
      # @param [Numeric] value
      # @return [Boolean] success
      def set_slider_value(name, value)
        return false unless @dialog
        cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
        data = @sliders[cname]
        return false unless data
        return false if @dialog.get_element_value("lcrs-" + cname).empty?
        cmd = "sliders[\"#{cname}\"].setValue(#{value.to_f}); update_slider(\"#{cname}\");"
        @dialog.execute_script(cmd)
        true
      end


      # @!visibility private


      def generate_slider_html(name, update_size = true)
        return false unless @dialog
        cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
        data = @sliders[cname]
        return false unless data
        return false unless @dialog.get_element_value("lcrs-" + cname).empty?
        cmd = "add_slider(\"#{cname}\", #{data[:default_value]}, #{data[:min]}, #{data[:max]}, #{data[:step]});"
        cmd << "update_size();" if update_size
        @dialog.execute_script(cmd)
        true
      end

      def degenerate_slider_html(name, update_size = true)
        return false unless @dialog
        cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
        return false if @dialog.get_element_value("lcrs-" + cname).empty?
        cmd = "remove_slider(\"#{cname}\");"
        cmd << "update_size();" if update_size
        @dialog.execute_script(cmd)
        true
      end

      def update_placement
        return false unless @dialog
        wsx = @border_size.x + @size.x
        wsy = @border_size.y + @size.y
        if RUBY_PLATFORM =~ /mswin|mingw/i && @handle
          vr = AMS::Sketchup.get_viewport_rect
          x = vr[2] - wsx
          y = vr[3] - wsy
          AMS::Window.set_pos(@handle, 0, x, y, wsx, wsy, 0x0234)
        else
          @dialog.set_size(wsx, wsy)
        end
        true
      end

      def swo_on_size_move(x,y, w,h)
        update_placement
      end

      def swo_on_viewport_size(w, h)
        update_placement
      end

    end # class << self
  end # module ControlPanel
end # module MSPhysics
