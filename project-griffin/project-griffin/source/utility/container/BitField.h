/* ----==== BITFIELD.H ====---- */

#pragma once

#include <sstream>

using std::string;
using std::ostringstream;

///// STRUCTURES /////

//========================================================================================
//	The C++ bool type is inadequate for storage of flag data since it uses 32 bits of
//	space to contain 1 bit of data. All bits of this type can be used to store flag
//	information. The objects containing bit fields are responsible for defining and
//	maintaining the data that each bit represents.
//========================================================================================
template <class T>
class BitField {
	private:
		T		mBits;

	public:
		// Operators
		operator T() const { return mBits; }
		inline BitField &	operator=(const BitField &bf);
		inline BitField &	operator=(const T set);
		inline bool			operator&(const T test);
		bool				operator==(const BitField &bf) const { return (bits == bf.mBits); }
		bool				operator!=(const BitField &bf) const { return (bits != bf.mBits); }
		
		// Mutators
		void			set(const T set) { mBits = set; }
		void			clear() { mBits = 0; }
		void			setBit(const int index) { mBits |= (1 << index); }
		void			setBit(const int index, const T set) { if (set) setBit(index); else clearBit(index); }
		void			clearBit(const int index) { mBits &= ~(1 << index); }
		void			toggleBit(const int index) { mBits ^= ~(1 << index); }

		// Accessors
		T				bits() const { return mBits; }
		bool			isEmpty() const { return (mBits == 0); }
		inline T		getBit(const int index) const;	// returns the value of the bit (0 or 1)
		inline bool		testBit(const int index) const;	// test a single bit
		inline bool		testFlags(const T test) const;	// test if multiple bits are same
		inline bool		testAny(const T test) const;	// test if any of the bits are true in both

		// Info
		inline int		totalBits() const;	// return number of bits for this type
		inline int		totalSet() const;	// return number of bits set to true
		
		// Debug
		inline string	toString() const;
		inline string	toString(int numBits) const;

		// Constructors / Destructor
		explicit BitField() : mBits(0) {}
		explicit BitField(const T set) : mBits(set) {}
		explicit BitField(const BitField &bf) : mBits(bf.mBits) {}
		~BitField() {}
};


///// INLINE FUNCTIONS /////

// Operators

template <class T>
inline BitField<T> & BitField<T>::operator=(const BitField<T> &bf)
{
	mBits = bf.bits;
	return (*this);
}


template <class T>
inline BitField<T> & BitField<T>::operator=(const T set)
{
	mBits = set;
	return (*this);
}

// Accessors

template <class T>
inline T BitField<T>::getBit(const int index) const
{
	return ((mBits & (1 << index)) >> index);
}

template <class T>
inline bool BitField<T>::testBit(const int index) const
{
	return (mBits & (1 << index));
}

template <class T>
inline bool BitField<T>::testFlags(const T test) const
{
	return ((value & test) == test);
}

template <class T>
inline bool BitField<T>::testAny(const T test) const
{
	return (value & test);
}

// Info

template <class T>
inline int BitField<T>::totalBits() const
{
	return (sizeof(T) << 3);
}

template <class T>
inline int BitField<T>::totalSet() const
{
	int count = 0;
	int total = totalBits();
	T testValue = value;
	for (int i = total; i > 0; --i) {
		count += (testValue & 1);
		testValue >>= 1;
	}
	return count;
}

// Debug
template <class T>
inline string BitField<T>::toString() const
{
	ostringstream oss;
	//int highest = math.highestBitSet(mBits);
	for (int b = 0; b < totalBits(); ++b) oss << getBit(b);
	return oss.str();
}

template <class T>
inline string BitField<T>::toString(int numBits) const
{
	ostringstream oss;
	//int highest = math.highestBitSet(mBits);
	for (int b = 0; b < (numBits<=totalBits()?numBits:totalBits()); ++b) oss << getBit(b);
	return oss.str();
}