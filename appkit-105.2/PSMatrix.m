/*
	PSMatrix.m
   	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
 
	This file defines the PSMatrix (Private) Class of the AppKit
	
	Modified:
	16Oct86	wrp	file creation from scratch
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "PSMatrix.h"
#import <math.h>
#import <dpsclient/wraps.h>


static void
NXTransformRect(PSMatrix *aMatrix, NXRect *aRect, void (*transformFunc)());
static void
NXInverseTransform(PSMatrix *aMatrix, NXPoint *aPoint);
static void
NXTransform(PSMatrix *aMatrix, NXPoint *aPoint);

static void
sinAndCos(float angle, float *sine, float *cosine);

#define	RADS_PER_DEGREE		(3.14159265358979323846/180.0)

#define	A	elems[0]
#define	B	elems[1]
#define	C	elems[2]
#define	D	elems[3]
#define	E	elems[4]
#define	F	elems[5]
#define	EI	elems[6]
#define	FI	elems[7]
#define	DET	elems[8]
#define	SX	elems[9]
#define	SY	elems[10]
#define	THETA	elems[11]


@implementation PSMatrix


+ new
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

- init
{
    [super init];
    [self makeIdentity];
    return self;
}


- _doRotationOnly
{
    mFlags.rotationOnly = YES;
    return self;
}


- (NXCoord)getRotationAngle
{
    register NXCoord   *elems = matrixElements;

    return (THETA);
}


- scale:(NXCoord)sx :(NXCoord)sy
{
    register NXCoord   *elems = matrixElements;

    if (!mFlags.rotationOnly) {
	SX *= sx;
	SY *= sy;
	A *= sx;
	D *= sy;
	if (mFlags.rotated) {
	    B *= sx;
	    C *= sy;
	}
	[self _computeInv];
    }
    return self;
}


- translate:(NXCoord)tx :(NXCoord)ty
{
    register NXCoord   *elems = matrixElements;

    if (!mFlags.rotationOnly) {
	E += A * tx;
	F += D * ty;
	if (mFlags.rotated) {
	    E += C * ty;
	    F += B * tx;
	}
	[self _computeInv];
    }
    return self;
}


- rotate:(NXCoord)angle
{
    register NXCoord   *elems = matrixElements;
    NXCoord             cosTheta, sinTheta, temp;

    sinAndCos(angle, &sinTheta, &cosTheta);
    if (mFlags.rotated) {
	temp = A;
	A = A * cosTheta + C * sinTheta;
	C = C * cosTheta - temp * sinTheta;
	temp = B;
	B = D * sinTheta + B * cosTheta;
	D = D * cosTheta - temp * sinTheta;
    } else {
	C = A * -sinTheta;
	A *= cosTheta;

	B = D * sinTheta;
	D *= cosTheta;
    }
    THETA += angle;
    mFlags.rotated = (B != 0.0 || C != 0.0);
    [self _computeInv];
    return self;
}


- scaleTo:(NXCoord)sx :(NXCoord)sy
{
    register NXCoord   *elems = matrixElements;
    NXCoord             cosTheta, sinTheta;

    if (!mFlags.rotationOnly) {
    /* put in error code to check that sx != 0 and sy != 0 */
	A = SX = (SX < 0.0) ? -sx : sx;
	D = SY = (SY < 0.0) ? -sy : sy;

	if (mFlags.rotated) {
	    sinAndCos(THETA, &sinTheta, &cosTheta);
	    A *= cosTheta;
	    D *= cosTheta;
	    B = SY * sinTheta;
	    C = -SX * sinTheta;
	}
	[self _computeInv];
    }
    return self;
}


- translateTo:(NXCoord)tx :(NXCoord)ty
{
    register NXCoord   *elems = matrixElements;

    if (!mFlags.rotationOnly) {
	E = A * tx;
	F = D * ty;
	if (mFlags.rotated) {
	    E += C * ty;
	    F += B * tx;
	}
	[self _computeInv];
    }
    return self;
}


- rotateTo:(NXCoord)angle
{
    register NXCoord   *elems = matrixElements;
    NXCoord             cosTheta, sinTheta;

    if ((THETA = angle) == 0.0) {
	A = SX;
	D = SY;
	B = C = 0.0;
    } else {
	sinAndCos(THETA, &sinTheta, &cosTheta);
	A = SX * cosTheta;
	B = SY * sinTheta;
	C = -SX * sinTheta;
	D = SY * cosTheta;
    }
    mFlags.rotated = (B != 0.0 || C != 0.0);
    [self _computeInv];
    return self;
}


#define	mA	0
#define	mB	1
#define	mC	2
#define	mD	3
#define	mTX	4
#define	mTY	5

static void
concat(const NXCoord *m, NXCoord *current)
{
    NXCoord t[6];
    int i;
    
    t[mA] = m[mA] * current[mA] + m[mB] * current[mC];
    t[mB] = m[mA] * current[mB] + m[mB] * current[mD];
    t[mC] = m[mC] * current[mA] + m[mD] * current[mC];
    t[mD] = m[mC] * current[mB] + m[mD] * current[mD];
    t[mTX]= m[mTX] * current[mA] + m[mTY] * current[mC] + current[mTX];
    t[mTY]= m[mTX] * current[mB] + m[mTY] * current[mD] + current[mTY];
    
    for (i = 0; i< 6; i++)
	current[i] = t[i];
}


- concat:(PSMatrix *)aMatrix
{
    register NXCoord   *elems = matrixElements;

    if (!aMatrix->mFlags.identity) {
	concat(aMatrix->matrixElements, elems);
	mFlags.rotated = (B != 0.0 || C != 0.0);
	[self _computeInv];
    }
    return self;
}


- send
{
    register NXCoord   *elems = matrixElements;
  /* note: scaled is only valid if not rotated */
    BOOL rotated, scaled, translated;
    
    if (!mFlags.identity) {
	rotated = (B != 0.0) || (C != 0.0);
	translated = (E != 0.0) || (F != 0.0);
	scaled = (A != 1.0) || (D != 1.0);
	if (rotated || (translated && scaled))
	    PSconcat(elems);	/* sends array of 6 NXCoords then "concat" */
	else {
	    if (translated)
		PStranslate(E, F);
	    else if (scaled)
		PSscale(A, D);
	}
    }
    return self;
}

- sendInv
{
    register NXCoord   *elems = matrixElements;

  /* note: scaled is only valid if not rotated */
    BOOL rotated, scaled, translated;
    
    if (!mFlags.identity) {
	rotated = (B != 0.0) || (C != 0.0);
	translated = (E != 0.0) || (F != 0.0);
	scaled = (A != 1.0) || (D != 1.0);
	if (rotated || (translated && scaled)) {
	    NXCoord invElems[6] = {D/DET, -B/DET, -C/DET, A/DET, EI, FI};
	    PSconcat(invElems);	/* sends array of 6 NXCoords then "concat" */
	} else {
	    if (translated)
		PStranslate(EI, FI);
	    else if (scaled)
		PSscale(D/DET, A/DET);
	}
    }
    return self;
}


- transform:(NXPoint *)aPoint
{
    NXTransform(self, aPoint);
    return self;
}


- invTransform:(NXPoint *)aPoint
{
    NXInverseTransform(self, aPoint);
    return self;
}


- makeIdentity
{
    register NXCoord   *elems = matrixElements;

    A = D = DET = SX = SY = 1.0;
    B = C = E = F = EI = FI = THETA = 0.0;
    mFlags.identity = YES;
    mFlags.rotated  = NO;
    return self;
}


- (BOOL)identity
{
    return mFlags.identity;
}


- (BOOL)rotated
{
    return mFlags.rotated;
}


- transformRect:(NXRect *)aRect
{
    NXTransformRect(self, aRect, NXTransform);
    return self;
}


- invTransformRect:(NXRect *)aRect
{
    NXTransformRect(self, aRect, NXInverseTransform);
    return self;
}


 /* private */

- _computeInv
{
    register NXCoord   *elems = matrixElements;

    DET = (A * D) - (B * C);
    EI = -E * D;
    FI = -A * F;
    if (mFlags.rotated) {
	EI += C * F;
	FI += E * B;
    }
    mFlags.identity = (!mFlags.rotated && (A == 1.0 && D == 1.0) && (E == 0.0 && F == 0.0));
    return self;
}


static void
NXTransformRect(PSMatrix *aMatrix, NXRect *aRect, void (*transformFunc)())
{
    NXPoint             sPoint, dPoint;
    NXPoint            *sp = &sPoint, *dp = &dPoint;
    NXRect              sRect, dRect;
    int                 index;

    if (aMatrix->mFlags.identity)
	return;

    sRect = *aRect;

    *sp = sRect.origin;
    for (index = 0; index < 4; index++) {
	switch (index) {
	case 0:
	    break;
	case 1:
	    sp->y += sRect.size.height;
	    break;
	case 2:
	    sp->x += sRect.size.width;
	    break;
	case 3:
	    sp->y -= sRect.size.height;
	    break;
	default:
	    break;
	}

	*dp = *sp;
	(*transformFunc) (aMatrix, dp);

	if (index == 0)
	    dRect.origin = *(NXPoint *)(&(dRect.size)) = *dp;

	if (dp->x < dRect.origin.x)
	    dRect.origin.x = dp->x;
	else if (dp->x > dRect.size.width)
	    dRect.size.width = dp->x;
	if (dp->y < dRect.origin.y)
	    dRect.origin.y = dp->y;
	else if (dp->y > dRect.size.height)
	    dRect.size.height = dp->y;
    }
    dRect.size.width -= dRect.origin.x;
    dRect.size.height -= dRect.origin.y;

    *aRect = dRect;
}

static void
NXInverseTransform(PSMatrix *aMatrix, NXPoint *aPoint)
{
    register NXCoord   *elems = aMatrix->matrixElements;
    register NXCoord    temp;

    if (aMatrix->mFlags.identity)
	return;
    if (aMatrix->mFlags.rotationOnly) {
	temp = aPoint->x;
	aPoint->x = D * temp - C * aPoint->y;
	aPoint->y = -B * temp + A * aPoint->y;
    } else if (!aMatrix->mFlags.rotated) {
	aPoint->x = (D * aPoint->x + EI) / DET;
	aPoint->y = (A * aPoint->y + FI) / DET;
    } else {
	temp = aPoint->x;
	aPoint->x = (D * temp - C * aPoint->y + EI) / DET;
	aPoint->y = (-B * temp + A * aPoint->y + FI) / DET;
    }
}

static void
NXTransform(PSMatrix *aMatrix, NXPoint *aPoint)
{
    register NXCoord   *elems = aMatrix->matrixElements;
    register NXCoord    temp;

    if (aMatrix->mFlags.identity)
	return;
    if (aMatrix->mFlags.rotationOnly) {
	temp = aPoint->x;
	aPoint->x = A * temp + C * aPoint->y;
	aPoint->y = B * temp + D * aPoint->y;
    } else if (aMatrix->mFlags.rotated) {
	temp = aPoint->x;
	aPoint->x = A * temp + C * aPoint->y + E;
	aPoint->y = B * temp + D * aPoint->y + F;
    } else {
	aPoint->x = A * aPoint->x + E;
	aPoint->y = D * aPoint->y + F;
    }
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteArray(stream, "f", 12, &matrixElements);
    NXWriteTypes(stream, "s", &mFlags);
    return self;
};


#define	IDENTITYMASK		((unsigned)0x01)
#define	ROTATEDMASK		((unsigned)0x02)
#define	ROTATIONONLYMASK	((unsigned)0x10)

- read:(NXTypedStream *) stream
{
    [super read:stream];
    if (NXSystemVersion(stream) < 901) {
	int temp;
	
	NXReadArray(stream, "f", 12, &matrixElements);
	
	NXReadTypes(stream, "i", &temp);
	mFlags.identity = !(temp & IDENTITYMASK);	/* note: this is inverted */
	mFlags.rotated = (temp & ROTATEDMASK);
	mFlags.rotationOnly = (temp & ROTATIONONLYMASK);
    } else {
	NXReadArray(stream, "f", 12, &matrixElements);
	NXReadTypes(stream, "s", &mFlags);
    }
    
    return self;
};


@end


static void
sinAndCos(float angle, float *sine, float *cosine)
{
    int                 iAngle;
    int                 quadrant;
    const static char   trigTable[5] = {0, 1, 0, -1, 0};

    iAngle = angle;
    if (iAngle == angle && !(iAngle % 90)) {
	quadrant = (iAngle / 90) % 4;
	if (quadrant < 0)
	    quadrant += 4;
	*sine = trigTable[quadrant];
	*cosine = trigTable[quadrant + 1];
    } else {
	*sine = sin(angle * RADS_PER_DEGREE);
	*cosine = cos(angle * RADS_PER_DEGREE);
    }
}



/*
  
Modifications (starting at 0.8):
  
12/10/88 trey	Changed call to _NXConcat to PSConcat
 2/11/89  bs	Added read: and write:
 3/10/88 trey	Specialcased trig operations to ensure sin/cos of right angles
		 is exact
*/




