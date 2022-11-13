#pragma once
class Circle
{
  public:
    explicit Circle(double rad) : radius{rad}
    {
    }

    double getRadius() const noexcept
    {
        return radius;
    }

  private:
    double radius;
};