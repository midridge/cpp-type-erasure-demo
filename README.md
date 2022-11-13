# C++ Type Erasure Demo
> The code is reproduced from [this CppCon video](https://youtu.be/4eeESJQk-mw)
## Prerequisite
- Know how to use C++ smart pointers
- Know what is vtable
- Know C++ template programming
## Intro
Static type system can sometimes be annoying. Until very recently, I only knew one way when I want to treat different types as one (for example saving different items in an array or vector)

Conventionally dynamic polymorphism is achived by inheritance, such as
```c++
class Shape { virtual void draw() const = 0; };
class Circle : public Shape { ... };
class Square : public Shape { ... };
```
And then you can use the base class pointer Shape to refer to the derived class, like this
```c++
vector<unique_ptr<Shape>> shapes;
shapes.push_back(make_unique<Circle>(Circle{}));
shapes.push_back(make_unique<Square>(Square{}));
for (auto &shape : shapes)
    shape->draw();
```
Or just
```c++
Shape *shape = new Circle{};
shape->draw();
delete shape;
```
It looks nice but ...

- What if I don't like logics to be encapsulated in class for some reason?

- What if I am worry about performance issue introduced by virtual functions?

- What if Circle and Square are already written and there is no base class for them?

- What if I don't want save pointers in my vector?

## Strategy Pattern

For the first what if, strategy design pattern by GOF is a solution.

First we have to set up a inheritance hierarchy for each concrete shape and implement draw logic inside:
```c++
class DrawCircleStrategy { virtual void draw(Circle) = 0 };
class MyDrawCircleStrategy : public DrawCircleStrategy { void draw() override; }
class HisDrawCircleStrategy : public DrawCircleStrategy { void draw() override;}

class DrawSquareStrategy { virtual void draw(Square) = 0 };
class MyDrawSquareStrategy : public DrawSquareStrategy { void draw() override; };
class HerDrawSquareStrategy : public DrawSquareStrategy { void draw() override; };
```

Then each derived shape class holds a member called draw_strategy:

```c++
class Shape { virtual void draw() const = 0; };
class Circle : public Shape {
  public:
    Circle(unique_ptr<DrawCircleStrategy> strategy)
        : draw_strategy{ strategy }
    {}

    void draw() override {
        draw_strategy->draw(*this);
    }

  private:
    unique_ptr<DrawCircleStrategy> draw_strategy;
};
class Circle : public Shape {
  public:
    Circle(unique_ptr<DrawCircleStrategy> strategy)
        : draw_strategy{ strategy }
    {}

    void draw() override {
        draw_strategy->draw(*this);
    }

  private:
    unique_ptr<DrawCircleStrategy> draw_strategy;
};
```

Instantiation, push_back, call draw as before:
```c++
vector<unique_ptr<Shape>> shapes;
shapes.push_back(make_unique<Circle>(make_unique<MyDrawCircleStrategy>()));
shapes.push_back(make_unique<Square>(make_unique<HerDrawSquareStrategy>()));
for (auto &shape : shapes)
    shape->draw();
```

The draw logic is extracted out in a not so elegant way. To accomplish this, we have to pay:

- Another layer of vtable lookup (protential performance penalty)
- More pointers when using
- Proliferation of base classes (using virtual interface requires remembering many things)

Hopefully, there is a way to address these drawbacks and the mentioned What-Ifs all together

## External Polymorphism

We try to isolate draw logic with another design pattern called 
[External polymorphism](https://www.dre.vanderbilt.edu/~schmidt/PDF/External-Polymorphism.pdf)

First assume no hierarchy and relation for Circle and Square:
```c++
class Circle {
  public:
    explicit Circle(double rad) : radius{ rad } {}
    double getRadius() const noexcept { return radius; }
  private:
    double radius;
};
class Square {
  public:
    explicit Square(double s) : side{ s } {}
    double getSide() const noexcept { return side; }
  private:
    double side;
};
```

Notice no draw or other logic known by these two classes

Then build up a inheritance hierarchy that handles draw call:

```c++
struct ShapeConcept
{
    virtual ~ShapeConcept() = default;
    virtual void draw() const = 0;
};

template <typename T> struct ShapeModel : public ShapeConcept
{
    ShapeModel(T &&value) : object{ std::forward<T>(value) }
    {}
    ShapeModel(T const &value) : object{ value }
    {}

    void draw() const override {
        drawShape(object);
    }

    T object;
};
```
The `drawShape()` function is not declared before.
This is OK because ShapeModel is just a template.
`drawShape()` will be only searched and tested when template is
instantiated

Therefore we get a free function that can be defined anywhere!
Even in 3rd party libs!

let's define them here:
```c++
void drawShape(Circle const& circle) {
    std::cout << "Circle: " << circle.getRadius() << std::endl;
}
void drawShape(Square const &square) {
    std::cout << "Square: " << square.getSide() << std::endl;
}
```

Let's see how to use these stuff

```c++
int main() {
    vector<unique_ptr<ShapeConcept>> shapes;
    shapes.push_bac(make_unique<ShapeModel<Circle>>(Circle{ 2.0 }));
    shapes.push_bac(make_unique<ShapeModel<Square>>(Square{ 3.0 }));
    for (auto const &shape : shapes)
        draw(shape);
}
```

Still we are fill the vector in the base class pointers, but notice we have accomplished what we have aimed when using strategies

If wish to add a new concrete shape (like triangle), the job can't be simpler. Just define Triangle and write drawShape for it
```c++
class Triangle {};
void drawShape(Triangle const &) {
    std::cout << "Δ" << std::endl;
}
```

## Type Erasure Wrapper

Let's try to eliminate the pointer usage (they are long and ugly)

Wrap ShapeConcept and ShapeModel in a class named Shape

```c++
class Shape
{
  private:
    struct ShapeConcept {
        virtual ~ShapeConcept() = default;
        virtual void draw() const = 0;
        virtual unique_ptr<ShapeConcept> clone() const = 0;
    };

    template <typename T>
    struct ShapeModel : public ShapeConcept {
        ShapeModel(T &&value) : object{forward<T>(value)}
        {}
        ShapeModel(T const &value) : object{value}
        {}
        unique_ptr<ShapeConcept> clone() const override {
            return make_unique<ShapeModel>(*this);
        }
        void draw() const override {
            drawShape(object);
        }
        T object;
    };

    friend void draw(Shape const &shape) {
        shape.pimpl->draw();
    }

    unique_ptr<ShapeConcept> pimpl;

  public:
    template <typename T>
    Shape(T const &x) 
        : pimpl{make_unique<ShapeModel<T>>(x)}
    {}

    Shape(Shape const &s) : pimpl{s.pimpl->clone()}
    {}
    Shape &operator=(Shape const &s) {
        pimpl = s.pimpl->clone();
        return *this;
    }
    Shape(Shape &&s) = default;
    Shape &operator=(Shape &&s) = default;
};
```

Notice a friend function `draw()` is defined inline, this is because later we will call `draw()` on Shape class not on `ShapeModel` anymore. Feel free to name it another way or define it as method

The key is the templated Shape constructor.
Shape itself remains a plain class by it holds a base class (`ShapeConcept`) pointer `pimpl`.

When instantiating an object of Shape,
the `T` in `ShapeModel<T>` is auto deducted,
then `ShapeModel<T>*` is saved to `pimpl`

Here a design pattern call Bridge is used because the base class pointer `pimpl` "bridges" `Shape` with unlimited types.
(plainly put it: just the trivial virtual behavior of base pointer)

In the whole process, 
`Shape` has no idea about what exactly `T` is.
Therefore, when copying `Shape` object, trouble may come.

Here another design pattern call Prototype comes.
We let `ShapeModel<T>` it self to clone itself and return a base class pointer

Here is the use case:
```c++
int main() {
    vector<Shape> shape;
    shapes.emplace_back(Circle{2.0});
    shapes.emplace_back(Square{2.0});
    shapes.emplace_back(Triangle{});
    for (auto const &shape : shapes)
        draw(shape);
}
```

Actually it is impossible to walk around virtual functions,
but the virtual interface can be hidden in the private part of Shape

The pointers are all gone leaving us a clean interface
