/*
    Copyright (C) 2013 by Maxim Biro <nurupo.contributions@gmail.com>

    This file is part of Tox Qt GUI.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the COPYING file for more details.
*/

#include "core.h"
#include "settings.h"

#include <cstdint>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>

const QString Core::CONFIG_FILE_NAME = "data.tox";

Core::Core(QObject *parent) :
    QObject(parent),
    tox(nullptr)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Core::process);
    connect(&Settings::getInstance(), &Settings::dhtServerListChanged, this, &Core::bootstrapDht);
}

Core::~Core()
{
    if (tox) {
        saveConfiguration();
        tox_kill(tox);
    }
}

void Core::onFriendRequest(Tox*/* tox*/, uint8_t* cUserId, uint8_t* cMessage, uint16_t cMessageSize, void* core)
{
    emit static_cast<Core*>(core)->friendRequestRecieved(CUserId::toString(cUserId), CString::toString(cMessage, cMessageSize));
}

void Core::onFriendMessage(Tox*/* tox*/, int friendId, uint8_t* cMessage, uint16_t cMessageSize, void* core)
{
    emit static_cast<Core*>(core)->friendMessageRecieved(friendId, CString::toString(cMessage, cMessageSize));
}

void Core::onFriendNameChange(Tox*/* tox*/, int friendId, uint8_t* cName, uint16_t cNameSize, void* core)
{
    emit static_cast<Core*>(core)->friendUsernameChanged(friendId, CString::toString(cName, cNameSize));
}

void Core::onStatusMessageChanged(Tox*/* tox*/, int friendId, uint8_t* cMessage, uint16_t cMessageSize, void* core)
{
    emit static_cast<Core*>(core)->friendStatusMessageChanged(friendId, CString::toString(cMessage, cMessageSize));
}

void Core::onUserStatusChanged(Tox*/* tox*/, int friendId, uint8_t userstatus, void* core)
{
    Status status;
    switch (userstatus) {
        case TOX_USERSTATUS_NONE:
            status = Status::Online;
            break;
        case TOX_USERSTATUS_AWAY:
            status = Status::Away;
            break;
        case TOX_USERSTATUS_BUSY:
            status = Status::Busy;
            break;
        default:
            status = Status::Online;
            break;
    }
    emit static_cast<Core*>(core)->friendStatusChanged(friendId, status);
}

void Core::onConnectionStatusChanged(Tox*/* tox*/, int friendId, uint8_t status, void* core)
{
    Status friendStatus = status ? Status::Online : Status::Offline;
    emit static_cast<Core*>(core)->friendStatusChanged(friendId, friendStatus);
    if (friendStatus == Status::Offline) {
        static_cast<Core*>(core)->checkLastOnline(friendId);
    }
}

void Core::onAction(Tox*/* tox*/, int friendId, uint8_t *cMessage, uint16_t cMessageSize, void *core)
{
    emit static_cast<Core*>(core)->actionReceived(friendId, CString::toString(cMessage, cMessageSize));
}

void Core::acceptFriendRequest(const QString& userId)
{
    int friendId = tox_add_friend_norequest(tox, CUserId(userId).data());
    if (friendId == -1) {
        emit failedToAddFriend(userId);
    } else {
        emit friendAdded(friendId, userId);
    }
}

void Core::requestFriendship(const QString& friendAddress, const QString& message)
{
    CString cMessage(message);

    int friendId = tox_add_friend(tox, CFriendAddress(friendAddress).data(), cMessage.data(), cMessage.size());
    const QString userId = friendAddress.mid(0, TOX_CLIENT_ID_SIZE * 2);
    // TODO: better error handling
    if (friendId < 0) {
        emit failedToAddFriend(userId);
    } else {
        emit friendAdded(friendId, userId);
    }
}

void Core::sendMessage(int friendId, const QString& message)
{
    CString cMessage(message);

    int messageId = tox_send_message(tox, friendId, cMessage.data(), cMessage.size());
    emit messageSentResult(friendId, message, messageId);
}

void Core::sendAction(int friendId, const QString &action)
{
    CString cMessage(action);
    int ret = tox_send_action(tox, friendId, cMessage.data(), cMessage.size());
    emit actionSentResult(friendId, action, ret);
}

void Core::removeFriend(int friendId)
{
    if (tox_del_friend(tox, friendId) == -1) {
        emit failedToRemoveFriend(friendId);
    } else {
        emit friendRemoved(friendId);
    }
}

void Core::setUsername(const QString& username)
{
    CString cUsername(username);

    if (tox_set_name(tox, cUsername.data(), cUsername.size()) == -1) {
        emit failedToSetUsername(username);
    } else {
        emit usernameSet(username);
    }
}

void Core::setStatusMessage(const QString& message)
{
    CString cMessage(message);

    if (tox_set_status_message(tox, cMessage.data(), cMessage.size()) == -1) {
        emit failedToSetStatusMessage(message);
    } else {
        emit statusMessageSet(message);
    }
}

void Core::setStatus(Status status)
{
    TOX_USERSTATUS userstatus;
    switch (status) {
        case Status::Online:
            userstatus = TOX_USERSTATUS_NONE;
            break;
        case Status::Away:
            userstatus = TOX_USERSTATUS_AWAY;
            break;
        case Status::Busy:
            userstatus = TOX_USERSTATUS_BUSY;
            break;
        default:
            userstatus = TOX_USERSTATUS_INVALID;
            break;
    }

    if (tox_set_user_status(tox, userstatus) == 0) {
        emit statusSet(status);
    } else {
        emit failedToSetStatus(status);
    }
}

void Core::bootstrapDht()
{
    const Settings& s = Settings::getInstance();
    QList<Settings::DhtServer> dhtServerList = s.getDhtServerList();

    for (const Settings::DhtServer& dhtServer : dhtServerList) {
       tox_bootstrap_from_address(tox, dhtServer.address.toLatin1().data(), 0, htons(dhtServer.port), CUserId(dhtServer.userId).data());
    }
}

void Core::process()
{
    tox_do(tox);
#ifdef DEBUG
    //we want to see the debug messages immediately
    fflush(stdout);
#endif
    checkConnection();
}

void Core::checkConnection()
{
    static bool isConnected = false;

    if (tox_isconnected(tox) && !isConnected) {
        emit connected();
        isConnected = true;
    } else if (!tox_isconnected(tox) && isConnected) {
        emit disconnected();
        isConnected = false;
    }
}

void Core::loadConfiguration()
{
    QString path = QStandardPaths::locate(QStandardPaths::StandardLocation::ConfigLocation, CONFIG_FILE_NAME);
    if (path.isEmpty()) {
        qWarning() << "The Tox configuration file was not found";
        return;
    }

    QFile configurationFile(path);
    if (!configurationFile.open(QIODevice::ReadOnly)) {
        qCritical() << "File " << path << " cannot be opened";
        return;
    }

    qint64 fileSize = configurationFile.size();
    if (fileSize > 0) {
        QByteArray data = configurationFile.readAll();
        tox_load(tox, reinterpret_cast<uint8_t *>(data.data()), data.size());
    }

    configurationFile.close();

    loadFriends();
}

void Core::saveConfiguration()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::ConfigLocation);
    if (path.isEmpty()) {
        qCritical() << "Error finding data location directory";
        return;
    }

    QDir directory(path);
    if (!directory.exists(path) && !directory.mkpath(directory.absolutePath())) {
        qCritical() << "Error while creating directory " << path;
        return;
    }

    path += '/' + CONFIG_FILE_NAME;
    QSaveFile configurationFile(path);
    if (!configurationFile.open(QIODevice::WriteOnly)) {
        qCritical() << "File " << path << " cannot be opened";
        return;
    }

    uint32_t fileSize = tox_size(tox);
    if (fileSize > 0 && fileSize <= INT32_MAX) {
        uint8_t* data = new uint8_t[fileSize];
        tox_save(tox, data);
        configurationFile.write(reinterpret_cast<char *>(data), fileSize);
        configurationFile.commit();
        delete data;
    }
}

void Core::loadFriends()
{
    const uint32_t friendCount = tox_count_friendlist(tox);
    if (friendCount > 0) {
        // assuming there are not that many friends to fill up the whole stack
        int32_t ids[friendCount];
        tox_get_friendlist(tox, ids, friendCount);
        uint8_t clientId[TOX_CLIENT_ID_SIZE];
        for (int32_t i = 0; i < static_cast<int32_t>(friendCount); ++i) {
            if (tox_get_client_id(tox, ids[i], clientId) == 0) {
                emit friendAdded(ids[i], CUserId::toString(clientId));

                const int nameSize = tox_get_name_size(tox, ids[i]);
                if (nameSize > 0) {
                    uint8_t name[nameSize];
                    if (tox_get_name(tox, ids[i], name) == nameSize) {
                        emit friendUsernameChanged(ids[i], CString::toString(name, nameSize));
                    }
                }

                const int statusMessageSize = tox_get_status_message_size(tox, ids[i]);
                if (statusMessageSize > 0) {
                    uint8_t statusMessage[statusMessageSize];
                    if (tox_get_status_message(tox, ids[i], statusMessage, statusMessageSize) == statusMessageSize) {
                        emit friendStatusMessageChanged(ids[i], CString::toString(statusMessage, statusMessageSize));
                    }
                }

                checkLastOnline(ids[i]);
            }
        }
    }
}

void Core::checkLastOnline(int friendId) {
    const uint64_t lastOnline = tox_get_last_online(tox, friendId);
    if (lastOnline > 0) {
        emit friendLastSeenChanged(friendId, QDateTime::fromTime_t(lastOnline));
    }
}

void Core::start()
{
    tox = tox_new(0);

    if (tox == nullptr) {
        emit failedToStart();
        return;
    }

    loadConfiguration();

    tox_callback_friend_request(tox, onFriendRequest, this);
    tox_callback_friend_message(tox, onFriendMessage, this);
    tox_callback_friend_action(tox, onAction, this);
    tox_callback_name_change(tox, onFriendNameChange, this);
    tox_callback_status_message(tox, onStatusMessageChanged, this);
    tox_callback_user_status(tox, onUserStatusChanged, this);
    tox_callback_connection_status(tox, onConnectionStatusChanged, this);

    uint8_t friendAddress[TOX_FRIEND_ADDRESS_SIZE];
    tox_get_address(tox, friendAddress);

    emit friendAddressGenerated(CFriendAddress::toString(friendAddress));

    CString cUsername(Settings::getInstance().getUsername());
    tox_set_name(tox, cUsername.data(), cUsername.size());

    CString cStatusMessage(Settings::getInstance().getStatusMessage());
    tox_set_status_message(tox, cStatusMessage.data(), cStatusMessage.size());

    bootstrapDht();

    timer->setInterval(45);
    timer->start();
}


// CData

Core::CData::CData(const QString &data, uint16_t byteSize)
{
    cData = new uint8_t[byteSize];
    cDataSize = fromString(data, cData);
}

Core::CData::~CData()
{
    delete[] cData;
}

uint8_t* Core::CData::data()
{
    return cData;
}

uint16_t Core::CData::size()
{
    return cDataSize;
}

QString Core::CData::toString(uint8_t* cData, uint16_t cDataSize)
{
    return QString(QByteArray(reinterpret_cast<char*>(cData), cDataSize).toHex()).toUpper();
}

uint16_t Core::CData::fromString(const QString& data, uint8_t* cData)
{
    QByteArray arr = QByteArray::fromHex(data.toLower().toLatin1());
    memcpy(cData, reinterpret_cast<uint8_t*>(arr.data()), arr.size());
    return arr.size();
}


// CUserId

Core::CUserId::CUserId(const QString &userId) :
    CData(userId, SIZE)
{
    // intentionally left empty
}

QString Core::CUserId::toString(uint8_t* cUserId)
{
    return CData::toString(cUserId, SIZE);
}


// CFriendAddress

Core::CFriendAddress::CFriendAddress(const QString &friendAddress) :
    CData(friendAddress, SIZE)
{
    // intentionally left empty
}

QString Core::CFriendAddress::toString(uint8_t* cFriendAddress)
{
    return CData::toString(cFriendAddress, SIZE);
}


// CString

Core::CString::CString(const QString& string)
{
    cString = new uint8_t[string.length() * MAX_SIZE_OF_UTF8_ENCODED_CHARACTER]();
    cStringSize = fromString(string, cString);
}

Core::CString::~CString()
{
    delete[] cString;
}

uint8_t* Core::CString::data()
{
    return cString;
}

uint16_t Core::CString::size()
{
    return cStringSize;
}

QString Core::CString::toString(uint8_t* cString, uint16_t cStringSize)
{
    return QString::fromUtf8(reinterpret_cast<char*>(cString), cStringSize);
}

uint16_t Core::CString::fromString(const QString& string, uint8_t* cString)
{
    QByteArray byteArray = QByteArray(string.toUtf8());
    memcpy(cString, reinterpret_cast<uint8_t*>(byteArray.data()), byteArray.size());
    return byteArray.size();
}
