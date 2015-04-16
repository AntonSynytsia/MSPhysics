module MSPhysics
  module ControlPanel

    @dialog = nil
    @handle = nil
    @title = 'MSPhysics Control Panel'
    @size = [0,0]

    class << self

      # Open/Close MSPhysics control panel.
      # @param [Boolean] state
      # @return [Boolean] success
      def show(state)
        return false if (state ? true : false) == is_visible?
        if state
          @dialog = ::UI::WebDialog.new(@title, false, nil, 300, 300, 800, 600, false)
          # Callbacks
          @dialog.add_action_callback('init'){ |dlg, params|
          }
          @dialog.add_action_callback('size_changed'){ |dlg, params|
            @size = eval(params)
            update_placement
          }
          @dialog.set_on_close {
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
            style = 0x94000000 # WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS
            AMS::Window.lock_update(@handle)
            AMS::Window.set_long(@handle, -20, style_ex)
            AMS::Window.set_long(@handle, -16, style)
            AMS::Window.lock_update(nil)
            AMS::Window.set_pos(@handle, 0, 0, 0, 0, 0, 0x0267)
            AMS::Window.set_layered_attributes(@handle, 0, 220, 2)
          end
        else
          @dialog.close
        end
        true
      end

      # Determine if control panel is visible.
      # @return [Boolean]
      def is_visible?
        @dialog ? true : false
      end

      # Create a range slider.
      # @note All sliders are removed once the dialog is closed.
      # @param [String] name Slider name.
      # @param [Numeric] starting_value Starting value.
      # @param [Numeric] min Minimum value.
      # @param [Numeric] max Maximum value.
      # @param [Numeric] step Snap step.
      # @return [Numeric] Slider value.
      # @raise [TypeError] if slider with given name already exists.
      # @raise [TypeError] if dialog is closed.
      def add_slider(name, starting_value = 0, min = 0, max = 1, step = 0)
      end

      # Get slider value.
      # @param [String] name Slider name.
      # @return [Numeric, nil] Slider value or nil if slider with given name
      #   doesn't exist.
      # @raise [TypeError] if dialog is closed.
      def get_slider_value(name)
      end

      # @!visibility private


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
