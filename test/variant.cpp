#include <iostream>
#include <stdexcept>
#include <typeinfo>

class any
{
public:
  template < typename T > any (const T & t):held_ (new holder < T > (t))
  {
  }
   ~any ()
  {
    delete held_;
  }
  template < typename U > U cast ()const
  {
    if (typeid (U) != held_->type_info ())
      throw std::runtime_error ("Bad any cast");
      return static_cast < holder < U > *>(held_)->t_;
  }
private:
  struct base_holder
  {
    virtual ~ base_holder ()
    {
    }
    virtual const std::type_info & type_info () const = 0;
  };

  template < typename T > struct holder:base_holder
  {
    holder (const T & t):t_ (t)
    {
    }
    const std::type_info & type_info () const
    {
      return typeid (t_);
    }
    T t_;
  };
private:
  base_holder * held_;
};

int
main ()
{
  any a1 (1);
  any a2 (2.2);
  any b (std::string ("abcd"));

  try
  {
    std::cout << a1.cast < int >() << std::endl;
    std::cout << a2.cast < double >() << std::endl;
    std::cout << b.cast < std::string >() << std::endl;

    std::cout << b.cast < double >() << std::endl;
  }
  catch (const std::exception & e)
  {
    std::cout << e.what () << std::endl;
  }
  return 0;
}
