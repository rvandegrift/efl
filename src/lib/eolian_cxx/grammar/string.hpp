#ifndef EOLIAN_CXX_STRING_HH
#define EOLIAN_CXX_STRING_HH

#include <cstdlib>
#include <cstring>

#include "grammar/generator.hpp"
#include "grammar/attributes.hpp"
#include "grammar/case.hpp"

namespace efl { namespace eolian { namespace grammar {

// literal
struct literal_generator
{
   literal_generator(const char* string)
     : string(string) {}

   template <typename OutputIterator, typename Attribute, typename Context>
   bool generate(OutputIterator sink, Attribute const&, Context const&) const
   {
      std::copy(string, string + std::strlen(string), sink);
      return true;
   }

   const char* string;
};

template <>
struct is_eager_generator<literal_generator> : std::true_type {};

template <>
struct is_generator<const char*> : std::true_type {};

template <int N>
struct is_generator<const char[N]> : std::true_type {};

literal_generator as_generator(char const* literal) { return literal; }

struct {
  literal_generator operator()(const char* literal) const
  {
    return literal_generator(literal);
  }
} const lit = {};

// string
struct string_generator
{
   template <typename OutputIterator, typename Attribute, typename Context>
   bool generate(OutputIterator sink, Attribute const& attribute, Context const&) const
   {
      if(tag_check<upper_case_tag, Context>::value)
        {
          std::transform(std::begin(attribute), std::end(attribute), sink, &::toupper);
        }
      else if(tag_check<lower_case_tag, Context>::value)
        {
          std::transform(std::begin(attribute), std::end(attribute), sink, &::tolower);
        }
      else
        {
          std::copy(std::begin(attribute), std::end(attribute), sink);
        }
      return true;
   }
};

struct specific_string_generator
{
   specific_string_generator(std::string string) : string(string) {}
  
   template <typename OutputIterator, typename Context>
   bool generate(OutputIterator sink, attributes::unused_type, Context const&) const
   {
      if(tag_check<upper_case_tag, Context>::value)
        {
          std::transform(std::begin(string), std::end(string), sink, &::toupper);
        }
      else if(tag_check<lower_case_tag, Context>::value)
        {
          std::transform(std::begin(string), std::end(string), sink, &::tolower);
        }
      else
        {
          std::copy(std::begin(string), std::end(string), sink);
        }
      return true;
   }

   std::string string;
};

struct string_replace_generator
{
   template <typename OutputIterator, typename Attribute, typename Context>
   bool generate(OutputIterator sink, Attribute const& string, Context const&) const
   {
      if(tag_check<upper_case_tag, Context>::value)
        {
          std::transform(std::begin(string), std::end(string), sink,
                         [&] (char c) -> char
                         {
                           if(c == from)
                             return to;
                           else
                             return ::toupper(c);
                         }
                         );
        }
      else if(tag_check<lower_case_tag, Context>::value)
        {
          std::transform(std::begin(string), std::end(string), sink,
                         [&] (char c) -> char
                         {
                           if(c == from)
                             return to;
                           else
                             return ::tolower(c);
                         });
        }
      else
        {
          std::transform(std::begin(string), std::end(string), sink
                         , [&] (char c) { return c == from ? to : c; });
        }
      return true;
   }

  char from, to;
};
      
template <>
struct is_eager_generator<string_generator> : std::true_type {};
template <>
struct is_eager_generator<specific_string_generator> : std::true_type {};
template <>
struct is_eager_generator<string_replace_generator> : std::true_type {};

struct string_generator_terminal
{
  specific_string_generator operator[](std::string string) const
  {
    return specific_string_generator{string};
  }
} const string = {};

struct string_replace_terminal
{
  string_replace_generator operator()(char from, char to) const
  {
    return string_replace_generator{from, to};
  }
} const string_replace = {};
      
template <>
struct is_generator<string_generator_terminal> : std::true_type {};
template <>
struct is_generator<std::string> : std::true_type {};

string_generator as_generator(string_generator_terminal)
{
  return string_generator{};
}
namespace type_traits {
template <>
struct attributes_needed<string_generator> : std::integral_constant<int, 1> {};  
template <>
struct attributes_needed<string_generator_terminal> : std::integral_constant<int, 1> {};  
template <>
struct attributes_needed<string_replace_generator> : std::integral_constant<int, 1> {};  
}      
      
} } }

namespace std {

::efl::eolian::grammar::specific_string_generator as_generator(std::string string)
{
  return ::efl::eolian::grammar::specific_string_generator{string};
}
  
}

#endif
