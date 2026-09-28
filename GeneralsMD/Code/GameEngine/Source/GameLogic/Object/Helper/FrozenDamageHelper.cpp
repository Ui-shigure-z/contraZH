/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: FrozenDamageHelper.cpp ////////////////////////////////////////////////////////////////////////
// Desc:   Object helper - Heals frozen damage since Body modules can't have Updates
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"

#include "GameLogic/Object.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/FrozenDamageHelper.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
FrozenDamageHelper::FrozenDamageHelper( Thing *thing, const ModuleData *modData ) : ObjectHelper( thing, modData )
{
	m_healingStepCountdown = 0;

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
FrozenDamageHelper::~FrozenDamageHelper()
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime FrozenDamageHelper::update()
{
	BodyModuleInterface *body = getObject()->getBodyModule();

	if( m_healingStepCountdown > 0 )
		m_healingStepCountdown--;
	if( m_healingStepCountdown > 0 )
		return UPDATE_SLEEP_NONE;

	m_healingStepCountdown = body->getFrozenDamageHealRate();

	DamageInfo removeFrozenDamage;
	removeFrozenDamage.in.m_damageType = DAMAGE_SUBDUAL_FROZEN_UNRESISTABLE;
	removeFrozenDamage.in.m_amount = -body->getFrozenDamageHealAmount();
	body->attemptDamage(&removeFrozenDamage);

	if( body->hasAnyFrozenDamage() )
		return UPDATE_SLEEP_NONE;
	else
		return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void FrozenDamageHelper::notifyFrozenDamage( Real amount )
{
	if( amount > 0 )
	{
		m_healingStepCountdown = getObject()->getBodyModule()->getFrozenDamageHealRate();
		setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FrozenDamageHelper::crc( Xfer *xfer )
{
	ObjectHelper::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FrozenDamageHelper::xfer( Xfer *xfer )
{
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	ObjectHelper::xfer( xfer );

	xfer->xferUnsignedInt( &m_healingStepCountdown );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FrozenDamageHelper::loadPostProcess()
{
	ObjectHelper::loadPostProcess();
}
