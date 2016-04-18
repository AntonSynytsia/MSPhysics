module MSPhysics
  module ControlPanel

    @dialog = nil
    @handle = nil
    @title = 'MSPhysics Control Panel'
    @size = [0,0]
    @sliders = {}

    class << self

      # Open/close MSPhysics control panel.
      # @param [Boolean] state
      # @return [Boolean] success
      def show(state)
        return false if (state ? true : false) == is_visible?
        if state
          @dialog = ::UI::WebDialog.new(@title, false, nil, 300, 300, 800, 600, false)
          update_size = true
          # Callbacks
          @dialog.add_action_callback('init'){ |dlg, params|
            update_size = false
            @sliders.each { |name, data|
              generate_slider_html(name)
            }
            update_size = true
          }
          @dialog.add_action_callback('size_changed'){ |dlg, params|
            @size = eval(params)
            update_placement if update_size
          }
          @dialog.set_on_close {
            AMS::Sketchup.include_dialog(@handle)
            @dialog = nil
            @handle = nil
            AMS::Sketchup.remove_observer(self)
          }
          # Set content
          dir = File.dirname(__FILE__)
          url = File.join(dir, 'html/control_panel.html')
          @dialog.set_file(url)
          # Show dialog
          RUBY_PLATFORM =~ /mswin|mingw/i ? @dialog.show : @dialog.show_modal
          # Find dialog window handle
          @handle = AMS::Sketchup.find_window_by_caption(@title)
          if @handle
            # Add observer
            AMS::Sketchup.add_observer(self)
            # Remove dialog caption and borders
            layered = AMS::System.get_windows_version < 6.0 ? 0 : 0x00080000
            style_ex = 0x00010000 | layered # WS_EX_CONTROLPARENT | WS_EX_LAYERED
            style = 0x54000000 # WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
            #~ style = 0x94000000 # WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS
            AMS::Window.lock_update(@handle)
            AMS::Window.set_long(@handle, -20, style_ex)
            AMS::Window.set_long(@handle, -16, style)
            AMS::Window.lock_update(nil)
            AMS::Window.set_pos(@handle, 0, 0, 0, 0, 0, 0x0267)
            AMS::Window.set_layered_attributes(@handle, 0, 200, 2)
            AMS::Sketchup.ignore_dialog(@handle)
          end
        else
          @dialog.close
        end
        true
      end

      # Determine whether control panel is visible.
      # @return [Boolean]
      def is_visible?
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
        cname = name.to_s.inspect[1...-1]
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
        @dialog.execute_script("remove_sliders(); size_changed();") if @dialog
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
        cname = name.to_s.inspect[1...-1]
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
        cname = name.to_s.inspect[1...-1]
        data = @sliders[cname]
        return false unless data
        return false if @dialog.get_element_value("lcrs-" + cname).empty?
        cmd = "sliders[\"#{cname}\"].setValue(#{value.to_f});"
        @dialog.execute_script(cmd)
        true
      end

      # @!visibility private


      def generate_slider_html(name)
        return false unless @dialog
        cname = name.to_s.inspect[1...-1]
        data = @sliders[cname]
        return false unless data
        return false unless @dialog.get_element_value("lcrs-" + cname).empty?
        cmd = "add_slider(\"#{cname}\", #{data[:default_value]}, #{data[:min]}, #{data[:max]}, #{data[:step]});"
        cmd << "size_changed();"
        @dialog.execute_script(cmd)
        true
      end

      def degenerate_slider_html(name)
        return false unless @dialog
        cname = name.to_s.inspect[1...-1]
        return false if @dialog.get_element_value("lcrs-" + cname).empty?
        cmd = "remove_slider(\"#{cname}\");"
        cmd << "size_changed();"
        @dialog.execute_script(cmd)
        true
      end

      def update_placement
        return false unless @dialog
        vr = AMS::Sketchup.get_viewport_rect
        x = vr[2] - @size[0]
        y = vr[3] - @size[1]
        AMS::Window.set_pos(@handle, 0, x, y, @size[0], @size[1], 0x0234)
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
