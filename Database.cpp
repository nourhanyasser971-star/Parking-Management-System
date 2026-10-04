#include <iostream>
#include <string>
#include <cstdlib>
#include <libpq-fe.h>
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
    // TODO: /* list */ getAllZones();
    // TODO: /* list */ getSlotsByZone(int zoneId);
    // TODO: bool updateSlotStatus(int slotId, int newStatus);

    // ---- Reservation ----
    // TODO: bool insertReservation(/* Reservation params */);
    // TODO: bool hasConflictingReservation(int slotId, const string& date,
    //                                      const string& startTime, const string& endTime);
    // TODO: bool cancelReservation(int reservationId);

    // ---- ParkingSession ----
    bool insertSession(int reservationId, const string& checkInTime, int* outId)
    {
        string r = to_string(reservationId);

        PGresult* res = PQexec(conn,
            ("INSERT INTO parking_session (reservation_id, check_in_time) VALUES (" +
             r + ", '" + checkInTime + "') RETURNING session_id").c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            return false;
        }
        *outId = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);

        // Slot becomes Occupied
        res = PQexec(conn,
            ("UPDATE parking_slot SET status = 'Occupied' WHERE slot_id = "
             "(SELECT slot_id FROM reservation WHERE reservation_id = " + r + ")").c_str());
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);
        return ok;
    }
	
    bool closeSession(int sessionId, const string& checkOutTime)
    {
        PGresult* res = PQexec(conn,
            ("UPDATE parking_session SET check_out_time = '" + checkOutTime +
             "', is_active = FALSE WHERE session_id = " + to_string(sessionId)).c_str());
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);
        return ok;
    }

    // ---- Payment ----
    bool insertPayment(int sessionId, double amount, int* outId)
    {
        PGresult* res = PQexec(conn,
            ("INSERT INTO payment (session_id, amount) VALUES (" + to_string(sessionId) +
             ", " + to_string(amount) + ") RETURNING payment_id").c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            return false;
        }
        *outId = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return true;
    }
	
    bool markPaymentAsPaid(int paymentId)
    {
        string p = to_string(paymentId);

        PGresult* res = PQexec(conn,
            ("UPDATE payment SET status = 'Paid', paid_at = NOW() WHERE payment_id = " + p).c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            PQclear(res);
            return false;
        }
        PQclear(res);

        // Slot goes back to Available
        res = PQexec(conn,
            ("UPDATE parking_slot SET status = 'Available' WHERE slot_id = "
             "(SELECT r.slot_id FROM payment pay "
             "JOIN parking_session s ON s.session_id = pay.session_id "
             "JOIN reservation r ON r.reservation_id = s.reservation_id "
             "WHERE pay.payment_id = " + p + ")").c_str());
        bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
        PQclear(res);
        return ok;
    }
};
