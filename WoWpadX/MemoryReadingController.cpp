#include "MemoryReadingController.h"
#include "ControllerManager.h"
#include "Gamepad/gamepad.h"
#include "BindManager.h"
#include "AppSettings.h"
#include "WoWReader.h"
#include <QDebug>
#include <vector>
#include <algorithm>
#include <QDesktopServices>

/*
//
// Memory Debug ListView Model Implementation
//
*/

MemoryDebugModel::MemoryDebugModel(QObject* parent) : QAbstractListModel(parent) {
}

int MemoryDebugModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return m_entries.count();
}

QVariant MemoryDebugModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.count())
        return QVariant();

    const DebugEntry& entry = m_entries.at(index.row());

    switch (role) {
    case NameRole: return entry.name;
    case ValueRole: return entry.value;
    default: return QVariant();
    }
}

QHash<int, QByteArray> MemoryDebugModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ValueRole] = "value";
    return roles;
}

void MemoryDebugModel::addData(const QString& name, const QString& value) {
    beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
    m_entries.append({ name, value });
    endInsertRows();
    emit countChanged();
}

void MemoryDebugModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

/*
//
// Aoe ComboBox Model Implementation
//
*/

AoeOptionsModel::AoeOptionsModel(QObject* parent) : QAbstractListModel(parent) {
}

int AoeOptionsModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return m_options.count();
}

QVariant AoeOptionsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_options.count())
        return QVariant();

    const AoeOption& option = m_options.at(index.row());

    switch (role) {
    case IconRole: return option.icon;
    case LabelRole: return option.label;
    default: return QVariant();
    }
}

QVariantMap AoeOptionsModel::get(int index) const {
    QVariantMap map;
    if (index < 0 || index >= rowCount())
        return map;

    QModelIndex idx = this->index(index, 0);
    map["label"] = data(idx, LabelRole);
    map["buttonImg"] = data(idx, IconRole);
    return map;
}

QHash<int, QByteArray> AoeOptionsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IconRole] = "buttonImg";
    roles[LabelRole] = "label";
    return roles;
}

const AoeOption* AoeOptionsModel::getOption(int index) const {
    if (index < 0 || index >= m_options.size())
        return nullptr;
    return &m_options.at(index);
}

void AoeOptionsModel::addOption(const QString& icon, const QString& label, const GamepadBinding& binding) {
    beginInsertRows(QModelIndex(), m_options.count(), m_options.count());
    m_options.append({ icon, label, binding});
    endInsertRows();
}

void AoeOptionsModel::clear() {
    beginResetModel();
    m_options.clear();
    endResetModel(); 
}


/*
//
// MemoryReadingController class Implementation
//
*/

MemoryReadingController::MemoryReadingController(QObject* parent) : QObject(parent) {

    connect(AppSettings::instance(), &AppSettings::buttonStyleChanged,
        this, &MemoryReadingController::buttonStyleChanged);
    connect(AppSettings::instance(), &AppSettings::memoryAoeCancelChanged,
        this, &MemoryReadingController::buttonStyleChanged);
    connect(AppSettings::instance(), &AppSettings::memoryAoeConfirmChanged,
        this, &MemoryReadingController::buttonStyleChanged);

    buttonStyleChanged();
}

int MemoryReadingController::aoeConfirmIndex() const { return m_aoeConfirmIndex; }
int MemoryReadingController::aoeCancelIndex() const { return m_aoeCancelIndex; }
QString MemoryReadingController::imageMenuConfirm() const { return m_imageMenuConfirm; }
QString MemoryReadingController::imageMenuUp() const { return m_imageMenuUp; }
QString MemoryReadingController::imageMenuDown() const { return m_imageMenuDown; }

void MemoryReadingController::setAoeConfirmIndex(int index) {
    if (m_aoeConfirmIndex != index) {
        m_aoeConfirmIndex = index;
        emit aoeCancelIndexChanged();
    }
}

void MemoryReadingController::setAoeCancelIndex(int index) {
    if (m_aoeCancelIndex != index) {
        m_aoeCancelIndex = index;
        emit aoeCancelIndexChanged();
    }
}


void MemoryReadingController::buttonStyleChanged()
{
    m_imageMenuConfirm = ControllerManager::instance()->GetButtonIcon(GamepadBinding::South);
    m_imageMenuUp = ControllerManager::instance()->GetButtonIcon(GamepadBinding::DPadUp);
    m_imageMenuDown = ControllerManager::instance()->GetButtonIcon(GamepadBinding::DPadDown);

    auto _binds = BindManager::currentKeybinds;

    m_aoeConfirmOptions.clear();
    m_aoeCancelOptions.clear();

    std::vector<GamepadBinding> shoulders = {
        GamepadBinding::LeftShoulder,
        GamepadBinding::LeftTrigger,
        GamepadBinding::RightShoulder,
        GamepadBinding::RightTrigger
    };

    auto eraseButton = [](auto& vec, const auto& button) {
        vec.erase(std::remove(vec.begin(), vec.end(), button), vec.end());
    };


    switch (AppSettings::instance()->modifierStyle())
    {
    case 0:
        eraseButton(shoulders, GamepadBinding::RightShoulder);
        eraseButton(shoulders, GamepadBinding::RightTrigger); 
        break;
    case 1:
        eraseButton(shoulders, GamepadBinding::LeftShoulder);
        eraseButton(shoulders, GamepadBinding::RightShoulder);
        break;
    case 2:
        eraseButton(shoulders, GamepadBinding::LeftShoulder);
        eraseButton(shoulders, GamepadBinding::LeftTrigger);
        break;
    case 3:
        eraseButton(shoulders, GamepadBinding::RightTrigger);
        eraseButton(shoulders, GamepadBinding::LeftTrigger);
        break;
    }

    _binds.erase(
        std::remove_if(_binds.begin(), _binds.end(),
            [&](const Keybind& bind) {
        return std::find(shoulders.begin(), shoulders.end(), bind.bindType) != shoulders.end();
    }),
        _binds.end()
    );

    _binds.erase(
        std::remove_if(_binds.begin(), _binds.end(),
            [](const Keybind& bind) {
        switch (bind.bindType) {
        case GamepadBinding::LeftStick:
        case GamepadBinding::LeftStickUp:
        case GamepadBinding::LeftStickDown:
        case GamepadBinding::LeftStickLeft:
        case GamepadBinding::LeftStickRight:
        case GamepadBinding::LeftStickHorz:
        case GamepadBinding::LeftStickVert:
            return true;
        default:
            return false;
        }
    }),
        _binds.end()
    );

    m_aoeCancelIndex = -1;
    m_aoeConfirmIndex = -1;
    emit aoeConfirmIndexChanged();
    emit aoeCancelIndexChanged();


    QString models[] = { "m_aoeCancelOptions", "m_aoeConfirmOptions"};

    for (int i = 0; i < _binds.size(); i++)
    {

        for (QString model : models)
        {
            if (AppSettings::instance()->memoryAoeCancel() == (int)_binds[i].bindType)
            {
                if (model == "m_aoeCancelOptions")
                {
                    m_aoeCancelOptions.addOption(ControllerManager::instance()->GetButtonIcon(_binds[i].bindType),
                        ControllerManager::instance()->GetButtonName(_binds[i].bindType), _binds[i].bindType);


                    m_aoeCancelIndex = m_aoeCancelOptions.rowCount() - 1;
                    emit aoeCancelIndexChanged();

                }
            }
            else if (AppSettings::instance()->memoryAoeConfirm() == (int)_binds[i].bindType)
            {
                if (model == "m_aoeConfirmOptions")
                {
                    m_aoeConfirmOptions.addOption(ControllerManager::instance()->GetButtonIcon(_binds[i].bindType),
                        ControllerManager::instance()->GetButtonName(_binds[i].bindType), _binds[i].bindType);

                    m_aoeConfirmIndex = m_aoeConfirmOptions.rowCount() - 1;
                    emit aoeConfirmIndexChanged();
                }
            }
            else
            { 
                if (model == "m_aoeConfirmOptions")
                {
                    m_aoeConfirmOptions.addOption(ControllerManager::instance()->GetButtonIcon(_binds[i].bindType),
                        ControllerManager::instance()->GetButtonName(_binds[i].bindType), _binds[i].bindType);
                }
                else
                {
                    m_aoeCancelOptions.addOption(ControllerManager::instance()->GetButtonIcon(_binds[i].bindType),
                        ControllerManager::instance()->GetButtonName(_binds[i].bindType), _binds[i].bindType);
                }
            }
        }
    } 


    emit imageMenuConfirmChanged();
    emit imageMenuDownChanged();
    emit imageMenuUpChanged();
}

MemoryDebugModel* MemoryReadingController::debugModel() {
    return &m_debugModel;
}

AoeOptionsModel* MemoryReadingController::aoeConfirmModel() {
    return &m_aoeConfirmOptions;
}

AoeOptionsModel* MemoryReadingController::aoeCancelModel() {
    return &m_aoeCancelOptions;
}

void MemoryReadingController::buttonRefreshValuesClicked() {
    qDebug() << "Refresh Values Clicked";

    m_debugModel.clear();

    try
    {
        auto health = WoWReader::readPlayerHealth();

        m_debugModel.addData("PlayerBase", 
            QString("%1/%2").arg(health.first).arg(health.second));

        m_debugModel.addData("GameState",
            QString("%1").arg(WoWReader::readGameState())); 

        m_debugModel.addData("MouselookState",
            QString("%1").arg(WoWReader::readMouselook()));

        m_debugModel.addData("WalkState",
            QString("%1").arg(WoWReader::readMovementState()));

        m_debugModel.addData("AoeState",
            QString("%1").arg(WoWReader::readAoeState()));
    }
    catch (int x)
    { 
        qDebug() << "Refresh Error";
    }


}

void MemoryReadingController::textMoreInfoClicked() {
    QDesktopServices::openUrl(QUrl("https://github.com/leoaviana/WoWpadX/blob/master/Docs/PixelBridge.md"));
}

void MemoryReadingController::aoeOverrideChanged(int confirmIndex, int cancelIndex) {
    if (confirmIndex >= 0 && confirmIndex < m_aoeConfirmOptions.rowCount() &&
        cancelIndex >= 0 && cancelIndex < m_aoeCancelOptions.rowCount()) {

        auto cancelOption = m_aoeCancelOptions.getOption(cancelIndex);
        auto confirmOption = m_aoeConfirmOptions.getOption(confirmIndex);

        if (AppSettings::instance()->memoryAoeCancel() != (int)cancelOption->binding)
        {
            AppSettings::instance()->setMemoryAoeCancel((int)cancelOption->binding);
        }

        if (AppSettings::instance()->memoryAoeConfirm() != (int)confirmOption->binding)
        {
            AppSettings::instance()->setMemoryAoeConfirm((int)confirmOption->binding);
        }
        
    }
}
