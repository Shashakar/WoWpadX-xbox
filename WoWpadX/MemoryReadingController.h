#include <QAbstractListModel>
#include <QObject>
#include "Gamepad/gamepad.h"


/*
//
// Memory Debug ListView Model Implementation
//
*/

struct DebugEntry {
    QString name;
    QString address;
    QString value;
};

class MemoryDebugModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ValueRole
    };

    explicit MemoryDebugModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

    void addData(const QString& name, const QString& value);
    void clear();

signals:
    void countChanged();

private:
    QList<DebugEntry> m_entries;
};

/*
//
// Aoe Combobox Model Implementation
//
*/

struct AoeOption {
    QString icon;
    QString label;
    GamepadBinding binding = GamepadBinding::Invalid;
};

class AoeOptionsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IconRole = Qt::UserRole + 1,
        LabelRole
    };

    explicit AoeOptionsModel(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap get(int index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const AoeOption* getOption(int index) const;
    void addOption(const QString& icon, const QString& label, const GamepadBinding& binding);
    void clear();
private:
    QList<AoeOption> m_options;
};

/*
//
// MemoryReadingController class
//
*/

class MemoryReadingController : public QObject {
    Q_OBJECT
        Q_PROPERTY(MemoryDebugModel* debugModel READ debugModel CONSTANT)
        Q_PROPERTY(AoeOptionsModel* aoeConfirmModel READ aoeConfirmModel CONSTANT)
        Q_PROPERTY(AoeOptionsModel* aoeCancelModel READ aoeCancelModel CONSTANT)
        Q_PROPERTY(QString imageMenuConfirm READ imageMenuConfirm NOTIFY imageMenuConfirmChanged)
        Q_PROPERTY(QString imageMenuUp READ imageMenuUp NOTIFY imageMenuUpChanged)
        Q_PROPERTY(QString imageMenuDown READ imageMenuDown NOTIFY imageMenuDownChanged)
        Q_PROPERTY(int aoeConfirmIndex READ aoeConfirmIndex WRITE setAoeConfirmIndex NOTIFY aoeConfirmIndexChanged)
        Q_PROPERTY(int aoeCancelIndex READ aoeCancelIndex WRITE setAoeCancelIndex NOTIFY aoeCancelIndexChanged)

public:
    explicit MemoryReadingController(QObject* parent = nullptr);

    MemoryDebugModel* debugModel();
    AoeOptionsModel* aoeConfirmModel();
    AoeOptionsModel* aoeCancelModel();

    int aoeConfirmIndex() const;
    int aoeCancelIndex() const;


    void setAoeConfirmIndex(int index);
    void setAoeCancelIndex(int index);

    QString imageMenuConfirm() const;
    QString imageMenuUp() const;
    QString imageMenuDown() const;


    void buttonStyleChanged();

    Q_INVOKABLE void buttonRefreshValuesClicked();
    Q_INVOKABLE void textMoreInfoClicked();
    Q_INVOKABLE void aoeOverrideChanged(int confirmIndex, int cancelIndex);

signals:
    void imageMenuConfirmChanged();
    void imageMenuUpChanged();
    void imageMenuDownChanged();
    void aoeConfirmIndexChanged();
    void aoeCancelIndexChanged();

private:
    MemoryDebugModel m_debugModel;
    AoeOptionsModel m_aoeConfirmOptions;
    AoeOptionsModel m_aoeCancelOptions;


    int m_aoeConfirmIndex = -1;
    int m_aoeCancelIndex = -1;

    QString m_imageMenuConfirm = "qrc:/Controllers/DS/Buttons/CP_R_DOWN.png";
    QString m_imageMenuUp = "qrc:/Controllers/DS/Buttons/CP_L_UP.png";
    QString m_imageMenuDown = "qrc:/Controllers/DS/Buttons/CP_L_DOWN.png";
};