
//header file protector
#ifndef _BITS_VS_H_
#define _BITS_VS_H_

#include <bitset>

namespace ratl
{
	template <int bitSize>
	class bits_vs : public std::bitset<bitSize>
	{
	public:
		bool get_bit(int index) const
		{//gives the current setting of a given array
			return operator[](index);
		}

		void clear_bit(int index)
		{//clears an individual bit on the bit array.
			reset(index);
		}

		void set_bit(int index)
		{//sets the inputted bit to be true.
			set(index);
		}

		void clear()
		{//clears out the entire bit array.
			reset();
		}
	};
}

#endif