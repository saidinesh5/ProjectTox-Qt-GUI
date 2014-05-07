#ifndef ENGINE_H
#define ENGINE_H

#include <QObject>
#include "core.h"

class Engine : public QObject
{
    Q_OBJECT
public:
    explicit Engine(QObject *parent = 0);

signals:

public slots:

private:
    Core *m_core;
};

#endif // ENGINE_H
