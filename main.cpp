// =====================================================
// main.cpp  -  Parking Management System
//
// Only THIS file is compiled. It includes Database.cpp and GUI.cpp
// below (so there are no .h files). In Code::Blocks, Database.cpp and
// GUI.cpp are set to "do not compile / do not link" (already done in the .cbp).
// =====================================================
#include <iostream>
#include <string>
#include <vector>
#include <memory>
using namespace std;


// =====================================================
// Employee
// =====================================================
class Employee
{
private:
    int id;
    string username;
    string password;
    string name;

public:
    // ---- Constructors ----
    Employee() : id(0), username(""), password(""), name("") {}
    Employee(int id, string username, string password, string name)
    {
        this->id = id;
        this->username = username;
        this->password = password;
        this->name = name;
    }

    // ---- Getters / Setters ----
    int getId() const { return id; }
    string getUsername() const { return username; }
    string getPassword() const { return password; }
    string getName() const { return name; }
    void setName(const string& name) { this->name = name; }

    // ---- Behavior ----
    bool login(const string& username, const string& password) const
    {
        return this->username == username && this->password == password;
    }
};


// =====================================================
// Customer
// =====================================================
class Customer
{
private:
    int id;
    string name;
    string phone;
    string email;

public:
    // ---- Constructors ----
    Customer() : id(0), name(""), phone(""), email("") {}
    Customer(int id, string name, string phone, string email)
    {
        this->id = id;
        this->name = name;
        this->phone = phone;
        this->email = email;
    }

    // ---- Getters / Setters ----
    int getId() const { return id; }
    string getName() const { return name; }
    string getPhone() const { return phone; }
    string getEmail() const { return email; }

    void setId(int id) { this->id = id; }
    void setName(const string& name) { this->name = name; }
    void setPhone(const string& phone) { this->phone = phone; }
    void setEmail(const string& email) { this->email = email; }
};


// =====================================================
// Vehicle (Abstract Base Class)
// =====================================================
class Vehicle
{
protected:
    int id;
    int customerId;
    string plateNumber;
    string model;

public:
    // ---- Constructors ----
    Vehicle() : id(0), customerId(0), plateNumber(""), model("") {}
    Vehicle(int id, int customerId, string plateNumber, string model)
    {
        this->id = id;
        this->customerId = customerId;
        this->plateNumber = plateNumber;
        this->model = model;
    }
    virtual ~Vehicle() {}

    // ---- Getters / Setters ----
    int getId() const { return id; }
    int getCustomerId() const { return customerId; }
    string getPlateNumber() const { return plateNumber; }
    string getModel() const { return model; }
    void setId(int id) { this->id = id; }

    // ---- Pure Virtual (Polymorphism) ----
    virtual string getVehicleType() const = 0;
    virtual void displayInfo() const = 0;
    virtual int getExtraAttribute() const = 0;       // doors (Car) or engine cc (Motorcycle)
    virtual string getExtraDescription() const = 0;  // text for the GUI, e.g. "4 doors"
};


// =====================================================
// Car (inherits Vehicle)
// =====================================================
class Car : public Vehicle
{
private:
    int numberOfDoors;

public:
    // ---- Constructors ----
    Car() : Vehicle(), numberOfDoors(4) {}
    Car(int id, int customerId, string plateNumber, string model, int numberOfDoors = 4)
        : Vehicle(id, customerId, plateNumber, model)
    {
        this->numberOfDoors = numberOfDoors;
    }

    // ---- Overrides ----
    string getVehicleType() const override { return "Car"; }
    int getExtraAttribute() const override { return numberOfDoors; }
    string getExtraDescription() const override { return to_string(numberOfDoors) + " doors"; }
    void displayInfo() const override
    {
        cout << "[Car] ID: " << id << " | Plate: " << plateNumber
             << " | Model: " << model << " | Doors: " << numberOfDoors << endl;
    }
};


// =====================================================
// Motorcycle (inherits Vehicle)
// =====================================================
class Motorcycle : public Vehicle
{
private:
    int engineCC;

public:
    // ---- Constructors ----
    Motorcycle() : Vehicle(), engineCC(0) {}
    Motorcycle(int id, int customerId, string plateNumber, string model, int engineCC = 0)
        : Vehicle(id, customerId, plateNumber, model)
    {
        this->engineCC = engineCC;
    }

    // ---- Overrides ----
    string getVehicleType() const override { return "Motorcycle"; }
    int getExtraAttribute() const override { return engineCC; }
    string getExtraDescription() const override { return to_string(engineCC) + " cc"; }
    void displayInfo() const override
    {
        cout << "[Motorcycle] ID: " << id << " | Plate: " << plateNumber
             << " | Model: " << model << " | Engine: " << engineCC << " cc" << endl;
    }
};


// =====================================================
// Factory (from the team's code): builds the right Vehicle from a DB row.
// A new type (e.g. Truck) = add one line here, nothing else changes.
// "extra" = number of doors (Car) or engine cc (Motorcycle).
// The caller owns the returned pointer (nullptr if the type is unknown).
// =====================================================
inline Vehicle* createVehicle(const string& type, int id, int customerId,
                              const string& plate, const string& model, int extra)
{
    if (type == "Car")        return new Car(id, customerId, plate, model, extra > 0 ? extra : 4);
    if (type == "Motorcycle") return new Motorcycle(id, customerId, plate, model, extra);
    return nullptr;
}


// =====================================================
// Zone
// =====================================================
class Zone
{
private:
    int id;
    string name;

public:
    // ---- Constructors ----
    Zone()
    {
        id = 0;
        name = "";
    }
    Zone(int id, string name)
    {
        this->id = id;
        this->name = name;
    }

    // ---- Getters / Setters ----
    int getId() const { return id; }
    string getName() const { return name; }
};


// =====================================================
// ParkingSlot
// =====================================================
enum class SlotStatus
{
    Available,
    Reserved,
    Occupied
};

// Helpers to convert between the enum and the text stored in the database
inline string slotStatusToString(SlotStatus s)
{
    if (s == SlotStatus::Reserved) return "Reserved";
    if (s == SlotStatus::Occupied) return "Occupied";
    return "Available";
}

inline SlotStatus slotStatusFromString(const string& s)
{
    if (s == "Reserved") return SlotStatus::Reserved;
    if (s == "Occupied") return SlotStatus::Occupied;
    return SlotStatus::Available;
}

class ParkingSlot
{
private:
    int id;
    int zoneId;
    string slotCode; // e.g. A01
    SlotStatus status;

public:
    // ---- Constructors ----
    ParkingSlot()
    {
        id = 0;
        zoneId = 0;
        slotCode = "";
        status = SlotStatus::Available; // (was uninitialized before)
    }
    ParkingSlot(int id, int zoneId, string slotCode)
    {
        this->id = id;
        this->zoneId = zoneId;
        this->slotCode = slotCode;
        this->status = SlotStatus::Available;
    }

    // ---- Getters / Setters ----
    int getId() const { return id; }
    int getZoneId() const { return zoneId; }
    string getSlotCode() const { return slotCode; }
    SlotStatus getStatus() const { return status; }
    void setStatus(SlotStatus newStatus) { status = newStatus; }

    // ---- Behavior ----
    bool isAvailable() const
    {
        return status == SlotStatus::Available;
    }
};


// =====================================================
// PricingStrategy (Abstract) - Strategy Pattern
// =====================================================
class PricingStrategy
{
public:
    virtual ~PricingStrategy() {}
    virtual double calculatePrice(int durationMinutes) const = 0;
};


// =====================================================
// NormalPricing  (10 per started hour, minimum 1 hour)
// =====================================================
class NormalPricing : public PricingStrategy
{
public:
    double calculatePrice(int durationMinutes) const override
    {
        const double RATE_PER_HOUR = 10.0; // change the price here
        int hours = (durationMinutes + 59) / 60; // round up
        if (hours < 1) hours = 1;
        return hours * RATE_PER_HOUR;
    }
};


// =====================================================
// VipPricing  (6 per started hour, minimum 1 hour)
// =====================================================
class VipPricing : public PricingStrategy
{
public:
    double calculatePrice(int durationMinutes) const override
    {
        const double RATE_PER_HOUR = 6.0; // change the VIP price here
        int hours = (durationMinutes + 59) / 60; // round up
        if (hours < 1) hours = 1;
        return hours * RATE_PER_HOUR;
    }
};


// =====================================================
// Reservation
// =====================================================
enum class ReservationStatus
{
    Pending,
    Confirmed,
    Cancelled
};

inline string reservationStatusToString(ReservationStatus s)
{
    if (s == ReservationStatus::Confirmed) return "Confirmed";
    if (s == ReservationStatus::Cancelled) return "Cancelled";
    return "Pending";
}

inline ReservationStatus reservationStatusFromString(const string& s)
{
    if (s == "Confirmed") return ReservationStatus::Confirmed;
    if (s == "Cancelled") return ReservationStatus::Cancelled;
    return ReservationStatus::Pending;
}

class Reservation
{
private:
    int id;
    int customerId;
    int vehicleId;
    int slotId;
    string date;      // YYYY-MM-DD
    string startTime; // HH:MM
    string endTime;   // HH:MM
    ReservationStatus status;

public:
    // ---- Constructors ----
    Reservation()
    {
        id = 0;
        customerId = 0;
        vehicleId = 0;
        slotId = 0;
        date = "";
        startTime = "";
        endTime = "";
        status = ReservationStatus::Pending;
    }

    Reservation(int customerId, int vehicleId, int slotId, string date, string startTime, string endTime)
    {
        this->id = 0;
        this->customerId = customerId;
        this->vehicleId = vehicleId;
        this->slotId = slotId;
        this->date = date;
        this->startTime = startTime;
        this->endTime = endTime;
        this->status = ReservationStatus::Pending;
    }

    // ---- Getters / Setters ----
    int getId() const { return id; }
    int getCustomerId() const { return customerId; }
    int getVehicleId() const { return vehicleId; }
    int getSlotId() const { return slotId; }
    string getDate() const { return date; }
    string getStartTime() const { return startTime; }
    string getEndTime() const { return endTime; }
    ReservationStatus getStatus() const { return status; }

    void setId(int id) { this->id = id; }
    void setStatus(ReservationStatus s) { status = s; }
};


// =====================================================
// ParkingSession
// =====================================================
class ParkingSession
{
private:
    int id;
    int reservationId;
    string checkInTime;
    string checkOutTime;
    bool isActive;
    int durationMinutes;

public:
    // ---- Constructors ----
    ParkingSession()
    {
        id = 0;
        reservationId = 0;
        checkInTime = "";
        checkOutTime = "";
        isActive = false;
        durationMinutes = 0;
    }
    ParkingSession(int reservationId)
    {
        id = 0;
        this->reservationId = reservationId;
        checkInTime = "";
        checkOutTime = "";
        isActive = false;
        durationMinutes = 0;
    }
    // Used when loading a session from the database
    ParkingSession(int id, int reservationId, string checkIn, string checkOut, bool active, int durationMinutes)
    {
        this->id = id;
        this->reservationId = reservationId;
        this->checkInTime = checkIn;
        this->checkOutTime = checkOut;
        this->isActive = active;
        this->durationMinutes = durationMinutes;
    }

    // ---- Getters ----
    int getId() const { return id; }
    int getReservationId() const { return reservationId; }
    string getCheckInTime() const { return checkInTime; }
    string getCheckOutTime() const { return checkOutTime; }
    bool getIsActive() const { return isActive; }

    // ---- Behavior ----
    void checkIn() { isActive = true; }
    void checkOut() { isActive = false; }
    int getDurationMinutes() const { return durationMinutes; }
};


// =====================================================
// Payment
// =====================================================
enum class PaymentStatus
{
    Pending,
    Paid
};

class Payment
{
private:
    int id;
    int sessionId;
    double amount;
    PaymentStatus status;

public:
    // ---- Constructors ----
    Payment() : id(0), sessionId(0), amount(0.0), status(PaymentStatus::Pending) {}
    Payment(int sessionId, double amount)
    {
        this->id = 0;
        this->sessionId = sessionId;
        this->amount = amount;
        this->status = PaymentStatus::Pending;
    }
    // Used when loading a payment from the database
    Payment(int id, int sessionId, double amount, PaymentStatus status)
    {
        this->id = id;
        this->sessionId = sessionId;
        this->amount = amount;
        this->status = status;
    }

    // ---- Getters ----
    int getId() const { return id; }
    int getSessionId() const { return sessionId; }
    double getAmount() const { return amount; }
    PaymentStatus getStatus() const { return status; }

    // ---- Behavior ----
    bool processPayment()
    {
        if (status == PaymentStatus::Paid) return false; // already paid
        status = PaymentStatus::Paid;
        return true;
    }

    double calculateAmount(const ParkingSession& session, const PricingStrategy& strategy)
    {
        amount = strategy.calculatePrice(session.getDurationMinutes());
        return amount;
    }
};



// =====================================================
// The other two files (included here, in this order)
// =====================================================
#include "Database.cpp"
#include "GUI.cpp"


// =====================================================
// MAIN
// =====================================================
int main()
{
    cout << "Parking Management System - Starting..." << endl;

    // The Database object is created here, but the connection itself is made
    // from the first screen of the GUI (so the password is never written in the code).
    Database db;

    // Initialize GUI (ImGui window + main loop)
    GUI gui(db);
    if (!gui.init())
    {
        cerr << "Could not start the GUI." << endl;
        return 1;
    }

    // Screens order: Connect -> Login -> Dashboard
    gui.mainLoop();

    gui.shutdown();
    cout << "Program closed." << endl;
    return 0;
}
