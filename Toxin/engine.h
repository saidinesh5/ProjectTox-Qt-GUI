#ifndef ENGINE_H
#define ENGINE_H

#include <QObject>
#include "core.h"

class Engine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
public:
    explicit Engine(QObject *parent = 0);

signals:
    void usernameChanged();

public slots:
    QString username() const;
    bool setUsername(const QString& name);

private:
    Core *m_core;
    QString m_username;
};

#endif // ENGINE_H
