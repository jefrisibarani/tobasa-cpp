#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "tobasalis/lis/common.h"

namespace tbs {
namespace lis {
namespace conf {

/** \addtogroup LIS
 * @{
 */


struct TcpIpServer
{
   std::string listenAddress;       // "127.0.0.1"
   int         listenPort;          // 5151
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TcpIpServer, listenAddress, listenPort)


struct TcpIpClient
{
   std::string serverAddress;       // "127.0.0.1"
   int         serverPort;          // 5151
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TcpIpClient, serverAddress, serverPort)


struct TcpIp
{
   std::string activeMode;
   TcpIpServer server;
   TcpIpClient client;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TcpIp, activeMode, server, client)


struct SerialPort
{
   std::string portName;            // "COM3"
   int         baudRate;            // 9600
   std::string parity;              // "None"
   int         dataBits;            // 8
   std::string stopBits;            // "One"
   std::string handShake;           // "None"
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SerialPort, portName, baudRate, parity, dataBits, stopBits, handShake)


struct Instrument
{
   std::string type;
   std::string dataLink;
   std::string name;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Instrument, type, dataLink, name)


struct Connection
{
   int         receiveTimeOutSeconds;           // 30 seconds
   int         enqTimeOutSeconds;               // 15 seconds
   int         ackTimeOutSeconds;               // 15 seconds
   int         sendRetry;                       // 5
   bool        sendEachRecordInOneFrame;        // true
   bool        sendRecordAsIntermediateFrame;   // false
   bool        incomingDataAsIntermediateFrame; // false
   std::string activeConnection;                // tcpIp
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Connection, receiveTimeOutSeconds, enqTimeOutSeconds, 
   ackTimeOutSeconds, sendRetry, sendEachRecordInOneFrame, sendRecordAsIntermediateFrame, 
   incomingDataAsIntermediateFrame, activeConnection)


struct ConnectionType
{
   TcpIp      tcpIp;
   SerialPort serialPort;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConnectionType, tcpIp, serialPort)


//! Database context configuration options
struct Database
{
   std::string dbDriver;
   std::string connectionString;
   std::string password;
   bool logInternalSqlQuery;
   bool logSqlQuery;   
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Database, dbDriver, connectionString, password, logInternalSqlQuery, logSqlQuery)


struct Result
{
   bool saveData;          // true
   int  dbThreadPoolSize = 2;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Result, saveData, dbThreadPoolSize)


struct Delimiters
{
   std::string field;
   std::string repeat;
   std::string component;
   std::string escape;
   std::string subComponent;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Delimiters, field, repeat, component, escape, subComponent)


struct LisMessage
{
   bool       autoDetectDelimiters;
   Delimiters delimiters;
   bool       logOnLinkReceive  = true;
   bool       logOnParserRead   = false;
   bool       logOnMessageReady = false;  
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LisMessage, autoDetectDelimiters, delimiters, logOnLinkReceive, logOnParserRead, logOnMessageReady)


struct Engine
{
   std::string             hostId;
   std::string             hostProvider;
   std::string             role;
   bool                    autoStart;
   std::string             activeInstrument;
   std::vector<Instrument> instruments;
   LisMessage              lisMessage; 
   Connection              connection;
   ConnectionType          connectionTypes;
   Result                  result;
   Database                database;
   bool                    useDedicatedRunningThread;
   bool                    getDbDataWithNewConnection;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Engine, hostId, hostProvider, role, autoStart, 
   activeInstrument, instruments, lisMessage, connection, connectionTypes, result, 
   database, useDedicatedRunningThread, getDbDataWithNewConnection)

/** @}*/

} // namespace conf
} // namespace lis
} // namespace tbs
