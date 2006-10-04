namespace ratl
{
	template <class T, int I>
	class array_vs	
	{
	public:
		T arrayData[I];  //data array

		const static int CAPACITY = I; //the total size of this array object.

		T& operator[](int i)
		{
			return arrayData[i];
		}

		void fill( int fillValue ) 
		{//inits the entire array with this value.
			memset(arrayData, fillValue, sizeof(arrayData));
		}
	};
}