#include <iostream>
#include <string>

#include "DB.h"
#include "utils.h"

const std::string DATABASE = "db"; 
const std::string HOST = "host"; 
const std::string PORT = "port"; 
const std::string USERNAME = "username"; 
const std::string PASSWORD = "password";
const std::string BASE = "database";
const std::string ENCODEING = "encoding";

const int VID = 1;
const int TIME = 2;
const int REGION = 3;
const int CHANNEL = 4;
const int STATE = 5;

const int INSERT_VID = 1;
const int INSERT_TIME = 2;
const int INSERT_LICENSE = 3;
const int INSERT_MAKE = 4;
const int INSERT_COLOR = 5;
const int INSERT_TYPE = 6;

const int UPDATE_TIME = 1;
const int UPDATE_LICENSE = 2;
const int UPDATE_MAKE = 3;
const int UPDATE_COLOR = 4;
const int UPDATE_TYPE = 5;
const int UPDATE_VID = 6;

const std::string INSERT_INTO_EVENT = "INSERT INTO event(vid, etime, region, channel, estate, using_state) VALUES (?, ?, ?, ?, ?, '0')";
const std::string INSERT_INTO_VEHICLE = "INSERT INTO vehicle(vid, vtime, license, make, color, type) VALUES (?, ?, ?, ?, ?, ?)";
const std::string UPDATE_VEHICLE = "UPDATE vehicle SET vtime = ?, license = ?, make = ?, color = ?, type = ? WHERE vid = ?";
const std::string DELETE_EVENT = "DELETE event WHERE vid = ?";
const std::string DELETE_VEHICLE = "DELETE vehicle WHERE vid = ?";

const int INSERT_UPDATE_REGION = 1;
const int INSERT_UPDATE_CHANNEL = 2;
const int INSERT_UPDATE_VIDS = 3;
const int INSERT_UPDATE_VIDS_UPDATE = 4;
const std::string REGION_INSERT_UPDATE = "INSERT INTO region VALUES (?,?,?) ON DUPLICATE KEY UPDATE vids=?";

DB::DB(): Logging("DB") {}

int DB::init(libconfig::Config& config) {

    if (config.exists(DATABASE)) {
        const libconfig::Setting& db = config.lookup(DATABASE);  
        std::string host = db.lookup(HOST);
        int port = db.lookup(PORT);
        std::string username = db.lookup(USERNAME);
        std::string password = db.lookup(PASSWORD);
        std::string database = db.lookup(BASE);
        std::string encoding = db.lookup(ENCODEING);


        driver = sql::mysql::get_mysql_driver_instance();
        if (driver == NULL) {
            logError("driver is null");
            exit(1);
        } else {
            logInfo("driver is OK");
        }

        connection_properties["hostName"] = host;
        connection_properties["userName"] = username;
        connection_properties["password"] = password;
        connection_properties["schema"] = database;
        connection_properties["port"] = port;
        connection_properties["OPT_RECONNECT"] = true;
        connection_properties["OPT_CHARSET_NAME"] = encoding;
        connection_properties["preInit"] = "SET NAMES " + encoding;
    }
    return 0;
}

bool DB::start() {
    con = driver->connect(connection_properties);
        if (con == NULL) {
        logError("connection is null");
        return false;
    } else {
        logInfo("connection is open");
        insertIntoEvent = con -> prepareStatement(INSERT_INTO_EVENT);
        insertIntoVehicle = con -> prepareStatement(INSERT_INTO_VEHICLE);
        updateVehicle = con -> prepareStatement(UPDATE_VEHICLE);
        regionInsertUpdate = con -> prepareStatement(REGION_INSERT_UPDATE);
        return true;
    }
}

inline std::string mkString(std::vector<std::string> vs) {
    std::string rs;
    for (auto s: vs) {
        rs.append(s);
        rs.append(",");
    }
    return rs;
}

int DB::writeRegionInfo(FrameInfo& frameInfo, std::string channel) {
    if (con) {
        for (auto kv: frameInfo) {
            auto region = kv.first;
            auto targets = kv.second;

            std::vector<Target> targetVector;
            for (auto kv: targets) {
                targetVector.push_back(kv.second);
            }
            // sort by y, the closer to the viewer.
            std::sort(targetVector.begin(), targetVector.end(), [](const Target &t1, const Target &t2) -> bool {
                return t1.rect.y > t2.rect.y;
            }
            );

            std::vector<std::string> vids;
            for (auto t: targetVector) {
                if (t.state == STAY || t.state == IN)
                vids.push_back(t.id);
            }
            auto vidString = mkString(vids);

            regionInsertUpdate -> setString(INSERT_UPDATE_REGION, region);
            regionInsertUpdate -> setString(INSERT_UPDATE_CHANNEL, channel);
            regionInsertUpdate -> setString(INSERT_UPDATE_VIDS, vidString);
            regionInsertUpdate -> setString(INSERT_UPDATE_VIDS_UPDATE, vidString);
            regionInsertUpdate -> execute();
            logDebug("write info region: " + region + " channel: " + channel + " vids: " + vidString);
            
        }
    }
    return 0;
}

int DB::writeTarget(TargetInfo& t) {
    if (con) {
        std::string timeString = time_t_to_string(t.time);
        if (t.state != IN) {
            if (t.stayCount % 5 == 0) {
                // STAY(n times) and OUT update v table
                updateVehicle -> setString(UPDATE_TIME, timeString);
                updateVehicle -> setString(UPDATE_LICENSE, t.license);
                updateVehicle -> setString(UPDATE_MAKE, t.make);
                updateVehicle -> setString(UPDATE_COLOR, t.color);
                updateVehicle -> setString(UPDATE_TYPE, t.type);
                updateVehicle -> setString(UPDATE_VID, t.vid);
                updateVehicle -> execute();
                logDebug("updateVehicle done: " + t.vid);
            }
        } else {
            // IN insert into v table
            insertIntoVehicle -> setString(INSERT_VID, t.vid);
            insertIntoVehicle -> setString(INSERT_TIME, timeString);
            insertIntoVehicle -> setString(INSERT_LICENSE, t.license);
            insertIntoVehicle -> setString(INSERT_MAKE, t.make);
            insertIntoVehicle -> setString(INSERT_COLOR, t.color);
            insertIntoVehicle -> setString(INSERT_TYPE, t.type);
            insertIntoVehicle -> execute();
            logDebug("insertIntoVehicle done: " + t.vid);
        }
        insertIntoEvent -> setString(VID, t.vid);
        insertIntoEvent -> setString(TIME, timeString);
        insertIntoEvent -> setString(REGION, t.region);
        insertIntoEvent -> setString(CHANNEL, t.channel);
        insertIntoEvent -> setInt(STATE, t.state);
        insertIntoEvent -> execute();
        logDebug("insertIntoEvent done: " + t.vid);
    } else {
        logDebug(t.toString());
    }
    return 0;
}

DB::~DB() {

    if (insertIntoEvent) {
        insertIntoEvent -> close();
        delete insertIntoEvent;
    }

    if (insertIntoVehicle) {
        insertIntoVehicle -> close();
        delete insertIntoVehicle;
    }

    if (updateVehicle) {
        updateVehicle -> close();
        delete updateVehicle;
    }

    if (con) {
        con -> close();
        delete con;
    }
}
