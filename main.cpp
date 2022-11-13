#include "Shape.h"
#include "Circle.h"
#include "Square.h"

#include <iostream>
#include <memory>
#include <vector>

class Triangle {};

void drawShape(Circle const &circle) {
    std::cout << "Circle: " << circle.getRadius() << std::endl;
}

void drawShape(Square const &square) {
    std::cout << "Square: " << square.getSide() << std::endl;
}

void drawShape(Triangle const &) {
    std::cout << "Δ" << std::endl;
}

void drawAllShaped(std::vector<Shape> const &shapes)
{
    for (auto const &shape : shapes)
    {
        draw(shape);
    }
}


int main(int, char**) {
    Shape square{Square(230)};
    Shape circle{Circle{230}};
    draw(circle);
    Shape another_circle = circle; // call copy ctor
    circle = square; // call copy assign

    draw(circle);
    draw(square);
    draw(another_circle);

    using Shapes = std::vector<Shape>;
    Shapes shapes;

    shapes.emplace_back(Circle{2.0});
    shapes.emplace_back(Square{2.0});
    shapes.emplace_back(Triangle{});

    drawAllShaped(shapes);
}
