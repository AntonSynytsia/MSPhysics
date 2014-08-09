module MSPhysics
  class BodyContext < Body

    include CommonContext

    # @!visibility private
    @@_error_reference = nil

    # @!visibility private
    def self._error_reference
      @@_error_reference
    end

    # @!visibility private
    def self._error_reference=(ref)
      @@_error_reference = ref
    end

    # @overload initialize(world, ent, type, shape, material, bb_tra)
    #   Create a body from scratch.
    #   @param [AMS::FFI:::Pointer] world A pointer to the newton world.
    #   @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    #   @param [Symbol, String] type Body type. See {TYPES}.
    #   @param [Symbol, String] shape Collision shape. See {Collision::SHAPES}.
    #   @param [Material] material
    # @overload initialize(body, tra)
    #   Create a copy of the body.
    #   @param [BodyContext] body A body Object.
    #   @param [Geom::Transformation, Array<Numeric>] tra
    def initialize(*args)
      super(*args)
      @_script = ''
      @_events = {
        :onStart                => nil,
        :onUpdate               => nil,
        :onPreUpdate            => nil,
        :onPostUpdate           => nil,
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
        :onXButtonDown          => nil,
        :onXButtonUp            => nil,
        :onXButtonDoubleClick   => nil,
        :onMouseWheelRotate     => nil,
        :onMouseWheelTilt       => nil
      }
      @_script_enabled = true
    end

    # Get the string of code assigned to the body context.
    # @return [String]
    def get_script
      check_validity
      @_script
    end

    # Add to the existing script or overwrite the current script.
    # @param [String] code
    def add_script(code)
      check_validity
      begin
        eval(code.to_s)
      rescue Exception => e
        @@_error_reference = [@_entity.entityID]
        index = Sketchup.active_model.entities.to_a.index(@_entity)
        test = '(eval):'
        ref = e.backtrace[0].to_s
        found = ref.include?(test)
        unless found
          ref = e.message.to_s
          found = ref.include?(test)
        end
        if found
          line = ref.split(':')[1].to_i
          @@_error_reference << line
          raise "Exception in entity [#{index}], line #{line}!\n#{e}"
        else
          raise "Exception in entity [#{index}]!\n#{e}"
        end
      end
      @_script += "\n" + code.to_s
    end

    # Clear and assign new script to the body.
    # @param [String] code
    def set_script(code)
      check_validity
      clear_script
      @_script = code.to_s
      begin
        eval(@_script)
      rescue Exception => e
        @@_error_reference = [@_entity.entityID]
        index = Sketchup.active_model.entities.to_a.index(@_entity)
        test = '(eval):'
        ref = e.backtrace[0].to_s
        found = ref.include?(test)
        unless found
          ref = e.message.to_s
          found = ref.include?(test)
        end
        if found
          line = ref.split(':')[1].to_i
          @@_error_reference << line
          raise "Exception in entity [#{index}], line #{line}!\n#{e}"
        else
          raise "Exception in entity [#{index}]!\n#{e}"
        end
      end
    end

    # Clear body script.
    def clear_script
      check_validity
      @_events.keys.each { |key|
        @_events[key] = nil
      }
      @_script = ''
    end

    # Assign a block of code to an event or a list of events.
    # @param [Symbol, String] events Event name(s).
    # @yield A block of code.
    # @return [Fixnum] The number of events assigned.
    # @example
    #   on(:keyDown, :keyUp, :keyExtended){ |key, value, char|
    #     simulation.log_line(key)
    #   }
    def on(*events, &block)
      check_validity
      count = 0
      events.flatten.each{ |evt|
        evt = evt.to_s.downcase
        evt.insert(0, 'on') if evt[0..1] != 'on'
        found = false
        @_events.keys.each{ |key|
          next if key.to_s.downcase != evt
          evt = key
          found = true
          break
        }
        next unless found
        @_events[evt] = block
        count += 1
      }
      count
    end

    # Get the procedure object assigned to an event.
    # @param [Symbol] event Event name.
    # @return [Proc, NilClass] A procedure object (if successful).
    def get_proc(event)
      check_validity
      @_events[event]
    end

    # Assign a procedure object to an event.
    # @param [Symbol] event Event name.
    # @param [Proc] proc A block of code.
    # @return [Boolean] +true+ (if successful).
    def set_proc(event, proc)
      check_validity
      return false unless @_events.keys.include?(event)
      @_events[event] = proc
      true
    end

    alias assign_proc set_proc

    # Remove the procedure object assigned to an event.
    # @param [Symbol] event Event name.
    # @return [Boolean] +true+ (if successful).
    def delete_proc(event)
      check_validity
      e = @_events[event]
      return false unless e
      @_events[event] = nil
      true
    end

    alias remove_proc delete_proc

    # Determine whether the procedure is assigned to an event.
    # @param [Symbol] event
    # @return [Boolean]
    def proc_assigned?(event)
      check_validity
      @_events[event] ? true : false
    end

    # Trigger an event.
    # @param [Symbol] event Event name.
    # @param [*args] args Event arguments.
    # @return [Boolean] +true+ (if successful).
    # @api private
    def call_event(event, *args)
      check_validity
      return false unless @_script_enabled
      evt = @_events[event]
      return false unless evt
      begin
        evt.call(*args)
      rescue Exception => e
        @@_error_reference = [@_entity.entityID]
        index = Sketchup.active_model.entities.to_a.index(@_entity)
        test = '(eval):'
        ref = e.backtrace[0].to_s
        found = ref.include?(test)
        unless found
          ref = e.message.to_s
          found = ref.include?(test)
        end
        if found
          line = ref.split(':')[1].to_i
          @@_error_reference << line
          raise "Exception in entity [#{index}], line #{line}!\n#{event} error:\n#{e}"
        else
          raise "Exception in entity [#{index}]!\n#{event} error:\n#{e}"
        end
      end
      true
    end

    # Enable/Disable body script. Disabling script will prevent all body
    # procedures from being called.
    # @param [Boolean] state
    def script_enabled=(state)
      check_validity
      @_script_enabled = state ? true : false
    end

    # Determine whether body script is enabled.
    # @return [Boolean]
    def script_enabled?
      check_validity
      @_script_enabled
    end

    # @!group Simulation Events

    # Assign a block of code to the onStart event.
    # @yield This event is triggered once when simulation starts, hence when the
    #   frame is zero. No transformation updates are made at this point.
    def onStart(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onUpdate event.
    # @yield This event is triggered every frame after the simulation starts,
    #   hence when the frame is greater than zero. Specifically, it is called
    #   after the newton update takes place.
    def onUpdate(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onPreUpdate event.
    # @yield This event is triggered every frame before the newton update
    #   occurs.
    def onPreUpdate(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onUpdate event.
    # @yield This event is triggered every frame after the {#onUpdate} event is
    #   called.
    def onPostUpdate(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onEnd event.
    # @yield This event is triggered once when simulation ends; right before the
    #   bodies are moved back to their starting transformation.
    def onEnd(&block)
      assign_proc(__method__, block)
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
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onPlay event.
    # @yield This event is triggered when simulation is played. It is not called
    #   when simulation starts.
    def onPlay(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onPause event.
    # @yield This event is triggered when simulation is paused.
    def onPause(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onTouch event.
    # @yield This event is triggered when the body is touched by another body.
    # @yieldparam [Body] toucher
    # @yieldparam [Geom::Point3d] position
    # @yieldparam [Geom::Vector3d] normal
    # @yieldparam [Geom::Vector3d] force in Newtons.
    # @yieldparam [Numeric] speed in meters per second.
    def onTouch(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onTouching event.
    # @yield This event is triggered every frame when the body is in an extended
    #   contact with another body.
    # @yieldparam [Body] toucher
    # @yieldparam [Geom::Point3d] position
    # @yieldparam [Geom::Vector3d] normal
    # @yieldparam [Geom::Vector3d] force in Newtons.
    # @yieldparam [Numeric] speed in meters per second.
    def onTouching(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onUntouch event.
    # @yield This event is triggered when the body is no longer in contact with
    #   another body. When this procedure is triggered it doesn't always mean
    #   the body is free from all contacts. It means that a particular +toucher+
    #   has stopped touching the body. In many cases, this procedure is
    #   triggered when the body changes to sleep, freeze, or static mode, as
    #   well. Being dependent on this event is not the best option. To determine
    #   whether the body is in contact with any other body, use
    #   {#get_bodies_in_contact} method instead.
    # @yieldparam [Body] toucher
    def onUntouch(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onClick event.
    # @yield This event is triggered when the body is clicked.
    # @yieldparam [Geom::Point3d] pos Clicked position in global space.
    def onClick(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onDrag event.
    # @yield This event is triggered whenever the body is dragged by a mouse.
    def onDrag(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onUnclick event.
    # @yield This event is triggered when the body is unclicked.
    def onUnclick(&block)
      assign_proc(__method__, block)
    end

    # @!endgroup
    # @!group User Input Events

    # Assign a block of code to the onKeyDown event.
    # @yield This event is called when the key is pressed.
    # @yieldparam [String] key Virtual key name.
    # @yieldparam [Fixnum] val Virtual key constant value.
    # @yieldparam [String] char Actual key character.
    def onKeyDown(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onKeyUp event.
    # @yield This event is called when the key is released.
    # @yieldparam [String] key Virtual key name.
    # @yieldparam [Fixnum] val Virtual key constant value.
    # @yieldparam [String] char Actual key character.
    def onKeyUp(&block)
      assign_proc(__method__, block)
    end
    # Assign a block of code to the onKeyExtended event.
    # @yield This event is called when the key is held down.
    # @yieldparam [String] key Virtual key name.
    # @yieldparam [Fixnum] val Virtual key constant value.
    # @yieldparam [String] char Actual key character.
    def onKeyExtended(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onMouseMove event.
    # @yield This event is called when the mouse is moved.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMouseMove(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onLButtonDown event.
    # @yield This event is called when the left mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onLButtonDown(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onLButtonUp event.
    # @yield This event is called when the left mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onLButtonUp(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onLButtonDoubleClick event.
    # @yield This event is called when the left mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onLButtonDoubleClick(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onRButtonDown event.
    # @yield This event is called when the right mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onRButtonDown(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onRButtonUp event.
    # @yield This event is called when the right mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onRButtonUp(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onRButtonDoubleClick event.
    # @yield This event is called when the right mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onRButtonDoubleClick(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onMButtonDown event.
    # @yield This event is called when the middle mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMButtonDown(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onMButtonUp event.
    # @yield This event is called when the middle mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMButtonUp(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onMButtonDoubleClick event.
    # @yield This event is called when the middle mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMButtonDoubleClick(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onXButtonDown event.
    # @yield This event is called when the X mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButtonDown(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onXButtonUp event.
    # @yield This event is called when the X mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButtonUp(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onXButtonDoubleClick event.
    # @yield This event is called when the X mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButtonDoubleClick(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onMouseWheelRotate event.
    # @yield This event is called when the mouse wheel is rotated.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    # @yieldparam [Fixnum] dir Rotate direction: +-1+ - down, +1+ - up.
    def onMouseWheelRotate(&block)
      assign_proc(__method__, block)
    end

    # Assign a block of code to the onMouseWheelTilt event.
    # @yield This event is called when the mouse wheel is tilted.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    # @yieldparam [Fixnum] dir Tilt direction: +-1+ - left, +1+ - right.
    def onMouseWheelTilt(&block)
      assign_proc(__method__, block)
    end

    # @!endgroup
  end # class BodyContext
end # module MSPhysics
