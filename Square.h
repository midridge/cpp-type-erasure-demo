#pragma once

class Square
{
  public:
    explicit Square(double s) : side{s}
    {
    }

    double getSide() const noexcept
    {
        return side;
    }

  private:
    double side;
};