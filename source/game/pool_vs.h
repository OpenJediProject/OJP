//This is technically supposed to be a dynamically allocated array structure but I've implimented it statically.

namespace ratl
{
	template <class dataType, int maxSize>
	class pool_vs	
	{
	public:
		pool_vs()
		{//sets all elements to be not in use.
			for(int i = 0; i < maxSize; i++)
			{
				inuse[i] = false;
			}
		}

		//substript operator
		dataType& operator[](int i)
		{
			return poolData[i];
		}

		int alloc()
		{//allocates a empty slot in the pool and returns the index of it.  This doesn't do a full check.
			for(int i = 0; i < maxSize; i++)
			{
				if(!inuse[i])
				{//empty element, make it be in use and then return the index of it.
					inuse[i] = true;
					return i;
				}
			}

			//default case, this shouldn't happen
			assert(0);
			return 0;
		}

		bool full()
		{//checks to see if the pool is full.
			for(int i = 0; i < maxSize; i++)
			{
				if(!inuse[i])
				{
					return false;
				}
			}

			return true;
		}

		void free(int index)
		{//frees the data at the given pool index
			inuse[index] = false;
			memset( &poolData[index], 0, sizeof(dataType) );  //clear that memory area.
		}

	private:
		dataType poolData[maxSize];  //actual data in pool
		bool inuse[maxSize];  //inuse flags
	};
}