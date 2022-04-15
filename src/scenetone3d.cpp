//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006  Jani Vaarala <flame@pygmyprojects.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  --------------------------------------------------------------------------


#include <e32base.h>
#include <e32math.h>
#include "scenetone3d.h"

#define SUBDIVISION 3
#define POWER_MULTIPLIER 0.00000000001f

#define FALLOFF_SPEED 25
#define FALLOFF_MIN   1111

void CScenetone3D::initGLES()
{
#if defined(SCENETONE_INCLUDE_VISUALIZER)
    glClearColor        (1.f,0.f,0.1f,1.f);
    glEnable            (GL_DEPTH_TEST);
    glDisable			(GL_CULL_FACE);
    glMatrixMode        (GL_PROJECTION);
    glLoadIdentity		();
    glFrustumf          (-1.f,1.f,-1.f,1.f,3.f,1000.f);
    glMatrixMode        (GL_MODELVIEW);
    glShadeModel        (GL_SMOOTH);
    glVertexPointer     (3,GL_SHORT,0,iLiveVertices);
    glColorPointer      (4,GL_UNSIGNED_BYTE,0,iColors);
    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_COLOR_ARRAY);
#endif
}

#define SCALE 32767
#define ONE_PER_SQRT2 0.70710678118654746f

static void adjustvertex(float *x, float *y, float *z)
{
	TReal s = (TReal)(*x**x + *y**y+ *z**z);
	TReal t;
	Math::Sqrt(t,s);

	float l = 1.0f/(float)t;
	*x *= l; *y *= l; *z *= l;
}

static short *generatesphere(int subdivision)
{
	// starting with 8 triangles
	int finalcount,i,j;
	
	finalcount = 8;
	for(i = 0 ; i < subdivision ; i++)		finalcount *= 4;
	
	float *mem  = new float[finalcount*3*3];
	float *mem2 = new float[finalcount*3*3];

	float *p = mem;

	/// Initialize first one
	// back top triangle (ccw)
	*p++   	= 0;					*p++   = 1.0;				*p++   	  = 0;				// top  vertex 
	*p++	=  ONE_PER_SQRT2;		*p++   = 0.0;				*p++	  = -ONE_PER_SQRT2; // back right
	*p++	= -ONE_PER_SQRT2;		*p++   = 0.0;				*p++	  = -ONE_PER_SQRT2; // back left

	// right top triangle
	*p++   = 0;					*p++  = 1.0;				*p++  = 0;				// top  vertex 
	*p++   = ONE_PER_SQRT2;     *p++  = 0.0;				*p++  = ONE_PER_SQRT2;  // front right
	*p++   =  ONE_PER_SQRT2;	*p++  = 0.0;				*p++  = -ONE_PER_SQRT2; // back right

	// front top triangle
	*p++   = 0;					 *p++  = 1.0;				*p++  = 0;				// top  vertex 
	*p++   =  -ONE_PER_SQRT2;     *p++  = 0.0;				*p++  = ONE_PER_SQRT2;  // front left
	*p++   =  ONE_PER_SQRT2;     *p++  = 0.0;				*p++  = ONE_PER_SQRT2;  // front right

	// left top triangle
	*p++  = 0;					*p++   = 1.0;				*p++   = 0;				// top  vertex 
	*p++  = -ONE_PER_SQRT2;		*p++   = 0.0;				*p++   = -ONE_PER_SQRT2; // back left
	*p++  =  -ONE_PER_SQRT2;     *p++   = 0.0;				*p++   = ONE_PER_SQRT2;  // front left


	// back bot triangle
	*p++  = 0;					*p++   = -1.0;				*p++      = 0;				// bottom vertex 
	*p++  = -ONE_PER_SQRT2;		*p++   = 0.0;				*p++	  = -ONE_PER_SQRT2; // back left
	*p++  =  ONE_PER_SQRT2;		*p++   = 0.0;				*p++	  = -ONE_PER_SQRT2; // back right

	// right bot triangle
	*p++  = 0;					*p++  = -1.0;				*p++   = 0;				// bottom vertex 
	*p++  =  ONE_PER_SQRT2;		*p++  = 0.0;				*p++   = -ONE_PER_SQRT2; // back right
	*p++  =  ONE_PER_SQRT2;     *p++  = 0.0;				*p++   = ONE_PER_SQRT2;  // front right

	// front bot triangle
	*p++  = 0;					*p++  = -1.0;				*p++  = 0;				// bottom vertex 
	*p++  =  ONE_PER_SQRT2;     *p++  = 0.0;				*p++  = ONE_PER_SQRT2;  // front right
	*p++  =  -ONE_PER_SQRT2;     *p++  = 0.0;				*p++  = ONE_PER_SQRT2;  // front left

	// left bot triangle
	*p++  = 0;					*p++  = -1.0;				*p++  = 0;				// bottom vertex 
	*p++  =  -ONE_PER_SQRT2;     *p++  = 0.0;				*p++  = ONE_PER_SQRT2;  // front left
	*p++  = -ONE_PER_SQRT2;		*p++  = 0.0;				*p++  = -ONE_PER_SQRT2; // back left

	float *ptr1 = mem;
	float *ptr2 = mem2;
	int count = 8;

	for(i=0;i<subdivision;i++)
	{
		// split each triangle into four, by generating extra vertices into center of edges and moving vertices
		// onto the sphere so that ray from center of the sphere to the vertex on the sphere goes through the original
		// split vertex.. also keep the BF cull (CCW) info the same
		
		float *b1 = ptr1;
		float *b2 = ptr2;

		for(j=0;j<count;j++)
		{
			float in0x,in0y,in0z;
			float in1x,in1y,in1z;
			float in2x,in2y,in2z;
			float n0x,n0y,n0z;
			float n1x,n1y,n1z;
			float n2x,n2y,n2z;

			// process one triangle from *b1 to *b2
			in0x = *b1++; in0y = *b1++;	in0z = *b1++;
			in1x = *b1++; in1y = *b1++; in1z = *b1++;
			in2x = *b1++; in2y = *b1++; in2z = *b1++;
			
			// new intermediate vertices
			n0x = (in1x+in0x)/2.0; n0y = (in1y+in0y)/2.0; n0z = (in1z+in0z)/2.0;
			n1x = (in1x+in2x)/2.0; n1y = (in1y+in2y)/2.0; n1z = (in1z+in2z)/2.0;
			n2x = (in2x+in0x)/2.0; n2y = (in2y+in0y)/2.0; n2z = (in2z+in0z)/2.0;

			// adjust the vertices
			adjustvertex(&n0x, &n0y, &n0z);		
			adjustvertex(&n1x, &n1y, &n1z);		
			adjustvertex(&n2x, &n2y, &n2z);		

			//   0___n0___1
			//    \  /\  /
			//    n2/__\n1		
			//      \  /
			//       \/
			//        2
			// emit new triangles
			
			*b2++ = in0x; *b2++ = in0y; *b2++ = in0z;	// 0-n0-n2
			*b2++ = n0x;  *b2++ = n0y;  *b2++ = n0z;
			*b2++ = n2x;  *b2++ = n2y;  *b2++ = n2z;
			
			*b2++ = n0x;  *b2++ = n0y;  *b2++ = n0z;	// n0-n1-n2
			*b2++ = n1x;  *b2++ = n1y;  *b2++ = n1z;
			*b2++ = n2x;  *b2++ = n2y;  *b2++ = n2z;

			*b2++ = n0x;  *b2++ = n0y;  *b2++ = n0z;	// n0-1-n1
			*b2++ = in1x; *b2++ = in1y; *b2++ = in1z;
			*b2++ = n1x;  *b2++ = n1y;  *b2++ = n1z;

			*b2++ = n2x;  *b2++ = n2y;  *b2++ = n2z;	// n2-n1-2
			*b2++ = n1x;  *b2++ = n1y;  *b2++ = n1z;
			*b2++ = in2x; *b2++ = in2y; *b2++ = in2z;
		}
		// swap the source and destination
		float *t = ptr1;
		ptr1 = ptr2;
		ptr2 = t;

		count = count * 4;
	}
	
	// We are done, convert the data to short format
	delete ptr2;
	
	short *out = new short[finalcount*3*3];
	for(i=0;i<finalcount*3*3;i++)
	{
		out[i] = (short)(SCALE*ptr1[i]);
	}

	delete ptr1;
	return out;
}




void CScenetone3D::UpdateViewport(TInt aWidth, TInt aHeight)
{
#if defined(SCENETONE_INCLUDE_VISUALIZER)
	if(iGLInitialized)
	{
		glViewport(0,0,aWidth,aHeight);	
	}
#endif
}

void CScenetone3D::Start()
{
#if defined(SCENETONE_INCLUDE_VISUALIZER)
    initGLES();

    /* Create a periodic timer for display refresh */
    iPeriodic = CPeriodic::NewL( CActive::EPriorityIdle );
    iPeriodic->Start( 100, 100, TCallBack( CScenetone3D::DrawCallback, this ) );

    iFrame = 0;
	iSampleCounter = 0;
	int i;
	
	for(i=0;i<iTriangleCount;i++)
	{
		iTriangleCoeffs[i] = FALLOFF_MIN;
	}
#endif // SCENETONE_INCLUDE_VISUALIZER
}

void CScenetone3D::Stop()
{
	if(iPeriodic)
	{
		iPeriodic->Deque();
		delete iPeriodic;
		iPeriodic = NULL;
	}
}

void CScenetone3D::DoMorphs()
{	
	int i;
	TInt16 *in     = &iVertices[0];
	TInt16 *out    = &iLiveVertices[0];
	TInt   *scales = &iTriangleCoeffs[0];

	for(i=0; i<iTriangleCount; i++)
	{
		int scale = *scales;

		*out++ = (short)((((int)(*in++))*scale) >> 15);
		*out++ = (short)((((int)(*in++))*scale) >> 15);
		*out++ = (short)((((int)(*in++))*scale) >> 15);

		*out++ = (short)((((int)(*in++))*scale) >> 15);
		*out++ = (short)((((int)(*in++))*scale) >> 15);
		*out++ = (short)((((int)(*in++))*scale) >> 15);

		*out++ = (short)((((int)(*in++))*scale) >> 15);
		*out++ = (short)((((int)(*in++))*scale) >> 15);
		*out++ = (short)((((int)(*in++))*scale) >> 15);

		scale = scale - FALLOFF_SPEED;
		if(scale < FALLOFF_MIN)	scale = FALLOFF_MIN;

		*scales++ = scale;
	}	
}

void CScenetone3D::NewSamples(TUint8 *aNewSamples, TInt aBytes, TBool aIsStereo)
{
#if defined(SCENETONE_INCLUDE_VISUALIZER)
	// Take in new sampledata, do following:
	//
	//	1) calculate average power of signal -> overall radius
	//	   -> to be used in modelview matrix
	//  2) calculate sample "target triangle" from every 10th sample as:
	//			tri_idx = sample & (TRIANGLEAMOUNT-1)
	//	3) calculate triangle target scale as
	//			(sample+32768) -> positive (positive fixed point with 65536 scale)
	//
	//	triangle <tri_idx> is modified such that its distance from unit ball centre is scaled
	//	with the fixed point scale. vertex colors for that tri are calculated as:
	//
	//	base color = (tri_idx * 0x812837) & 0xffffff00
	//	v0 = fixed pt scale * base_color
	//	v1 = (fixed pt scale >> 1) * base_color)
	// 	v2 = ((fixed pt scale >> 1) + (fixed_pt_scale>>2)) * base_color
	
	// first calculate signal average power (from Left channel)
	
	TInt16 *samples = (TInt16*) aNewSamples;
	int sampleamo = aBytes/2;
	if(aIsStereo) sampleamo >>= 1;

	int hsb = -1;
	int num = sampleamo;
	while(1)
	{
		if(!num)	break;
		num >>= 1;
		hsb++;
	}

	int i,j;
	int amount = 1 << hsb;
	int accums = 1 << (hsb-4);

	int waveletbuf[16];
	int values[8];
	int *valueptr = &values[0];
		
	// calculate 4 Haar wavelet levels (one is DC, then one down etc...)
	// we just use the first values from each subband for visualization
	// we advance forward by 8 positions in triangle stream (pow-of-2-triangles)
	// so the triangle positions should "stick"...

	// first accum round is done from samples to integers
	for(i=0;i<amount/accums;i++)
	{
		int accumvalue = 0;
		for(j=0;j<accums;j++)
		{
			accumvalue += samples[i*accums+j];
		}
		waveletbuf[i] = accumvalue / accums;    // todo: with a shift
	}
	*valueptr++ = waveletbuf[0];
	
	// next rounds are done from the accumulated numbers by adding them together
	for(i=1;i<4;i++)
	{
		int *ibuf = &waveletbuf[0];
		int *obuf = &waveletbuf[0];
		
		for(j=0;j<(1<<(4-i));j++)
		{
			int sum = (ibuf[0] + ibuf[1]); ibuf += 2;
			*obuf++ = sum / 2;
		}
		*valueptr++ = waveletbuf[0];
	}
	
	TInt *col = (TInt*)iColors;
	const TInt primaries[4] =
	{
		0x00400000,
		0x00004000,
		0x00000040,
		0x00404040	
	};
	
	// perhaps the addressing should be just totally random?
	for(i=0;i<4;i++)
	{
		int target = (iSampleCounter + i*3) & (iTriangleCount-1);
		int val = (int)(values[i]);
		if(val < 0) val = -val;
		
		iTriangleCoeffs[target] = val;

		col[target*3] = primaries[i] + 0xff000000;
		col[target*3+1] = primaries[i]*2 + 0xff000000;
		col[target*3+2] = primaries[i]*3 + 0xff000000;
	}
	iSampleCounter+=5;
		
#if 0	
	int i;
	short *in,*out;
	for(i=0;i<sampleamo/16;i++)
	{
		int target = (iSampleCounter + i) & (iTriangleCount-1);
		in  = &iVertices[target*3*3];
		out = &iLiveVertices[target*3*3];
		
		// avg out on 16 samples (todo: check that we have 16! :-)
		int scale  = ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += ((*samples++));
		scale     += 0x8000*16;
		scale     >>= 4;

		*out++ = (short)((((int)(*in++))*scale) >> 16);
		*out++ = (short)((((int)(*in++))*scale) >> 16);
		*out++ = (short)((((int)(*in++))*scale) >> 16);

		*out++ = (short)((((int)(*in++))*scale) >> 16);
		*out++ = (short)((((int)(*in++))*scale) >> 16);
		*out++ = (short)((((int)(*in++))*scale) >> 16);

		*out++ = (short)((((int)(*in++))*scale) >> 16);
		*out++ = (short)((((int)(*in++))*scale) >> 16);
		*out++ = (short)((((int)(*in++))*scale) >> 16);
	}		
	iSampleCounter += (sampleamo/16);
#endif


#endif // SCENETONE_INCLUDE_VISULIZER
} 

/************************************************************
 * Initialize OpenGL ES context and initial OpenGL ES state *
 ************************************************************/

void CScenetone3D::Construct(RWindow aWin)
{
	iPeriodic = NULL;
	iVertices = NULL;
	iGLInitialized = EFalse;

#if defined(SCENETONE_INCLUDE_VISUALIZER)
    iWin = aWin;

	iModelScale = 1.0f/7000;
	iMorphOff = 0;

	iTriangleCount = 8;
	int i;
	for(i = 0 ; i < SUBDIVISION ; i++)		iTriangleCount *= 4;

    iEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(iEglDisplay == NULL )
    	User::Exit(-1);

    if(eglInitialize(iEglDisplay,NULL,NULL) == EGL_FALSE)
    	User::Exit(-1);

    EGLConfig  config,colorDepth;
    EGLint     numOfConfigs = 0; 
        
    switch( iWin.DisplayMode() )
    {
        case (EColor4K):   { colorDepth = 12; break; }
        case (EColor64K):  { colorDepth = 16; break; }
        case (EColor16M):  { colorDepth = 24; break; }
        default:
                             colorDepth = 32;
    }

    EGLint attrib_list[] = {       EGL_BUFFER_SIZE, colorDepth,
                                   EGL_DEPTH_SIZE,  15,
                                   EGL_NONE                                 };

    if(eglChooseConfig(iEglDisplay,attrib_list,&config,1,&numOfConfigs ) == EGL_FALSE)
                User::Exit(-1);

    iEglSurface = eglCreateWindowSurface( iEglDisplay, config, &iWin, NULL );
    if( iEglSurface == NULL )
                User::Exit(-1);

    iEglContext = eglCreateContext( iEglDisplay, config, EGL_NO_CONTEXT, NULL );
    
    if( iEglContext == NULL )
                User::Exit(-1);

    if( eglMakeCurrent( iEglDisplay, iEglSurface, iEglSurface, iEglContext ) == EGL_FALSE )
    	User::Exit(-1);
    
	iGLInitialized = ETrue;

	iVertices = generatesphere(SUBDIVISION);
	iLiveVertices = new short[iTriangleCount*3*3];
	iTriangleCoeffs = new int[iTriangleCount];
	Mem::Copy(iLiveVertices, iVertices, iTriangleCount*3*3*sizeof(short));

	unsigned char * tiColors   = (unsigned char *) new char[iTriangleCount*3*4];
	iColors = tiColors;

	for(i=0;i<iTriangleCount*3;i++)
	{
		iColors[i*4+0] = (unsigned char)i*i*i+3*i;	
		iColors[i*4+1] = (unsigned char)i*i+5*i;	
		iColors[i*4+2] = (unsigned char)i*i*i-15*i;	
		iColors[i*4+3] = 255;
	}
#endif
}

CScenetone3D::~CScenetone3D()
{
#if defined(SCENETONE_INCLUDE_VISUALIZER)
	iPeriodic->Deque();
	if(iPeriodic)
	{
		delete iPeriodic;
	}
	
	delete iVertices;
	delete iColors;
	
	eglMakeCurrent( iEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
	eglDestroyContext( iEglDisplay, iEglContext );
	eglDestroySurface( iEglDisplay, iEglSurface );
	eglTerminate( iEglDisplay );
#endif
}

/************************************************************
 * Draw callback executed regularly by a timer callback
 ************************************************************/


TInt CScenetone3D::DrawCallback( TAny* aInstance )
{
    CScenetone3D* instance = static_cast<CScenetone3D*>(aInstance);
#if defined(SCENETONE_INCLUDE_VISUALIZER)
	instance->DoMorphs();
        
    glClear                 (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity          ();

    glTranslatef            (0,0,-5.0f);
	glRotatef				(instance->iFrame>>1, 1.0,0.75,0.25);
	glScalef				(instance->iModelScale, instance->iModelScale, instance->iModelScale);
    glDrawArrays            (GL_TRIANGLES,0,instance->iTriangleCount*3);

    eglSwapBuffers          (instance->iEglDisplay, instance->iEglSurface);

    /* To keep the background light on */
    if (!(instance->iFrame%100))        User::ResetInactivityTime();
    
    instance->iFrame++;
#endif
    return 0;
}
