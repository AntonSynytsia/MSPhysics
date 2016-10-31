module MSPhysics

  # @since 1.0.0
  class JointConnectionTool

    # @!visibility private
    @@instance = nil

    class << self

      # Activate joint connection tool.
      # @return [Boolean] success
      def activate
        return false if @@instance
        Sketchup.active_model.select_tool(self.new)
        true
      end

      # Deactivate joint connection tool.
      # @return [Boolean] success
      def deactivate
        return false unless @@instance
        Sketchup.active_model.select_tool(nil)
        true
      end

      # Determine whether joint connection tool is active.
      # @return [Boolean]
      def active?
        @@instance ? true : false
      end

      # Get all connected and potentially connected joints.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Boolean] consider_world Whether to consider if entities have a
      #   body context.
      # @return [Array] An array of two elements. The first element contains an
      #   array of connected joints and their data. The second element contains
      #   an array of potentially connected joints and their data. Each joint
      #   data represents an array containing joint group, joint parent group,
      #   and joint transformation in global space.
      def get_connected_joints(body, consider_world = false)
        data = [[], []]
        sim_inst = MSPhysics::Simulation.instance
        ids = body.get_attribute('MSPhysics Body', 'Connected Joints')
        return data if (!ids.is_a?(Array) ||
          body.get_attribute('MSPhysics Body', 'Ignore', false) ||
          (consider_world && (sim_inst.nil? || sim_inst.find_body_by_group(body).nil?)))
        ids = ids.grep(Fixnum)
        if body.get_attribute('MSPhysics Body', 'Connect Closest Joints', MSPhysics::DEFAULT_BODY_SETTINGS[:connect_closest_joinst])
          bbsc = {}
          Sketchup.active_model.entities.each { |ent|
            next if ((!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) ||
              ent.get_attribute('MSPhysics', 'Type', 'Body') != 'Body')
            bbsc[ent] = AMS::Group.get_bounding_box_from_faces(ent, true, ent.transformation) { |e|
              e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
            }.center
          }
          body_center = bbsc[body]
          Sketchup.active_model.entities.each { |ent|
            next if (!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) || ent == body
            type = ent.get_attribute('MSPhysics', 'Type', 'Body')
            if type == 'Body'
              next if (ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
                (consider_world && sim_inst.find_body_by_group(ent).nil?))
              ptra = ent.transformation
              cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
              cents.each { |cent|
                next if ((!cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)) ||
                  cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint')
                id = cent.get_attribute('MSPhysics Joint', 'ID')
                next unless ids.include?(id)
                jtra = ptra * AMS::Geometry.extract_matrix_scale(cent.transformation)
                dist = jtra.origin.distance(body_center)
                dist = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist).to_f : dist.round(3)
                jconnected = true
                Sketchup.active_model.entities.each { |ent2|
                  next if ((!ent2.is_a?(Sketchup::Group) && !ent2.is_a?(Sketchup::ComponentInstance)) ||
                    ent2.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' ||
                    ent2.get_attribute('MSPhysics Body', 'Ignore', false) ||
                    ent2 == ent ||
                    ent2 == body ||
                    (consider_world && sim_inst.find_body_by_group(ent2).nil?))
                  ids2 = ent2.get_attribute('MSPhysics Body', 'Connected Joints')
                  if ids2.is_a?(Array) && ids2.include?(id)
                    dist2 = jtra.origin.distance(bbsc[ent2])
                    dist2 = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist2).to_f : dist2.round(3)
                    if dist2 < dist
                      jconnected = false
                      break
                    end
                  end
                }
                data[jconnected ? 0 : 1] << [cent, ent, jtra]
              }
            elsif type == 'Joint'
              id = ent.get_attribute('MSPhysics Joint', 'ID')
              next unless ids.include?(id)
              jtra = AMS::Geometry.extract_matrix_scale(ent.transformation)
              dist = jtra.origin.distance(body_center)
              dist = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist).to_f : dist.round(3)
              jconnected = true
              Sketchup.active_model.entities.each { |ent2|
                next if ((!ent2.is_a?(Sketchup::Group) && !ent2.is_a?(Sketchup::ComponentInstance)) ||
                  ent2.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' ||
                  ent2.get_attribute('MSPhysics Body', 'Ignore', false) ||
                  ent2 == body ||
                  (consider_world && sim_inst.find_body_by_group(ent2).nil?))
                ids2 = ent2.get_attribute('MSPhysics Body', 'Connected Joints')
                if ids2.is_a?(Array) && ids2.include?(id)
                  dist2 = jtra.origin.distance(bbsc[ent2])
                  dist2 = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist2).to_f : dist2.round(3)
                  if dist2 < dist
                    jconnected = false
                    break
                  end
                end
              }
              data[jconnected ? 0 : 1] << [ent, nil, jtra]
            end
          }
        else
          Sketchup.active_model.entities.each { |ent|
            next if (!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) || ent == body
            type = ent.get_attribute('MSPhysics', 'Type', 'Body')
            if type == 'Body'
              next if (ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
                (consider_world && sim_inst.find_body_by_group(ent).nil?))
              ptra = ent.transformation
              cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
              cents.each { |cent|
                next if ((!cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)) ||
                  cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint' ||
                  !ids.include?(cent.get_attribute('MSPhysics Joint', 'ID')))
                jtra = ptra * AMS::Geometry.extract_matrix_scale(cent.transformation)
                data[0] << [cent, ent, jtra]
              }
            elsif type == 'Joint'
              next unless ids.include?(ent.get_attribute('MSPhysics Joint', 'ID'))
              jtra = AMS::Geometry.extract_matrix_scale(ent.transformation)
              data[0] << [ent, nil, jtra]
            end
          }
        end
        data
      end

      # Get all connected and potentially connected bodies.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] jparent
      # @param [Boolean] consider_world Whether to consider if entities have a
      #   body context.
      # @return [Array] An array of two elements. The first element contains an
      #   array of connected bodies. The second element contains an array of
      #   potentially connected bodies.
      def get_connected_bodies(joint, jparent, consider_world = false)
        data = [[], []]
        sim_inst = MSPhysics::Simulation.instance
        return data if consider_world && sim_inst.nil?
        id = joint.get_attribute('MSPhysics Joint', 'ID', nil)
        return data unless id.is_a?(Fixnum)
        return data if (jparent &&
          (jparent.get_attribute('MSPhysics Body', 'Ignore', false) ||
          (consider_world && sim_inst.find_body_by_group(jparent).nil?)))
        bodies = {}
        jorigin = joint.transformation.origin
        jorigin.transform!(jparent.transformation) if jparent
        # Get all connected bodies.
        Sketchup.active_model.entities.each { |ent|
          next if ((!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) ||
            ent.get_attribute('MSPhysics', 'Type', 'Body') != 'Body' ||
            (jparent && ent == jparent) ||
            ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
            (consider_world && sim_inst.find_body_by_group(ent).nil?))
          ids = ent.get_attribute('MSPhysics Body', 'Connected Joints')
          if ids.is_a?(Array) && ids.include?(id)
            bb = AMS::Group.get_bounding_box_from_faces(ent, true, ent.transformation) { |e|
              e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
            }
            dist = jorigin.distance(bb.center)
            bodies[ent] = RUBY_VERSION =~ /1.8/ ? sprintf("%.3f", dist).to_f : dist.round(3)
          end
        }
        # Filter out closest bodies.
        bodies.each { |ent, dist|
          if ent.get_attribute('MSPhysics Body', 'Connect Closest Joints', MSPhysics::DEFAULT_BODY_SETTINGS[:connect_closest_joinst])
            found_closer = false
            bodies.each { |ent2, dist2|
              if ent2 != ent && dist2 < dist
                found_closer = true
                break
              end
            }
            data[found_closer ? 1 : 0] << ent
          else
            data[0] << ent
          end
        }
        data
      end

      # Get all joints and their connected bodies.
      # @return [Hash] <tt>{ jtra => [joint_group, joint_parent_body, joint_id, {connected_body => distance}] }</tt>
      def map_joints_with_connected_bodies
        fjdata = {}
        sim_inst = MSPhysics::Simulation.instance
        return fjdata if sim_inst.nil?
        # Gather all bodies and their data.
        fgdata = {} # { group => [body, center, connected_ids, connect_closest] }
        jid_to_tras = {} # { id => [jtra] }
        Sketchup.active_model.entities.each { |ent|
          if ((ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)) &&
            ent.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' &&
            !ent.get_attribute('MSPhysics Body', 'Ignore', false))
            body = sim_inst.find_body_by_group(ent)
            if body
              bb = AMS::Group.get_bounding_box_from_faces(ent, true, ent.transformation) { |e|
                e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
              }
              connected_ids = ent.get_attribute('MSPhysics Body', 'Connected Joints')
              connected_ids = connected_ids.is_a?(Array) ? connected_ids.grep(Fixnum).uniq : []
              connect_closest = ent.get_attribute('MSPhysics Body', 'Connect Closest Joints', MSPhysics::DEFAULT_BODY_SETTINGS[:connect_closest_joinst])
              fgdata[ent] = [body, bb.center, connected_ids, connect_closest]
            end
          end
        }
        # Gather all joints and their data.
        Sketchup.active_model.entities.each { |ent|
          next if !ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)
          if ent.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && fgdata.has_key?(ent)
            ptra = ent.transformation
            cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
            cents.each { |cent|
              if ((cent.is_a?(Sketchup::Group) || cent.is_a?(Sketchup::ComponentInstance)) &&
                cent.get_attribute('MSPhysics', 'Type', 'Body') == 'Joint')
                id = cent.get_attribute('MSPhysics Joint', 'ID', nil)
                if id.is_a?(Fixnum)
                  jtra = ptra * AMS::Geometry.extract_matrix_scale(cent.transformation)
                  fjdata[jtra] = [cent, fgdata[ent][0], id, {}]
                  if jid_to_tras.has_key?(id)
                    jid_to_tras[id] << jtra
                  else
                    jid_to_tras[id] = [jtra]
                  end
                end
              end
            }
          elsif ent.get_attribute('MSPhysics', 'Type', 'Body') == 'Joint'
            id = ent.get_attribute('MSPhysics Joint', 'ID', nil)
            if id.is_a?(Fixnum)
              jtra = AMS::Geometry.extract_matrix_scale(ent.transformation)
              fjdata[jtra] = [ent, nil, id, {}]
              if jid_to_tras.has_key?(id)
                jid_to_tras[id] << jtra
              else
                jid_to_tras[id] = [jtra]
              end
            end
          end
        }
        # Fill in joint data with connected bodies.
        fgdata.each { |gent, ginfo|
          ginfo[2].each { |id|
            jtras = jid_to_tras[id]
            next unless jtras
            jtras.each { |jtra|
              jdata = fjdata[jtra]
              next if jdata[1] == ginfo[0] # Skip if joint parent body is the current group body.
              dist = ginfo[1].distance(jtra.origin).to_f
              if ginfo[3] # if connect closest
                min = nil
                jdata[3].each { |cbody, cdist|
                  min = cdist if !min || cdist < min
                }
                if min.nil? || dist - min < 0.01
                  jdata[3].clear
                  jdata[3][ginfo[0]] = dist
                end
              else
                jdata[3][ginfo[0]] = dist
              end
            }
          }
        }
=begin
        # Now filter closest joints.
        fjdata.each { |jtra, jinfo|
          jinfo[3].reject! { |body, dist| # Iterate through all connected bodies and their distances
            bremove = false
            if fgdata[body.group][3] # if connect closest enabled
              fjdata.each { |sjtra, sjinfo| # Once again iterate through all joints
                next if sjtra == jtra
                if sjinfo[2] == jinfo[2]
                  sjinfo[3].each { |sbody, sdist|
                    if sdist - dist < -0.01
                      bremove = true
                      break
                    end
                  }
                  break if bremove
                end
              }
            end
            bremove
          }
        }
=end
        # Return data
        fjdata
      end

      # Get all gears connected to a joint.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] jparent
      # @param [Boolean] include_cgears Whether to include consequent gears. For
      #   example, if joint A is geared with joint B, then if include_cgears is
      #   true, joint B will be displayed as geared with joint A; otherwise, if
      #   include_cgears is false, joint B won't be displayed as geared with
      #   joint A.
      # @param [Boolean] consider_world Whether to consider if entities have a
      #   body context.
      # @return [Array] An array of joint data. Each joint data represents an
      #   array containing joint group, joint parent group, joint
      #   transformation in global space, a type of gear (0,1,2), and gear
      #   ratio (numeric, nil).
      def get_geared_joints(joint, jparent, include_cgears, consider_world = false)
        data = []
        sim_inst = MSPhysics::Simulation.instance
        return data if consider_world && sim_inst.nil?
        id = joint.get_attribute('MSPhysics Joint', 'ID', nil)
        return data unless id.is_a?(Fixnum)
        return data if (jparent &&
          (jparent.get_attribute('MSPhysics Body', 'Ignore', false) ||
          (consider_world && sim_inst.find_body_by_group(jparent).nil?)))
        get_geared_data(joint).each { |gear_id, gear_type, gear_ratio|
          next if gear_id == id
          get_joints_by_id(gear_id).each { |je, jp|
            next if jp && consider_world && sim_inst.find_body_by_group(jp).nil?
            jtra = jp ? jp.transformation * AMS::Geometry.extract_matrix_scale(je.transformation) : je.transformation
            data << [je, jp, jtra, gear_id, gear_type, gear_ratio]
          }
        }
        if include_cgears
          Sketchup.active_model.entities.each { |ent|
            next if (!ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)) || ent == joint
            type = ent.get_attribute('MSPhysics', 'Type', 'Body')
            if type == 'Body'
              next if (ent.get_attribute('MSPhysics Body', 'Ignore', false) ||
                (consider_world && sim_inst.find_body_by_group(ent).nil?))
              ptra = ent.transformation
              cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
              cents.each { |cent|
                next if ((!cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)) ||
                  cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint')
                cjid = cent.get_attribute('MSPhysics Joint', 'ID')
                next unless cjid.is_a?(Fixnum)
                get_geared_data(cent).each { |gear_id, gear_type, gear_ratio|
                  if gear_id == id
                    jtra = ptra * AMS::Geometry.extract_matrix_scale(cent.transformation)
                    data << [cent, ent, jtra, cjid, gear_type, gear_ratio]
                  end
                }
              }
            elsif type == 'Joint'
              cjid = ent.get_attribute('MSPhysics Joint', 'ID')
              next unless cjid.is_a?(Fixnum)
              get_geared_data(ent).each { |gear_id, gear_type, gear_ratio|
                if gear_id == id
                  jtra = AMS::Geometry.extract_matrix_scale(ent.transformation)
                  data << [ent, nil, jtra, cjid, gear_type, gear_ratio]
                end
              }
            end
          }
        end
        data
      end

      # Get joint by its id.
      # @param [Fixnum] joint_id
      # @return [Array] An array of joint data. Each joint data represents an
      #   array of two elements. The first element of joint data is joint
      #   entity. The second element of joint data is joint parent entity.
      def get_joints_by_id(joint_id)
        data = []
        Sketchup.active_model.entities.each { |ent|
          next if !ent.is_a?(Sketchup::Group) && !ent.is_a?(Sketchup::ComponentInstance)
          type = ent.get_attribute('MSPhysics', 'Type', 'Body')
          if type == 'Body'
            next if ent.get_attribute('MSPhysics Body', 'Ignore', false)
            cents = ent.is_a?(Sketchup::ComponentInstance) ? ent.definition.entities : ent.entities
            cents.each { |cent|
              next if !cent.is_a?(Sketchup::Group) && !cent.is_a?(Sketchup::ComponentInstance)
              next if cent.get_attribute('MSPhysics', 'Type', 'Body') != 'Joint'
              id = cent.get_attribute('MSPhysics Joint', 'ID')
              data << [cent, ent] if id == joint_id
            }
          elsif type == 'Joint'
            id = ent.get_attribute('MSPhysics Joint', 'ID')
            data << [ent, nil] if id == joint_id
          end
        }
        data
      end

      # Get all joint ids connected to a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @return [Array<Fixnum>]
      def get_connected_joint_ids(body)
        ids = body.get_attribute('MSPhysics Body', 'Connected Joints')
        ids.is_a?(Array) ? ids.grep(Fixnum).uniq : []
      end

      # Set connected joint ids of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Array<Fixnum>] ids
      def set_connected_joint_ids(body, ids)
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
      end

      # Connect joint id to a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Fixnum] id
      # @return [Array<Fixnum>] The new joint ids of a group/component.
      def connect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids << id unless ids.include?(id)
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

      # Connect joint id from a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Fixnum] id
      # @return [Array<Fixnum>] The new joint ids of a group/component.
      def disconnect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids.delete(id)
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

      # Toggle connect joint id to a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] body
      # @param [Fixnum] id
      # @return [Array<Fixnum>] The new joint ids of a group/component.
      def toggle_connect_joint_id(body, id)
        ids = get_connected_joint_ids(body)
        ids.include?(id) ? ids.delete(id) : ids << id
        body.set_attribute('MSPhysics Body', 'Connected Joints', ids)
        ids
      end

      # Get joint geared data.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @return [Array] An array of geared data. Each data is an array
      #   containing joint ID (fixnum), gear type (0,1,2), and gear ratio
      #   (numeric, nil).
      def get_geared_data(joint)
        parced_data = []
        data = joint.get_attribute('MSPhysics Joint', 'Gears')
        return parced_data unless data.is_a?(Array)
        parced_hash = {}
        data.each { |info|
          if info.is_a?(Array) && info[0].is_a?(Fixnum) && info[1].is_a?(Fixnum) && (info[2].is_a?(Numeric) || info[2].nil?)
            parced_hash[[info[0], info[1]]] = info[2]
          end
        }
        parced_hash.each { |k,v|
          parced_data << [k[0], k[1], v.is_a?(String) ? v.to_f : v]
        }
        parced_hash.clear
        parced_data
      end

      # Set joint geared data.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Array, nil] data An array of geared data. Each data must be an
      #   array containing joint ID (fixnum), gear type (0,1,2), and gear ratio
      #   (numeric, nil). Pass nil to delete data.
      def set_geared_data(joint, data)
        if data
          joint.set_attribute('MSPhysics Joint', 'Gears', data)
        else
          joint.delete_attribute('MSPhysics Joint', 'Gears')
        end
      end

      # Gear joint A to joint B.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointA
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointB
      # @param [Numeric, nil] gear_ratio
      # @return [Boolean] success
      def gear_joints(jointA, jointB, gear_type, gear_ratio = nil)
        gear_ratio = gear_ratio.to_i.is_a?(Bignum) ? gear_ratio.to_s : gear_ratio
        idA = jointA.get_attribute('MSPhysics Joint', 'ID')
        idB = jointB.get_attribute('MSPhysics Joint', 'ID')
        return false if !idA.is_a?(Fixnum) || !idB.is_a?(Fixnum)
        dataA = get_geared_data(jointA)
        found = false
        dataA.each { |info|
          if info[0] == idB && info[1] == gear_type
            info[2] = gear_ratio
            found = true
            break
          end
        }
        if found
          jointA.set_attribute('MSPhysics Joint', 'Gears', dataA)
          return true
        end
        dataB = get_geared_data(jointB)
        dataB.each { |info|
          if info[0] == idA && info[1] == gear_type
            info[2] = gear_ratio
            found = true
            break
          end
        }
        if found
          jointB.set_attribute('MSPhysics Joint', 'Gears', dataB)
          return true
        end
        dataA << [idB, gear_type, gear_ratio]
        jointA.set_attribute('MSPhysics Joint', 'Gears', dataA)
        true
      end

      # Ungear joint A from joint B.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointA
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointB
      # @param [Fixnum] gear_type
      # @return [Boolean] success
      def ungear_joints(jointA, jointB, gear_type)
        idA = jointA.get_attribute('MSPhysics Joint', 'ID')
        idB = jointB.get_attribute('MSPhysics Joint', 'ID')
        return false if !idA.is_a?(Fixnum) || !idB.is_a?(Fixnum)
        index = nil
        dataA = get_geared_data(jointA)
        for i in 0...dataA.size
          if dataA[i][0] == idB && dataA[i][1] == gear_type
            index = i
            break
          end
        end
        if index
          dataA.delete_at(index)
          jointA.set_attribute('MSPhysics Joint', 'Gears', dataA)
          return true
        end
        dataB = get_geared_data(jointB)
        for i in 0...dataB.size
          if dataB[i][0] == idA && dataB[i][1] == gear_type
            index = i
            break
          end
        end
        if index
          dataB.delete_at(index)
          jointB.set_attribute('MSPhysics Joint', 'Gears', dataB)
          return true
        end
        false
      end

      # Determine whether joints A and B are geared.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointA
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointB
      # @param [Fixnum] gear_type
      # @return [Boolean]
      def joints_geared?(jointA, jointB, gear_type)
        idA = jointA.get_attribute('MSPhysics Joint', 'ID')
        idB = jointB.get_attribute('MSPhysics Joint', 'ID')
        return false if !idA.is_a?(Fixnum) || !idB.is_a?(Fixnum)
        get_geared_data(jointA).each { |info|
          return true if info[0] == idB && info[1] == gear_type
        }
        get_geared_data(jointB).each { |info|
          return true if info[0] == idA && info[1] == gear_type
        }
        false
      end

      # Toggle gear joints A and B.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointA
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointB
      # @param [Fixnum] gear_type
      # @param [Numeric, nil] gear_ratio
      # @return [Boolean] success
      def toggle_gear_joints(jointA, jointB, gear_type, gear_ratio = nil)
        if joints_geared?(jointA, jointB, gear_type)
          ungear_joints(jointA, jointB, gear_type)
        else
          gear_joints(jointA, jointB, gear_type, gear_ratio)
        end
      end

      # Get gear ratio between joints A and B.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointA
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointB
      # @param [Fixnum] gear_type
      # @return [Numeric, nil]
      def get_gear_ratio(jointA, jointB, gear_type)
        idA = jointA.get_attribute('MSPhysics Joint', 'ID')
        idB = jointB.get_attribute('MSPhysics Joint', 'ID')
        return if !idA.is_a?(Fixnum) || !idB.is_a?(Fixnum)
        get_geared_data(jointA).each { |info|
          return info[2] if info[0] == idB && info[1] == gear_type
        }
        get_geared_data(jointB).each { |info|
          return info[2] if info[0] == idA && info[1] == gear_type
        }
        nil
      end

      # Set gear ratio between joints A and B.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointA
      # @param [Sketchup::Group, Sketchup::ComponentInstance] jointB
      # @param [Fixnum] gear_type
      # @param [Numeric, nil] gear_ratio
      # @return [Boolean] success
      def set_gear_ratio(jointA, jointB, gear_type, gear_ratio)
        gear_joints(jointA, jointB, gear_type, gear_ratio)
      end

      # Get all points on CurvySlider or CurvyPiston joints.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] jparent
      # @return [Array<Geom::Point3d>] An array of points in global space.
      def get_points_on_curve(joint, jparent)
        closest_dist = nil
        start_vertex = nil
        AMS::Group.get_entities(joint).each { |e|
          next unless e.is_a?(Sketchup::Edge)
          dist1 = e.start.position.distance(ORIGIN)
          dist2 = e.end.position.distance(ORIGIN)
          if dist1 < dist2
            vertex = e.start
            dist = dist1
          else
            vertex = e.end
            dist = dist2
          end
          if closest_dist.nil? || dist < closest_dist
            closest_dist = dist
            start_vertex = vertex
            break if closest_dist < 1.0e-8
          end
        }
        verts = []
        if start_vertex
          used_edges = []
          verts << start_vertex
          edge = start_vertex.edges[0]
          verts << edge.other_vertex(start_vertex)
          used_edges << edge
          while true
            edge = nil
            lastv = verts.last
            lastv.edges.each { |e|
              unless used_edges.include?(e)
                edge = e
                break
              end
            }
            break unless edge
            verts << edge.other_vertex(lastv)
            used_edges << edge
          end
        end
        tra = joint.transformation
        if jparent
          tra = jparent.transformation * tra
        end
        verts.map { |v| v.position.transform(tra) }
      end

      # Get curve length of a CurvySlider or CurvyPiston joint.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] joint
      # @return [Numeric] Length in inches.
      def get_curve_length(joint, jparent)
        length = 0.0
        last_pt = nil
        get_points_on_curve(joint, jparent).each { |pt|
          if last_pt
            length += last_pt.distance(pt)
          end
          last_pt = pt
        }
        length
      end

    end # class << self

    def initialize
      model = Sketchup.active_model
      # Close active path
      state = true
      while state
        state = model.close_active
      end
      model.selection.clear
      @control_down = false
      @shift_down = false
      @parent = nil
      @picked = nil
      @picked_type = nil
      @identical_picked_joints = []
      @connected = []
      @connected_gears = []
      @cursor_id = MSPhysics::CURSORS[:select]
      @color = {
        :picked     => Sketchup::Color.new(0, 0, 255),
        :identical  => Sketchup::Color.new(0, 0, 255),
        :connected  => Sketchup::Color.new(0, 225, 0),
        :potential  => Sketchup::Color.new(200, 40, 250),
        :gears      => Sketchup::Color.new(250, 0, 0),
        :curve      => Sketchup::Color.new(255, 255, 0)
      }
      @line_width = {
        :picked     => 2,
        :identical  => 2,
        :connected  => 2,
        :potential  => 2,
        :gears      => 2,
        :curve      => 2
      }
      @line_stipple = {
        :picked     => '',
        :identical  => '-',
        :connected  => '',
        :potential  => '_',
        :gears      => '',
        :curve      => ''
      }
      @pins = [0,1,1,3,3,2,2,0, 4,5,5,7,7,6,6,4, 0,4,1,5,2,6,3,7]
      @scale = 1.02
    end

    private

    def update_status_text
      msg = 'Click to select body or joint. Use CTRL, SHIFT, or both to connect/disconnect.   BLUE = Selected   GREEN = Connected   DASHED-MAGENTA = Potentially connected   DASHED-BLUE = Secondary selected joints with same ID   RED = Geared joints'
      Sketchup.set_status_text(msg, SB_PROMPT)
    end

    def refresh_viewport
      Sketchup.active_model.active_view.invalidate
      onSetCursor
      update_status_text
    end

    def update_state
      @control_down = AMS::Keyboard.control_down?
      @shift_down = AMS::Keyboard.shift_down?
      if @control_down && @shift_down
        @cursor_id = MSPhysics::CURSORS[:select_plus_minus]
      elsif @control_down
        @cursor_id = MSPhysics::CURSORS[:select_plus]
      elsif @shift_down
        @cursor_id = MSPhysics::CURSORS[:select_minus]
      else
        @cursor_id = MSPhysics::CURSORS[:select]
      end
      refresh_viewport
    end

    def attach_joint_proc(body, joint)
      op = 'MSPhysics - Connecting Joint'
      model = Sketchup.active_model
      Sketchup.version.to_i > 6 ? model.start_operation(op, true) : model.start_operation(op)
      id = joint.get_attribute('MSPhysics Joint', 'ID', nil)
      if !id.is_a?(Fixnum)
        id = JointTool.generate_uniq_id
        joint.set_attribute('MSPhysics Joint', 'ID', id)
      end
      case @cursor_id
        when MSPhysics::CURSORS[:select_plus]
          JointConnectionTool.connect_joint_id(body, id)
        when MSPhysics::CURSORS[:select_minus]
          JointConnectionTool.disconnect_joint_id(body, id)
        when MSPhysics::CURSORS[:select_plus_minus]
          JointConnectionTool.toggle_connect_joint_id(body, id)
      end
      model.commit_operation
    end

    def gear_joints_proc(jointA, jointB)
      typeA = jointA.get_attribute('MSPhysics Joint', 'Type')
      typeB = jointB.get_attribute('MSPhysics Joint', 'Type')
      if ((typeA == 'Slider' && typeB == 'Slider') ||
          (typeA == 'Slider' && typeB == 'Piston' || typeB == 'Slider' && typeA == 'Piston'))
        gear_type = 1
      elsif ((typeA == 'Hinge' && typeB == 'Hinge') ||
          (typeA == 'Hinge' && typeB == 'Motor' || typeB == 'Hinge' && typeA == 'Motor') ||
          (typeA == 'Hinge' && typeB == 'Servo' || typeB == 'Hinge' && typeA == 'Servo'))
        gear_type = 2
      elsif ((typeA == 'Slider' && typeB == 'Hinge' || typeB == 'Slider' && typeA == 'Hinge') ||
          (typeA == 'Slider' && typeB == 'Motor' || typeB == 'Slider' && typeA == 'Motor') ||
          (typeA == 'Slider' && typeB == 'Servo' || typeB == 'Slider' && typeA == 'Servo') ||
          (typeA == 'Piston' && typeB == 'Hinge' || typeB == 'Piston' && typeA == 'Hinge'))
        gear_type = 3
      else
        ::UI.messagebox("Creating gears between '#{typeA}' and '#{typeB}' is not allowed!")
        return false
      end
      op = 'MSPhysics - Gearing Joints'
      model = Sketchup.active_model
      Sketchup.version.to_i > 6 ? model.start_operation(op, true) : model.start_operation(op)
      idA = jointA.get_attribute('MSPhysics Joint', 'ID')
      idB = jointB.get_attribute('MSPhysics Joint', 'ID')
      if !idA.is_a?(Fixnum)
        idA = JointTool.generate_uniq_id
        jointA.set_attribute('MSPhysics Joint', 'ID', idA)
      end
      if !idB.is_a?(Fixnum)
        idB = JointTool.generate_uniq_id
        jointB.set_attribute('MSPhysics Joint', 'ID', idB)
      end
      if idA == idB
        ::UI.messagebox("Gearing joints with identical IDs is not allowed! If you want to gear both of these joints, you may use the context menu command to generate a unique ID for one of them.")
        model.commit_operation
        return false
      end
      case @cursor_id
        when MSPhysics::CURSORS[:select_plus]
          JointConnectionTool.gear_joints(jointA, jointB, gear_type)
        when MSPhysics::CURSORS[:select_minus]
          JointConnectionTool.ungear_joints(jointA, jointB, gear_type)
        when MSPhysics::CURSORS[:select_plus_minus]
          JointConnectionTool.toggle_gear_joints(jointA, jointB, gear_type)
      end
      model.commit_operation
      true
    end

    public
    # @!visibility private


    def activate
      @@instance = self
      update_status_text
    end

    def deactivate(view)
      view.invalidate
      @@instance = nil
    end

    def onMouseEnter(view)
      update_state
    end

    def resume(view)
      @control_down = false
      @shift_down = false
      update_state
    end

    def draw(view)
      return unless @picked
      if !@picked.valid? || (@parent && !@parent.valid?)
        @parent = nil
        @picked = nil
        @picked_type = nil
        @identical_picked_joints.clear
        @connected.clear
        @connected_gears.clear
        return
      end
      # Draw picked
      view.line_width = @line_width[:picked]
      view.drawing_color = @color[:picked]
      view.line_stipple = @line_stipple[:picked]
      edges = []
      bcurvy_joint = false
      curvy_obj = nil
      type = @picked.get_attribute('MSPhysics', 'Type')
      if type == 'Joint'
        jtype = @picked.get_attribute('MSPhysics Joint', 'Type')
        if jtype == 'CurvySlider' || jtype == 'CurvyPiston'
          bcurvy_joint = true
          AMS::Group.get_entities(@picked).each { |e|
            if e.is_a?(Sketchup::ComponentInstance) && (e.definition.name == 'curvy_slider' || e.definition.name == 'curvy_piston')
              curvy_obj = e
              break
            end
          }
        end
      end
      if curvy_obj
        definition = curvy_obj.respond_to?(:definition) ? curvy_obj.definition : curvy_obj.entities[0].parent
      else
        definition = @picked.respond_to?(:definition) ? @picked.definition : @picked.entities[0].parent
      end
      bb = definition.bounds
      center = bb.center
      if curvy_obj
        picked_tra = @picked.transformation * curvy_obj.transformation
      else
        picked_tra = @picked.transformation
      end
      @pins.each { |n|
        pt = bb.corner(n)
        v = center.vector_to(pt)
        for i in 0..2; v[i] *= @scale end
        pt = center + v
        pt.transform!(picked_tra)
        edges << pt
      }
      if @parent
        parent_tra = @parent.transformation
        edges.each { |pt| pt.transform!(parent_tra) }
      end
      view.draw(GL_LINES, edges)
      if bcurvy_joint
        view.line_width = @line_width[:curve]
        view.drawing_color = @color[:curve]
        view.line_stipple = @line_stipple[:curve]
        curve_pts = JointConnectionTool.get_points_on_curve(@picked, @parent)
        view.draw(GL_LINE_STRIP, curve_pts) if curve_pts.size > 1
      end
      # Draw identical picked joints
      view.line_width = @line_width[:identical]
      view.drawing_color = @color[:identical]
      view.line_stipple = @line_stipple[:identical]
      @identical_picked_joints.each { |joint, jparent|
        next if joint.deleted? || (jparent && jparent.deleted?) || (joint == @picked && jparent == @parent)
        edges = []
        definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
        bb = definition.bounds
        center = bb.center
        joint_tra = joint.transformation
        @pins.each { |n|
          pt = bb.corner(n)
          v = center.vector_to(pt)
          for i in 0..2; v[i] *= @scale end
          pt = center + v
          pt.transform!(joint_tra)
          edges << pt
        }
        if jparent
          jparent_tra = jparent.transformation
          edges.each { |pt| pt.transform!(jparent_tra) }
        end
        view.draw(GL_LINES, edges)
      }
      # Draw connected and potentially connected
      if @picked_type == 'Body'
        # Draw connected joints
        view.line_width = @line_width[:connected]
        view.drawing_color = @color[:connected]
        view.line_stipple = @line_stipple[:connected]
        @connected[0].each { |joint, jparent, jtra|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          bcurvy_joint = false
          curvy_obj = nil
          type = joint.get_attribute('MSPhysics', 'Type')
          if type == 'Joint'
            jtype = joint.get_attribute('MSPhysics Joint', 'Type')
            if jtype == 'CurvySlider' || jtype == 'CurvyPiston'
              bcurvy_joint = true
              AMS::Group.get_entities(joint).each { |e|
                if e.is_a?(Sketchup::ComponentInstance) && (e.definition.name == 'curvy_slider' || e.definition.name == 'curvy_piston')
                  curvy_obj = e
                  break
                end
              }
            end
          end
          if curvy_obj
            definition = curvy_obj.respond_to?(:definition) ? curvy_obj.definition : curvy_obj.entities[0].parent
          else
            definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          end
          bb = definition.bounds
          center = bb.center
          if curvy_obj
            joint_tra = joint.transformation * curvy_obj.transformation
          else
            joint_tra = joint.transformation
          end
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(joint_tra)
            edges << pt
          }
          if jparent
            jparent_tra = jparent.transformation
            edges.each { |pt| pt.transform!(jparent_tra) }
          end
          view.draw(GL_LINES, edges)
          if bcurvy_joint
            view.line_width = @line_width[:curve]
            view.drawing_color = @color[:curve]
            view.line_stipple = @line_stipple[:curve]
            curve_pts = JointConnectionTool.get_points_on_curve(joint, jparent)
            view.draw(GL_LINE_STRIP, curve_pts) if curve_pts.size > 1
          end
        }
        # Draw potentially connected joints
        view.line_width = @line_width[:potential]
        view.drawing_color = @color[:potential]
        view.line_stipple = @line_stipple[:potential]
        @connected[1].each { |joint, jparent, jtra|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          bcurvy_joint = false
          curvy_obj = nil
          type = joint.get_attribute('MSPhysics', 'Type')
          if type == 'Joint'
            jtype = joint.get_attribute('MSPhysics Joint', 'Type')
            if jtype == 'CurvySlider' || jtype == 'CurvyPiston'
              bcurvy_joint = true
              AMS::Group.get_entities(joint).each { |e|
                if e.is_a?(Sketchup::ComponentInstance) && (e.definition.name == 'curvy_slider' || e.definition.name == 'curvy_piston')
                  curvy_obj = e
                  break
                end
              }
            end
          end
          if curvy_obj
            definition = curvy_obj.respond_to?(:definition) ? curvy_obj.definition : curvy_obj.entities[0].parent
          else
            definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          end
          bb = definition.bounds
          center = bb.center
          if curvy_obj
            joint_tra = joint.transformation * curvy_obj.transformation
          else
            joint_tra = joint.transformation
          end
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(joint_tra)
            edges << pt
          }
          if jparent
            jparent_tra = jparent.transformation
            edges.each { |pt| pt.transform!(jparent_tra) }
          end
          view.draw(GL_LINES, edges)
          if bcurvy_joint
            view.line_width = @line_width[:curve]
            view.drawing_color = @color[:curve]
            view.line_stipple = @line_stipple[:curve]
            curve_pts = JointConnectionTool.get_points_on_curve(joint, jparent)
            view.draw(GL_LINE_STRIP, curve_pts) if curve_pts.size > 1
          end
        }
      elsif @picked_type == 'Joint'
        # Draw connected bodies
        view.line_width = @line_width[:connected]
        view.drawing_color = @color[:connected]
        view.line_stipple = @line_stipple[:connected]
        @connected[0].each { |body|
          next if body.deleted?
          edges = []
          definition = body.respond_to?(:definition) ? body.definition : body.entities[0].parent
          bb = definition.bounds
          center = bb.center
          body_tra = body.transformation
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(body_tra)
            edges << pt
          }
          view.draw(GL_LINES, edges)
        }
        # Draw potentially connected bodies
        view.line_width = @line_width[:potential]
        view.drawing_color = @color[:potential]
        view.line_stipple = @line_stipple[:potential]
        @connected[1].each { |body|
          next if body.deleted?
          edges = []
          definition = body.respond_to?(:definition) ? body.definition : body.entities[0].parent
          bb = definition.bounds
          center = bb.center
          body_tra = body.transformation
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(body_tra)
            edges << pt
          }
          view.draw(GL_LINES, edges)
        }
        # Draw connected gears
        view.line_width = @line_width[:gears]
        view.drawing_color = @color[:gears]
        view.line_stipple = @line_stipple[:gears]
        @connected_gears.each { |joint, jparent, jtra, gear_id, gear_type, gear_ratio|
          next if joint.deleted? || (jparent && jparent.deleted?)
          edges = []
          definition = joint.respond_to?(:definition) ? joint.definition : joint.entities[0].parent
          bb = definition.bounds
          center = bb.center
          @pins.each { |n|
            pt = bb.corner(n)
            v = center.vector_to(pt)
            for i in 0..2; v[i] *= @scale end
            pt = center + v
            pt.transform!(jtra)
            edges << pt
          }
          view.draw(GL_LINES, edges)
        }
      end
    end

    def onSetCursor
      ::UI.set_cursor(@cursor_id)
    end

    def onKeyDown(key, rpt, flags, view)
      return if rpt != 1
      if key == COPY_MODIFIER_KEY
        @control_down = true
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_minus] : MSPhysics::CURSORS[:select_plus]
        refresh_viewport
      elsif key == CONSTRAIN_MODIFIER_KEY
        @shift_down = true
        @cursor_id = @control_down ? MSPhysics::CURSORS[:select_minus] : MSPhysics::CURSORS[:select_plus_minus]
        refresh_viewport
      end
    end

    def onKeyUp(key, rpt, flags, view)
      if key == COPY_MODIFIER_KEY
        @control_down = false
        @cursor_id = @shift_down ? MSPhysics::CURSORS[:select_plus_minus] : MSPhysics::CURSORS[:select]
        refresh_viewport
      elsif key == CONSTRAIN_MODIFIER_KEY
        @shift_down = false
        @cursor_id = @control_down ? MSPhysics::CURSORS[:select_plus] : MSPhysics::CURSORS[:select]
        refresh_viewport
      end
    end

    def onLButtonDown(flags, x, y, view)
      model = Sketchup.active_model
      ray = view.pickray(x,y)
      res = nil#model.raytest(ray)
      if res
        path = res[1]
      else
        ph = view.pick_helper
        ph.do_pick(x,y)
        path = ph.path_at(0)
      end
      if @control_down || @shift_down
        return if path.nil? || @picked.nil?
        if !@picked.valid? || (@parent && !@parent.valid?)
          @parent = nil
          @picked = nil
          @picked_type = nil
          @identical_picked_joints.clear
          @connected.clear
          @connected_gears.clear
          return
        end
        case @picked_type
        when 'Body'
          if (path[0] == @picked ||
              #(path[0].respond_to?(:definition) && @picked.respond_to?(:definition) && path[0].definition == @picked.definition) ||
              (MSPhysics.get_entity_type(path[0]) == 'Body' && path[0].get_attribute('MSPhysics Body', 'Ignore', false)))
            ::UI.beep
            return
          end
          to_connect = nil
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body'
            to_connect = path[1] if MSPhysics.get_entity_type(path[1]) == 'Joint'
          elsif type == 'Joint'
            to_connect = path[0]
          end
          if to_connect
            attach_joint_proc(@picked, to_connect)
            @connected = JointConnectionTool.get_connected_joints(@picked)
          else
            ::UI.beep
          end
        when 'Joint'
          #if (path[0] == @picked ||
          #    path[0] == @parent ||
          #    #(@picked.parent.is_a?(Sketchup::ComponentDefinition) && @picked.parent.instances.include?(path[0])) ||
          #    MSPhysics.get_entity_type(path[0]) != 'Body' ||
          #    path[0].get_attribute('MSPhysics Body', 'Ignore', false))
          #  ::UI.beep
          #  return
          #end
          #attach_joint_proc(path[0], @picked)
          #@connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
          if path[0] == @picked || (path[0] == @parent && path[1] == @picked)
            ::UI.beep
            return
          end
          to_connect = nil
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body'
            if !path[0].get_attribute('MSPhysics Body', 'Ignore', false)
              to_connect = MSPhysics.get_entity_type(path[1]) == 'Joint' ? path[1] : path[0]
            end
          elsif type == 'Joint'
            to_connect = path[0]
          end
          if to_connect
            if MSPhysics.get_entity_type(to_connect) == 'Body'
              attach_joint_proc(to_connect, @picked)
              @connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
            else
              state = gear_joints_proc(@picked, to_connect)
              @connected_gears = JointConnectionTool.get_geared_joints(@picked, @parent, true, false) if state
            end
          else
            ::UI.beep
          end
        end
      else
        @parent = nil
        @picked = nil
        @picked_type = nil
        @identical_picked_joints.clear
        @connected.clear
        @connected_gears.clear
        if path && (path[0].is_a?(Sketchup::Group) || path[0].is_a?(Sketchup::ComponentInstance))
          type = MSPhysics.get_entity_type(path[0])
          if type == 'Body' && !path[0].get_attribute('MSPhysics Body', 'Ignore', false)
            if MSPhysics.get_entity_type(path[1]) == 'Joint'
              @parent = path[0]
              @picked = path[1]
              @picked_type = 'Joint'
              id = @picked.get_attribute('MSPhysics Joint', 'ID')
              if id.is_a?(Fixnum)
                @identical_picked_joints = JointConnectionTool.get_joints_by_id(id)
              end
              @connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
              @connected_gears = JointConnectionTool.get_geared_joints(@picked, @parent, true, false)
            else
              @parent = nil
              @picked = path[0]
              @picked_type = 'Body'
              @connected = JointConnectionTool.get_connected_joints(@picked)
            end
          elsif type == 'Joint'
            @parent = nil
            @picked = path[0]
            @picked_type = 'Joint'
            id = @picked.get_attribute('MSPhysics Joint', 'ID')
            if id.is_a?(Fixnum)
              @identical_picked_joints = JointConnectionTool.get_joints_by_id(id)
            end
            @connected = JointConnectionTool.get_connected_bodies(@picked, @parent)
            @connected_gears = JointConnectionTool.get_geared_joints(@picked, @parent, true, false)
          else
            ::UI.beep
          end
        end
      end
      refresh_viewport
    end

    def onLButtonDoubleClick(flags, x, y, view)
      onLButtonDown(flags, x, y, view)
    end

  end # class JointConnectionTool
end # module MSPhysics
