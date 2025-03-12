//-----------------------------------------------------------
__inline double Arrondi1( double val )
{
	return floor(val*10 + .5)/10;
}
//-----------------------------------------------------------
__inline double Arrondi2( double val )
{
	return floor(val*100 + .5)/100;
}
//------------------------------------------------------------
__inline double Arrondi4( double val )
{
	return floor(val*10000 + .5)/10000;
}
//------------------------------------------------------------
__inline double ArrondiVar( double val, int prec)
{
	switch (prec)
	{
	case 1:
		return Arrondi1(val);
	case 2:
		return Arrondi2(val);
	default:
		return Arrondi4(val);
	}
}
//------------------------------------------------------------
#define		Type_char		1
#define		Type_int		2
#define		Type_long		3
#define		Type_float		4
#define		Type_double		5

typedef struct _WRITENUMBERSTRUCT {
	int		iSize;
	int		iType;
	union UNKNOWN    // Declare union type
	{
		char   ch;
		int    i;
		long   l;
		float  f;
		double d;
	} value;
	char *pchBuffer;
	BOOL bEuro;
} WRITENUMBERSTRUCT;
//-------------------------------------------------------------
BOOL WriteNumber ( WRITENUMBERSTRUCT *wns );
