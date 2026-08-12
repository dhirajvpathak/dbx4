#ifndef DBX4_GEOSPATIAL_TYPE_H
#define DBX4_GEOSPATIAL_TYPE_H

#include <string>
#include <vector>
#include <cmath>
#include <iostream>

namespace dbx4 {

struct Point {
    double x = 0;
    double y = 0;
    
    Point() {}
    Point(double x, double y) : x(x), y(y) {}
    
    // Distance to another point (Euclidean)
    double distance_to(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    std::string to_string() const {
        return "POINT(" + std::to_string(x) + "," + std::to_string(y) + ")";
    }
};

struct Polygon {
    std::vector<Point> points_;
    
    Polygon() {}
    
    void add_point(const Point& p) {
        points_.push_back(p);
    }
    
    // Check if point is inside polygon (simple containment)
    bool contains(const Point& p) const {
        if (points_.size() < 3) return false;
        
        int intersections = 0;
        for (size_t i = 0; i < points_.size(); ++i) {
            Point p1 = points_[i];
            Point p2 = points_[(i + 1) % points_.size()];
            
            if ((p1.y <= p.y && p.y < p2.y) || (p2.y <= p.y && p.y < p1.y)) {
                double x_intersect = p1.x + (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                if (p.x < x_intersect) intersections++;
            }
        }
        
        return intersections % 2 == 1;
    }
    
    std::string to_string() const {
        std::string result = "POLYGON(";
        for (size_t i = 0; i < points_.size(); ++i) {
            if (i > 0) result += ",";
            result += "(" + std::to_string(points_[i].x) + "," + std::to_string(points_[i].y) + ")";
        }
        result += ")";
        return result;
    }
};

}

#endif
