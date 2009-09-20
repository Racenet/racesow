/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_GENERIC_CONSTANT_H)
#define INCLUDED_GENERIC_CONSTANT_H

/// \file
/// \brief Language extensions for constants that are guaranteed to be evaluated at compile-time.

/// \brief A compile-time-constant as a type.
template<typename Type>
struct ConstantWrapper
{
  typedef typename Type::Value Value;
  operator Value() const
  {
    return Type::evaluate();
  }
};
template<typename TextOutputStreamType,  typename Type>
inline TextOutputStreamType& ostream_write(TextOutputStreamType& ostream, const ConstantWrapper<Type>& c)
{
  return ostream_write(ostream, typename Type::Value(c));
}

#define TYPE_CONSTANT(name, value, type) struct name##_CONSTANT_ { typedef type Value; static Value evaluate() { return value; } }; typedef ConstantWrapper<name##_CONSTANT_> name
#define STRING_CONSTANT(name, value) TYPE_CONSTANT(name, value, const char*)
#define INTEGER_CONSTANT(name, value) TYPE_CONSTANT(name, value, int)

STRING_CONSTANT(EmptyString, "");

#endif
