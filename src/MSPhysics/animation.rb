module MSPhysics
  class Animation

    def initialize
      @record = {}
      @fetched_record = {}
      @speed = 1
      @frame = 0
      @position = 0
      @active = false
      @paused = false
      @resumed = true
      @entered = true
      @min_frame = nil
      @max_frame = nil
    end

    # @return [Fixnum]
    def min_frame
      @min_frame.to_i
    end

    # @return [Fixnum]
    def max_frame
      @max_frame.to_i
    end

    # Add entities transformation to record.
    # @note You cannot modify record while the tool is active.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @param [Fixnum] frame
    # @return [Boolean] true if successful.
    def push_record(entity, frame)
      frame = frame.to_i.abs
      return false if @active
      unless [Sketchup::Group, Sketchup::ComponentInstance].include?(entity.class)
        raise ArgumentError, "Expected group or a component, but got #{entity.class}."
      end
      id = entity.entityID
      @record[id] ||= []
      @record[id][frame] = entity.transformation
      @min_frame = frame if @min_frame.nil? or frame < @min_frame
      @max_frame = frame if @max_frame.nil? or frame > @max_frame
      true
    end

    # Clear record list.
    # @note You cannot modify record while the tool is active.
    # @return [Boolean] true if successful.
    def clear_record
      return false if @active
      @record.clear
      @record_size = 0
      true
    end

    # Determine whether the tool is active.
    def active?
      @active
    end

    # Play animation.
    def play
      return if @active and !@paused
      if @active
        @paused = false
      else
        Sketchup.active_model.select_tool(self)
      end
    end

    # Determine whether animation is playing.
    # @return [Boolean]
    def playing?
      return false unless @active
      !@paused
    end

    # Pause animation.
    def pause
      return unless @active
      @paused = true
    end

    # Determine whether animation is paused.
    # @return [Boolean]
    def paused?
      return false unless @active
      @paused
    end

    # Toggle play animation.
    def toggle_play
      playing? ? pause : play
    end

    # Reset animation.
    def reset
      Sketchup.active_model.select_tool(nil)
    end

    # Get replay speed factor.
    # @return [Numeric]
    def speed
      @speed
    end

    # Set replay speed factor and direction via sign.
    # @param [Numeric] value A value between -20 and 20.
    def speed=(value)
      @speed = MSPhysics.clamp(value, -20, 20)
    end

    # Get replay frame.
    # @return [Fixnum]
    def frame
      @frame.to_i
    end

    # Set replay frame.
    # @param [Fixnum] value
    def frame=(value)
      @frame = MSPhysics.clamp(value.to_i.abs, @min_frame, @max_frame)
      @position = @frame
    end


    # @!visibility private


    def activate
      model = Sketchup.active_model
      args = ['MSPhysics Animation']
      args << true if Sketchup.version.to_i > 6
      model.start_operation(*args)
      state = true
      while state
        state = model.close_active
      end
      @active = true
      @fetched_record.clear
      @record.keys.each { |id|
        e = MSPhysics.get_entity_by_id(id)
        next unless e
        @fetched_record[e] = @record[id].dup
      }
      model.active_view.animation = self
      show_dialog
    end

    def deactivate(view)
      view.animation = nil
      Sketchup.active_model.abort_operation
      @frame = 0
      @position = 0
      @active = false
      @paused = false
      @resumed = true
      @entered = true
      @fetched_record.clear
      close_dialog
    end

    def nextFrame(view)
      if @paused
        view.show_frame
        return true
      end
      @position += @speed
      self.frame = @position
      @fetched_record.keys.each { |ent|
        @fetched_record.delete(ent) if ent.deleted?
        tra = @fetched_record[ent][@frame]
        next unless tra
        ent.transformation = tra if tra
      }
      if @resumed and @entered
        Sketchup.status_text = "Frame : #{@frame}    Speed : #{@speed}"
      end
      view.show_frame
      true
    end

    def getExtents
      model = Sketchup.active_model
      bb = model.bounds
      if Sketchup.version.to_i > 6
        model.entities.each { |ent|
          bb.add(ent.bounds)
        }
      end
      bb
    end

    def resume(view)
      @resumed = true
    end

    def suspend(view)
      @resumed = false
    end

    def onMouseEnter(view)
      @entered = true
    end

    def onMouseLeave(view)
      @entered = false
    end

    private

    def show_dialog
    end

    def close_dialog
    end

  end # class Animation
end # module MSPhysics
