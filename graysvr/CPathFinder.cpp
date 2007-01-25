#include "../graysvr/CPathFinder.h"

unsigned long CPathFinder::Heuristic(CPathFinderPointRef& Pt1,CPathFinderPointRef& Pt2)
{
	return 10*(abs(Pt1.m_Point->m_x - Pt2.m_Point->m_x) + abs(Pt1.m_Point->m_y - Pt2.m_Point->m_y));
}

void CPathFinder::GetChildren(CPathFinderPointRef& Point, list<CPathFinderPointRef>& ChildrenRefList )
{
	int RealX = 0, RealY = 0;
	for ( int x = -1; x != 2; ++x)
	{
		for ( int y = -1; y != 2; ++y)
		{
			if ( x == 0 && y == 0 )
				continue;
			RealX = x + Point.m_Point->m_x;
			RealY = y + Point.m_Point->m_y;
			if ( RealX < 0 || RealY < 0 || RealX > 23 || RealY > 23 )
				continue;
			if ( m_Points[RealX][RealY].m_Walkable == false )
				continue;
			if ( x != 0 && y != 0 ) // Diagonal
			{
				//Don't go diagonally between two non walkable blocks
				if ( x == -1 && y == -1 && RealX > 0 && RealY > 0)
					if ( m_Points[RealX-1][RealY].m_Walkable == false && m_Points[RealX][RealY-1].m_Walkable == false )
						continue;
				if ( x == 1 && y == 1 && RealY < 23 && RealX < 23 )
					if ( m_Points[RealX+1][RealY].m_Walkable == false && m_Points[RealX][RealY+1].m_Walkable == false )
						continue;
				if ( x == -1 && y == 1 && RealX > 0 && RealY < 23 )
					if ( m_Points[RealX-1][RealY].m_Walkable == false && m_Points[RealX][RealY+1].m_Walkable == false )
						continue;
				if ( x == 1 && y == -1 && RealY > 0 && RealX < 23)
					if ( m_Points[RealX+1][RealY].m_Walkable == false && m_Points[RealX][RealY-1].m_Walkable == false )
						continue;
			}


			CPathFinderPointRef PtRef( m_Points[RealX][RealY] );
			ChildrenRefList.push_back( PtRef  );
		}
	}
}

CPathFinderPoint::CPathFinderPoint() : FValue(0), GValue(0), HValue(0), m_Parent(0)
{
	ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = 0;
	m_y = 0;
	m_z = 0;
}

CPathFinderPoint::CPathFinderPoint(const CPointMap& pt) : FValue(0), GValue(0), HValue(0), m_Parent(0)
{
	ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
}


bool CPathFinderPoint::operator < (const CPathFinderPoint& pt) const
{
	//ADDTOCALLSTACK("CPathFinderPoint::operator <");
	return (FValue < pt.FValue);
}

const CPathFinderPoint* CPathFinderPoint::GetParent() const
{
	ADDTOCALLSTACK("CPathFinderPoint::GetParent");
	return m_Parent;
}

void CPathFinderPoint::SetParent(CPathFinderPointRef& pt)
{
	ADDTOCALLSTACK("CPathFinderPoint::SetParent");
	m_Parent = pt.m_Point;
}

CPathFinder::CPathFinder(CChar *pChar, CPointMap ptTarget)
{
	ADDTOCALLSTACK("CPathFinder::CPathFinder");
	EXC_TRY("CPathFinder Constructor");

	CPointMap pt;
	int i;

	m_pChar = pChar;
	m_Target = ptTarget;

	pt = m_pChar->GetTopPoint();
	m_RealX = pt.m_x - PATH_SIZE/2;
	m_RealY = pt.m_y - PATH_SIZE/2;
	m_Target.m_x -= m_RealX;
	m_Target.m_y -= m_RealY;

	EXC_SET("FillMap");

	FillMap();

	EXC_CATCH;
}

CPathFinder::~CPathFinder()
{
	ADDTOCALLSTACK("CPathFinderPoint::~CPathFinderPoint");
}

int CPathFinder::FindPath() //A* algorithm
{
	ADDTOCALLSTACK("CPathFinder::FindPath");


	int X = m_pChar->GetTopPoint().m_x - m_RealX;
	int Y = m_pChar->GetTopPoint().m_y - m_RealY;

	if ( X < 0 || Y < 0 || X > 23 || Y > 23 || (m_pChar == 0) )
	{
		//Too far away
		Clear();
		return PATH_NONEXISTENT;
	}

	CPathFinderPointRef Start(m_Points[X][Y]); //Start point
	CPathFinderPointRef End(m_Points[m_Target.m_x][m_Target.m_y]); //End Point

	ASSERT(Start.m_Point);
	ASSERT(End.m_Point);

	Start.m_Point->GValue = 0;
	Start.m_Point->HValue = Heuristic(Start, End);
	Start.m_Point->FValue = Start.m_Point->HValue;

	m_Opened.push_back( Start );

	list<CPathFinderPointRef> Children;
	CPathFinderPointRef Child, Current;
	deque<CPathFinderPointRef>::iterator InOpened, InClosed;

	while ( !m_Opened.empty() )
	{
		std::sort(m_Opened.begin(), m_Opened.end());
		Current = *m_Opened.begin();

		m_Opened.pop_front();
		m_Closed.push_back( Current );

		if ( Current == End )
		{
			CPathFinderPointRef PathRef = Current;
			while ( PathRef.m_Point->GetParent() ) //Rebuild path + save
			{
				PathRef.m_Point = const_cast<CPathFinderPoint*>(PathRef.m_Point->GetParent());
				m_LastPath.push_front( CPointMap(PathRef.m_Point->m_x+m_RealX,PathRef.m_Point->m_y+m_RealY) );
			}
			Clear();
			return PATH_FOUND;
		}
		else
		{
			Children.clear();
			GetChildren( Current, Children );

			while ( !Children.empty() )
			{
				Child = Children.front();
				Children.pop_front();

				InClosed = std::find( m_Closed.begin(), m_Closed.end(), Child );
				InOpened = std::find( m_Opened.begin(), m_Opened.end(), Child );

				if ( InClosed != m_Closed.end() )
					continue;

				if ( InOpened == m_Opened.end() )
				{
					Child.m_Point->SetParent( Current );
					Child.m_Point->GValue = Current.m_Point->GValue;

					if ( Child.m_Point->m_x == Current.m_Point->m_x || Child.m_Point->m_y == Current.m_Point->m_y )
						Child.m_Point->GValue += 10; //Not diagonal
					else
						Child.m_Point->GValue += 14; //Diagonal

					Child.m_Point->HValue = Heuristic( Child, End );
					Child.m_Point->FValue = Child.m_Point->GValue + Child.m_Point->HValue;
					m_Opened.push_back( Child );
					//sort ( m_Opened.begin(), m_Opened.end() );
				}
				else
				{
					if ( Child.m_Point->GValue < Current.m_Point->GValue )
					{
						Child.m_Point->SetParent( Current );
						if ( Child.m_Point->m_x == Current.m_Point->m_x || Child.m_Point->m_y == Current.m_Point->m_y )
							Child.m_Point->GValue += 10;
						else
							Child.m_Point->GValue += 14;
						Child.m_Point->FValue = Child.m_Point->GValue + Child.m_Point->HValue;
						//sort ( m_Opened.begin(), m_Opened.end() );
					}
				}
			}
		}
	}


	Clear();
	return PATH_NONEXISTENT;
}

void CPathFinder::Clear()
{
	ADDTOCALLSTACK("CPathFinder::Clear");
	m_Target = CPointMap(0,0);
	m_pChar = 0;
	m_Opened.clear();
	m_Closed.clear();
	m_RealX = 0;
	m_RealY = 0;
}

void CPathFinder::FillMap()
{
	ADDTOCALLSTACK("CPathFinder::FillMap");
	int x, y;
	CRegionBase	*pArea;
	CPointMap	pt, ptChar;

	EXC_TRY("FillMap");
	pt = ptChar = m_pChar->GetTopPoint();

	for ( x = 0 ; x != PATH_SIZE; ++x )
	{
		for ( y = 0; y != PATH_SIZE; ++y )
		{
			pt.m_x = x + m_RealX;
			pt.m_y = y + m_RealY;
			if (IsSetEF( EF_NewPositionChecks ))
				pArea = m_pChar->CanMoveWalkTo(pt, true, true, ptChar.GetDir(pt), true);
			else
				pArea = m_pChar->CanMoveWalkTo(pt, true, true, ptChar.GetDir(pt));
			m_Points[x][y].m_Walkable = pArea ? PATH_WALKABLE : PATH_UNWALKABLE;
			m_Points[x][y] = CPointMap(x,y);

			//DEBUG_ERR(( "[%i:%i:%i]",m_Points[x][y].GetPoint()->m_x,m_Points[x][y].GetPoint()->m_y,m_Points[x][y].m_Walkable ));
		}
	}

	//DEBUG_ERR(( "TARGET: [%i:%i]\n", m_Target.m_x, m_Target.m_y ));

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_DEBUG_END;
}

CPointMap CPathFinder::ReadStep(size_t Step)
{
	ADDTOCALLSTACK("CPathFinder::ReadStep");
	return m_LastPath[Step];
}

size_t CPathFinder::LastPathSize()
{
	ADDTOCALLSTACK("CPathFinder::LastPathSize");
	return m_LastPath.size();
}

void CPathFinder::ClearLastPath()
{
	ADDTOCALLSTACK("CPathFinder::ClearLastPath");
	m_LastPath.clear();
}

