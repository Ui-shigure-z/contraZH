// FILE: CommandSequence.cpp /////////////////////////////////////////////////////////////////////////////////
// Author: ShigureUi, September 2026
// Desc:   Advanced waypoint system
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Common/ActionManager.h"
#include "Common/BuildAssistant.h"
#include "Common/SpecialPower.h"
#include "Common/ThingFactory.h"

#include "GameLogic/CommandSequence.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/DeployStyleAIUpdate.h"
#include "GameLogic/Module/OverchargeBehavior.h"
#include "GameLogic/Module/SpawnBehavior.h"
#include "GameLogic/Module/SpecialPowerModule.h"

//-----------------------------------------------------------------------------
//         Public Data
//-----------------------------------------------------------------------------
CommandSequence *TheCommandSequence = nullptr;  ///< the command sequence singleton

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CommandSequence::CommandSequence()
{
}

CommandSequence::~CommandSequence()
{
}

void CommandSequence::init()
{
	m_uniqueID = 0;
	m_commandNodeCount = 0;
}

void CommandSequence::reset()
{
	IDToNodeMap::iterator it;
	for (it = m_allNodes.begin(); it != m_allNodes.end(); it++)
	{
		delete it->second;
	}

	m_allNodes.clear();
	m_uniqueID = 0;
	m_commandNodeCount = 0;
}

void CommandSequence::update()
{

}

void CommandSequence::crc(Xfer* xfer)
{
}

void CommandSequence::xfer(Xfer* xfer)
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion(&version, currentVersion);

	xfer->xferUnsignedInt(&m_uniqueID);
	xfer->xferInt(&m_commandNodeCount);
	
	if (xfer->getXferMode() == XFER_LOAD)
	{
		for (Int i = 0; i < m_commandNodeCount; i++)
		{
			UnsignedInt nodeID;
			xfer->xferUnsignedInt(&nodeID);

			CommandNode *cmdNode = new CommandNode();
			m_allNodes.insert(std::make_pair(nodeID, cmdNode));

			xferCommandNode(xfer, cmdNode);
		}
	}
	else
	{
		for (IDToNodeMap::iterator it = m_allNodes.begin(); it != m_allNodes.end(); it++)
		{
			CommandNode *cmdNode = it->second;
			xfer->xferUnsignedInt(&cmdNode->m_nodeID);

			xferCommandNode(xfer, cmdNode);
		}
	}
}

void CommandSequence::loadPostProcess()
{
}

void CommandSequence::xferCommandNode(Xfer *xfer, CommandNode *cmdNode)
{
	xfer->xferInt((Int*)&cmdNode->m_commandType);

	switch (cmdNode->m_commandType)
	{
			// no argument
		case COMMAND_DO_EVACUATE:
		case COMMAND_INTERNET_HACK:
		case COMMAND_TOGGLE_OVERCHARGE:
		case COMMAND_TOGGLE_HOLD_FIRE:
		case COMMAND_TOGGLE_DEPLOY:
			break;
			// single pos
		case COMMAND_DO_MOVETO:
		case COMMAND_DO_REVERSE_MOVETO:
		{
			xfer->xferCoord3D(&cmdNode->m_loc);
			break;
		}
		// single target
		case COMMAND_GET_REPAIRED:
		case COMMAND_GET_HEALED:
		case COMMAND_ENTER:
		case COMMAND_DO_EXIT:
		case COMMAND_DO_REPAIR:
		case COMMAND_DOCK:
		{
			xfer->xferObjectID(&cmdNode->m_targetID);
			break;
		}
		// guard pos
		case COMMAND_DO_GUARD_POSITION:
		{
			xfer->xferCoord3D(&cmdNode->m_loc);
			xfer->xferInt((Int*) & cmdNode->m_guardMode);
			break;
		}
		// guard target
		case COMMAND_DO_GUARD_OBJECT:
		{
			xfer->xferObjectID(&cmdNode->m_targetID);
			xfer->xferInt((Int*)&cmdNode->m_guardMode);
			break;
		}
		// pos and target
		case COMMAND_DO_COMBATDROP:
		{
			xfer->xferCoord3D(&cmdNode->m_loc);
			xfer->xferObjectID(&cmdNode->m_targetID);
			break;
		}
		// pos and maxshots
		case COMMAND_DO_ATTACK_MOVETO:
		{
			xfer->xferCoord3D(&cmdNode->m_loc);
			xfer->xferInt(&cmdNode->m_maxShotsToFire);
			break;
		}
		// do weapon at location
		case COMMAND_DO_WEAPON:
		case COMMAND_DO_WEAPON_AT_LOCATION:
		{
			xfer->xferInt((Int*)&cmdNode->m_weaponSlot);
			xfer->xferCoord3D(&cmdNode->m_loc);
			xfer->xferInt(&cmdNode->m_maxShotsToFire);
			xfer->xferBool(&cmdNode->m_releaseAfter);
			break;
		}
		// do attack object including do weapon at object
		case COMMAND_DO_ATTACK_OBJECT:
		case COMMAND_DO_FORCE_ATTACK_OBJECT:
		{
			xfer->xferInt((Int*)&cmdNode->m_weaponSlot);
			xfer->xferObjectID(&cmdNode->m_targetID);
			xfer->xferInt(&cmdNode->m_maxShotsToFire);
			break;
		}
		// dozer construct
		case COMMAND_DOZER_CONSTRUCT:
		case COMMAND_DOZER_CONSTRUCT_LINE:
		{
			xfer->xferCoord3D(&cmdNode->m_loc);
			xfer->xferUnsignedShort(&cmdNode->m_templateID);
			xfer->xferReal(&cmdNode->m_angle);
			if (cmdNode->m_commandType == COMMAND_DOZER_CONSTRUCT_LINE)
				xfer->xferCoord3D(&cmdNode->m_loc2);
			break;
		}
		// do special
		case COMMAND_DO_SPECIAL_POWER:
		{
			xfer->xferUnsignedInt(&cmdNode->m_specialPowerID);
			xfer->xferUnsignedInt(&cmdNode->m_specialOptions);
			break;
		}
		// do special at location
		case COMMAND_DO_SPECIAL_POWER_AT_POSITION:
		case COMMAND_DO_MOVING_SPECIAL_POWER_AT_POSITION:
		{
			xfer->xferUnsignedInt(&cmdNode->m_specialPowerID);
			xfer->xferCoord3D(&cmdNode->m_loc);
			xfer->xferReal(&cmdNode->m_angle);
			xfer->xferObjectID(&cmdNode->m_targetID);
			xfer->xferUnsignedInt(&cmdNode->m_specialOptions);
			break;
		}
		// do special at object
		case COMMAND_DO_SPECIAL_POWER_AT_OBJECT:
		{
			xfer->xferUnsignedInt(&cmdNode->m_specialPowerID);
			xfer->xferObjectID(&cmdNode->m_targetID);
			xfer->xferUnsignedInt(&cmdNode->m_specialOptions);
			break;
		}
		// switch weapon
		case COMMAND_SWITCH_WEAPON:
		{
			xfer->xferInt((Int*)&cmdNode->m_weaponSlot);
			break;
		}
	}

	if (xfer->getXferMode() == XFER_LOAD)
	{
		Int cnt;
		ObjectID objID;
		UnsignedInt nextCmd;
		xfer->xferInt(&cnt);
		for (Int i = 0; i < cnt; i++)
		{
			xfer->xferObjectID(&objID);
			xfer->xferUnsignedInt(&nextCmd);

			cmdNode->m_nextCommand.insert(std::make_pair(objID, nextCmd));
		}

		if (isTypicalMovingCommand(cmdNode->m_commandType))
		{
			Coord3D dest;
			xfer->xferInt(&cnt);
			for (Int i = 0; i < cnt; i++)
			{
				xfer->xferObjectID(&objID);
				xfer->xferCoord3D(&dest);

				cmdNode->m_allocatedDest.insert(std::make_pair(objID, dest));
			}
		}
	}
	else
	{
		Int cnt;
		ObjectID objID;
		cnt = cmdNode->m_nextCommand.size();
		xfer->xferInt(&cnt);

		for (ObjectToNodeMap::iterator it = cmdNode->m_nextCommand.begin(); it != cmdNode->m_nextCommand.end(); it++)
		{
			objID = it->first;
			xfer->xferObjectID(&objID);
			xfer->xferUnsignedInt(&it->second);
		}

		if (isTypicalMovingCommand(cmdNode->m_commandType))
		{
			cnt = cmdNode->m_allocatedDest.size();
			xfer->xferInt(&cnt);

			for (ObjectToLocMap::iterator it = cmdNode->m_allocatedDest.begin(); it != cmdNode->m_allocatedDest.end(); it++)
			{
				objID = it->first;
				xfer->xferObjectID(&objID);
				xfer->xferCoord3D(&it->second);
			}
		}
	}
}

Bool CommandSequence::hasAnyCommand(Object *obj)
{
	return obj->getCurrentCommand();
}

void CommandSequence::clearObjectCommand(Object *obj)
{
	while (removeCurrentCommand(obj));
	if (obj)
		obj->clearPendingExitList();
}

CommandSequence::CommandNode *CommandSequence::newCommandNode(CommandType commandType)
{
	m_uniqueID++;
	CommandNode *commandNode = new CommandNode();
	commandNode->m_commandType = commandType;
	commandNode->m_nodeID = m_uniqueID;
	m_allNodes.insert(std::make_pair(m_uniqueID, commandNode));
	m_commandNodeCount++;
	return commandNode;
}


UnsignedInt CommandSequence::newNoArgumentCommand(CommandType commandType)
{
	switch (commandType)
	{
	case COMMAND_DO_EVACUATE:
	case COMMAND_INTERNET_HACK:
	case COMMAND_TOGGLE_OVERCHARGE:
	case COMMAND_TOGGLE_HOLD_FIRE:
	case COMMAND_TOGGLE_DEPLOY:
		break;
	default:
		return 0;
	}
	CommandNode* commandNode = newCommandNode(commandType);
	return commandNode->m_nodeID;
}

//ShigureUi 07/10/2026 note all sanity check needs a recheck
UnsignedInt CommandSequence::newSinglePosCommand(CommandType commandType, const Coord3D *pos)
{
	switch (commandType)
	{
	case COMMAND_DO_MOVETO:
	case COMMAND_DO_REVERSE_MOVETO:
		break;
	default:
		return 0;
	}
	CommandNode *commandNode = newCommandNode(commandType);
	commandNode->m_loc = *pos;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newSingleTargetCommand(CommandType commandType, Object *obj)
{
	switch (commandType)
	{
	case COMMAND_GET_REPAIRED:
	case COMMAND_GET_HEALED:
	case COMMAND_ENTER:
	case COMMAND_DO_EXIT:
	case COMMAND_DO_REPAIR:
	case COMMAND_DOCK:
		break;
	default:
		return 0;
	}

	CommandNode *commandNode = newCommandNode(commandType);
	commandNode->m_targetID = obj ? obj->getID() : INVALID_ID;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newTargetNPosCommand(CommandType commandType, Object* obj, const Coord3D* pos)
{
	switch (commandType)
	{
	case COMMAND_DO_COMBATDROP:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_targetID = obj ? obj->getID() : INVALID_ID;
	commandNode->m_loc = *pos;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newPosNMaxShotsCommand(CommandType commandType, const Coord3D *pos, Int maxShotsToFire)
{
	switch (commandType)
	{
	case COMMAND_DO_ATTACK_MOVETO:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_loc = *pos;
	commandNode->m_maxShotsToFire = maxShotsToFire;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newGuardCommand(CommandType commandType, const Coord3D *pos, Object *obj, GuardMode guardMode)
{
	switch (commandType)
	{
	case COMMAND_DO_GUARD_POSITION:
	case COMMAND_DO_GUARD_OBJECT:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	if (commandNode->m_commandType == COMMAND_DO_GUARD_POSITION)
		commandNode->m_loc = *pos;
	else
		commandNode->m_targetID = obj->getID();
	commandNode->m_guardMode = guardMode;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newDoWeaponAtLocCommand(CommandType commandType, WeaponSlotType weaponSlot, const Coord3D *pos, Int maxShotsToFire, Bool releaseAfter)
{
	switch (commandType)
	{
	case COMMAND_DO_WEAPON:
	case COMMAND_DO_WEAPON_AT_LOCATION:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_weaponSlot = weaponSlot;
	commandNode->m_loc = *pos;
	commandNode->m_maxShotsToFire = maxShotsToFire;
	commandNode->m_releaseAfter = releaseAfter;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newDoWeaponAtObjCommand(CommandType commandType, WeaponSlotType weaponSlot, Object* obj, Int maxShotsToFire)
{
	switch (commandType)
	{
	case COMMAND_DO_ATTACK_OBJECT:
	case COMMAND_DO_FORCE_ATTACK_OBJECT:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_weaponSlot = weaponSlot;
	commandNode->m_targetID = obj->getID();
	commandNode->m_maxShotsToFire = maxShotsToFire;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newDozerConstructCommand(CommandType commandType, const ThingTemplate *thing, const Coord3D *loc1, const Coord3D* loc2, Real angle)
{
	switch (commandType)
	{
	case COMMAND_DOZER_CONSTRUCT:
	case COMMAND_DOZER_CONSTRUCT_LINE:
		break;
	default:
		return 0;
	}

	if (commandType == COMMAND_DOZER_CONSTRUCT && !loc1)
		return 0;
	else if (commandType == COMMAND_DOZER_CONSTRUCT_LINE && (!loc1 || !loc2))
		return 0;

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_loc = *loc1;
	commandNode->m_templateID = thing->getTemplateID();
	commandNode->m_angle = angle;
	if (commandType == COMMAND_DOZER_CONSTRUCT_LINE)
		commandNode->m_loc2 = *loc2;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newDoSpecialCommand(CommandType commandType, UnsignedInt specialPowerID, UnsignedInt commandOptions)
{
	switch (commandType)
	{
	case COMMAND_DO_SPECIAL_POWER:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_specialPowerID = specialPowerID;
	commandNode->m_specialOptions = commandOptions;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newDoSpecialAtLocCommand(CommandType commandType, UnsignedInt specialPowerID, const Coord3D *location, Real angle, const Object* objectInWay, UnsignedInt commandOptions)
{
	switch (commandType)
	{
	case COMMAND_DO_SPECIAL_POWER_AT_POSITION:
	case COMMAND_DO_MOVING_SPECIAL_POWER_AT_POSITION:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_specialPowerID = specialPowerID;
	commandNode->m_loc = *location;
	commandNode->m_angle = angle;
	commandNode->m_targetID = objectInWay == nullptr ? INVALID_ID : objectInWay->getID();
	commandNode->m_specialOptions = commandOptions;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newDoSpecialAtObjCommand(CommandType commandType, UnsignedInt specialPowerID, Object* target, UnsignedInt commandOptions)
{
	switch (commandType)
	{
	case COMMAND_DO_SPECIAL_POWER_AT_OBJECT:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_specialPowerID = specialPowerID;
	commandNode->m_targetID = target->getID();
	commandNode->m_specialOptions = commandOptions;
	return commandNode->m_nodeID;
}

UnsignedInt CommandSequence::newSwitchWeaponCommand(CommandType commandType, WeaponSlotType weaponSlot)
{
	switch (commandType)
	{
	case COMMAND_SWITCH_WEAPON:
		break;
	default:
		return 0;
	}

	CommandNode* commandNode = newCommandNode(commandType);
	commandNode->m_weaponSlot = weaponSlot;
	return commandNode->m_nodeID;
}

Bool CommandSequence::includeUnitInCommand(UnsignedInt commandNodeID, Object *obj)
{
	if (!obj)
		return false;

	CommandNode *commandNode = findCommandByID(commandNodeID);
	CommandNode *lastCommandNode = findCommandByID(obj->getLastCommand());

	if (lastCommandNode && isEndCommand(lastCommandNode->m_commandType))
		return false;

	commandNode->m_nextCommand.insert(std::make_pair(obj->getID(), 0));

	if (lastCommandNode)
	{
		ObjectToNodeMap::iterator it = lastCommandNode->m_nextCommand.find(obj->getID());
		if (it == lastCommandNode->m_nextCommand.end() || it->second)
			// very wrong
			return false;
		it->second = commandNodeID;

		obj->setLastCommand(commandNodeID);
	}
	else
	{
		obj->setCurrentCommand(commandNodeID);
		obj->setLastCommand(commandNodeID);
		obj->setCommandVerifyStatus(STATUS_FRESH);
	}

	return true;
}

Bool CommandSequence::includeUnitInTypicalMovingCommand(UnsignedInt commandNodeID, Object *obj, Coord3D objDest)
{
	if (!includeUnitInCommand(commandNodeID, obj))
		return false;

	CommandNode *commandNode = findCommandByID(commandNodeID);

	if (isTypicalMovingCommand(commandNode->m_commandType))
		commandNode->m_allocatedDest.insert(std::make_pair(obj->getID(), objDest));

	return true;
}

Bool CommandSequence::isTypicalMovingCommand(CommandType commandType)
{
	switch (commandType)
	{
		case COMMAND_DO_MOVETO:
		case COMMAND_DO_REVERSE_MOVETO:
		case COMMAND_DO_MOVING_SPECIAL_POWER_AT_POSITION:
			return true;
		default:
			return false;
	}
}

inline Bool CommandSequence::isEndCommand(CommandType commandType)
{
	switch (commandType)
	{
	case COMMAND_DO_GUARD_POSITION:
		return true;
	default:
		return false;
	}
}

inline Bool CommandSequence::hasEndCommand(Object *obj)
{
	CommandNode *cmdNode = findCommandByID(obj->getLastCommand());
	if (isEndCommand(cmdNode->m_commandType))
		return true;
	return false;
}

inline Bool CommandSequence::isLocatableCommand(UnsignedInt commandNodeID)
{
	CommandNode *cmdNode = findCommandByID(commandNodeID);
	if (cmdNode == nullptr)
		return false;

	switch (cmdNode->m_commandType)
	{
		case COMMAND_NONE:
		case COMMAND_DO_WEAPON:
		case COMMAND_DO_SPECIAL_POWER:
		case COMMAND_DO_EXIT:
		case COMMAND_DO_EVACUATE:
		case COMMAND_INTERNET_HACK:
		case COMMAND_SWITCH_WEAPON:
		case COMMAND_TOGGLE_HOLD_FIRE:
		case COMMAND_TOGGLE_DEPLOY:
			return false;
			break;
		case COMMAND_DO_GUARD_OBJECT:
		case COMMAND_DO_ATTACK_OBJECT:
		case COMMAND_DO_FORCE_ATTACK_OBJECT:
		case COMMAND_DOZER_CONSTRUCT:
		case COMMAND_DO_SPECIAL_POWER_AT_OBJECT:
		case COMMAND_GET_REPAIRED:
		case COMMAND_GET_HEALED:
		case COMMAND_DO_REPAIR:
		case COMMAND_RESUME_CONSTRUCTION:
		case COMMAND_ENTER:
		case COMMAND_DOCK:
			return true;
			break;
		case COMMAND_DO_COMBATDROP:
		{
			if (!cmdNode->m_targetID || TheGameLogic->findObjectByID(cmdNode->m_targetID))
				return true;
			else
				return false;
		}
		default:
			return true;
	}
}

void CommandSequence::cleanUpEmptyCommand(UnsignedInt commandNodeID)
{
	CommandNode *cmdNode = findCommandByID(commandNodeID);

	if (cmdNode->m_nextCommand.empty())
	{
		m_commandNodeCount--;
		delete cmdNode;
	}
}

UnsignedInt CommandSequence::getFirstLocatableCommand(Object *obj)
{
	if (!obj)
		return 0;

	ObjectID objID = obj->getID();
	UnsignedInt curCmdID = obj->getCurrentCommand();
	while (curCmdID && !isLocatableCommand(curCmdID))
	{
		curCmdID = findCommandByID(curCmdID)->m_nextCommand.find(objID)->second;
	}

	return curCmdID;
}

UnsignedInt CommandSequence::getNextLocatableCommand(Object* obj, UnsignedInt commandNodeID)
{
	if (!obj)
		return 0;

	ObjectID objID = obj->getID();
	UnsignedInt curCmdID = commandNodeID;
	do
	{
		curCmdID = findCommandByID(curCmdID)->m_nextCommand.find(objID)->second;
	}
	while (curCmdID && !isLocatableCommand(curCmdID));

	return curCmdID;
}

Bool CommandSequence::getCommandLoc(UnsignedInt commandNodeID, Coord3D &loc)
{
	if (!isLocatableCommand(commandNodeID))
		return false;

	CommandNode *cmdNode = findCommandByID(commandNodeID);

	Object* obj = nullptr;

	switch (cmdNode->m_commandType)
	{
		case COMMAND_DO_MOVETO:
		case COMMAND_DO_REVERSE_MOVETO:
		case COMMAND_DO_ATTACK_MOVETO:
		case COMMAND_DO_GUARD_POSITION:
		case COMMAND_DO_WEAPON_AT_LOCATION:
		case COMMAND_DO_SPECIAL_POWER_AT_POSITION:
		case COMMAND_DO_MOVING_SPECIAL_POWER_AT_POSITION:
		case COMMAND_DOZER_CONSTRUCT:
		case COMMAND_DOZER_CONSTRUCT_LINE:
			loc = cmdNode->m_loc;
			break;
		case COMMAND_DO_GUARD_OBJECT:
		case COMMAND_DO_ATTACK_OBJECT:
		case COMMAND_DO_FORCE_ATTACK_OBJECT:
		case COMMAND_DO_SPECIAL_POWER_AT_OBJECT:
		case COMMAND_GET_REPAIRED:
		case COMMAND_GET_HEALED:
		case COMMAND_DO_REPAIR:
		case COMMAND_RESUME_CONSTRUCTION:
		case COMMAND_ENTER:
		case COMMAND_DOCK:
			obj = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (obj == nullptr)
				return false;
			loc = *obj->getPosition();
			break;
		case COMMAND_DO_COMBATDROP:
			if (cmdNode->m_targetID == INVALID_ID)
			{
				loc = cmdNode->m_loc;
			}
			else
			{
				obj = TheGameLogic->findObjectByID(cmdNode->m_targetID);
				if (obj == nullptr)
					return false;
				loc = *obj->getPosition();
			}
			break;
		default:
			return false;
	}
	return true;
}

CommandSequence::CommandNode *CommandSequence::findCommandByID(UnsignedInt commandNodeID)
{
	//if (m_prevNodeID == commandNodeID)
	//	return m_prevNode;

	std::map<UnsignedInt, CommandNode*>::iterator it;
	it = m_allNodes.find(commandNodeID);
	if (it == m_allNodes.end())
		return nullptr;

	return it->second;
}

UnsignedInt CommandSequence::removeCurrentCommand(Object *obj, Bool doCleanUp)
{
	CommandNode *node = findCommandByID(obj->getCurrentCommand());

	if (node == nullptr)
		return 0;

	ObjectToNodeMap::iterator it = node->m_nextCommand.find(obj->getID());

	if (it == node->m_nextCommand.end())
		return 0;

	UnsignedInt ret = it->second;

	if (ret)
	{
		obj->setCurrentCommand(it->second);
		obj->setCommandVerifyStatus(STATUS_FRESH);
	}
	else
	{
		obj->setCurrentCommand(0);
		obj->setLastCommand(0);
		obj->setCommandVerifyStatus(STATUS_NONE);
	}

	node->m_nextCommand.erase(it);

	if (isTypicalMovingCommand(node->m_commandType))
	{
		ObjectToLocMap::iterator lit = node->m_allocatedDest.find(obj->getID());
		node->m_allocatedDest.erase(lit);
	}

	if (doCleanUp && node->m_nextCommand.empty())
	{
		IDToNodeMap::iterator iit = m_allNodes.find(node->m_nodeID);
		if (iit != m_allNodes.end())
			m_allNodes.erase(iit);
		delete node;
		m_commandNodeCount--;
	}

	return ret;
}


void CommandSequence::doCurrentCommand(Object *obj, CommandVerifyStatus cvs)
{
	CommandNode *cmdNode = findCommandByID(obj->getCurrentCommand());
	if (!cmdNode)
		return;

	prepareCommandForVerification(obj);

	CommandType cmdType = cmdNode->m_commandType;
	const CommandSourceType cmdSource = CMD_FROM_PLAYER;

	AIUpdateInterface *ai = obj->getAIUpdateInterface();

	Object *target;

	switch (cmdType)
	{
		case COMMAND_DO_MOVETO:
		case COMMAND_DO_REVERSE_MOVETO:
		{
			Coord3D unitLoc;
			ObjectToLocMap::iterator it = cmdNode->m_allocatedDest.find(obj->getID());
			if (it == cmdNode->m_allocatedDest.end())
				break;
			unitLoc = it->second;

			if (ai)
			{
				ai->handleStealthCombatUnitMove();
				if (cmdType == COMMAND_DO_MOVETO)
					ai->aiMoveToPosition(&unitLoc, cmdSource);
				else
					ai->aiReverseMoveToPosition(&unitLoc, cmdSource);
			}
			break;
		}
		case COMMAND_DO_ATTACK_MOVETO:
		{
			obj->releaseWeaponLock(LOCKED_TEMPORARILY);

			if (ai)
			{
				if (obj->isAbleToAttack())
					ai->aiAttackMoveToPosition(&cmdNode->m_loc, cmdNode->m_maxShotsToFire, cmdSource);
				else
					ai->aiMoveToPosition(&cmdNode->m_loc, cmdSource);
			}
			break;
		}
		case COMMAND_DO_GUARD_POSITION:
		{
			if (ai)
			{
				ai->aiGuardPosition(&cmdNode->m_loc, cmdNode->m_guardMode, cmdSource);
			}
			break;
		}
		case COMMAND_DO_GUARD_OBJECT:
		{
			target = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (ai && target)
			{
				ai->aiGuardObject(target, cmdNode->m_guardMode, cmdSource);
			}
			break;
		}
		case COMMAND_DO_WEAPON:
		case COMMAND_DO_WEAPON_AT_LOCATION:
		{
			Coord3D attackPos = cmdType == COMMAND_DO_WEAPON ? *obj->getPosition() : cmdNode->m_loc;
			if (cmdNode->m_weaponSlot == WEAPON_NONE)
			{
				obj->releaseWeaponLock(LOCKED_TEMPORARILY);
			}
			else
			{
				obj->setWeaponLock(cmdNode->m_weaponSlot, LOCKED_TEMPORARILY);
			}

			//This code allows garrisoned buildings to force attack a ground position
			//-----------------------------------------------------------------------
			//Determine if this object is a garrisoned container capable of firing!
			//If so, order everyone inside to attack as well!
			ContainModuleInterface* contain = obj->getContain();
			if (contain && contain->isPassengerAllowedToFire())
			{
				//Loop through each member and order them to attack the same target (if possible)
				const ContainedItemsList* items = contain->getContainedItemsList();
				if (items)
				{
					for (ContainedItemsList::const_iterator it = items->begin(); it != items->end(); ++it)
					{
						Object* garrisonedMember = *it;

						if (!contain->isPassengerAllowedToFire(garrisonedMember->getID())) continue;

						CanAttackResult result = garrisonedMember->getAbleToUseWeaponAgainstTarget(ATTACK_NEW_TARGET, nullptr, &attackPos, cmdSource);
						if (result == ATTACKRESULT_POSSIBLE || result == ATTACKRESULT_POSSIBLE_AFTER_MOVING)
						{
							AIUpdateInterface* memberAI = garrisonedMember->getAI();
							if (memberAI)
							{
								memberAI->aiAttackPosition(&attackPos, cmdNode->m_maxShotsToFire, cmdSource);
							}
						}
					}
				}
			}

			//Also handle slaves. If we have slaves, then order them to stop too!
			SpawnBehaviorInterface* spawnInterface = obj->getSpawnBehaviorInterface();
			if (spawnInterface && !spawnInterface->doSlavesHaveFreedom())
			{
				spawnInterface->orderSlavesToAttackPosition(&attackPos, cmdNode->m_maxShotsToFire, cmdSource);
			}

			if (ai)
			{
				ai->aiAttackPosition(&attackPos, cmdNode->m_maxShotsToFire, cmdSource);
			}

			if (cmdNode->m_releaseAfter)
			{
				obj->releaseWeaponLock(LOCKED_TEMPORARILY);
			}

			break;
		}
		case COMMAND_DO_ATTACK_OBJECT:
		case COMMAND_DO_FORCE_ATTACK_OBJECT:
		{
			Bool forced = cmdType == COMMAND_DO_FORCE_ATTACK_OBJECT;

			target = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!target)
				break;

			if (cmdNode->m_weaponSlot == WEAPON_NONE)
			{
				obj->releaseWeaponLock(LOCKED_TEMPORARILY);
			}
			else
			{
				obj->setWeaponLock(cmdNode->m_weaponSlot, LOCKED_TEMPORARILY);
			}

			//Determine if this object is a garrisoned container capable of firing!
			//If so, order everyone inside to attack as well!
			ContainModuleInterface* contain = obj->getContain();
			if (contain && contain->isPassengerAllowedToFire())
			{
				//Loop through each member and order them to attack the same target (if possible)
				const ContainedItemsList* items = contain->getContainedItemsList();
				if (items)
				{
					for (ContainedItemsList::const_iterator it = items->begin(); it != items->end(); ++it)
					{
						Object* garrisonedMember = *it;

						if (!contain->isPassengerAllowedToFire(garrisonedMember->getID())) continue;

						CanAttackResult result = garrisonedMember->getAbleToAttackSpecificObject(forced ? ATTACK_NEW_TARGET_FORCED : ATTACK_NEW_TARGET, target, cmdSource);
						if (result == ATTACKRESULT_POSSIBLE || result == ATTACKRESULT_POSSIBLE_AFTER_MOVING)
						{
							AIUpdateInterface* memberAI = garrisonedMember->getAI();
							if (memberAI)
							{
								if (forced)
									memberAI->aiForceAttackObject(target, cmdNode->m_maxShotsToFire, cmdSource);
								else
									memberAI->aiAttackObject(target, cmdNode->m_maxShotsToFire, cmdSource);
							}
						}
					}
				}
			}

			//Do a check to see if we have a hive object that has slaved objects.
			SpawnBehaviorInterface* spawnInterface = obj->getSpawnBehaviorInterface();
			if (spawnInterface && !spawnInterface->doSlavesHaveFreedom())
			{
				spawnInterface->orderSlavesToAttackTarget(target, cmdNode->m_maxShotsToFire, cmdSource);
			}

			//Order the specific group object to attack!
			if (ai && obj != target)
			{
				if (forced)
					ai->aiForceAttackObject(target, cmdNode->m_maxShotsToFire, cmdSource);
				else
					ai->aiAttackObject(target, cmdNode->m_maxShotsToFire, cmdSource);
			}
			break;
		}
		case COMMAND_DOZER_CONSTRUCT:
		{
			const ThingTemplate* thing = TheThingFactory->findByTemplateID(cmdNode->m_templateID);
			if (!thing)
				break;
			TheBuildAssistant->buildObjectNow(obj, thing, &cmdNode->m_loc, cmdNode->m_angle,
				obj->getControllingPlayer());
			break;
		}
		// unused
		case COMMAND_DOZER_CONSTRUCT_LINE:
		{
			const ThingTemplate* thing = TheThingFactory->findByTemplateID(cmdNode->m_templateID);
			if (!thing)
				break;
			TheBuildAssistant->buildObjectLineNow(obj, thing, &cmdNode->m_loc, &cmdNode->m_loc2, cmdNode->m_angle,
				obj->getControllingPlayer());
			break;
		}
		case COMMAND_DO_SPECIAL_POWER:
		{
			const SpecialPowerTemplate *spTemplate = TheSpecialPowerStore->findSpecialPowerTemplateByID(cmdNode->m_specialPowerID);
			SpecialPowerModuleInterface* mod = obj->getSpecialPowerModule(spTemplate);
			if (!mod || !mod->isReady())
				break;

			mod->doSpecialPower(cmdNode->m_specialOptions);
			obj->friend_setUndetectedDefector(FALSE);// My secret is out

			break;
		}
		case COMMAND_DO_SPECIAL_POWER_AT_POSITION:
		case COMMAND_DO_MOVING_SPECIAL_POWER_AT_POSITION:
		{
			const SpecialPowerTemplate *spTemplate = TheSpecialPowerStore->findSpecialPowerTemplateByID(cmdNode->m_specialPowerID);
			SpecialPowerModuleInterface *mod = obj->getSpecialPowerModule(spTemplate);
			if (!mod || !mod->isReady())
				break;

			Coord3D unitLoc = cmdNode->m_loc;

			if (cmdType == COMMAND_DO_MOVING_SPECIAL_POWER_AT_POSITION)
			{
				ObjectToLocMap::iterator it = cmdNode->m_allocatedDest.find(obj->getID());
				if (it == cmdNode->m_allocatedDest.end())
					break;

				unitLoc = it->second;
			}

			mod->doSpecialPowerAtLocation(&unitLoc, cmdNode->m_angle, cmdNode->m_specialOptions);

			obj->friend_setUndetectedDefector(FALSE);// My secret is out

			break;
		}
		case COMMAND_DO_SPECIAL_POWER_AT_OBJECT:
		{
			Object *target = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!target)
				break;

			const SpecialPowerTemplate* spTemplate = TheSpecialPowerStore->findSpecialPowerTemplateByID(cmdNode->m_specialPowerID);
			SpecialPowerModuleInterface* mod = obj->getSpecialPowerModule(spTemplate);
			
			if (!mod || !mod->isReady())
				break;

			mod->doSpecialPowerAtObject(target, cmdNode->m_specialOptions);
			obj->friend_setUndetectedDefector(FALSE);// My secret is out

			break;
		}
		case COMMAND_DO_EXIT:
		{
			Object *objWantToExit = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!objWantToExit || objWantToExit->getContainedBy() != obj)
				break;

			objWantToExit->releaseWeaponLock(LOCKED_TEMPORARILY);	// release any temporary locks.

			if (objWantToExit->getControllingPlayer() != obj->getControllingPlayer())
				break;

			// exit whatever objectWantingToExit is INSIDE of
			ai = objWantToExit->getAIUpdateInterface();
			if (ai)
				ai->aiExit(obj, cmdSource);

			break;
		}
		case COMMAND_DO_EVACUATE:
		{
			ContainModuleInterface *contain = obj->getContain();
			if (contain)
			{
				const ContainedItemsList* items = contain->getContainedItemsList();
				if (items)
				{
					for (ContainedItemsList::const_iterator it = items->begin(); it != items->end(); ++it)
					{
						Object *member = *it;
						ai = member->getAIUpdateInterface();
						if (ai)
						{
							ai->aiExit(obj, CMD_FROM_PLAYER);
						}
					}
				}
			}

			break;
		}
		case COMMAND_DO_COMBATDROP:
		{
			AIUpdateInterface* ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			if (cmdNode->m_targetID == INVALID_ID)
				ai->aiCombatDrop(nullptr, cmdNode->m_loc, cmdSource);
			else
			{
				Object *target = TheGameLogic->findObjectByID(cmdNode->m_targetID);
				if (!target)
					break;

				ai->aiCombatDrop(target, *target->getPosition(), cmdSource);
			}

			break;
		}

		case COMMAND_GET_REPAIRED:
		{
			AIUpdateInterface* ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			Object *repairDepot = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!repairDepot)
				break;

			if (TheActionManager->canGetRepairedAt(obj, repairDepot, cmdSource))
				ai->aiGetRepaired(repairDepot, cmdSource);
			else
				break;

			break;
		}

		case COMMAND_GET_HEALED:
		{
			AIUpdateInterface* ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			Object* healDepot = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!healDepot)
				break;

			if (TheActionManager->canGetHealedAt(obj, healDepot, cmdSource))
				ai->aiGetHealed(healDepot, cmdSource);
			else
				break;

			break;
		}
		case COMMAND_DO_REPAIR:
		{
			AIUpdateInterface* ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			Object *target = TheGameLogic->findObjectByID(cmdNode->m_targetID);

			if (!target)
				break;

			ai->aiRepair(target, cmdSource);

			break;
		}
		case COMMAND_RESUME_CONSTRUCTION:
			break;
		case COMMAND_ENTER:
		{
			AIUpdateInterface *ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			Object *target = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!target)
				break;

			if (TheActionManager->canEnterObject(obj, target, cmdSource, DONT_CHECK_CAPACITY))
			{
				obj->releaseWeaponLock(LOCKED_TEMPORARILY);
				ai->aiEnter(target, cmdSource);
			}
			else
				break;

			break;
		}
		case COMMAND_DOCK:
		{
			AIUpdateInterface* ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			Object* target = TheGameLogic->findObjectByID(cmdNode->m_targetID);
			if (!target)
				break;

			ai->aiDock(target, cmdSource);

			break;
		}
		case COMMAND_INTERNET_HACK:
		{
			AIUpdateInterface *ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			ai->aiDock(target, cmdSource);

			break;
		}
		case COMMAND_TOGGLE_OVERCHARGE:
		{
			OverchargeBehaviorInterface* obi;
			for (BehaviorModule** bmi = obj->getBehaviorModules(); *bmi; ++bmi)
			{
				obi = (*bmi)->getOverchargeBehaviorInterface();
				if (obi)
				{
					obi->toggle();
				}
			}

			break;
		}
		case COMMAND_SWITCH_WEAPON:
		{
			obj->setWeaponLock(cmdNode->m_weaponSlot, LOCKED_PERMANENTLY);

			break;
		}
		case COMMAND_TOGGLE_HOLD_FIRE:
		{
			AIUpdateInterface *ai = obj->getAIUpdateInterface();
			if (!ai)
				break;

			ai->setHoldingFire(!ai->isHoldingFire());

			break;
		}
		case COMMAND_TOGGLE_DEPLOY:
		{
			AIUpdateInterface* ai = obj->getAI();
			DeployStyleAIUpdate* deployAI = ai ? ai->getDeployStyleAIUpdate() : nullptr;
			if (deployAI == nullptr)
				break;

			if (!deployAI->isDeployedOrDeploying())
				deployAI->toggleManualDeploy();
			else
				break;

			break;
		}
		default:
			// shouldn't happen
			break;
	}

	CommandVerifyStatus newCvs = obj->getCommandVerifyStatus();
	if (newCvs == STATUS_AWAITING || newCvs == STATUS_EXIT_VERIFIED)
		removeCurrentCommand(obj);

	if (cvs == STATUS_EXIT_VERIFIED || newCvs == STATUS_EXIT_VERIFIED)
		obj->setCommandVerifyStatus(STATUS_EXIT_VERIFIED);
}

void CommandSequence::proceedNextCommand(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	while (cvs == STATUS_FRESH || (cvs == STATUS_EXIT_VERIFIED && isCurrentExitCommand(obj)))
	{
		doCurrentCommand(obj, cvs);
		cvs = obj->getCommandVerifyStatus();
	}
}

inline void CommandSequence::prepareCommandForVerification(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_FRESH || cvs == STATUS_UNIT_PENDING)
	{
		obj->setCommandVerifyStatus(STATUS_AWAITING);
	}
}

Bool CommandSequence::isStillAwaiting(Object *obj)
{
	return obj->getCommandVerifyStatus() == STATUS_AWAITING;
}

Bool CommandSequence::dozerPendingTask(Object *obj)
{
	if (obj->getCommandVerifyStatus() == STATUS_AWAITING)
	{
		obj->setCommandVerifyStatus(STATUS_DOZER_PENDING);
		return true;
	}

	return false;
}

Bool CommandSequence::oclPending(Object* obj)
{
	if (obj->getCommandVerifyStatus() == STATUS_AWAITING)
	{
		obj->setCommandVerifyStatus(STATUS_OCL_PENDING);
		return true;
	}

	return false;
}

Bool CommandSequence::verifyObjectCommand(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_AWAITING || cvs == STATUS_DOZER_PENDING)
	{
		obj->setCommandVerifyStatus(STATUS_VERIFIED);
		return true;
	}

	return false;
}

Bool CommandSequence::unitPendingCommand(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_AWAITING)
	{
		obj->setCommandVerifyStatus(STATUS_UNIT_PENDING);
		return true;
	}

	return false;
}

Bool CommandSequence::unitOnHeld(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_VERIFIED)
	{
		obj->setCommandVerifyStatus(STATUS_ON_HELD);
		return true;
	}

	return false;
}

Bool CommandSequence::verifyExitCommand(Object *objWantToExit, Object *objectContainingExiter)
{
	CommandNode *cmdNode = findCommandByID(objectContainingExiter->getCurrentCommand());
	if (cmdNode &&
		(((cmdNode->m_commandType == COMMAND_DO_EXIT && cmdNode->m_targetID == objWantToExit->getID()))
			|| cmdNode->m_commandType == COMMAND_DO_EVACUATE))
	{
		objectContainingExiter->addToPendingExitList(objWantToExit);
		CommandVerifyStatus cvs = objectContainingExiter->getCommandVerifyStatus();
		if (cvs == STATUS_AWAITING)
		{
			objectContainingExiter->setCommandVerifyStatus(STATUS_EXIT_VERIFIED);
			return true;
		}
	}

	return false;
}

Bool CommandSequence::verifyOCLPendingCommand(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_OCL_PENDING || cvs == STATUS_OCL_COMPLETE)
	{
		obj->setCommandVerifyStatus(STATUS_OCL_VERIFIED);
		return true;
	}

	return false;
}

Bool CommandSequence::terminateDozerPending(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_DOZER_PENDING)
	{
		obj->setCommandVerifyStatus(STATUS_COMPLETE);
		return true;
	}

	return false;
}

Bool CommandSequence::terminateOCLPending(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_OCL_PENDING)
	{
		obj->setCommandVerifyStatus(STATUS_COMPLETE);
		return true;
	}

	return false;
}

Bool CommandSequence::terminateVerifiedCommand(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_VERIFIED)
	{
		obj->setCommandVerifyStatus(STATUS_COMPLETE);
		return true;
	}

	return false;
}

Bool CommandSequence::confirmExitCommand(Object *objWantToExit, Object *objectContainingExiter)
{
	CommandVerifyStatus cvs = objectContainingExiter->getCommandVerifyStatus();
	if (cvs == STATUS_EXIT_VERIFIED)
	{
		objectContainingExiter->removeFromPendingExitList(objWantToExit);
		return true;
	}

	return false;
}

Bool CommandSequence::terminateOCLVerifiedCommand(Object* obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();
	if (cvs == STATUS_OCL_VERIFIED)
	{
		obj->setCommandVerifyStatus(STATUS_OCL_COMPLETE);
		return true;
	}

	return false;
}

Bool CommandSequence::isCurrentExitCommand(Object *obj)
{
	CommandNode *cmdNode = findCommandByID(obj->getCurrentCommand());

	if (cmdNode && (cmdNode->m_commandType == COMMAND_DO_EXIT || cmdNode->m_commandType == COMMAND_DO_EVACUATE))
		return true;
	return false;
}

// unused
Bool CommandSequence::isExitCommandComplete(Object *obj)
{
	CommandVerifyStatus cvs = obj->getCommandVerifyStatus();

	if (cvs == STATUS_EXIT_VERIFIED && obj->isPendingExitListEmpty())
		return true;
	return false;
}
