#include <iostream>
#include <string>
using namespace std;

class Database
{
private:
    // TODO: connection object/handle (e.g. libpq PGconn*)
    bool connected;

public:
    // ---- Constructors ----
    Database();
    // TODO: ~Database(); // make sure to disconnect safely

    // ---- Connection Management ----
    // TODO: bool connect(const string& host, const string& dbname,
    //                    const string& user, const string& password, int port);
    // TODO: void disconnect();
    // TODO: bool isConnected() const;

    // ---- Generic Query Helpers ----
    // TODO: bool executeQuery(const string& sql);
    // TODO: /* ResultType */ selectQuery(const string& sql);

    // ---- Employee ----
    // TODO: bool insertEmployee(/* Employee params */);
    // TODO: bool validateLogin(const string& username, const string& password);

    // ---- Customer ----
    // TODO: bool insertCustomer(/* Customer params */);
    // TODO: bool updateCustomer(/* Customer params */);
    // TODO: bool deleteCustomer(int customerId);
    // TODO: /* list */ getAllCustomers();

    // ---- Vehicle ----
    // TODO: bool insertVehicle(/* Vehicle params */);
    // TODO: /* list */ getVehiclesByCustomer(int customerId);

    // ---- Zone / ParkingSlot ----
    vector<Zone> Database::getAllZones()
    {
        PGresult* res = PQexec (conn, "SELECT zone_id, name FROM zone ORDER BY zone_id;");
        if(PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            int rowCount = PQntubles(res);
            for(int i = 0; i < rowCount; i++)
            {
                int id = stoi(PQgetvalue(res, i, 0));
                string name = PQgetvalue(res, i, 0));
                zoneList.push_back(Zone(id, name));
            }
        }

        PQclear(res);
        return zoneList;
    }
    vector<ParkingSlot> Dtabadse::getSlotsByZone(int zoneId)
    {
        vector<ParkingSlot> slotsList;
        string zoneIdStr = to_string(zoneId);
        const char* paramValues[] = {zoneIdStr.c_str()};
        const char* query = "SELECT slot_id, zone_id, slot_code, status FROM parking_slot WHERE zone_id = $1;";
        PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, nullptr, nullptr, 0);

        if(PqresultStatus(res) == PGRES_TUPLES_OK)
        {
            int rowCount = PQtuples(res);
            for(int i = 0; i < rowCount; i++)
            {
                int id = stoi(PQgetvalue(res, i, 0));
                int zId = stoi(PQgetValue(res, i, 1));
                string code = PQgetValue(res, i, 2);
                string dbStatus = PQgetValue(res, i, 3);

                SlotStatus currentStatus = SlotStatus::Available;
                if(dbStatus == "Reserved"){
                    currentStatus = SlotStatus::Reserved;
                }
                else if (dbStatus == "Occupied"){
                    currentStatus = SlotStatus::Occupied;
                }

                ParkingSlot slot(id, zId, code);
                slot.setStatus(currentStatus);

                slotsList.push_back(slot);
            }
        }

        PQclear(res);
        return slotsList;
    }
    bool Database::updateSlotStatus(int slotId, int newStatus)
    {
        strig slotIdStr = to_string(slotId);
        string statusStr = "Available";
        if(newStatus == 1)
            statusStr = "Reserved";
        else if (newStatus == 2)
            statusStr = "Occupied";

      const char* paramValues[] = { slotIdStr.c_str(), statusStr.c_str() };
      const char* query = "UPDATE parking _slot SET status = $2 WHERE slot_id = $1;";

      PGresult* res = PQexecParams(conn, query, 2, nullptr, paramValues, nullptr, nullptr, 0);
      bool isSuccess = (PQresultStatus(res) == PGRES_COMMAND_OK);

      PQclear(res);
      return isSuccess;
    }
    // ---- Reservation ----
  // داخل تعريف الـ class Database في Database.cpp:

public:
    // ---- Reservation ----
    bool insertReservation(int customerId, int vehicleId, int slotId, const string& date, const string& startTime, const string& endTime)
    {
        // SQL الافتراضي:
        // string sql = "INSERT INTO reservation (customer_id, vehicle_id, slot_id, reserve_date, start_time, end_time, status) "
        //              "VALUES (" + to_string(customerId) + ", " + to_string(vehicleId) + ", " + to_string(slotId) + ", '" + date + "', '" + startTime + "', '" + endTime + "', 'Pending');";

        // return executeQuery(sql);
        return true;
    }

    bool hasConflictingReservation(int slotId, const string& date, const string& startTime, const string& endTime)
    {
        // SQL الافتراضي للتحقق من تقاطع الأوقات لنفس الموقف (slot_id) وفي نفس اليوم (reserve_date)
        // string sql = "SELECT COUNT(*) FROM reservation WHERE slot_id = " + to_string(slotId) +
        //              " AND reserve_date = '" + date + "'" +
        //              " AND status != 'Cancelled'" +
        //              " AND (start_time < '" + endTime + "' AND end_time > '" + startTime + "');";

        // int count = selectQueryCount(sql);
        // return count > 0;
        return false;
    }

    bool cancelReservation(int reservationId)
    {
        // SQL الافتراضي:
        // string sql = "UPDATE reservation SET status = 'Cancelled' WHERE reservation_id = " + to_string(reservationId) + ";";

        // return executeQuery(sql);
        return true;
    }
    // TODO: bool hasConflictingReservation(int slotId, const string& date,
    //                                      const string& startTime, const string& endTime);
    // TODO: bool cancelReservation(int reservationId);

    // ---- ParkingSession ----
    // TODO: bool insertSession(int reservationId, const string& checkInTime);
    // TODO: bool closeSession(int sessionId, const string& checkOutTime);

    // ---- Payment ----
    // TODO: bool insertPayment(int sessionId, double amount);
    // TODO: bool markPaymentAsPaid(int paymentId);
};
