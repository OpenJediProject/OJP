#ifndef _GRAPH_VS_H
#define	_GRAPH_VS_H

#include "vector_vs.h"
#include "user_vs.h"

namespace ragl
{
	template <class Node, int nodeNum, class Edge, int edgeNum, int edgesPerNode>
	class graph_vs
	{
	public:
		Edge edgeData[edgeNum];
		Node nodeData[nodeNum];

		//returns the edge for a given index
		Edge& get_edge(int index);

		//returns the node for a given index
		Node& get_node(int index)
		{
			return nodeData[index];
		}
			

		//get the node index of the edge that plans these two nodes.
		//node indexes for edges are negative!
		int get_edge_across( const int nodeIndexA, const int nodeIndexB );

		//I assume this blanks out the class object.
		void clear(void);

		//I think this returns the number of nodes in the system or something.
		int size_nodes(void);

		//return the beginning node in the graph?
		Node& nodes_begin(void);

		//return the end node in the graph?
		Node& nodes_end(void);

		//return first node in graph?
		Edge& edges_begin(void);

		//returns first open edge position in graph.
		Edge& edges_end(void);

		//remove the edge that connects these nodes together
		void remove_edge(int nodeA, int nodeB);

		//I assume this reports if the given node has neighbors or not.
		bool node_has_neighbors(int node);

		//add a new node to the graph.
		int insert_node(const Node insertNode);

		//associates a given edge with the nodes it connects.
		void connect_node(const Edge, int nodeA, int nodeB);

		void astar();

		//returns the number of edges in the graph system.
		int size_edges();

		template <int nodesPerCell, int xcellNum, int ycellNum>
		class cells
		{
		public:
			//datatype for a cell's 
			typedef ratl::vector_vs <int, nodesPerCell> TCellNodes;

			struct SCell
			{
				//I don't actually know the size requirement for these
				TCellNodes mNodes;
				TCellNodes mEdges;
				float minx;
				float miny;
				float maxx;
				float maxy;
				bool inuse;
			};

			//constructor using graph_vs as input
			cells(graph_vs <Node, nodeNum, Edge, edgeNum, edgesPerNode> a)
			{//not sure that this needs to do anything right now other than init the array's inuse things.
				for(int x = 0; x < xcellNum; x++)
				{
					for(int y = 0; y < ycellNum; y++)
					{
						cellData[x][y].inuse = false;
					}
				}

				blankCell.inuse = false;

				SIZEX = 0;
				SIZEY = 0;

				cellGraph = &a;
			}

			
			void clear(void)
			{//blanks out the cell data.
				for(int x = 0; x < xcellNum; x++)
				{
					for(int y = 0; y < ycellNum; y++)
					{
						cellData[x][y].inuse = false;
					}
				}

				SIZEX = 0;
				SIZEY = 0;
			}

			//fills the cells with nodes with cellRange being the distance range possible for each node cell.
			void fill_cells_nodes(int cellRange)
			{
				for(graph_vs::TNodes::iterator atIter=cellGraph->nodes_begin(); 
					atIter!=cellGraph->nodes_end(); atIter++)
				{//scan thru all the nodes one at a time and add them to the cell system
					get_cell(*atIter->mPoint[0], *atIter->mPoint[1]);

				}
			}

			//return the cell for the given x/y coordinate
			SCell& get_cell(float x, float y)
			{
				for(int i = 0; i < xcellNum; i++)
				{
					for(int j = 0; j < ycellNum; j++)
					{
						if(PointInCell(cellData[i][j], x, y))
						{//point is inside this cell
							return cellData[i][j];
						}
					}
				}

				//not found, return blank cell.
				return blankCell;
			}

			//Add this node to the cell list
			void expand_bounds(int nodeHandle)
			{//expands the maximum positive bounds of the cells by the location of this node
				//assumes that nodeHandle is always valid
				Node& currentNode = cellGraph->get_node(nodeHandle);

				if( currentNode.mPoint[0] > SIZEX )
				{//expand x
					SIZEX = currentNode.mPoint[0];
				}

				if( currentNode.mPoint[1] > SIZEY )
				{//expand y
					SIZEY = currentNode.mPoint[1];
				}
			}

			//fills the cells in with edges.
			void fill_cells_edges(int cellRange);

			
			void get_cell_upperleft( float x, float y, float& x1, float& y1 )
			{//returns the upper left corner of the cell that contains x/y (x1/y1)
				//does nothing if x/y isn't in a cell
				for(int i = 0; i < xcellNum; i++)
				{
					for(int j = 0; j < ycellNum; j++)
					{
						if(PointInCell(cellData[i][j], x, y))
						{//point is inside this cell
							x1 = cellData[i][j].minx;
							y1 = cellData[i][j].maxy;
						}
					}
				}
			}

			void get_cell_lowerright( float x, float y, float& x1, float& y1 )
			{//returns the lower right corner of the cell that contains x/y (x1/y1)				
				for(int i = 0; i < xcellNum; i++)
				{
					for(int j = 0; j < ycellNum; j++)
					{
						if(PointInCell(cellData[i][j], x, y))
						{//point is inside this cell
							x1 = cellData[i][j].maxx;
							y1 = cellData[i][j].miny;
						}
					}
				}
			}

			//these are the maximum current domain that this cell system covers.
			//(only counts positive direction on axis)
			int SIZEX;
			int SIZEY;

		private:
			//the actual cell data
			SCell cellData[xcellNum][ycellNum];

			//this cell is a blank cell that is used for when we need to return a SCell& but don't 
			//have a valid cell to return.
			SCell blankCell;

			//pointer to the graph_vs that this cell system covers
			graph_vs *cellGraph;

			bool PointInCell (SCell& cell, float pointx, float pointy)
			{//checks to see if a point is inside this particular cell
				if(cell.inuse && pointx >= cell.minx 
							&& pointy >= cell.miny 
							&& pointx <= cell.maxx 
							&& pointy <= cell.maxy)
				{
					return true;
				}

				return false;
			}
		};


		typedef user_vs user;

		class search
		{
		public:
			//paths appear to be stored in reverse (IE, path_begin points to 


			int mStart;  //index of the starting node
			int mEnd;

			//sets path pointer to the beginning of the path.
			void path_begin();

			//checks to see if the path pointer is at the end of the path.
			bool path_end();

			//increments the path pointer down the path.
			void path_inc();

			//returns the node index that the path pointer is currently pointing at.
			int path_at();

			//I assume this just returns weither or not we successfully found a path.
			bool success();
		};

		//get the index number for a given edge
		int edge_index( Edge desiredEdge);

		class TNodes;
		friend class TNodes;

		class TNodes
		{
		public:
			// CLASS iterator
			class iterator;
			friend class iterator;

			class iterator
			{	// iterator for mutable _Tree
			public:

				Node* currentNode;

				iterator()
				{	// construct with null node pointer
				}

				iterator(Node startNode);

				bool operator==(Node b);

				bool operator!=(Node b);

				Node& operator*()
				{	// return designated value
					return ((*currentNode));
				}

				Node* operator->()
				{//return pointer to Node object
					return currentNode;
				}
				/*
				Node operator*()
				{	// return designated value
					return ((reference)**(iterator *)this);
				}

				
				mapValue* operator->() const
				{	// return pointer to class object
					return (&**currentPair->value);
				}
				*/
				iterator& operator++();
				/*
				{	// preincrement
					++(*(iterator *)this);
					return (*this);
				}
				*/

				iterator operator++(int)
				{	// postincrement
					iterator _Tmp = *this;
					++*this;
					return (_Tmp);
				}

				iterator& operator--()
				{	// predecrement
					--(*(iterator *)this);
					return (*this);
				}

				iterator operator--(int)
				{	// postdecrement
					iterator _Tmp = *this;
					--*this;
					return (_Tmp);
				}

				//returns index of the currentNode
				int index();
			};
		};

		class TEdges;
		friend class TEdges;

		class TEdges
		{
		public:
			// CLASS iterator
			class iterator;
			friend class iterator;

			class iterator
			{	// iterator for mutable _Tree
			public:

				Edge* currentEdge;

				iterator()
				{	// construct with null node pointer
				}

				iterator(Edge startEdge);

				bool operator==(Edge b);

				bool operator!=(Edge b);

				Edge& operator*()
				{	// return designated value
					return ((*currentEdge));
				}

				/*
				reference operator*() const
				{	// return designated value
					return ((reference)**(iterator *)this);
				}
				*/

				/*
				mapValue* operator->() const
				{	// return pointer to class object
					return (&**currentPair->value);
				}
				*/

				iterator& operator++();
				/*
				{	// preincrement
					++(*(iterator *)this);
					return (*this);
				}
				*/

				iterator operator++(int)
				{	// postincrement
					iterator _Tmp = *this;
					++*this;
					return (_Tmp);
				}

				iterator& operator--()
				{	// predecrement
					--(*(iterator *)this);
					return (*this);
				}

				iterator operator--(int)
				{	// postdecrement
					iterator _Tmp = *this;
					--*this;
					return (_Tmp);
				}

				//returns index of the currentEdge
				int index();
			};
		};

		struct neighbors_s
		{
			int mNode;
		};

		typedef struct neighbors_s neighbors_t;

		//don't know the max neighbor limit for this.
		typedef ratl::vector_vs <neighbors_t, nodeNum> TNodeNeighbors;		
		TNodeNeighbors& get_node_neighbors(int nodeIndex);

		void astar(search searchInput, user userInput);

		//this appears to be a debug print thingy, this should probably be outputing to a file or something.
		void ProfilePrint( const char *fmt, ... );

		//spew data contents maybe?
		void ProfileSpew();


	};
}

#endif