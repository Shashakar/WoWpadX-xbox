#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QStringList>
#include "SDL3/SDL.h"

struct GamepadDevice {
    QString image;
    QString type;
    QString name; 
    SDL_Gamepad* ptr = nullptr;
};

class AvailableDevicesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ImageRole = Qt::UserRole + 1,
        TypeRole,
        NameRole
    };

    explicit AvailableDevicesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const GamepadDevice* getDevice(int index) const;

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

    Q_INVOKABLE void addDevice(const QString& image, const QString& type, const QString& name, SDL_Gamepad* ptr);
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    QList<GamepadDevice> m_devices;
};

class DevicesController : public QObject
{
    Q_OBJECT
        Q_PROPERTY(AvailableDevicesModel* availableDevicesModel READ availableDevicesModel CONSTANT)
        Q_PROPERTY(AvailableDevicesModel* selectedDeviceModel READ selectedDeviceModel CONSTANT)

public:
    explicit DevicesController(QObject* parent = nullptr);

    AvailableDevicesModel* availableDevicesModel();
    AvailableDevicesModel* selectedDeviceModel();

    Q_INVOKABLE void selectDevice(int index); 

private:
    AvailableDevicesModel m_availableDevices;
    AvailableDevicesModel m_selectedDevice;

    void updateControllerList();
    void updateSelectedController();
};
