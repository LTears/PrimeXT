/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== explode.cpp ========================================================

  Explosion-related code

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"

// Spark Shower
class CShower : public CBaseEntity
{
	DECLARE_CLASS( CShower, CBaseEntity );

	void Spawn( void );
	void Think( void );
	void Touch( CBaseEntity *pOther );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS( spark_shower, CShower );

void CShower::Spawn( void )
{
	Vector velocity = RANDOM_FLOAT( 200, 300 ) * GetAbsAngles();
	velocity.x += RANDOM_FLOAT(-100.f,100.f);
	velocity.y += RANDOM_FLOAT(-100.f,100.f);

	if ( velocity.z >= 0 )
		velocity.z += 200;
	else
		velocity.z -= 200;

	SetAbsVelocity( velocity );

	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/grenade.mdl");	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT( 0.5, 1.5 );

	SetAbsAngles( g_vecZero );
}

void CShower::Think( void )
{
	UTIL_Sparks( GetAbsOrigin() );

	pev->speed -= 0.1;
	if ( pev->speed > 0 )
		SetNextThink( 0.1 );
	else
		UTIL_Remove( this );
	pev->flags &= ~FL_ONGROUND;
}

void CShower::Touch( CBaseEntity *pOther )
{
	if ( pev->flags & FL_ONGROUND )
		SetAbsVelocity( GetAbsVelocity() * 0.1 );
	else
		SetAbsVelocity( GetAbsVelocity() * 0.6 );

	if ( (GetAbsVelocity().x * GetAbsVelocity().x + GetAbsVelocity().y * GetAbsVelocity().y ) < 10.0 )
		pev->speed = 0;
}

class CEnvExplosion : public CBaseMonster
{
	DECLARE_CLASS( CEnvExplosion, CBaseMonster );
public:
	void Spawn( );
	void Smoke ( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	DECLARE_DATADESC();

	int m_iMagnitude;// how large is the fireball? how much damage?
	int m_spriteScale; // what's the exact fireball sprite scale? 
};

BEGIN_DATADESC( CEnvExplosion )
	DEFINE_FIELD( m_iMagnitude, FIELD_INTEGER ),
	DEFINE_FIELD( m_spriteScale, FIELD_INTEGER ),
	DEFINE_FUNCTION( Smoke ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_explosion, CEnvExplosion );

void CEnvExplosion::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "iMagnitude" ))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvExplosion::Spawn( void )
{ 
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;

	float flSpriteScale;
	flSpriteScale = ( m_iMagnitude - 50) * 0.6;
	
	if ( flSpriteScale < 10 )
	{
		flSpriteScale = 10;
	}

	m_spriteScale = (int)flSpriteScale;
}

void CEnvExplosion::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	TraceResult tr;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	Vector		vecSpot;// trace starts here!

	vecSpot = GetAbsOrigin() + Vector( 0, 0, 8 );
	
	UTIL_TraceLine( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, edict(), &tr );
	
	// Pull out of the wall a bit
	if ( tr.flFraction != 1.0 )
	{
		SetAbsOrigin( tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6f ));
	}

	// draw decal
	if (! ( pev->spawnflags & SF_ENVEXPLOSION_NODECAL))
	{
		if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
		{
			UTIL_DecalTrace( &tr, "{scorch1" );
		}
		else
		{
			UTIL_DecalTrace( &tr, "{scorch2" );
		}
	}

	Vector absOrigin = GetAbsOrigin();

	// draw fireball
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, absOrigin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( absOrigin.x );
			WRITE_COORD( absOrigin.y );
			WRITE_COORD( absOrigin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, absOrigin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( absOrigin.x );
			WRITE_COORD( absOrigin.y );
			WRITE_COORD( absOrigin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}

	// do damage
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE ) )
	{
		RadiusDamage ( pev, pev, m_iMagnitude, CLASS_NONE, DMG_BLAST );
	}

	SetThink( &CEnvExplosion::Smoke );
	pev->nextthink = gpGlobals->time + 0.3;

	// draw sparks
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG(0,3);

		for ( int i = 0; i < sparkCount; i++ )
		{
			Create( "spark_shower", absOrigin, tr.vecPlaneNormal, NULL );
		}
	}
}

void CEnvExplosion::Smoke( void )
{
	Vector absOrigin = GetAbsOrigin();

	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, absOrigin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( absOrigin.x );
			WRITE_COORD( absOrigin.y );
			WRITE_COORD( absOrigin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	}
	
	if ( !(pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE) )
	{
		UTIL_Remove( this );
	}
}


// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate( const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, BOOL doDamage )
{
	KeyValueData	kvd;
	char			buf[128];

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, angles, pOwner );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	if ( !doDamage )
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->Use( NULL, NULL, USE_TOGGLE, 0 );
}

