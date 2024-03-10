#pragma once
#include "Plugin.h"
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/math/Vec3.h>
#include <mc/network/NetworkBlockPosition.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/PacketSender.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/AddVolumeEntityPacket.h>
#include <mc/network/packet/ChangeDimensionPacket.h>
#include <mc/network/packet/ClientboundMapItemDataPacket.h>
#include <mc/network/packet/LevelChunkPacket.h>
#include <mc/network/packet/PlayerActionPacket.h>
#include <mc/network/packet/PlayerFogPacket.h>
#include <mc/network/packet/RemoveVolumeEntityPacket.h>
#include <mc/network/packet/SetSpawnPositionPacket.h>
#include <mc/network/packet/SpawnParticleEffectPacket.h>
#include <mc/network/packet/StartGamePacket.h>
#include <mc/network/packet/SubChunkPacket.h>
#include <mc/network/packet/SubChunkRequestPacket.h>
#include <mc/network/packet/UpdateBlockPacket.h>
#include <mc/server/common/PropertiesSettings.h>
#include <mc/util/VarIntDataOutput.h>
#include <mc/world/ActorRuntimeID.h>
#include <mc/world/level/ChangeDimensionRequest.h>
#include <mc/world/level/Level.h>

extern ll::Logger logger;
extern void       PatchNetherHeight();