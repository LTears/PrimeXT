/*
mathlib.h - math subroutines for GLSL
Copyright (C) 2014 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef MATHLIB_H
#define MATHLIB_H

#ifndef saturate
#define saturate( x )	clamp( x, 0.0, 1.0 )
#endif

#define VectorMax( lb )	(max( lb.r, max( lb.g, lb.b )))
#define sqr( x )		( x * x )
#define rcp( x )		( 1.0 / x )

#define UnpackVector( c )	fract(( c ) * vec3( 1.0, 256.0, 65536.0 ))
#define UnpackNormal( c )	((fract(( c ) * vec3( 1.0, 256.0, 65536.0 )) * 2.0) - 1.0)

// remap a value in the range [A,B] to [C,D].
float RemapVal( float val, float A, float B, float C, float D )
{
	return C + (D - C) * (val - A) / (B - A);
}

float GetLuminance( vec3 color ) { return dot( color, vec3( 0.2126, 0.7152, 0.0722 )); }

void MakeNormalVectors( const vec3 forward, inout vec3 right, inout vec3 up )
{
	// this rotate and negate guarantees a vector not colinear with the original
	right = vec3( forward.z, -forward.x, forward.y );
	right -= forward * dot( right, forward );
	up = cross( normalize( right ), forward );
}

float linearizeDepth( float zfar, float depth )
{
	return -zfar * Z_NEAR / ( depth * ( zfar - Z_NEAR ) - zfar );
}

float ComputeStaticBump( const vec3 L, const vec3 N, const float ambientClip )
{
	vec3 srcB = L;

	// remap static bump to positive range
//	srcB.x = RemapVal( srcB.x, -1.0, 1.0, 0.0, 1.0 );
//	srcB.y = RemapVal( srcB.y, -1.0, 1.0, 0.0, 1.0 );
	srcB.z = RemapVal( srcB.z, -1.0, 1.0, ambientClip, 1.0 );
	vec3 B = normalize( srcB );

	return saturate( dot( N, B ));
}

vec3 ColorNormalize( const in vec3 color, float threshold )
{
	if( color.r > threshold || color.g > threshold || color.b > threshold )
	{
		float max = color.r;

		if( color.g > max )
			max = color.g;
		if( color.b > max )
			max = color.b;

		if( max == 0.0 )
			return color;

		return ( color * threshold ) / max;
	}

	return color;
}

vec3 ApplyGamma( in vec3 color, float gamma, float contrast )
{
	color.r = pow( color.r, gamma ) * contrast;
	color.g = pow( color.g, gamma ) * contrast;
	color.b = pow( color.b, gamma ) * contrast;

	return color;
}

float get_shadow_offset( in float lightCos )
{
	float bias = 0.0005 * tan( acos( saturate( lightCos )));
	return clamp( bias, 0.0, 0.005 );
}

#endif//MATHLIB_H