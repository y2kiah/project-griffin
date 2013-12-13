
//  Copyright (C) 1999--2001  Petter Urkedal (petter.urkedal@matfys.lth.se)

//  This file is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.

//  This file is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

//  As a special exception, you may use this file as part of a free
//  software library without restriction.  Specifically, if other files
//  instantiate templates or use macros or inline functions from this
//  file, or you compile this file and link it with other files to
//  produce an executable, this file does not by itself cause the
//  resulting executable to be covered by the GNU General Public
//  License.  This exception does not however invalidate any other
//  reasons why the executable file might be covered by the GNU General
//  Public License.

//  $Id: meta.h,v 1.1 2002/05/30 22:52:07 petter_urkedal Exp $



#ifndef MORE_GEN_META_H
#define MORE_GEN_META_H

namespace more {


struct nulltype {
    typedef nulltype first_type;
    typedef nulltype second_type;
    operator bool() { return false; }
    operator int() { return 0; }
};
struct truetype {
    operator bool() { return true; }
    operator int() { return 1; }
};

template <class T, class U>
  struct typepair {
      typedef T first_type;
      typedef U second_type;
  };

template<typename T> struct typepair_traits {
    static const int atomic = 1;
    static const int linear = 0;
    static const int arity = 0;
};
template<> struct typepair_traits<nulltype> {
    static const int atomic = 1;
    static const int linear = 1;
    static const int arity = 0;
};

template<typename T, typename U>
  struct typepair_traits< typepair<T, U> > {
      static const int atomic = 0;
      static const int linear =
	typepair_traits<T>::atomic && typepair_traits<U>::linear;
      static const int arity =
	typepair_traits<U>::arity + 1;
  };


//  --- Logical operations ---
//
//  Please note the difference between equals_ and equiv_.  The last
//  evaluates to truetype if either non or both arguments are nulltype;
//  otherwise it returns nulltype.  equals_ compares the argument
//  for exact equality.

template <class Cond, class Positive, class Negative = nulltype>
struct if_ { typedef Positive eval; };

template <class Positive, class Negative>
struct if_<nulltype, Positive, Negative> { typedef Negative eval; };

template <class T> struct not_ { typedef nulltype eval; };
template <> struct not_<nulltype> { typedef truetype eval; };

template <class T, class U>  struct equals_ { typedef nulltype eval; };
template <class T>           struct equals_<T, T> { typedef T eval; };
template <> struct equals_<nulltype, nulltype> { typedef truetype eval; };

template <class T, class U>  struct equiv_ { typedef truetype eval; };
template <class T>    struct equiv_<T, nulltype> { typedef nulltype eval; };
template <class U>    struct equiv_<nulltype, U> { typedef nulltype eval; };
template <> struct equiv_<nulltype, nulltype> { typedef truetype eval; };

template <class T1 = nulltype, class T2 = nulltype, class T3 = nulltype,
          class T4 = nulltype, class T5 = nulltype, class T6 = nulltype>
struct or_ { typedef T1 eval; };
template <class T2, class T3, class T4, class T5, class T6>
struct or_<nulltype, T2, T3, T4, T5, T6> { typedef T2 eval; };
template <class T3, class T4, class T5, class T6>
struct or_<nulltype, nulltype, T3, T4, T5, T6> { typedef T3 eval; };
template <class T4, class T5, class T6>
struct or_<nulltype, nulltype, nulltype, T4, T5, T6> { typedef T4 eval; };
template <class T5, class T6>
struct or_<nulltype, nulltype, nulltype, nulltype, T5, T6>
    { typedef T5 eval; };
template <class T6>
struct or_<nulltype, nulltype, nulltype, nulltype, nulltype, T6>
    { typedef T6 eval; };

#if 0
template <class T1 = truetype, class T2 = T1, class T3 = T2,
          class T4 = T3,       class T5 = T4, class T6 = T5>
struct and_ { typedef T6 eval; };
template <class T2, class T3, class T4, class T5, class T6>
struct and_<nulltype, T2, T3, T4, T5, T6> { typedef nulltype eval; };
template <class T1, class T3, class T4, class T5, class T6>
struct and_<T1, nulltype, T3, T4, T5, T6> { typedef nulltype eval; };
template <class T1, class T2, class T4, class T5, class T6>
struct and_<T1, T2, nulltype, T4, T5, T6> { typedef nulltype eval; };
template <class T1, class T2, class T3, class T5, class T6>
struct and_<T1, T2, T3, nulltype, T5, T6> { typedef nulltype eval; };
template <class T1, class T2, class T3, class T4, class T6>
struct and_<T1, T2, T3, T4, nulltype, T6> { typedef nulltype eval; };
template <class T1, class T2, class T3, class T4, class T5>
struct and_<T1, T2, T3, T4, T5, nulltype> { typedef nulltype eval; };
#else
template <class T1 = truetype, class T2 = T1, class T3 = T2,
          class T4 = T3,       class T5 = T4, class T6 = T5>
struct and_ : public and_<T2, T3, T4, T5, T6, truetype> {};
template <class T2, class T3, class T4, class T5, class T6>
struct and_<nulltype, T2, T3, T4, T5, T6> { typedef nulltype eval; };
template <class T>
struct and_<T, truetype, truetype, truetype, truetype, truetype>
    { typedef T eval; };
#endif

template <class T, class U>  struct xor_ { typedef nulltype eval; };
template <class T>           struct xor_<T, nulltype> { typedef T eval; };
template <class U>           struct xor_<nulltype, U> { typedef U eval; };



//  --- append_ ---
//
//  Appends an element to a type-list.
//
template <class List, class Value>
struct append_ { typedef typename List::is_not_a_list eval; };

template <class V>
struct append_<nulltype, V> { typedef typepair<V, nulltype> eval; };

template <class E, class Tail, class V>
struct append_<typepair<E, Tail>, V>
{ typedef typepair<E, typename append_<Tail, V>::eval> eval; };


//  --- concat_ ---
//
//  Concatinates two lists.

template <class List, class List1>
struct concat_ { typedef typename List::is_not_a_list eval; };

template <class List1>
struct concat_<nulltype, List1> { typedef List1 eval; };

template <class E, class Tail, class List1>
struct concat_<typepair<E, Tail>, List1>
{ typedef typepair<E, typename concat_<Tail, List1>::eval> eval; };


//  --- tree_to_list ---
//
//  Expands a tree into a list

template <class List>
struct tree_to_list_ { typedef typepair<List, nulltype> eval; };

template <class List, class List1>
struct tree_to_list_< typepair<List, List1> > {
    typedef typename concat_
	< typename tree_to_list_<List>::eval,
	  typename tree_to_list_<List1>::eval
        >::eval eval;
};


//  --- is_in_ ---

template <class T, class List>
struct is_in_ { typedef typename List::is_not_a_list eval; };

template <class T, class U, class Tail>
struct is_in_< T, typepair<U, Tail> >
{ typedef typename is_in_<T, Tail>::eval eval; };

template <class T, class Tail>
struct is_in_< T, typepair<T, Tail> > { typedef truetype eval; };

template <class T>
struct is_in_<T, nulltype> { typedef nulltype eval; };



//  --- typeassert_ ---
//
//  Produces a compile-time error if its argument evaluates to nulltype
//  The argument should not be evaluated in advance, since we want the
//  compiler to print out the type-expression, if possible.


template <class T>
struct typeassert_ {
    typedef typename if_
      < typename not_< typename T::eval >::eval,
	typename T::ASSERTION_FAILED,
	T >::eval eval;
};



//  --- LIST ---
//
//  Defines a convenient type-function for producing lists with up to
//  eight elements.

template <class T1 = nulltype, class T2 = nulltype, class T3 = nulltype,
          class T4 = nulltype, class T5 = nulltype, class T6 = nulltype,
          class T7 = nulltype, class T8 = nulltype, class T9 = nulltype,
          class T10= nulltype, class T11= nulltype, class T12= nulltype>
struct list_ {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, typepair
	< T7, typepair< T8, typepair< T9, typepair
        <T10, typepair<T11, typepair<T12, nulltype > > > > > > > > > > > >
	eval;
};

template <class T1, class T2, class T3, class T4, class T5, class T6,
          class T7, class T8, class T9, class T10, class T11>
struct list_<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, typepair
	< T7, typepair< T8, typepair< T9, typepair
	<T10, typepair<T11, nulltype > > > > > > > > > > >
	eval;
};

template <class T1, class T2, class T3, class T4, class T5, class T6,
          class T7, class T8, class T9, class T10>
struct list_<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, typepair
	< T7, typepair< T8, typepair< T9, typepair
	<T10, nulltype > > > > > > > > > >
	eval;
};

template <class T1, class T2, class T3, class T4, class T5, class T6,
          class T7, class T8, class T9>
struct list_<T1, T2, T3, T4, T5, T6, T7, T8, T9> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, typepair
	< T7, typepair< T8, typepair< T9, nulltype > > > > > > > > >
	eval;
};

template <class T1, class T2, class T3, class T4, class T5, class T6,
          class T7, class T8>
struct list_<T1, T2, T3, T4, T5, T6, T7, T8> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, typepair
	< T7, typepair< T8, nulltype > > > > > > > >
	eval;
};

template <class T1, class T2, class T3, class T4, class T5, class T6,
          class T7>
struct list_<T1, T2, T3, T4, T5, T6, T7> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, typepair
	< T7, nulltype > > > > > > >
	eval;
};

template <class T1, class T2, class T3, class T4, class T5, class T6>
struct list_<T1, T2, T3, T4, T5, T6> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, typepair< T6, nulltype > > > > > > eval;
};

template <class T1, class T2, class T3, class T4, class T5>
struct list_<T1, T2, T3, T4, T5> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair
	< T4, typepair< T5, nulltype > > > > > eval;
};

template <class T1, class T2, class T3, class T4>
struct list_<T1, T2, T3, T4> {
    typedef typepair
	< T1, typepair< T2, typepair< T3, typepair< T4, nulltype > > > >
	eval;
};

template <class T1, class T2, class T3> struct list_<T1, T2, T3>
{ typedef typepair< T1, typepair< T2, typepair< T3, nulltype > > > eval; };

template <class T1, class T2> struct list_<T1, T2>
{ typedef typepair< T1, typepair< T2, nulltype > > eval; };

template <class T1> struct list_<T1>
{ typedef typepair< T1, nulltype > eval; };

template <> struct list_<> { typedef nulltype eval; };



//  --- order_ ---

template <class T1, class T2, class Ordering>
  class order_ { typedef typename Ordering::is_not_a_list eval; };
template <class T1, class T2, class T3, class Tail>
  struct order_< T1, T2, typepair<T3, Tail> >
    { typedef typename order_<T1, T2, Tail>::eval eval; };
template <class T1, class T2, class Tail>
  struct order_< T1, T2, typepair<T1, Tail> >
    { typedef typepair<T1, T2> eval; };
template <class T1, class T2, class Tail>
  struct order_< T1, T2, typepair<T2, Tail> >
    { typedef typepair<T2, T1> eval; };
template <class T, class Tail>
  struct order_< T, T, typepair<T, Tail> >
    { typedef typepair<T, T> eval; };
template <class T1, class T2>
  struct order_< T1, T2, nulltype >
    { typedef nulltype eval; };



//  --- Balanced types for (+, -, *, /, %) ---

#if 0
typedef list_< char, short, int, long, float, double, long double >::eval
    balanced_type_order;

template <class T1, class T2>
class commutative_balanced_type_ { typedef nulltype eval; };

template <class T1, class T2>
class balanced_type_ {
    typedef typename order_<T1, T2, balanced_type_order>::eval order;
public:
    typedef typename or_
      < typename commutative_balanced_type_<T1, T2>::eval,
	typename commutative_balanced_type_<T2, T1>::eval,
	typename order::second_type,
	typename if_
	  < typename is_in_<T1, balanced_type_order>::eval, T2
	  > ::eval,
	typename if_
	  < typename is_in_<T2, balanced_type_order>::eval, T1
	  > ::eval
      > ::eval eval;
};

template <class T1, class T2>
struct commutative_plus_type_ { typedef nulltype eval; };

template <class T1, class T2>
struct plus_type_ {
    typedef typename or_
      < typename commutative_plus_type_<T1, T2>::eval,
	typename commutative_plus_type_<T2, T1>::eval,
	typename balanced_type_<T1, T2>::eval >::eval eval;
};

template <class T1, class T2>
struct commutative_minus_type_ { typedef nulltype eval; };

template <class T1, class T2>
struct minus_type_ {
    typedef typename or_
      < typename commutative_minus_type_<T1, T2>::eval,
	typename commutative_minus_type_<T2, T1>::eval,
	typename balanced_type_<T1, T2>::eval >::eval eval;
};

template <class T1, class T2>
struct commutative_times_type_ { typedef nulltype eval; };

template <class T1, class T2>
struct times_type_ {
    typedef typename or_
      < typename commutative_times_type_<T1, T2>::eval,
	typename commutative_times_type_<T2, T1>::eval,
	typename balanced_type_<T1, T2>::eval >::eval eval;
};

template <class T1, class T2>
struct commutative_divides_type_ { typedef nulltype eval; };

template <class T1, class T2>
struct divides_type_ {
    typedef typename or_
      < typename commutative_divides_type_<T1, T2>::eval,
	typename commutative_divides_type_<T2, T1>::eval,
	typename balanced_type_<T1, T2>::eval >::eval eval;
};

template <class T1, class T2>
struct commutative_remainder_type_ { typedef nulltype eval; };

template <class T1, class T2>
struct remainder_type_ {
    typedef typename or_
      < typename commutative_remainder_type_<T1, T2>::eval,
	typename commutative_remainder_type_<T2, T1>::eval,
	typename balanced_type_<T1, T2>::eval >::eval eval;
};
#endif


//  --- Definition of is_a_<T, U> ---

template<typename T> inline char _meta_is_a_1(const void*, const T*) {}
template<typename T> inline long _meta_is_a_1(const T*, const T*) {}

template <int N> struct _longsize {};
template <> struct _longsize<sizeof(char)> { typedef nulltype eval; };
template <> struct _longsize<sizeof(long)> { typedef truetype eval; };

#if 0
template <class T, class U> class is_a_ {
private:
    enum { size = sizeof(_meta_is_a_1(static_cast<T*>(0),
				      static_cast<U*>(0))) };
public:
    typedef typename _longsize<size>::eval eval;
};
#endif


//  --- is_pointer_<T> ---

template <class T> struct is_pointer_ { typedef nulltype eval; };
template <class T> struct is_pointer_<T*> { typedef truetype eval; };


//  --- is_class_<T> ---

template <class T> struct is_class_ { typedef truetype eval; };
template <class T> struct is_class_<T*> { typedef nulltype eval; };
template <> struct is_class_<bool> { typedef nulltype eval; };
template <> struct is_class_<char> { typedef nulltype eval; };
template <> struct is_class_<signed char> { typedef nulltype eval; };
template <> struct is_class_<unsigned char> { typedef nulltype eval; };
template <> struct is_class_<short> { typedef nulltype eval; };
template <> struct is_class_<unsigned short> { typedef nulltype eval; };
template <> struct is_class_<int> { typedef nulltype eval; };
template <> struct is_class_<unsigned int> { typedef nulltype eval; };
template <> struct is_class_<long> { typedef nulltype eval; };
template <> struct is_class_<unsigned long> { typedef nulltype eval; };
template <> struct is_class_<float> { typedef nulltype eval; };
template <> struct is_class_<double> { typedef nulltype eval; };
template <> struct is_class_<long double> { typedef nulltype eval; };

template <class T> struct is_etype_
{ typedef typename not_<typename is_class_<T>::eval>::eval eval; };


//  --- is_true_ ---

template <bool Cond> struct is_true_ { typedef nulltype eval; };
template <> struct is_true_<true> { typedef truetype eval; };


//  --- is_const_ ---

template<typename T> struct is_const_ { typedef nulltype eval; };
template<typename T> struct is_const_<const T> { typedef truetype eval; };

//  --- bigger_type_ ---

template <class T, class U> struct bigger_type_ {
    typedef typename if_
      < typename is_true_< (sizeof(T) >= sizeof(U)) >::eval,
        T, U >::eval eval;
};

//  --- to_unsigned_ ---

template<typename T> struct to_unsigned_ { typedef T eval; };
template<> struct to_unsigned_<char> { typedef unsigned char eval; };
template<> struct to_unsigned_<signed char> { typedef unsigned char eval; };
template<> struct to_unsigned_<short> { typedef unsigned short eval; };
template<> struct to_unsigned_<int> { typedef unsigned int eval; };
template<> struct to_unsigned_<long> { typedef unsigned long eval; };
#ifdef MORE_HAVE_LONG_LONG
template<> struct to_unsigned_<long long> { typedef unsigned long long eval; };
#endif

//  --- to_signed_ ---

template<typename T> struct to_signed_ { typedef T eval; };
template<> struct to_signed_<char> { typedef signed char eval; };
template<> struct to_signed_<unsigned char> { typedef signed char eval; };
template<> struct to_signed_<unsigned short> { typedef short eval; };
template<> struct to_signed_<unsigned int> { typedef int eval; };
template<> struct to_signed_<unsigned long> { typedef long eval; };
#ifdef MORE_HAVE_LONG_LONG
template<> struct to_signed_<unsigned long long> { typedef long long eval; };
#endif


} // more

#endif
