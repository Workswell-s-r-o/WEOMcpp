#ifndef CORE_PALETTESMANAGER_H
#define CORE_PALETTESMANAGER_H

#include "core/misc/palette.h"

#include <boost/signals2.hpp>
#include <optional>
#include <mutex>


namespace core
{
class PropertiesWtc640;

class PalettesManager
{
    explicit PalettesManager(const std::shared_ptr<PropertiesWtc640>& properties);

public:
    ~PalettesManager();
    PalettesManager(const PalettesManager&) = delete;
    PalettesManager& operator=(const PalettesManager&) = delete;

    static std::shared_ptr<PalettesManager> createInstance(const std::shared_ptr<PropertiesWtc640>& properties);

    const std::vector<core::Palette> getPalettes();

    static const core::Palette& getDefaultGrayPalette();
    const core::Palette getSelectedPalette();

    boost::signals2::signal<void()> palettesChanged;
    boost::signals2::signal<void()> indexChanged;

private:
    void updatePalettesFromDevice();
    void updateIndexFromDevice();

    void setPalettesFromDevice(const std::optional<std::vector<core::Palette>>& palettesFromDevice);
    void setIndexFromDevice(const std::optional<uint8_t>& indexFromDevice);

    std::shared_ptr<PropertiesWtc640> m_properties;

    const std::vector<core::Palette> getAllPalettes();
    std::optional<std::vector<core::Palette>> m_palettesFromDevice;
    std::optional<uint8_t> m_currentPaletteIndexFromDevice;

    std::weak_ptr<PalettesManager> m_weakThis;

    std::mutex m_mutex;
};

} // namespace core

#endif // CORE_PALETTESMANAGER_H
