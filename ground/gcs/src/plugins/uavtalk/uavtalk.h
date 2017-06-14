/**
 ******************************************************************************
 * @file       uavtalk.h
 *
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVTalkPlugin UAVTalk Plugin
 * @{
 * @brief Implementation of the UAVTalk protocol which serializes and
 * deserializes UAVObjects
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef UAVTALK_H
#define UAVTALK_H

#include <QtCore>
#include <QIODevice>
#include <QMap>
#include <QSemaphore>
#include "uavobjectmanager.h"
#include "uavtalk_global.h"
#include <QtNetwork/QUdpSocket>

class UAVTALK_EXPORT UAVTalk : public QObject
{
    Q_OBJECT

public:
    typedef struct
    {
        quint32 txBytes;
        quint32 rxBytes;
        quint32 txObjectBytes;
        quint32 rxObjectBytes;
        quint32 rxObjects;
        quint32 txObjects;
        quint32 txErrors;
        quint32 rxErrors;
    } ComStats;

    UAVTalk(QIODevice *iodev, UAVObjectManager *objMngr);
    ~UAVTalk();
    bool sendObject(UAVObject *obj, bool acked, bool allInstances);
    bool sendObjectRequest(UAVObject *obj, bool allInstances);
    ComStats getStats();

    bool processInput();

signals:
    // The only signals we send to the upper level are when we
    // either receive an ACK or a NACK for a request.
    void ackReceived(UAVObject *obj);
    void nackReceived(UAVObject *obj);

private slots:
    void processInputStream(void);

protected:
    // Constants
    static const int VER_MASK = 0x70;
    static const int TYPE_MASK = 0x0f;

    static const int TYPE_VER = 0x20;
    static const int TYPE_OBJ = 0x00;
    static const int TYPE_OBJ_REQ = 0x01;
    static const int TYPE_OBJ_ACK = 0x02;
    static const int TYPE_ACK = 0x03;
    static const int TYPE_NACK = 0x04;

    static const int MIN_HEADER_LENGTH = 8; // sync(1), type (1), size(2), object ID(4)
    static const int MAX_HEADER_LENGTH =
        10; // sync(1), type (1), size(2), object ID (4), instance ID(2, not used in single objects)

    static const int CHECKSUM_LENGTH = 1;

    static const int MAX_PAYLOAD_LENGTH = 256;

    static const int MAX_PACKET_LENGTH = (MAX_HEADER_LENGTH + MAX_PAYLOAD_LENGTH + CHECKSUM_LENGTH);

    static const quint16 ALL_INSTANCES = 0xFFFF;
    static const quint16 OBJID_NOTFOUND = 0x0000;

    static const int TX_BACKLOG_SIZE = 2 * 1024;
    static const quint8 crc_table[256];

#pragma pack(push)
#pragma pack(1)
    typedef struct {
        quint8 sync;
        quint8 type;
        quint8 size;
        quint8 resv;
        quint32 objId;
    } UAVTalkHeader;
#pragma pack(pop)

    // Variables
    QPointer<QIODevice> io;
    UAVObjectManager *objMngr;

    // This is a tradeoff between the frequency of the need to
    // compact/copy left and buffer size.
    quint8 rxBuffer[MAX_PACKET_LENGTH * 12];
    quint8 txBuffer[MAX_PACKET_LENGTH];

    // Variables used by the receive state machine

    quint32 startOffset;
    quint32 filledBytes;

    ComStats stats;

    // Methods
    bool objectTransaction(UAVObject *obj, quint8 type, bool allInstances);
    virtual bool receiveObject(quint8 type, quint32 objId, quint16 instId,
            quint8 *data, qint32 length);
    UAVObject *updateObject(quint32 objId, quint16 instId, quint8 *data);
    bool transmitNack(quint32 objId);
    bool transmitObject(UAVObject *obj, quint8 type, bool allInstances);
    bool transmitSingleObject(UAVObject *obj, quint8 type, bool allInstances);
    quint8 updateCRC(quint8 crc, const quint8 *data, qint32 length);
};

#endif // UAVTALK_H
