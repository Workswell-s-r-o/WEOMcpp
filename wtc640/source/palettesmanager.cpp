#include "core/wtc640/palettesmanager.h"

#include "core/logging.h"
#include "core/wtc640/propertieswtc640.h"
#include "core/wtc640/propertyidwtc640.h"
#include "core/properties/transactionchanges.h"
#include "core/properties/properties.inl"

#include <boost/algorithm/algorithm.hpp>

namespace core
{

PalettesManager::PalettesManager(const std::shared_ptr<PropertiesWtc640>& properties) :
    m_properties(properties)
{
    m_properties.get()->transactionFinished.connect([this](const core::TransactionSummary& transactionSummary)
    {
        const auto& userPalettes = core::PropertyIdWtc640::PALETTES_USER_CURRENT;
        if (transactionSummary.getTransactionChanges().anyStatusChanged(userPalettes.begin(), userPalettes.end()))
        {
            setPalettesFromDevice(std::nullopt);
        }
        if (transactionSummary.getTransactionChanges().valueChanged(core::PropertyIdWtc640::PALETTE_INDEX_CURRENT))
        {
            setIndexFromDevice(std::nullopt);
        }
    });
}

PalettesManager::~PalettesManager()
{
}

std::shared_ptr<PalettesManager> PalettesManager::createInstance(const std::shared_ptr<PropertiesWtc640>& properties)
{
    const auto instance = std::shared_ptr<PalettesManager>(new PalettesManager(properties));
    instance->m_weakThis = instance;

    return instance;
}

const std::vector<core::Palette> PalettesManager::getPalettes()
{
    updatePalettesFromDevice();

    return getAllPalettes();
}

const core::Palette& PalettesManager::getDefaultGrayPalette()
{
    static const core::Palette DEFAULT_GREY_PALETTE;
    return DEFAULT_GREY_PALETTE;
}

const core::Palette PalettesManager::getSelectedPalette()
{
    {
        std::unique_lock lock(m_mutex);
        if (!m_currentPaletteIndexFromDevice)
        {
            lock.unlock();
            updateIndexFromDevice();
        }
    }

    updatePalettesFromDevice();

    const std::scoped_lock lock(m_mutex);
    if(!m_currentPaletteIndexFromDevice.has_value() || m_currentPaletteIndexFromDevice.value() >= m_palettesFromDevice.value().size())
    {
        return getDefaultGrayPalette();
    }

    return m_palettesFromDevice.value()[m_currentPaletteIndexFromDevice.value()];
}

void PalettesManager::updatePalettesFromDevice()
{
    const std::scoped_lock lock(m_mutex);

    if (!m_palettesFromDevice.has_value())
    {
        m_palettesFromDevice = std::vector<core::Palette>{};
        assert(m_palettesFromDevice.has_value());

        std::thread([this, instance = m_weakThis.lock()]() mutable
        {
            std::vector<core::Palette> palettesFromDevice;
            {
                const auto transaction = m_properties->createConnectionExclusiveTransactionWtc640(false);

                const size_t PALETTES_COUNT = core::PropertyIdWtc640::getPalettesCount();
                for (int i = 0; i < PALETTES_COUNT; i++)
                {
                    const auto palette = transaction.getConnectionExclusiveTransaction().getPropertiesTransaction().getValue<core::Palette>(core::PropertyIdWtc640::getPaletteCurrentId(i));
                    if (palette.containsValue())
                    {
                        palettesFromDevice.push_back(palette.getValue());
                    }
                }
            }
            setPalettesFromDevice(palettesFromDevice);
        }).detach();
    }
}

void PalettesManager::updateIndexFromDevice()
{
    std::thread([this, instance = m_weakThis.lock()]() mutable
    {
        auto transaction = m_properties->tryCreatePropertiesTransaction(std::chrono::milliseconds(0));
        if(!transaction.has_value())
        {
            return;
        }
        const auto index = transaction.value().getValue<unsigned>(core::PropertyIdWtc640::PALETTE_INDEX_CURRENT);
        if(index.containsValue())
        {
            setIndexFromDevice(index.getValue());
        }
    }).detach();
}

void PalettesManager::setPalettesFromDevice(const std::optional<std::vector<core::Palette>>& palettesFromDevice)
{
    {
        const std::scoped_lock lock(m_mutex);

        m_palettesFromDevice = palettesFromDevice;
    }

    palettesChanged();
}

void PalettesManager::setIndexFromDevice(const std::optional<uint8_t>& indexFromDevice)
{
    {
        const std::scoped_lock lock(m_mutex);
        
        m_currentPaletteIndexFromDevice = indexFromDevice;
    }

    indexChanged();
}

const std::vector<core::Palette> core::PalettesManager::getAllPalettes()
{
    const std::scoped_lock lock(m_mutex);
    if (m_palettesFromDevice.has_value())
    {
        return {m_palettesFromDevice->begin(), m_palettesFromDevice->end()};
    }

    return {};
}

} // namespace core
