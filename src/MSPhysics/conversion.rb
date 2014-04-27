module MSPhysics
  module Conversion

    module_function

    @inch_to_mm = 25.4
    @oz_to_g = 28.3495

    # Convert length from one unit format to another.
    # @param [Numeric] length
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Numeric]
    def convert_length(length, from = :in, to = :m)
      # First convert 'from' to inches as basis.
      ratio1 = case from.to_s.downcase.to_sym
        when :mm, :millimeter, :millimeters
          1/@inch_to_mm
        when :cm, :centimeter, :centimeters
          10/@inch_to_mm
        when :dm, :decimeter, :decimeters
          100/@inch_to_mm
        when :m, :meter, :meters
          1000/@inch_to_mm
        when :km, :kilometer, :kilometers
          10**6/@inch_to_mm
        when :in, :inch, :inches
          1
        when :ft, :foot, :feet
          12
        when :yd, :yard, :yards
          36
        when :mi, :mile, :miles
          63360
        else
          return length
      end
      # Now convert inches to 'to'.
      ratio2 = case to.to_s.downcase.to_sym
        when :mm, :millimeter, :millimeters
          @inch_to_mm
        when :cm, :centimeter, :centimeters
          @inch_to_mm/10
        when :dm, :decimeter, :decimeters
          @inch_to_mm/100
        when :m, :meter, :meters
          @inch_to_mm/1000
        when :km, :kilometer, :kilometers
          @inch_to_mm/10**6
        when :in, :inch, :inches
          1
        when :ft, :foot, :feet
          1/12.0
        when :yd, :yard, :yards
          1/36.0
        when :mi, :mile, :miles
          1/63360.0
        else
          return length
      end
      length * ratio1 * ratio2
    end

    # Convert 3d point from one unit format to another.
    # @param [Array<Numeric>, Geom::Point3d] point
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Geom::Point3d]
    def convert_point(point, from = :in, to = :m)
      x = convert_length(point[0], from, to)
      y = convert_length(point[1], from, to)
      z = convert_length(point[2], from, to)
      Geom::Point3d.new(x,y,z)
    end

    # Convert 3d vector from one unit format to another.
    # @param [Array<Numeric>, Geom::Vector3d] vector
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Geom::Vector3d]
    def convert_vector(vector, from = :in, to = :m)
      x = convert_length(vector[0], from, to)
      y = convert_length(vector[1], from, to)
      z = convert_length(vector[2], from, to)
      Geom::Vector3d.new(x,y,z)
    end

    # Convert an array of 3d points from one unit format to another.
    # @param [Array<Array<Numeric>>, Array<Geom::Point3d>] points
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Array<Geom::Point3d>]
    def convert_points(points, from = :in, to = :m)
      points = [points] unless points.is_a?(Array)
      new_pts = []
      points.each{ |pt|
        x = convert_length(pt[0], from, to)
        y = convert_length(pt[1], from, to)
        z = convert_length(pt[2], from, to)
        new_pts.push Geom::Point3d.new(x,y,z)
      }
      new_pts
    end

    # Convert area from one unit format to another.
    # @param [Numeric] area
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Numeric]
    def convert_area(area, from = :in, to = :m)
      ratio = convert_length(1, from, to)
      area * ratio**2
    end

    # Convert volume from one unit format to another.
    # @param [Numeric] volume
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Numeric]
    def convert_volume(volume, from = :in, to = :m)
      ratio = convert_length(1, from, to)
      volume * ratio**3
    end

    # Convert mass from one format to another.
    # @param [Numeric] mass
    # @param [Symbol, String] from
    # @param [Symbol, String] to
    # @return [Numeric]
    def convert_mass(mass, from = :lb, to = :kg)
      # First convert 'from' to ounces
      ratio1 = case from.to_s.downcase.to_sym
        when :mg, :milligram, :milligrams
          1e-3/@oz_to_g
        when :cg, :centigram, :centigrams
          1e-2/@oz_to_g
        when :g, :gram, :grams
          1/@oz_to_g
        when :hg, :hectogram, :hectograms
          1e+2/@oz_to_g
        when :kg, :kilogram, :kilograms
          1e+3/@oz_to_g
        when :t, :ton, :tons
          1e+6/@oz_to_g
        when :oz, :ounce, :ounces
          1
        when :lb, :lbs, :pound, :pounds
          16
        else
          return mass
      end
      # Now convert ounces to 'to'.
      ratio2 = case to.to_s.downcase.to_sym
        when :mg, :milligram, :milligrams
          @oz_to_g/1e-3
        when :cg, :centigram, :centigrams
          @oz_to_g/1e-2
        when :g, :gram, :grams
          @oz_to_g
        when :hg, :hectogram, :hectograms
          @oz_to_g/1e+2
        when :kg, :kilogram, :grams
          @oz_to_g/1e+3
        when :t, :ton, :tons
          @oz_to_g/1e+6
        when :oz, :ounce, :ounces
          1
        when :lb, :lbs, :pound, :pounds
          1/16.0
        else
          return mass
      end
      mass * ratio1 * ratio2
    end

  end # module Conversion
end # module MSPhysics
