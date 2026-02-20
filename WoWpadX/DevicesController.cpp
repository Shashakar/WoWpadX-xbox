#include "DevicesController.h"
#include "ControllerManager.h"
#include <QDebug>

// --- AvailableDevicesModel Implementation ---
AvailableDevicesModel::AvailableDevicesModel(QObject* parent) : QAbstractListModel(parent) {}

int AvailableDevicesModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return m_devices.count();
}

QVariant AvailableDevicesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_devices.count())
        return QVariant();

    const GamepadDevice& device = m_devices.at(index.row());

    switch (role) {
    case ImageRole: return device.image;
    case TypeRole:  return device.type;
    case NameRole:  return device.name;
    default:        return QVariant();
    }
}

QHash<int, QByteArray> AvailableDevicesModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ImageRole] = "image";
    roles[TypeRole] = "type";
    roles[NameRole] = "name";
    return roles;
}

const GamepadDevice* AvailableDevicesModel::getDevice(int index) const {
    if (index < 0 || index >= m_devices.size())
        return nullptr;
    return &m_devices.at(index);
}

void AvailableDevicesModel::addDevice(const QString& image, const QString& type, const QString& name, SDL_Gamepad* ptr) {
    beginInsertRows(QModelIndex(), m_devices.count(), m_devices.count());
    m_devices.append({ image, type, name, ptr });
    endInsertRows(); 
    emit countChanged();
}

void AvailableDevicesModel::clear() {
    beginResetModel();
    m_devices.clear();
    endResetModel();
    emit countChanged();
}

// --- ControllerDevices Implementation ---
DevicesController::DevicesController(QObject* parent) : QObject(parent) {
    ControllerManager* manager = ControllerManager::instance();

    connect(manager, &ControllerManager::controllersChanged,
        this, &DevicesController::updateControllerList);

    connect(manager, &ControllerManager::activeControllerChanged,
        this, &DevicesController::updateSelectedController);

    updateControllerList();
    updateSelectedController();
}

void DevicesController::updateControllerList() { 
    m_availableDevices.clear();

    auto controllerList = ControllerManager::instance()->getControllers();

    for (int i = 0; i < controllerList.size(); i++)
    {
        QString name = SDL_GetGamepadName(controllerList[i]);
        QString type;

        if (name.contains("DualShock", Qt::CaseInsensitive) || name.contains("DualSense"))
            type = "PlayStation";
        else if (name.contains("Xbox", Qt::CaseInsensitive))
            type = "Xbox";
        else if (name.contains("Switch", Qt::CaseInsensitive) || name.contains("Joy-Con", Qt::CaseInsensitive))
            type = "Nintendo";
        else
            type = "Generic";

        QString image;
        if (type == "PlayStation")
            image = "qrc:/Resources/ds4.png";
        else if (type == "Xbox")
            image = "qrc:/Resources/xinput.png";
        else if (type == "Nintendo")
            image = "qrc:/Resources/switch.png";
        else
            image = "qrc:/Resources/generic.png";


        m_availableDevices.addDevice(image, type, name, controllerList[i]);
    }
}

void DevicesController::updateSelectedController() {

    if (ControllerManager::instance()->getActiveController() == NULL)
    {
        m_selectedDevice.clear();
        return;
    }

    QString name = SDL_GetJoystickName(SDL_GetGamepadJoystick(ControllerManager::instance()->getActiveController()));
    QString type;

    if (name.contains("DualShock", Qt::CaseInsensitive) || name.contains("DualSense"))
        type = "PlayStation";
    else if (name.contains("Xbox", Qt::CaseInsensitive))
        type = "Xbox";
    else if (name.contains("Switch", Qt::CaseInsensitive) || name.contains("Joy-Con", Qt::CaseInsensitive))
        type = "Nintendo";
    else
        type = "Generic";

    QString image;
    if (type == "PlayStation")
        image = "qrc:/Resources/ds4.png";
    else if (type == "Xbox")
        image = "qrc:/Resources/xinput.png";
    else if (type == "Nintendo")
        image = "qrc:/Resources/switch.png";
    else
        image = "qrc:/Resources/generic.png";


    m_selectedDevice.clear();
    m_selectedDevice.addDevice(image, type, name, ControllerManager::instance()->getActiveController());
}

AvailableDevicesModel* DevicesController::availableDevicesModel() {
    return &m_availableDevices;
}

AvailableDevicesModel* DevicesController::selectedDeviceModel() {
    return &m_selectedDevice;
}

void DevicesController::selectDevice(int index) {
    if (index < 0 || index >= m_availableDevices.rowCount())
        return;

    const GamepadDevice* device = m_availableDevices.getDevice(index);
    if (!device || !device->ptr)
        return;

    auto controllerList = ControllerManager::instance()->getControllers();


    bool found = std::find(controllerList.begin(), controllerList.end(), device->ptr) != controllerList.end();

    if (!found)
        return;

    ControllerManager::instance()->SetController(device->ptr, SDL_GetGamepadID(device->ptr));

    m_selectedDevice.clear();
    m_selectedDevice.addDevice(device->image, device->type, device->name, device->ptr);
}
