# BodyContext contains {MSPhysics::Body} events and event-related functions.
# @since 1.0.0
class MSPhysics::BodyContext < MSPhysics::CommonContext

  # @param [Body] body A body to be associated with the context.
  def initialize(body)
    @__body = body
    @__events = {
      :onStart                => nil,
      :onUpdate               => nil,
      :onPreUpdate            => nil,
      :onPostUpdate           => nil,
      :onTick                 => nil,
      :onPreFrame             => nil,
      :onPostFrame            => nil,
      :onEnd                  => nil,
      :onDraw                 => nil,
      :onPlay                 => nil,
      :onPause                => nil,
      :onTouch                => nil,
      :onTouching             => nil,
      :onUntouch              => nil,
      :onClick                => nil,
      :onDrag                 => nil,
      :onUnclick              => nil,
      :onKeyDown              => nil,
      :onKeyUp                => nil,
      :onKeyExtended          => nil,
      :onMouseMove            => nil,
      :onLButtonDown          => nil,
      :onLButtonUp            => nil,
      :onLButtonDoubleClick   => nil,
      :onRButtonDown          => nil,
      :onRButtonUp            => nil,
      :onRButtonDoubleClick   => nil,
      :onMButtonDown          => nil,
      :onMButtonUp            => nil,
      :onMButtonDoubleClick   => nil,
      :onXButton1Down         => nil,
      :onXButton1Up           => nil,
      :onXButton1DoubleClick  => nil,
      :onXButton2Down         => nil,
      :onXButton2Up           => nil,
      :onXButton2DoubleClick  => nil,
      :onMouseWheelRotate     => nil,
      :onMouseWheelTilt       => nil
    }

    super()
  end

  # @api private
  # @param [String] script
  # @param [String] script_name
  # @param [Fixnum] line
  def eval_script(script, script_name, line)
    eval(script, binding, script_name, line)
  end

  # Get the associated body.
  # @return [Body]
  def this
    @__body
  end

  # Assign a block of code to an event or a list of events.
  # @example
  #   on(:keyDown, :keyUp, :keyExtended) { |key, value, char|
  #     simulation.log_line(key)
  #   }
  # @param [Symbol, String] events
  # @yield A block of code.
  # @return [Fixnum] The number of events assigned.
  def on(*events, &block)
    count = 0
    events.flatten.each { |evt|
      evt = evt.to_s.downcase
      evt.insert(0, 'on') if evt[0..1] != 'on'
      found = false
      @__events.each { |key, val|
        next if key.to_s.downcase != evt
        evt = key
        found = true
        break
      }
      next unless found
      @__events[evt] = block
      count += 1
    }
    state = @__events[:onTouch] || @__events[:onTouching] || @__events[:onUntouch]
    MSPhysics::Newton::Body.set_record_touch_data_state(@__body.address, state)
    count
  end

  # Get a Proc object assigned to an event.
  # @param [Symbol, String] event
  # @return [Proc, nil] A Proc object or +nil+ if there is no procedure to an
  #   event.
  def get_event_proc(event)
    @__events[event.to_sym]
  end

  # Assign a Proc object to an event.
  # @param [Symbol, String] event
  # @param [Proc, nil] proc A Proc object or +nil+ to remove procedure from an
  #   event.
  # @return [Boolean] success
  def set_event_proc(event, proc)
    AMS.validate_type(proc, Proc, NilClass)
    return false unless @__events.keys.include?(event.to_sym)
    @__events[event.to_sym] = proc
    if event.to_s =~ /onTouch|onTouching|onUntouch/
      state = @__events[:onTouch] || @__events[:onTouching] || @__events[:onUntouch]
      MSPhysics::Newton::Body.set_record_touch_data_state(@__body.address, state)
    end
    true
  end

  # Determine whether particular event has a Proc object.
  # @param [Symbol, String] event
  # @return [Boolean]
  def event_proc_assigned?(event)
    @__events[event.to_sym] != nil
  end

  # Trigger an event.
  # @api private
  # @param [Symbol, String] event Event name.
  # @param [*args] args Event arguments.
  # @return [Boolean] success
  def call_event(event, *args)
    evt = @__events[event.to_sym]
    return false unless evt
    begin
      evt.call(*args)
    rescue Exception => err
      ref = nil
      test = MSPhysics::SCRIPT_NAME + ':'
      err_message = err.message
      err_backtrace = err.backtrace
      unless AMS::IS_RUBY_VERSION_18
        err_message.force_encoding('UTF-8')
        err_backtrace.each { |i| i.force_encoding('UTF-8') }
      end
      err_backtrace.each { |location|
        if location.include?(test)
          ref = location
          break
        end
      }
      ref = err_message if !ref && err_message.include?(test)
      line = ref ? ref.split(test, 2)[1].split(/\:/, 2)[0].to_i : nil
      msg = "#{err.class.to_s[0] =~ /a|e|i|o|u/i ? 'An' : 'A'} #{err.class} has occurred while calling body #{event} event#{line ? ', line ' + line.to_s : nil}:\n#{err_message}"
      raise MSPhysics::ScriptException.new(msg, err_backtrace, @__body.group, line)
    end
    true
  end

  # @!group Simulation Events

  # Assign a block of code to the onStart event.
  # @yield This event is triggered once when simulation starts, hence when the
  #   frame is zero. No world updates are made at this point.
  def onStart(&block)
    set_event_proc(:onStart, block)
  end

  # Assign a block of code to the onUpdate event.
  # @yield This event is triggered every time the world is updated. For
  #   instance, if the world update rate is three, this event will be
  #   triggered three times per frame. For performance purposes, group
  #   transformations are not updated at this point. Only the
  #   transformations for the bodies referencing the groups are updated.
  #   This event is suitable for applying forces and velocities.
  def onUpdate(&block)
    set_event_proc(:onUpdate, block)
  end

  # Assign a block of code to the onPreUpdate event.
  # @yield This event is triggered every time before the world is updated,
  #   particularly before the #{onUpdate} event. Just like #{onUpdate}, this
  #   event can be triggered multiple times per frame.
  def onPreUpdate(&block)
    set_event_proc(:onPreUpdate, block)
  end

  # Assign a block of code to the onUpdate event.
  # @yield This event is triggered every time after the world is updated,
  #   particularly after the #{onUpdate} event. Just like #{onUpdate}, this
  #   event can be triggered multiple times per frame.
  def onPostUpdate(&block)
    set_event_proc(:onPostUpdate, block)
  end

  # Assign a block of code to the onTick event.
  # @yield This event is triggered every time the frame is changed,
  #   specifically after all the world updates of the frame take place and
  #   after group transformations are updated.
  def onTick(&block)
    set_event_proc(:onTick, block)
  end

  # Assign a block of code to the onPreFrame event.
  # @yield This event is triggered every time before the frame is changed,
  #   particularly before the #{onTick} event.
  def onPreFrame(&block)
    set_event_proc(:onPreFrame, block)
  end

  # Assign a block of code to the onPostFrame event.
  # @yield This event is triggered every time after the frame is changed,
  #   particularly after the #{onTick} event.
  def onPostFrame(&block)
    set_event_proc(:onPostFrame, block)
  end

  # Assign a block of code to the onEnd event.
  # @yield This event is triggered when simulation ends, right before the
  #   bodies are moved back to their starting transformation.
  def onEnd(&block)
    set_event_proc(:onEnd, block)
  end

  # Assign a block of code to the onDraw event.
  # @yield This event is triggered whenever the view is redrawn, even when
  #   simulation is paused.
  # @yieldparam [Sketchup::View] view
  # @yieldparam [Geom::BoundingBox] bb
  # @example
  #   onDraw { |view, bb|
  #     pts = [[0,0,100], [100,100,100]]
  #     # Add points to the view bounding box to prevent the line from being
  #     # clipped.
  #     bb.add(pts)
  #     # Now, draw the line in red.
  #     view.drawing_color = 'red'
  #     view.draw(GL_LINES, pts)
  #   }
  def onDraw(&block)
    set_event_proc(:onDraw, block)
  end

  # Assign a block of code to the onPlay event.
  # @yield This event is triggered when simulation is played. It is not called
  #   when simulation starts.
  def onPlay(&block)
    set_event_proc(:onPlay, block)
  end

  # Assign a block of code to the onPause event.
  # @yield This event is triggered when simulation is paused.
  def onPause(&block)
    set_event_proc(:onPause, block)
  end

  # Assign a block of code to the onTouch event.
  # @yield This event is triggered when this body comes in contact with
  #   another body.
  # @yieldparam [Body] toucher
  # @yieldparam [Geom::Point3d] point
  # @yieldparam [Geom::Vector3d] normal
  # @yieldparam [Geom::Vector3d] force in Newtons.
  # @yieldparam [Numeric] speed in meters per second.
  def onTouch(&block)
    set_event_proc(:onTouch, block)
  end

  # Assign a block of code to the onTouching event.
  # @yield This event is triggered every frame when the body is in an extended
  #   contact with another body.
  # @yieldparam [Body] toucher
  def onTouching(&block)
    set_event_proc(:onTouching, block)
  end

  # Assign a block of code to the onUntouch event.
  # @yield This event is triggered when particular body is no longer in
  #   contact with another body. When this procedure is triggered it doesn't
  #   always mean the body is free from all contacts. This means particular
  #   +toucher+ has stopped touching the body.
  # @note Sometimes you may want to know whether particular body is in contact
  #   with another body. Relying on events is not always the best technique.
  #   To determine whether this body is in contact with another body, use
  #   <tt>this.touching_with?(some_other_body)</tt> or
  #   <tt>this.touching_bodies</tt>.
  # @yieldparam [Body] toucher
  def onUntouch(&block)
    set_event_proc(:onUntouch, block)
  end

  # Assign a block of code to the onClick event.
  # @yield This event is triggered when the body is clicked.
  # @yieldparam [Geom::Point3d] point Clicked position in global space.
  def onClick(&block)
    set_event_proc(:onClick, block)
  end

  # Assign a block of code to the onDrag event.
  # @yield This event is triggered whenever the body is dragged by a mouse.
  def onDrag(&block)
    set_event_proc(:onDrag, block)
  end

  # Assign a block of code to the onUnclick event.
  # @yield This event is triggered when the body is unclicked.
  def onUnclick(&block)
    set_event_proc(:onUnclick, block)
  end

  # @!endgroup
  # @!group User Input Events

  # Assign a block of code to the onKeyDown event.
  # @yield This event is called when the key is pressed.
  # @yieldparam [String] key Virtual key name.
  # @yieldparam [Fixnum] val Virtual key constant value.
  # @yieldparam [String] char Actual key character.
  # @note Windows only!
  # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
  def onKeyDown(&block)
    set_event_proc(:onKeyDown, block)
  end

  # Assign a block of code to the onKeyUp event.
  # @yield This event is called when the key is released.
  # @yieldparam [String] key Virtual key name.
  # @yieldparam [Fixnum] val Virtual key constant value.
  # @yieldparam [String] char Actual key character.
  # @note Windows only!
  # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
  def onKeyUp(&block)
    set_event_proc(:onKeyUp, block)
  end

  # Assign a block of code to the onKeyExtended event.
  # @yield This event is called when the key is held down.
  # @yieldparam [String] key Virtual key name.
  # @yieldparam [Fixnum] val Virtual key constant value.
  # @yieldparam [String] char Actual key character.
  # @note Windows only!
  # @see http://www.rubydoc.info/github/AntonSynytsia/AMS-Library/master/file/Keyboard.md Virtual-Key Names
  def onKeyExtended(&block)
    set_event_proc(:onKeyExtended, block)
  end

  # Assign a block of code to the onMouseMove event.
  # @yield This event is called when the mouse is moved.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @yieldparam [Sketchup::View] view
  def onMouseMove(&block)
    set_event_proc(:onMouseMove, block)
  end

  # Assign a block of code to the onLButtonDown event.
  # @yield This event is called when the left mouse button is pressed.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onLButtonDown(&block)
    set_event_proc(:onLButtonDown, block)
  end

  # Assign a block of code to the onLButtonUp event.
  # @yield This event is called when the left mouse button is released.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onLButtonUp(&block)
    set_event_proc(:onLButtonUp, block)
  end

  # Assign a block of code to the onLButtonDoubleClick event.
  # @yield This event is called when the left mouse button is double clicked.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onLButtonDoubleClick(&block)
    set_event_proc(:onLButtonDoubleClick, block)
  end

  # Assign a block of code to the onRButtonDown event.
  # @yield This event is called when the right mouse button is pressed.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onRButtonDown(&block)
    set_event_proc(:onRButtonDown, block)
  end

  # Assign a block of code to the onRButtonUp event.
  # @yield This event is called when the right mouse button is released.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onRButtonUp(&block)
    set_event_proc(:onRButtonUp, block)
  end

  # Assign a block of code to the onRButtonDoubleClick event.
  # @yield This event is called when the right mouse button is double clicked.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onRButtonDoubleClick(&block)
    set_event_proc(:onRButtonDoubleClick, block)
  end

  # Assign a block of code to the onMButtonDown event.
  # @yield This event is called when the middle mouse button is pressed.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onMButtonDown(&block)
    set_event_proc(:onMButtonDown, block)
  end

  # Assign a block of code to the onMButtonUp event.
  # @yield This event is called when the middle mouse button is released.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onMButtonUp(&block)
    set_event_proc(:onMButtonUp, block)
  end

  # Assign a block of code to the onMButtonDoubleClick event.
  # @yield This event is called when the middle mouse button is double clicked.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onMButtonDoubleClick(&block)
    set_event_proc(:onMButtonDoubleClick, block)
  end

  # Assign a block of code to the onXButton1Down event.
  # @yield This event is called when the X1 mouse button is pressed.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onXButton1Down(&block)
    set_event_proc(:onXButton1Down, block)
  end

  # Assign a block of code to the onXButton1Up event.
  # @yield This event is called when the X1 mouse button is released.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onXButton1Up(&block)
    set_event_proc(:onXButton1Up, block)
  end

  # Assign a block of code to the onXButton1DoubleClick event.
  # @yield This event is called when the X1 mouse button is double clicked.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onXButton1DoubleClick(&block)
    set_event_proc(:onXButton1DoubleClick, block)
  end

  # Assign a block of code to the onXButton2Down event.
  # @yield This event is called when the X2 mouse button is pressed.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onXButton2Down(&block)
    set_event_proc(:onXButton2Down, block)
  end

  # Assign a block of code to the onXButton2Up event.
  # @yield This event is called when the X2 mouse button is released.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onXButton2Up(&block)
    set_event_proc(:onXButton2Up, block)
  end

  # Assign a block of code to the onXButton2DoubleClick event.
  # @yield This event is called when the X2 mouse button is double clicked.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @note Windows only!
  def onXButton2DoubleClick(&block)
    set_event_proc(:onXButton2DoubleClick, block)
  end

  # Assign a block of code to the onMouseWheelRotate event.
  # @yield This event is called when the mouse wheel is rotated.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @yieldparam [Fixnum] dir Rotate direction: +-1+ - down, +1+ - up.
  # @note Windows only!
  def onMouseWheelRotate(&block)
    set_event_proc(:onMouseWheelRotate, block)
  end

  # Assign a block of code to the onMouseWheelTilt event.
  # @yield This event is called when the mouse wheel is tilted.
  # @yieldparam [Fixnum] x
  # @yieldparam [Fixnum] y
  # @yieldparam [Fixnum] dir Tilt direction: +-1+ - left, +1+ - right.
  # @note Windows only!
  def onMouseWheelTilt(&block)
    set_event_proc(:onMouseWheelTilt, block)
  end

  # @!endgroup

end # class MSPhysics::BodyContext
