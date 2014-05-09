#include "engine.h"
#include <QDebug>

Engine::Engine(QObject *parent) :
    QObject(parent),
    m_core(new Core(this))
{
    connect(m_core,SIGNAL(usernameSet(QString)), this, SLOT(setUsername(QString)));
    m_core->start();
    m_core->setUsername("John Doe");
}

QString Engine::username() const
{
    return m_username;
}

bool Engine::setUsername(const QString &name)
{
    if(name != m_username)
    {
        m_username = name;
        m_core->setUsername(name);
        emit usernameChanged();
        return true;
    }

    return false;
}
