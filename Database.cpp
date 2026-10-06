// =====================================================
// Database.cpp  -  PostgreSQL connection + all queries (libpq)
// NOTE: this file is included by main.cpp (after the model classes),
//       so it is NOT compiled on its own.
// =====================================================
#include <libpq-fe.h>
#include <cstdio>
#include <cstdlib>

class Database
{
private:
    PGconn* conn;        // libpq connection handle
    bool connected;
    string lastError;    // text of the last error (empty = no error)
    string lastSqlState; // e.g. "23503" = foreign key violation

    // Runs a query with parameters ($1, $2, ...). Returns nullptr on failure.
    PGresult* run(const string& sql, const vector<string>& params = vector<string>());
    int countQuery(const string& sql, const vector<string>& params = vector<string>());

public:
    // ---- Constructors ----
    Database();
    ~Database(); // disconnects safely
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // ---- Connection Management ----
    bool connect(const string& host, const string& dbname,
                 const string& user, const string& password, int port);
    void disconnect();
    bool isConnected() const;
    string getLastError() const;

    // ---- Generic Query Helper ----
    bool executeQuery(const string& sql, const vector<string>& params = vector<string>());

    // ---- Employee ----
    bool insertEmployee(const Employee& e);
    bool validateLogin(const string& username, const string& password, Employee& loggedIn);

    // ---- Customer ----
    bool insertCustomer(const Customer& c);
    bool updateCustomer(const Customer& c);
    bool deleteCustomer(int customerId);
    vector<Customer> getAllCustomers();

    // ---- Vehicle ----
    bool insertVehicle(const Vehicle& v);
    vector<unique_ptr<Vehicle> > getVehiclesByCustomer(int customerId);
    vector<unique_ptr<Vehicle> > getAllVehicles();

    // ---- Zone / ParkingSlot ----
    bool insertZone(const string& name);
    bool insertSlot(int zoneId, const string& slotCode);
    vector<Zone> getAllZones();
    vector<ParkingSlot> getAllSlots();
    vector<ParkingSlot> getSlotsByZone(int zoneId);
    bool updateSlotStatus(int slotId, SlotStatus newStatus);

    // ---- Reservation ----
    bool insertReservation(int customerId, int vehicleId, int slotId,
                           const string& date, const string& startTime, const string& endTime);
    bool hasConflictingReservation(int slotId, const string& date,
                                   const string& startTime, const string& endTime);
    bool confirmReservation(int reservationId);
    bool cancelReservation(int reservationId);
    vector<Reservation> getAllReservations();

    // ---- ParkingSession ----
    bool insertSession(int reservationId);   // check-in (time = now)
    bool closeSession(int sessionId);        // check-out (time = now)
    vector<ParkingSession> getAllSessions();

    // ---- Payment ----
    bool insertPayment(int sessionId, double amount);
    bool markPaymentAsPaid(int paymentId);
    vector<Payment> getAllPayments();
};




// ---------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------
static string cell(PGresult* res, int row, int col)
{
    if (PQgetisnull(res, row, col)) return "";
    return PQgetvalue(res, row, col);
}

static string moneyToString(double amount)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", amount);
    return buf;
}

// ---------------------------------------------------------------
// Constructors / Connection
// ---------------------------------------------------------------
Database::Database()
{
    conn = nullptr;
    connected = false;
    lastError = "";
    lastSqlState = "";
}

Database::~Database()
{
    disconnect();
}

bool Database::connect(const string& host, const string& dbname,
                       const string& user, const string& password, int port)
{
    disconnect();

    string portText = to_string(port);
    const char* keys[]   = { "host", "port", "dbname", "user", "password", "client_encoding", nullptr };
    const char* values[] = { host.c_str(), portText.c_str(), dbname.c_str(),
                             user.c_str(), password.c_str(), "UTF8", nullptr };

    conn = PQconnectdbParams(keys, values, 0);

    if (conn == nullptr || PQstatus(conn) != CONNECTION_OK)
    {
        lastError = conn ? PQerrorMessage(conn) : "Could not allocate a connection.";
        if (conn) PQfinish(conn);
        conn = nullptr;
        connected = false;
        return false;
    }

    connected = true;
    lastError = "";
    return true;
}

void Database::disconnect()
{
    if (conn != nullptr)
    {
        PQfinish(conn);
        conn = nullptr;
    }
    connected = false;
}

bool Database::isConnected() const
{
    return connected;
}

string Database::getLastError() const
{
    return lastError;
}

// ---------------------------------------------------------------
// Generic query helpers
// ---------------------------------------------------------------
PGresult* Database::run(const string& sql, const vector<string>& params)
{
    lastError = "";
    lastSqlState = "";

    if (!connected || conn == nullptr)
    {
        lastError = "Not connected to the database.";
        return nullptr;
    }

    vector<const char*> values;
    for (size_t i = 0; i < params.size(); i++)
        values.push_back(params[i].c_str());

    PGresult* res = PQexecParams(conn, sql.c_str(), (int)params.size(), nullptr,
                                 params.empty() ? nullptr : values.data(),
                                 nullptr, nullptr, 0);

    ExecStatusType st = res ? PQresultStatus(res) : PGRES_FATAL_ERROR;
    if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK)
    {
        lastError = res ? PQresultErrorMessage(res) : PQerrorMessage(conn);
        if (res)
        {
            const char* state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
            if (state) lastSqlState = state;
            PQclear(res);
        }
        return nullptr;
    }
    return res;
}

bool Database::executeQuery(const string& sql, const vector<string>& params)
{
    PGresult* res = run(sql, params);
    if (res == nullptr) return false;
    PQclear(res);
    return true;
}

int Database::countQuery(const string& sql, const vector<string>& params)
{
    PGresult* res = run(sql, params);
    if (res == nullptr) return -1; // -1 = the query failed
    int count = 0;
    if (PQntuples(res) > 0) count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

// ---------------------------------------------------------------
// Employee
// ---------------------------------------------------------------
bool Database::insertEmployee(const Employee& e)
{
    vector<string> p;
    p.push_back(e.getUsername());
    p.push_back(e.getPassword());
    p.push_back(e.getName());
    bool ok = executeQuery("INSERT INTO employee (username, password, name) VALUES ($1, $2, $3)", p);
    if (!ok && lastSqlState == "23505")
        lastError = "This username already exists.";
    return ok;
}

bool Database::validateLogin(const string& username, const string& password, Employee& loggedIn)
{
    vector<string> p;
    p.push_back(username);
    p.push_back(password);
    PGresult* res = run("SELECT employee_id, name FROM employee WHERE username = $1 AND password = $2", p);
    if (res == nullptr) return false;

    bool found = (PQntuples(res) == 1);
    if (found)
        loggedIn = Employee(atoi(PQgetvalue(res, 0, 0)), username, "", cell(res, 0, 1));
    PQclear(res);

    if (!found) lastError = "Wrong username or password.";
    return found;
}

// ---------------------------------------------------------------
// Customer
// ---------------------------------------------------------------
bool Database::insertCustomer(const Customer& c)
{
    vector<string> p;
    p.push_back(c.getName());
    p.push_back(c.getPhone());
    p.push_back(c.getEmail());
    bool ok = executeQuery("INSERT INTO customer (name, phone, email) VALUES ($1, $2, $3)", p);
    if (!ok && lastSqlState == "23505")
        lastError = "A customer with this phone number already exists.";
    return ok;
}

bool Database::updateCustomer(const Customer& c)
{
    vector<string> p;
    p.push_back(c.getName());
    p.push_back(c.getPhone());
    p.push_back(c.getEmail());
    p.push_back(to_string(c.getId()));
    bool ok = executeQuery("UPDATE customer SET name = $1, phone = $2, email = $3 WHERE customer_id = $4", p);
    if (!ok && lastSqlState == "23505")
        lastError = "A customer with this phone number already exists.";
    return ok;
}

bool Database::deleteCustomer(int customerId)
{
    vector<string> p;
    p.push_back(to_string(customerId));
    bool ok = executeQuery("DELETE FROM customer WHERE customer_id = $1", p);
    if (!ok && lastSqlState == "23503")
        lastError = "Cannot delete this customer: they still have vehicles or reservations.";
    return ok;
}

vector<Customer> Database::getAllCustomers()
{
    vector<Customer> list;
    PGresult* res = run("SELECT customer_id, name, phone, email FROM customer ORDER BY customer_id");
    if (res == nullptr) return list;
    for (int i = 0; i < PQntuples(res); i++)
        list.push_back(Customer(atoi(PQgetvalue(res, i, 0)), cell(res, i, 1), cell(res, i, 2), cell(res, i, 3)));
    PQclear(res);
    return list;
}

// ---------------------------------------------------------------
// Vehicle
// ---------------------------------------------------------------
bool Database::insertVehicle(const Vehicle& v)
{
    // edge cases from the team's version
    if (v.getPlateNumber().empty())
    {
        lastError = "Plate number is empty.";
        return false;
    }
    if (v.getExtraAttribute() <= 0)
    {
        lastError = "Number of doors / engine cc must be greater than 0.";
        return false;
    }

    vector<string> p;
    p.push_back(to_string(v.getCustomerId()));
    p.push_back(v.getPlateNumber());
    p.push_back(v.getModel());
    p.push_back(v.getVehicleType());                 // "Car" or "Motorcycle" (polymorphism)
    p.push_back(to_string(v.getExtraAttribute()));   // doors or cc

    // the extra value goes to number_of_doors (Car) or engine_cc (Motorcycle)
    bool ok = executeQuery(
        "INSERT INTO vehicle (customer_id, plate_number, model, vehicle_type, number_of_doors, engine_cc) "
        "VALUES ($1::int, $2, $3, $4::varchar, "
        "CASE WHEN $4::varchar = 'Car' THEN $5::int END, "
        "CASE WHEN $4::varchar = 'Motorcycle' THEN $5::int END)", p);

    if (!ok && lastSqlState == "23505")
        lastError = "A vehicle with this plate number already exists.";
    if (!ok && lastSqlState == "23503")
        lastError = "This customer does not exist.";
    return ok;
}

// Builds Car / Motorcycle objects from a query result (uses the factory in Models.h)
static vector<unique_ptr<Vehicle> > readVehicles(PGresult* res)
{
    vector<unique_ptr<Vehicle> > list;
    for (int i = 0; i < PQntuples(res); i++)
    {
        Vehicle* v = createVehicle(cell(res, i, 4),              // type
                                   atoi(PQgetvalue(res, i, 0)),  // vehicle_id
                                   atoi(PQgetvalue(res, i, 1)),  // customer_id
                                   cell(res, i, 2),              // plate
                                   cell(res, i, 3),              // model
                                   atoi(PQgetvalue(res, i, 5))); // doors or cc
        if (v != nullptr) list.push_back(unique_ptr<Vehicle>(v));
    }
    return list;
}

// columns: id, customer_id, plate, model, type, extra (doors for Car, cc for Motorcycle; 0 if empty)
static const char* VEHICLE_COLUMNS =
    "vehicle_id, customer_id, plate_number, model, vehicle_type, "
    "COALESCE(CASE WHEN vehicle_type = 'Car' THEN number_of_doors ELSE engine_cc END, 0)";

vector<unique_ptr<Vehicle> > Database::getVehiclesByCustomer(int customerId)
{
    vector<unique_ptr<Vehicle> > list;
    vector<string> p;
    p.push_back(to_string(customerId));
    PGresult* res = run(string("SELECT ") + VEHICLE_COLUMNS +
                        " FROM vehicle WHERE customer_id = $1 ORDER BY vehicle_id", p);
    if (res == nullptr) return list;
    list = readVehicles(res);
    PQclear(res);
    return list;
}

vector<unique_ptr<Vehicle> > Database::getAllVehicles()
{
    vector<unique_ptr<Vehicle> > list;
    PGresult* res = run(string("SELECT ") + VEHICLE_COLUMNS +
                        " FROM vehicle ORDER BY vehicle_id");
    if (res == nullptr) return list;
    list = readVehicles(res);
    PQclear(res);
    return list;
}

// ---------------------------------------------------------------
// Zone / ParkingSlot
// ---------------------------------------------------------------
bool Database::insertZone(const string& name)
{
    vector<string> p;
    p.push_back(name);
    return executeQuery("INSERT INTO zone (name) VALUES ($1)", p);
}

bool Database::insertSlot(int zoneId, const string& slotCode)
{
    vector<string> p;
    p.push_back(to_string(zoneId));
    p.push_back(slotCode);
    bool ok = executeQuery("INSERT INTO parking_slot (zone_id, slot_code) VALUES ($1, $2)", p);
    if (!ok && lastSqlState == "23505")
        lastError = "A slot with this code already exists.";
    return ok;
}

vector<Zone> Database::getAllZones()
{
    vector<Zone> list;
    PGresult* res = run("SELECT zone_id, name FROM zone ORDER BY zone_id");
    if (res == nullptr) return list;
    for (int i = 0; i < PQntuples(res); i++)
        list.push_back(Zone(atoi(PQgetvalue(res, i, 0)), cell(res, i, 1)));
    PQclear(res);
    return list;
}

static vector<ParkingSlot> readSlots(PGresult* res)
{
    vector<ParkingSlot> list;
    for (int i = 0; i < PQntuples(res); i++)
    {
        ParkingSlot s(atoi(PQgetvalue(res, i, 0)), atoi(PQgetvalue(res, i, 1)), cell(res, i, 2));
        s.setStatus(slotStatusFromString(cell(res, i, 3)));
        list.push_back(s);
    }
    return list;
}

vector<ParkingSlot> Database::getAllSlots()
{
    vector<ParkingSlot> list;
    PGresult* res = run("SELECT slot_id, zone_id, slot_code, status FROM parking_slot ORDER BY slot_code");
    if (res == nullptr) return list;
    list = readSlots(res);
    PQclear(res);
    return list;
}

vector<ParkingSlot> Database::getSlotsByZone(int zoneId)
{
    vector<ParkingSlot> list;
    vector<string> p;
    p.push_back(to_string(zoneId));
    PGresult* res = run("SELECT slot_id, zone_id, slot_code, status FROM parking_slot "
                        "WHERE zone_id = $1 ORDER BY slot_code", p);
    if (res == nullptr) return list;
    list = readSlots(res);
    PQclear(res);
    return list;
}

bool Database::updateSlotStatus(int slotId, SlotStatus newStatus)
{
    vector<string> p;
    p.push_back(slotStatusToString(newStatus));
    p.push_back(to_string(slotId));
    return executeQuery("UPDATE parking_slot SET status = $1 WHERE slot_id = $2", p);
}

// ---------------------------------------------------------------
// Reservation
// ---------------------------------------------------------------
bool Database::insertReservation(int customerId, int vehicleId, int slotId,
                                 const string& date, const string& startTime, const string& endTime)
{
    vector<string> p;
    p.push_back(to_string(customerId));
    p.push_back(to_string(vehicleId));
    p.push_back(to_string(slotId));
    p.push_back(date);
    p.push_back(startTime);
    p.push_back(endTime);
    return executeQuery("INSERT INTO reservation (customer_id, vehicle_id, slot_id, reserve_date, start_time, end_time, status) "
                        "VALUES ($1::int, $2::int, $3::int, $4::date, $5::time, $6::time, 'Pending')", p);
}

// Returns true if another (non-cancelled) reservation overlaps this time.
// If the query itself fails it also returns true (to be safe) and getLastError() is set.
bool Database::hasConflictingReservation(int slotId, const string& date,
                                         const string& startTime, const string& endTime)
{
    vector<string> p;
    p.push_back(to_string(slotId));
    p.push_back(date);
    p.push_back(startTime);
    p.push_back(endTime);
    int count = countQuery("SELECT COUNT(*) FROM reservation "
                           "WHERE slot_id = $1::int AND reserve_date = $2::date "
                           "AND status <> 'Cancelled' "
                           "AND start_time < $4::time AND end_time > $3::time", p);
    return count != 0; // 0 = no conflict, >0 = conflict, -1 = error
}

bool Database::confirmReservation(int reservationId)
{
    vector<string> p;
    p.push_back(to_string(reservationId));
    if (!executeQuery("UPDATE reservation SET status = 'Confirmed' "
                      "WHERE reservation_id = $1 AND status = 'Pending'", p))
        return false;
    // mark the slot as Reserved (only if it is free right now)
    return executeQuery("UPDATE parking_slot SET status = 'Reserved' "
                        "WHERE status = 'Available' AND slot_id = "
                        "(SELECT slot_id FROM reservation WHERE reservation_id = $1)", p);
}

bool Database::cancelReservation(int reservationId)
{
    vector<string> p;
    p.push_back(to_string(reservationId));
    if (!executeQuery("UPDATE reservation SET status = 'Cancelled' WHERE reservation_id = $1", p))
        return false;
    // free the slot if it was only Reserved
    return executeQuery("UPDATE parking_slot SET status = 'Available' "
                        "WHERE status = 'Reserved' AND slot_id = "
                        "(SELECT slot_id FROM reservation WHERE reservation_id = $1)", p);
}

vector<Reservation> Database::getAllReservations()
{
    vector<Reservation> list;
    PGresult* res = run("SELECT reservation_id, customer_id, vehicle_id, slot_id, "
                        "to_char(reserve_date, 'YYYY-MM-DD'), to_char(start_time, 'HH24:MI'), "
                        "to_char(end_time, 'HH24:MI'), status "
                        "FROM reservation ORDER BY reservation_id DESC");
    if (res == nullptr) return list;
    for (int i = 0; i < PQntuples(res); i++)
    {
        Reservation r(atoi(PQgetvalue(res, i, 1)), atoi(PQgetvalue(res, i, 2)), atoi(PQgetvalue(res, i, 3)),
                      cell(res, i, 4), cell(res, i, 5), cell(res, i, 6));
        r.setId(atoi(PQgetvalue(res, i, 0)));
        r.setStatus(reservationStatusFromString(cell(res, i, 7)));
        list.push_back(r);
    }
    PQclear(res);
    return list;
}

// ---------------------------------------------------------------
// ParkingSession
// ---------------------------------------------------------------
bool Database::insertSession(int reservationId)
{
    vector<string> p;
    p.push_back(to_string(reservationId));

    // 1) this reservation must not already have a session
    int existing = countQuery("SELECT COUNT(*) FROM parking_session WHERE reservation_id = $1", p);
    if (existing < 0) return false;
    if (existing > 0)
    {
        lastError = "This reservation already has a parking session.";
        return false;
    }

    // 2) the slot must not be occupied by someone else
    int occupied = countQuery("SELECT COUNT(*) FROM parking_slot WHERE status = 'Occupied' AND slot_id = "
                              "(SELECT slot_id FROM reservation WHERE reservation_id = $1)", p);
    if (occupied < 0) return false;
    if (occupied > 0)
    {
        lastError = "This slot is currently occupied.";
        return false;
    }

    // 3) create the session (check-in time = now)
    if (!executeQuery("INSERT INTO parking_session (reservation_id, check_in_time) VALUES ($1, NOW())", p))
        return false;

    // 4) update reservation + slot
    executeQuery("UPDATE reservation SET status = 'Confirmed' WHERE reservation_id = $1 AND status = 'Pending'", p);
    return executeQuery("UPDATE parking_slot SET status = 'Occupied' WHERE slot_id = "
                        "(SELECT slot_id FROM reservation WHERE reservation_id = $1)", p);
}

bool Database::closeSession(int sessionId)
{
    vector<string> p;
    p.push_back(to_string(sessionId));

    PGresult* res = run("UPDATE parking_session SET check_out_time = NOW(), is_active = FALSE "
                        "WHERE session_id = $1 AND is_active = TRUE", p);
    if (res == nullptr) return false;
    int changed = atoi(PQcmdTuples(res));
    PQclear(res);
    if (changed == 0)
    {
        lastError = "This session is already closed.";
        return false;
    }

    // free the slot
    return executeQuery("UPDATE parking_slot SET status = 'Available' WHERE slot_id = "
                        "(SELECT r.slot_id FROM reservation r "
                        " JOIN parking_session s ON s.reservation_id = r.reservation_id "
                        " WHERE s.session_id = $1)", p);
}

vector<ParkingSession> Database::getAllSessions()
{
    vector<ParkingSession> list;
    PGresult* res = run("SELECT session_id, reservation_id, "
                        "COALESCE(to_char(check_in_time, 'YYYY-MM-DD HH24:MI'), ''), "
                        "COALESCE(to_char(check_out_time, 'YYYY-MM-DD HH24:MI'), ''), "
                        "is_active, "
                        "COALESCE((EXTRACT(EPOCH FROM (COALESCE(check_out_time, NOW()) - check_in_time)) / 60)::int, 0) "
                        "FROM parking_session ORDER BY session_id DESC");
    if (res == nullptr) return list;
    for (int i = 0; i < PQntuples(res); i++)
    {
        bool active = (cell(res, i, 4) == "t");
        list.push_back(ParkingSession(atoi(PQgetvalue(res, i, 0)), atoi(PQgetvalue(res, i, 1)),
                                      cell(res, i, 2), cell(res, i, 3), active,
                                      atoi(PQgetvalue(res, i, 5))));
    }
    PQclear(res);
    return list;
}

// ---------------------------------------------------------------
// Payment
// ---------------------------------------------------------------
bool Database::insertPayment(int sessionId, double amount)
{
    vector<string> p;
    p.push_back(to_string(sessionId));

    int existing = countQuery("SELECT COUNT(*) FROM payment WHERE session_id = $1", p);
    if (existing < 0) return false;
    if (existing > 0)
    {
        lastError = "A payment already exists for this session.";
        return false;
    }

    p.push_back(moneyToString(amount));
    return executeQuery("INSERT INTO payment (session_id, amount) VALUES ($1::int, $2::numeric)", p);
}

bool Database::markPaymentAsPaid(int paymentId)
{
    vector<string> p;
    p.push_back(to_string(paymentId));
    return executeQuery("UPDATE payment SET status = 'Paid', paid_at = NOW() "
                        "WHERE payment_id = $1 AND status = 'Pending'", p);
}

vector<Payment> Database::getAllPayments()
{
    vector<Payment> list;
    PGresult* res = run("SELECT payment_id, session_id, amount::text, status FROM payment ORDER BY payment_id DESC");
    if (res == nullptr) return list;
    for (int i = 0; i < PQntuples(res); i++)
    {
        PaymentStatus st = (cell(res, i, 3) == "Paid") ? PaymentStatus::Paid : PaymentStatus::Pending;
        list.push_back(Payment(atoi(PQgetvalue(res, i, 0)), atoi(PQgetvalue(res, i, 1)),
                               atof(PQgetvalue(res, i, 2)), st));
    }
    PQclear(res);
    return list;
}
