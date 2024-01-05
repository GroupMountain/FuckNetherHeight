#include "Global.h"
#include "TCHelper.h"

void PatchNetherHeight() {
    uintptr_t regionStart =
        (uintptr_t)ll::memory::resolveSymbol("??0NetherDimension@@QEAA@AEAVILevel@@AEAVScheduler@@@Z");
    std::vector<uint16_t> pattern = TCHelper::splitHex("41 B9 00 00 80 00");
    uintptr_t             address = ModUtils::SigScan(pattern, regionStart, 114514);
    ModUtils::Replace(address, TCHelper::splitHex("41 B9 00 00 80 00"), TCHelper::splitHex8("41 B9 00 00 00 01"));
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    SubChunkRequestHandle,
    ll::memory::HookPriority::Lowest,
    ServerNetworkHandler,
    "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVSubChunkRequestPacket@@@Z",
    void,
    NetworkIdentifier const& id,
    SubChunkRequestPacket&   pkt
) {
    auto pl            = this->getServerPlayer(id, pkt.mClientSubId);
    pkt.mDimensionType = pl.value().getDimensionId();
    return origin(id, pkt);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    SubChunkPacketWrite,
    ll::memory::HookPriority::Lowest,
    SubChunkPacket,
    "?write@SubChunkPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    ChangeDimensionPacketWrite,
    ll::memory::HookPriority::Lowest,
    ChangeDimensionPacket,
    "?write@ChangeDimensionPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionId == 1) {
        this->mDimensionId = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    StartGamePacketWrite,
    ll::memory::HookPriority::Lowest,
    StartGamePacket,
    "?write@StartGamePacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mSettings.mSpawnSettings.dimension == 1) {
        this->mSettings.mSpawnSettings.dimension = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    ClientGenerationHook,
    ll::memory::HookPriority::Lowest,
    PropertiesSettings,
    &PropertiesSettings::isClientSideGenEnabled,
    bool
) {
    return false;
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    AddVolumeEntityPacketHook,
    ll::memory::HookPriority::Lowest,
    AddVolumeEntityPacket,
    "?write@AddVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    ClientboundMapItemDataPacketHook,
    ll::memory::HookPriority::Lowest,
    ClientboundMapItemDataPacket,
    "?write@ClientboundMapItemDataPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimension == 1) {
        this->mDimension = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    RemoveVolumeEntityPacketHook,
    ll::memory::HookPriority::Lowest,
    RemoveVolumeEntityPacket,
    "?write@RemoveVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    SetSpawnPositionPacketHook,
    ll::memory::HookPriority::Lowest,
    SetSpawnPositionPacket,
    "?write@SetSpawnPositionPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPED_INSTANCE_HOOK(
    SpawnParticleEffectPacketHook,
    ll::memory::HookPriority::Lowest,
    SpawnParticleEffectPacket,
    "?write@SpawnParticleEffectPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mVanillaDimensionId == 1) {
        this->mVanillaDimensionId = 2;
    }
    return origin(bs);
}