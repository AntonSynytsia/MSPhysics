# @since 1.0.0
module MSPhysics::ControlPanel

  USE_HTML_DIALOG = ::Sketchup.version.to_i > 16 ? true : false
  TITLE = 'MSPhysics Control Panel'
  FADE_DURATION = 2.0
  FADE_DELAY = 2.0
  MIN_OPACITY = 10
  MAX_OPACITY = 250

  @dialog = nil
  @handle = nil
  @size = [0,0]
  @border_size = [0,0]
  @sliders = {}
  @mouse_over = false
  @init_called = false
  @fade_time = nil
  @fade_wait_time = nil
  @test_result = nil

  class << self

    # Acquire handle to the control panel.
    # @return [Integer, nil]
    def handle
      @handle
    end

    # Open control panel.
    def open
      return if @dialog

      iws = [300, 300]
      if USE_HTML_DIALOG
        @dialog = ::UI::HtmlDialog.new({
          :dialog_title => TITLE,
          :preferences_key => TITLE,
          :scrollable => false,
          :resizable => true,
          :width => iws.x,
          :height => iws.y,
          :left => 800,
          :top => 600,
          :min_width => 50,
          :min_height => 50,
          :max_width => -1,
          :max_height => -1,
          :style => ::UI::HtmlDialog::STYLE_DIALOG
        })
      else
        @dialog = ::UI::WebDialog.new(TITLE, false, TITLE, iws.x, iws.y, 800, 600, true)
      end

      # Callbacks
      @dialog.add_action_callback('init') { |acs, params|
        unless @init_called
          @init_called = true
          ui_scale = ::Sketchup.version.to_i > 16 ? ::UI.scale_factor : 1.0
          ds = eval(params)
          for i in ds.size
            data[i] *= ui_scale
          end
          #@border_size = [iws.x - ds.x, iws.y - ds.y]
          if AMS::IS_PLATFORM_WINDOWS && @handle
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
        execute_js("update_size();")
      }

      @dialog.add_action_callback('size_changed') { |acs, params|
        @size = eval(params)
        update_placement
      }

      @dialog.add_action_callback('mouse_enter') { |acs, params|
        @mouse_over = true
        @fade_time = 0.0 unless @fade_time
        @fade_wait_time = nil
      }

      @dialog.add_action_callback('mouse_leave') { |acs, params|
        @mouse_over = false
        unless @fade_time
          @fade_time = FADE_DURATION
          @fade_wait_time = 0.0
        end
        AMS::Sketchup.bring_to_top if AMS::IS_PLATFORM_WINDOWS
      }

      @dialog.add_action_callback('update_note') { |acs, params|
        next unless AMS::IS_PLATFORM_WINDOWS
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
        execute_js(cmd)
      }
      @dialog.add_action_callback('test_result') { |acs, params|
        @test_result = eval(params)
      }

      # Set content
      dir = ::File.dirname(__FILE__)
      dir.force_encoding('UTF-8') unless AMS::IS_RUBY_VERSION_18
      url = ::File.join(dir, 'html/control_panel.html')
      @dialog.set_file(url)

      # Show dialog
      if AMS::IS_PLATFORM_WINDOWS || USE_HTML_DIALOG
        @dialog.show
      else
        @dialog.show_modal
      end

      # Assign the on_close callback. Important: This must be called after
      # showing dialog in order to work on Mac OS X.
      do_on_close = Proc.new {
        if AMS::IS_PLATFORM_WINDOWS
          AMS::Sketchup.include_dialog(@handle)
          AMS::Sketchup.remove_observer(self)
        end
        execute_js('blur_input(); uninit();')
        @dialog = nil
        @handle = nil
        @mouse_over = false
        @init_called = false
        true
      }
      if USE_HTML_DIALOG
        @dialog.set_can_close() { do_on_close.call }
      else
        @dialog.set_on_close() { do_on_close.call }
      end

      # Find dialog window handle
      @handle = AMS::IS_PLATFORM_WINDOWS ? AMS::Sketchup.find_window_by_caption(TITLE) : nil
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
        AMS::Window.set_layered_attributes(@handle, 0, MAX_OPACITY, 2)
        AMS::Sketchup.ignore_dialog(@handle)
        AMS::Sketchup.bring_to_top
        @fade_time = FADE_DURATION
        @fade_wait_time = 0.0
      end
    end

    # Close the control panel
    def close
      @dialog.close if @dialog
    end

    # Determine whether control panel is open.
    # @return [Boolean]
    def open?
      @dialog ? true : false
    end

    # Show the control panel window.
    # @note Windows only!
    def show
      if AMS::IS_PLATFORM_WINDOWS && @handle
        AMS::Window.show(@handle, 8)
      end
    end

    # Hide the control panel window.
    # @note Windows only!
    def hide
      if AMS::IS_PLATFORM_WINDOWS && @handle
        AMS::Window.show(@handle, 0)
      end
    end

    # Determine whether the control panel window is visible.
    # @return [Boolean]
    def visible?
      return false unless @dialog
      if AMS::IS_PLATFORM_WINDOWS && @handle
        AMS::Window.is_visible?(@handle)
      else
        true
      end
    end

    # Activate the control panel window.
    def bring_to_front
      @dialog.bring_to_front if @dialog
    end

    # Execute JavaScript in the control panel.
    # @param [String] code
    def execute_js(code)
      if @dialog && @init_called
        @dialog.execute_script(code)
      end
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
    # @return [Integer] Number of sliders removed.
    def remove_sliders
      size = @sliders.size
      @sliders.clear
      execute_js("remove_sliders(); update_size();")
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
      return if @dialog.nil? || !@init_called
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
      return false if @dialog.nil? || !@init_called
      cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
      data = @sliders[cname]
      return false if data.nil? || @dialog.get_element_value("lcrs-" + cname).empty?
      cmd = "sliders[\"#{cname}\"].setValue(#{value.to_f}); update_slider(\"#{cname}\");"
      execute_js(cmd)
      true
    end

    # Get the number of sliders in the control panel.
    # @return [Integer]
    def sliders_count
      @sliders.size
    end

    # Compute size of a text in pixels.
    # @note The control panel must be open for the values to be generated properly.
    # @param [String] text
    # @param [Hash] opts
    # @option opts [String] :font ("Ariel") Text font.
    # @option opts [Integer] :size (11) Font size in pixels.
    # @option opts [Boolean] :bold (false) Whether to have the text bold.
    # @option opts [Boolean] :italic (false) Whether to have the text
    #   italicized.
    # @return [Array, nil] An array of two values if successful
    def compute_text_size(text, opts = {})
      return false if @dialog.nil? || !@init_called
      text = text.to_s
      font = opts.has_key?(:font) ? opts[:font].to_s : "Ariel"
      size = opts.has_key?(:size) ? opts[:size].to_i : 11
      bold = opts.has_key?(:bold) ? (opts[:bold] ? true : false) : false
      italic = opts.has_key?(:italic) ? (opts[:italic] ? true : false) : false
      cmd = "compute_text_size(#{text.inspect}, #{font.inspect}, #{size}, #{bold}, #{italic})"
      execute_js(cmd)
      temp = @test_result
      @test_result = nil
      return temp
    end

    # @!visibility private


    def generate_slider_html(name, update_size = true)
      return false if @dialog.nil? || !@init_called
      cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
      data = @sliders[cname]
      return false unless data
      return false unless @dialog.get_element_value("lcrs-" + cname).empty?
      cmd = "add_slider(\"#{cname}\", #{data[:default_value]}, #{data[:min]}, #{data[:max]}, #{data[:step]});"
      cmd << "update_size();" if update_size
      execute_js(cmd)
      true
    end

    def degenerate_slider_html(name, update_size = true)
      return false if @dialog.nil? || !@init_called
      cname = name.to_s.gsub(/[\\\"\'\v\n\t\r\f]/, '')
      return false if @dialog.get_element_value("lcrs-" + cname).empty?
      cmd = "remove_slider(\"#{cname}\");"
      cmd << "update_size();" if update_size
      execute_js(cmd)
      true
    end

    def update_placement
      return false unless @dialog
      ui_scale = ::Sketchup.version.to_i > 16 ? ::UI.scale_factor : 1.0
      wsx = @border_size.x + @size.x * ui_scale
      wsy = @border_size.y + @size.y * ui_scale
      if AMS::IS_PLATFORM_WINDOWS && @handle
        vr = AMS::Sketchup.get_viewport_rect
        x = vr[2] - wsx
        y = vr[3] - wsy
        AMS::Window.set_pos(@handle, 0, x, y, wsx, wsy, 0x0234)
      else
        @dialog.set_size(wsx, wsy)
      end
      true
    end

    def udpdate_opacity
      return if @dialog.nil? || @fade_time.nil? || !AMS::IS_PLATFORM_WINDOWS || @handle.nil?
      if @fade_wait_time
        @fade_wait_time += MSPhysics::VIEW_UPDATE_TIMESTEP
        return if @fade_wait_time < FADE_DELAY
        @fade_wait_time = nil
      end
      if @mouse_over
        @fade_time += MSPhysics::VIEW_UPDATE_TIMESTEP
      else
        @fade_time -= MSPhysics::VIEW_UPDATE_TIMESTEP
      end
      if @fade_time <= 0.0
        alpha = 0.0
        @fade_time = nil
      elsif @fade_time >= FADE_DURATION
        alpha = 1.0
        @fade_time = nil
      else
        alpha = @fade_time / FADE_DURATION
      end
      #alpha = 0.5 * Math.sin(Math::PI * (alpha - 0.5)) + 0.5
      opacity = (MIN_OPACITY + (MAX_OPACITY - MIN_OPACITY) * alpha).round
      AMS::Window.set_layered_attributes(@handle, 0, opacity, 2)
    end

    def swo_on_size_move(x,y, w,h)
      update_placement
    end

    def swo_on_viewport_size(w, h)
      update_placement
    end

    def swo_on_exit_size_move(x, y, w, h)
      update_placement
    end

  end # class << self
end # module MSPhysics::ControlPanel
