/*
 *	horizon.h -- A procedural texture plugin 
 *
 */

/////////////////////////////
// Preprocessor Directives //
/////////////////////////////

#ifndef KM_HORIZON_H
#define KM_HORIZON_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <lwserver.h>
#include <lwrender.h>
#include <lwhandler.h>
#include <lwenviron.h>
#include <lwhost.h>
#include <lwxpanel.h>
#include <com_vecmatquat.h>

#define EMPTY -999999.0
#define TAIL -999998.0

#define ZEN_R   0.0 
#define ZEN_G   40.0/255 
#define ZEN_B   80.0/255

#define SKY_R   120.0/255 
#define SKY_G   180.0/255 
#define SKY_B   240.0/255

#define GND_R   50.0/255 
#define GND_G   40.0/255
#define GND_B   30.0/255

#define NAD_R   100.0/255 
#define NAD_G   80.0/255  
#define NAD_B   60.0/255

#define STAR_R   255.0/255 
#define STAR_G   255.0/255  
#define STAR_B   255.0/255

#define SPACE	0.0


///////////////////////////
// Variables and Structs //
///////////////////////////

static double csc = 1.0, gsc = 2.0, grd = .105;
static double dmin=0.0, dmax=0.0, noise = 0.0;
int seedFlag = 0;
int starFlag = 0;
int gui_map[] = {1};


/* interface stuff ----- */
static LWXPanelFuncs *xpanf;
static LWInstUpdate *lwupdate;
enum { ID_NUMSTARS = 0x8001, ID_RESET, ID_SPACECOLOR, ID_HORIZON, ID_ZENITHCOLOR, ID_SKYCOLOR, ID_SKYSQ, ID_GROUNDSQ, ID_GROUNDCOLOR, ID_NADIRCOLOR };


typedef struct st_HorizonBkgData {
	double	skySq, gndSq;
	double	zenith[3], sky[3], ground[3], nadir[3], star[3], space[3];
	double	pstar, hstar, starrad;

	//UI data
	char desc[ 80 ];
	int numstars;
	int reset;
	int useHorizon;

} HorizonBkgData;

struct st_node {
	double h;
	double p;
	double color;
	double radius;
	struct st_node *left;
	struct st_node *right;
};

struct st_node root;
struct st_node tail;


/////////////////////////
// Function Prototypes //
/////////////////////////

double squeezeColor(double sq, double frac, double *min, double *max, double *rgb);

void horizonDestroy(HorizonBkgData *dat);

LWError horizonCopy(HorizonBkgData *to, HorizonBkgData *from);

LWError horizonNewTime(HorizonBkgData *dat, LWFrame f, LWTime t);

int horizonFlags( HorizonBkgData *dat);

static double angsep( double h1, double p1, double h2, double p2 );

static void vec2hp( LWDVector n, double *h, double *p );

static void sample( HorizonBkgData *dat, double h, double p, double *color, int mode );

XCALL_( static LWError ) Evaluate( HorizonBkgData *dat, LWEnvironmentAccess *access);

static void randomStars( struct st_star *star, int numOfStars );

static void starSample( HorizonBkgData *dat, double h, double p, LWDVector n, double *color, int mode, struct st_node *root, struct st_node *tail, int numOfStars, LWEnvironmentAccess *access );

XCALL_( static LWError ) EvaluateStars( HorizonBkgData *dat, LWEnvironmentAccess *access);

XCALL_(int)HorizonActivate (long version, GlobalFunc *global, LWEnvironmentHandler *local, void *serverData);

void addVectorNode(double h, double p, double color, struct st_node *node, struct st_node *tail);

void putHPVector(double h, double p, double *tree);

int getVectorNode(HorizonBkgData *dat, double th, double tp, double *ccolor, double *ah, double *ap, struct st_node *node, struct st_node *tail);

int testNode(HorizonBkgData *dat, double th, double tp, double ah, double ap);

static double angsep( double h1, double p1, double h2, double p2 );

void freeTree(struct st_node *node, struct st_node *tail);

void star_reset( LWXPanelID panel, int ctrl_id );

struct st_node * makeNode (double heading, double pitch, double color);

XCALL_( static const char * )DescLn( HorizonBkgData *inst );

#endif
