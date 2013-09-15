/*
 *	starrynight.c -- A procedural star plugin 
 *
 *	Copyright 2004 Kevin MacPhail.
 *
 *  Add Anti-aliasing
 *
 *	In hindsight this plug-in would be more useful as a pixel filter
 *	than as an environment shader. Perhaps for the next version.
 *
 *	Modified from horizon.c
 */

#include "starrynight.h"


double squeezeColor(double sq, double frac, double *min, double *max, double *rgb)
{
    double val = 0.0f, c = 0.0f;
    int i;
	val = pow(frac,(1.0/sq));
    for(i=0;i<3;i++)
    {
        c = val*((max[i] - min[i])); // should be >0 ... 
        rgb[i] = (min[i] + c);
    }
    return val;
}


HorizonBkgData *horizonCreate(void *priv, void *cntxt, LWError *err)
{
    HorizonBkgData *dat;
    err = NULL;
    if(dat = malloc(sizeof(HorizonBkgData)))
    {
        memset( dat, 0, sizeof(HorizonBkgData) );
        dat->skySq = 2.0;
        dat->gndSq = 2.0;
        dat->zenith[0] = ZEN_R; dat->zenith[1] = ZEN_G; dat->zenith[2] = ZEN_B;
        dat->sky[0] = SKY_R; dat->sky[1] = SKY_G; dat->sky[2] = SKY_B;
        dat->ground[0] = GND_R; dat->ground[1] = GND_G; dat->ground[2] = GND_B;
        dat->nadir[0] = NAD_R; dat->nadir[1] = NAD_G; dat->nadir[2] = NAD_B;
		dat->star[0] = STAR_R; dat->star[1] = STAR_G; dat->star[2] = STAR_B; 
		dat->pstar = -PI/8.0; dat->hstar = 0.0; dat->starrad = 0.0015;
		dat->space[0] = SPACE; dat->space[1] = SPACE; dat->space[2] = SPACE;

		//UI data
		dat->numstars = 1000;
		dat->reset = 0;

    }
    return dat;
}


void horizonDestroy(HorizonBkgData *dat)
{
    if(dat)
        free(dat);
}


LWError horizonCopy(HorizonBkgData *to, HorizonBkgData *from)
{
    if(to && from)
        *to = *from;
    return NULL;
}


LWError horizonNewTime(HorizonBkgData *dat, LWFrame f, LWTime t)
{
    // Here, Now, Time Stands Still
    // Animated Backdrop Sky?
    // Catch This Time of Day
    return NULL;
}


int horizonFlags( HorizonBkgData *dat)
{
    return 0;
}


static void vec2hp( LWDVector n, double *h, double *p )
{
	*p = asin( -n[ 1 ] );

	if ( 1.0 - fabs( n[ 1 ] ) > 1e-5 ) {

		// not straight up or down
		*h = acos( n[ 2 ] / cos( *p ));

		if ( n[ 0 ] < 0.0 ) *h = 2 * PI - *h;
	}
	else *h = 0;
}


static void sample( HorizonBkgData *dat, double h, double p, double *color, int mode )
{
	if ( p >= 0.0 ) {
		color[ 0 ] = dat->ground[ 0 ];
		color[ 1 ] = dat->ground[ 1 ];
		color[ 2 ] = dat->ground[ 2 ];
		//return;
	}
	else {
		color[ 0 ] = dat->sky[ 0 ];
		color[ 1 ] = dat->sky[ 1 ];
		color[ 2 ] = dat->sky[ 2 ];
	}
}


static void starSample( HorizonBkgData *dat, double h, double p, LWDVector n, double *color, int mode,
						struct st_node *rroot, struct st_node *ttail, int numOfStars,
						LWEnvironmentAccess *access)
{
	int instar;
	double ccolor;
	double lum;
	double hh,pp;
	double f;
	

    f = (access->dir[1]);

	//Determine mode
	if ( mode == EHMODE_PREVIEW ){
		instar = 0;
	}
	else {
		instar = getVectorNode(dat, h, p, &ccolor, &hh, &pp, rroot, ttail);
	}

	//Below Horizon
	if ( p >= 0.0 ) {
		if ( instar && !dat->useHorizon) {
			color[ 0 ] = ccolor/255;
			color[ 1 ] = ccolor/255;
			color[ 2 ] = ccolor/255;
		}
		else if (dat->useHorizon) {
			squeezeColor(dat->gndSq, f, dat->ground, dat->nadir, (access->color));
		}
		else {
			color[ 0 ] = dat->space[0];
			color[ 1 ] = dat->space[1];
			color[ 2 ] = dat->space[2];
		}
		return;
	}

	//Above Horizon
	if ( instar ) {
		if (dat->useHorizon) {
			squeezeColor(dat->skySq, f, dat->sky, dat->zenith, (access->color));

			//Luminence calculation, using perceived color rather than just an average
			//lum = (color[ 0 ] + color[ 1 ] + color[ 2 ])/3;
			lum = 0.02125*color[ 0 ] + 0.7154*color[ 1 ] + 0.0721*color[ 2 ];

			if (lum < ccolor/255) {
				color[ 0 ] = ccolor/255;
				color[ 1 ] = ccolor/255;
				color[ 2 ] = ccolor/255;
			}
		}
		else {
			color[ 0 ] = ccolor/255;
			color[ 1 ] = ccolor/255;
			color[ 2 ] = ccolor/255;
		}	
	}
	else if (dat->useHorizon) {
			squeezeColor(dat->skySq, f, dat->sky, dat->zenith, (access->color));
	}
	else {
		color[ 0 ] = dat->space[0];
		color[ 1 ] = dat->space[1];
		color[ 2 ] = dat->space[2];
	}
	return;
}


struct st_node * makeNode (double heading, double pitch, double color) {

	struct st_node *node;

	node = malloc(sizeof(struct st_node));
	
	node->h = heading;	
	node->p = pitch;
	node->color = color;
	
	return node;
}


int testNode (HorizonBkgData *dat, double th, double tp, double hh, double pp){

	if (angsep( th, tp, hh, pp ) < dat->starrad)

		return 1;

	else

		return 0;
}

static double angsep( double h1, double p1, double h2, double p2 )
{
	double cd;
   
	if( h1 == h2 && p1 == p2 )
		return 0.0;
      
	cd = cos( p1 ) * cos( p2 ) * cos( fabs( h2 - h1 ))+ sin( p1 ) * sin( p2 );
   
	// catch small roundoff errors
   	if( cd >  1.0 ) cd =  1.0;
	if( cd < -1.0 ) cd = -1.0;
   
	return acos( cd );
}


// This function assumes the root and tail are initialized prior
// to being called and that the root points to the tail
// Walk the tree until we find the tail, then insert a new node

void addVectorNode (double h, double p, double color, struct st_node *node, struct st_node *tail) {

	int level = 1; // Level of Root

	double pHigh = (double)(PI/2.0);
	double pLow = (double)(-PI/2.0);
	double hHigh = (double)(PI*2.0);
	double hLow = 0.0;

	double hMid = 0.0;
	double pMid = 0.0;

	while(1) { 

		pMid = (double)(pHigh - (pHigh - pLow) / 2.0);
		hMid = (double)(hHigh - (hHigh - hLow) / 2.0);

		if (node->h == EMPTY && node->p == EMPTY) { // First element added

			node->h = h;
			node->p = p;
			node->color = color;

			break;
		}

		// Step to the next node
		else {

			if (level%2) { // Odd level -> Pitch
	
				if (p < pMid) {
	
					pHigh = pMid;
					if (node->left->p == TAIL ) {
						node->left = makeNode(h, p, color); // Make left pitch branch
						node->left->left = tail;
						node->left->right = tail;
						break;
					}
					else {
						node = node->left; // Move to left pitch branch
					}
				}
				else {
					
					pLow = pMid;
					if (node->right->p == TAIL ) {
						node->right = makeNode(h, p, color); // Make right pitch branch
						node->right->left = tail;
						node->right->right = tail;
						break;
					}
					else {
						node = node->right; // Move to right pitch branch
					}
				}
			}
			else { // Even level -> Heading
	
				if (h < hMid) {

					hHigh = hMid;
					if (node->left->h == TAIL ) {
						node->left = makeNode(h, p, color); // Make left pitch branch
						node->left->left = tail;
						node->left->right = tail;
						break;
					}
					else {
						node = node->left; // Move to left pitch branch
					}
				}
				else {

					hLow = hMid;
					if (node->right->h == TAIL ) {
						node->right = makeNode(h, p, color); // Make right pitch branch
						node->right->left = tail;
						node->right->right = tail;
						break;
					}
					else {
						node = node->right; // Move to right pitch branch
					}
				}
			}
			level++;
			continue;
		}

	}
}


int getVectorNode (HorizonBkgData *dat, double th, double tp, double *ccolor, double *ah, double *ap, struct st_node *node, struct st_node *tail){

	int level = 1; // Level of Root

	double pHigh = (double)(PI/2.0);
	double pLow = (double)(-PI/2.0);
	double hHigh = (double)(PI*2.0);
	double hLow = 0.0;

	double hMid = 0.0;
	double pMid = 0.0;

	while(1) { 

		pMid = pHigh - (double)((pHigh - pLow) / 2.0);
		hMid = hHigh - (double)((hHigh - hLow) / 2.0);

		// Test the first node for emptiness
		if (node->h == EMPTY && node->p == EMPTY) { // First element added
	
			return 0; // No elements in data structure

		}

		// The test for matching values
		else if (testNode(dat, th, tp, node->h, node->p)) {

			*ah = node->h;
			*ap = node->p;
			*ccolor = node->color;

			return 1;
		}

		// Step to the next node
		else {

			if (level%2) { // Odd level -> Pitch
	
				if (tp < pMid) {
	
					pHigh = pMid;

					if (testNode(dat, th, tp, node->h, node->p)) {
						*ah = node->h;
						*ap = node->p;
						*ccolor = node->color;
						return 1;
					}
					else {
						if (node->left == tail)
							return 0; // end of branch
						else
							node = node->left; // Move to left pitch branch
					}
				}
				else {
					
					pLow = pMid;

					if (testNode(dat, th, tp, node->h, node->p)) {
						*ah = node->h;
						*ap = node->p;
						*ccolor = node->color;
						return 1;
					}
					else {
						if (node->right == tail)
							return 0; // end of branch
						else
							node = node->right; // Move to right pitch branch
					}
				}
			}
			else { // Even level -> Heading
	
				if (th < hMid) {
	
					hHigh = hMid;

					if (testNode(dat, th, tp, node->h, node->p)) {
						*ah = node->h;
						*ap = node->p;
						*ccolor = node->color;
						return 1;
					}
					else {
						if (node->left == tail)
							return 0; // end of branch
						else
							node = node->left; // Move to left pitch branch
					}
				}
				else {
					
					hLow = hMid;

					if (testNode(dat, th, tp, node->h, node->p)) {
						*ah = node->h;
						*ap = node->p;
						*ccolor = node->color;
						return 1;
					}
					else {
						if (node->right == tail)
							return 0; // end of branch
						else
							node = node->right; // Move to right pitch branch
					}
				}
			}
			level++;
			continue;
		}
	}
}


/*
======================================================================
freeTree()

Deletes the existing binary tree structure.
====================================================================== */
void freeTree(struct st_node *node, struct st_node *tail){

	if(node->left != tail) {
		freeTree( node->left, tail);
	}
	if(node->right != tail) {
		freeTree( node->right, tail);
	}	
	if(node != &root)
		free(node);
	return;
}


/*
======================================================================
EvaluateStars()

Handler callback.  This is where the stars first start out.
====================================================================== */

XCALL_( static LWError )
EvaluateStars( HorizonBkgData *dat, LWEnvironmentAccess *access)
{
	int i,j;
	double color = 0.0;
	double vector[3];
	double h, p;

	//Create Random Stars
	if (!starFlag) {

		root.h = EMPTY; root.p = EMPTY; root.left = &tail; root.right = &tail;
		tail.h = TAIL; tail.p = TAIL;

		for (i=0; i<dat->numstars; i++) {
			for (j=0; j<3; j++) {
				vector[j] = ((double)rand()/(RAND_MAX+1.0))*2.0-1.0;
			}
			LWVEC_dnormalize ( vector );
			vec2hp( vector, &h, &p );

			if (dat->useHorizon)
				color = (double)(rand()/(RAND_MAX+1.0))*100+155;
			else 
				color = (double)(rand()/(RAND_MAX+1.0))*255;

			addVectorNode(h, p, color, &root, &tail);
		}
		starFlag = 1;
	}

	switch ( access->mode )
	{
		case EHMODE_PREVIEW:
			//sample( dat, access->h[ 0 ], access->p[ 0 ], access->colRect[ 0 ], access->mode );
			//sample( dat, access->h[ 0 ], access->p[ 1 ], access->colRect[ 1 ], access->mode );
			//sample( dat, access->h[ 1 ], access->p[ 0 ], access->colRect[ 2 ], access->mode );
			//sample( dat, access->h[ 1 ], access->p[ 1 ], access->colRect[ 3 ], access->mode );
			break;

		case EHMODE_REAL:
			vec2hp( access->dir, &h, &p );
			starSample( dat, h, p, access->dir, access->color, access->mode, &root, &tail, dat->numstars, access );
			break;

		default:
			break;
	}

	return NULL;
}


/*
======================================================================
StarryNightActivate()

StarryNight activation function.  Check the version and fill in the
callback fields of the handler structure.
====================================================================== */

XCALL_(int)StarryNightActivate (
    long                     version,
    GlobalFunc              *global,
    LWEnvironmentHandler     *local,
    void                    *serverData)
{

    XCALL_INIT;
    if(version != LWENVIRONMENT_VERSION)
        return AFUNC_BADVERSION;

    local->inst->create = horizonCreate;
    local->inst->destroy = horizonDestroy;
    local->inst->copy = horizonCopy;
    local->rend->newTime = horizonNewTime;
	local->inst->descln   = DescLn;
	local->evaluate = EvaluateStars;
    local->flags = horizonFlags;

    return AFUNC_OK;
}


/*
======================================================================
DescLn()

Handler callback.  Write a one-line text description of the instance
data.  Since the string must persist after this is called, it's part
of the instance.
====================================================================== */

XCALL_( static const char * )
DescLn( HorizonBkgData *inst )
{
	sprintf(inst->desc, "StarryNight: %d Stars", inst->numstars);

	return inst->desc;
}


/*
======================================================================
ui_get()

Xpanels callback for LWXP_VIEW panels.  Returns a pointer to the data
for a given control value.
====================================================================== */

void *ui_get( HorizonBkgData *inst, unsigned long vid )
{
	switch ( vid ) {
		case ID_NUMSTARS:		return &inst->numstars;
		case ID_RESET:			return &inst->reset;
		case ID_SPACECOLOR:		return &inst->space;
		case ID_HORIZON:		return &inst->useHorizon;
		case ID_ZENITHCOLOR:	return &inst->zenith;
		case ID_SKYCOLOR:		return &inst->sky;
		case ID_SKYSQ:			return &inst->skySq;
		case ID_GROUNDSQ:		return &inst->gndSq;
		case ID_GROUNDCOLOR:	return &inst->ground;
		case ID_NADIRCOLOR:		return &inst->nadir;
		default:				return NULL;
	}
}


/*
======================================================================
ui_set()

Xpanels callback for LWXP_VIEW panels.  Store a value in our instance
data.
====================================================================== */

int ui_set( HorizonBkgData *inst, unsigned long vid, void *value )
{
	double *d = ( double * ) value;
	int *i = (int *) value;

	switch ( vid ) {

		case ID_NUMSTARS:
			inst->numstars = *i;
			break;
		
		case ID_RESET:
			inst->reset = *i;
			break;

		case ID_SPACECOLOR:
			inst->space[0] = d[0];
			inst->space[1] = d[1];
			inst->space[2] = d[2];
			break;

		case ID_HORIZON:
			inst->useHorizon = *i;
			break;

		case ID_ZENITHCOLOR:
			inst->zenith[0] = d[0];
			inst->zenith[1] = d[1];
			inst->zenith[2] = d[2];
			break;

		case ID_SKYCOLOR:
			inst->sky[0] = d[0];
			inst->sky[1] = d[1];
			inst->sky[2] = d[2];
			break;

		case ID_SKYSQ:
			inst->skySq = *d;
			break;

		case ID_GROUNDSQ:
			inst->gndSq = *d;
			break;

		case ID_GROUNDCOLOR:
			inst->ground[0] = d[0];
			inst->ground[1] = d[1];
			inst->ground[2] = d[2];
			break;

		case ID_NADIRCOLOR:
			inst->nadir[0] = d[0];
			inst->nadir[1] = d[1];
			inst->nadir[2] = d[2];
			break;
		
		default:
			return 0;
   }

   return 1;
}


/*
======================================================================
ui_chgnotify()

XPanel callback.  XPanels calls this when an event occurs that affects
the value of one of your controls.  We use the instance update global
to tell Layout that our instance data has changed.
====================================================================== */

void ui_chgnotify( LWXPanelID panel, unsigned long cid, unsigned long vid,
   int event )
{
   void *dat;

   if ( event == LWXPEVENT_VALUE )
      if ( dat = xpanf->getData( panel, 0 ))
         lwupdate( LWENVIRONMENT_HCLASS, dat );
}


/*
======================================================================
ui_destroynotify()

Xpanels callback.  This is called after the interface is destroyed.
The argument is the pointer passed to LWXPanelFuncs->setData() for
control ID 0, in this case our instance data.
====================================================================== */

static void ui_destroynotify( HorizonBkgData *inst )
{
   //if ( shelff && inst->psclient ) {
   //   shelff->unsubscribe( inst->psclient );
   //   inst->psclient = NULL;
   //}
}


/*
======================================================================
star_reset()

Xpanels callback.  This is called when the reset button is called.
The current data structure is deleted and the starFlag reset to 0.
====================================================================== */

void star_reset( LWXPanelID panel, int ctrl_id )
{
	if(!starFlag) return;
	starFlag = 0;
	freeTree( &root, &tail);
	root.left = &tail; root.right = &tail;
}


/*
======================================================================
get_panel()

Create and initialize an LWXP_VIEW panel.  Called by Interface().
====================================================================== */

//LWXPanelID get_panel( Blotch *inst )
LWXPanelID get_panel( HorizonBkgData *inst )
{
   static LWXPanelControl xctl[] = {
      { ID_NUMSTARS,   "Number of Stars",   "integer"  },
      { ID_RESET,   "Reset Stars",   "vButton"  },
	  { ID_SPACECOLOR,   "Space Color",   "color"  },
	  { ID_HORIZON, "Use Horizon", "iBoolean" },
	  { ID_ZENITHCOLOR,   "Zenith Color",   "color"  },
	  { ID_SKYCOLOR,   "Sky Color",   "color"  },
	  { ID_SKYSQ,   "Sky Squeeze",   "float"  },
	  { ID_GROUNDSQ,   "Ground Squeeze",   "float"  },
	  { ID_GROUNDCOLOR,   "Ground Color",   "color"  },
	  { ID_NADIRCOLOR,   "Nadir Color",   "color"  },
      { 0 }
   };
   static LWXPanelDataDesc xdata[] = {
      { ID_NUMSTARS,   "Number of Stars",   "integer"  },
	  { ID_RESET,   "Reset Stars",   "vButton"  },
	  { ID_SPACECOLOR,   "Space Color",   "color"  },
	  { ID_ZENITHCOLOR,   "Zenith Color",   "color"  },
	  { ID_SKYCOLOR,   "Sky Color",   "color"  },
	  { ID_SKYSQ,   "Sky Squeeze",   "float"  },
	  { ID_GROUNDSQ,   "Ground Squeeze",   "float"  },
	  { ID_GROUNDCOLOR,   "Ground Color",   "color"  },
	  { ID_NADIRCOLOR,   "Nadir Color",   "color"  },
	  { ID_HORIZON, "Use Horizon", "integer" },
      { 0 }
   };
   static LWXPanelHint xhint[] = {
	  XpLABEL( 0, "Starry Night v1.0" ),
      XpBUTNOTIFY( ID_RESET, star_reset ),
      XpCHGNOTIFY( ui_chgnotify ),
      XpDESTROYNOTIFY( ui_destroynotify ),
	  XpENABLE_MAP_(ID_HORIZON, gui_map),
	  XpH(ID_SPACECOLOR),
	  XpH(0),
	  XpENABLE_(ID_HORIZON),
	  XpH(ID_ZENITHCOLOR),
	  XpH(ID_SKYCOLOR),
	  XpH(ID_SKYSQ),
	  XpH(ID_GROUNDSQ),
	  XpH(ID_GROUNDCOLOR),
	  XpH(ID_NADIRCOLOR),
	  XpH(0),
      XpEND
   };

   LWXPanelID panel;

   if ( panel = xpanf->create( LWXP_VIEW, xctl )) {
      xpanf->hint( panel, 0, xhint );
      xpanf->describe( panel, xdata, ui_get, ui_set );
      xpanf->viewInst( panel, inst );
      xpanf->setData( panel, 0, inst );
   }

   return panel;
}


/*
======================================================================
Interface()

The interface activation function.
====================================================================== */

XCALL_( int )
Interface( long version, GlobalFunc *global, LWInterface *local, void *serverData )
{
   if ( version != LWINTERFACE_VERSION ) return AFUNC_BADVERSION;

   lwupdate  = global( LWINSTUPDATE_GLOBAL,        GFUSE_TRANSIENT );
   xpanf     = global( LWXPANELFUNCS_GLOBAL,       GFUSE_TRANSIENT );
   if ( !lwupdate || !xpanf ) return AFUNC_BADGLOBAL;

   local->panel   = get_panel( local->inst );
   local->options = NULL;
   local->command = NULL;

   return local->panel ? AFUNC_OK : AFUNC_BADGLOBAL;
}


//Test, may not be needed... remove ServerTags below
ServerTagInfo ServerTags[] = {
   { "StarryNight", SRVTAG_USERNAME },
   { NULL }
};


ServerRecord ServerDesc[] = {
    { LWENVIRONMENT_HCLASS, "StarryNight", StarryNightActivate, ServerTags },
	{ LWENVIRONMENT_ICLASS, "StarryNight", Interface, ServerTags },
    { NULL }
};
