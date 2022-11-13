#pragma once

#include <iostream>
#include <memory>

// Shape can be initialized by any class (Circle Square...)
// as long as a drawShape function that accepts this type of value is visible to the compiler
class Shape
{
  private:
    struct ShapeConcept
    {
        virtual ~ShapeConcept() = default;

        virtual void draw() const = 0;
        virtual std::unique_ptr<ShapeConcept> clone() const = 0; // Prototype Design Pattern
        // Shape doesn't know the type "T" of the member pimpl (ShapeModel<T>)
        // It only know the base class ShapeConcept
        // Therefore it is impossible to reproduce a ShapeModel<T> by Shape
        // The copy procedure must be delegated to ShapeModel<T> it self
        // Here ShapeConcept declares a interface for Shape to call clone()
    };

    template <typename T> struct ShapeModel : public ShapeConcept
    {
        ShapeModel(T &&value) : object{std::forward<T>(value)}
        {
        }
        ShapeModel(T const &value) : object{value}
        {
        }

        std::unique_ptr<ShapeConcept> clone() const override
        {
            return std::make_unique<ShapeModel>(*this);
        }
        // clone implementation

        void draw() const override
        {
            drawShape(object);
        }

        T object;
    };

    friend void draw(Shape const &shape)
    {
        shape.pimpl->draw();
    }

    std::unique_ptr<ShapeConcept> pimpl; // Bridge Design Pattern: many to one

  public:
    template <typename T> Shape(T const &x) : pimpl{std::make_unique<ShapeModel<T>>(x)}
    {
    }

    // copy ctor & assign
    Shape(Shape const &s) : pimpl{s.pimpl->clone()}
    {
        std::cout << "copy ctor" << std::endl;
    }
    Shape &operator=(Shape const &s)
    {
        pimpl = s.pimpl->clone();
        std::cout << "copy assign" << std::endl;
        return *this;
    }
    // move ctor & assign
    // Note: move ctor and assign is well defined for std::unique_ptr
    Shape(Shape &&s) = default;
    Shape &operator=(Shape &&s) = default;
};