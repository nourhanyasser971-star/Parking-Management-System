// =====================================================
// GUI.cpp  -  Dear ImGui screens (GLFW + OpenGL)
// NOTE: this file is included by main.cpp (after Database.cpp),
//       so it is NOT compiled on its own.
// =====================================================
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

class GUI
{
public:
    // ---- Setup ----
    GUI(Database& database);
    bool init();      // create window + init ImGui context
    void mainLoop();  // main render loop, calls the screens below
    void shutdown();  // cleanup ImGui + window

    // ---- Screens ----
    void showConnectScreen();
    void showLoginScreen();
    void showDashboard();
    void showCustomersScreen();
    void showVehiclesScreen();
    void showParkingSlotsScreen();
    void showReservationsScreen();
    void showCheckInOutScreen();
    void showPaymentScreen();

private:
    // ---- Internal State ----
    enum class Screen { Connect, Login, Dashboard, Customers, Vehicles, ParkingSlots, Reservations, CheckInOut, Payment };

    Database& db;
    GLFWwindow* window;
    Screen currentScreen;
    Employee currentEmployee;

    bool dirty;            // true = reload data from the database
    string message;        // status message shown at the top
    bool messageIsError;

    // ---- Cached data (loaded from the database) ----
    vector<Customer> customers;
    vector<unique_ptr<Vehicle> > vehicles;
    vector<Zone> zones;
    vector<ParkingSlot> slots;
    vector<Reservation> reservations;
    vector<ParkingSession> sessions;
    vector<Payment> payments;

    // ---- Form inputs ----
    // connection
    char dbHost[64];
    int  dbPort;
    char dbName[64];
    char dbUser[64];
    char dbPass[64];
    // login / register
    char loginUser[50];
    char loginPass[50];
    char regName[100];
    char regUser[50];
    char regPass[50];
    // customers
    char custName[100];
    char custPhone[20];
    char custEmail[100];
    int  editingCustomerId;    // 0 = adding a new customer
    int  pendingDeleteCustomerId;
    // vehicles
    int  vehCustomerIdx;
    int  vehTypeIdx;
    int  vehExtra;   // doors (Car) or engine cc (Motorcycle)
    char vehPlate[20];
    char vehModel[50];
    // zones & slots
    char zoneNameBuf[50];
    int  slotZoneIdx;
    char slotCodeBuf[10];
    // reservations
    int  resCustomerIdx;
    int  resVehicleIdx;
    int  resSlotIdx;
    char resDate[16];
    char resStart[8];
    char resEnd[8];
    // payments
    bool useVipPricing;

    // ---- Helpers ----
    void renderFrame();
    void drawNavBar();
    void drawMessage();
    void goTo(Screen s);
    void setMessage(const string& text, bool isError);
    void loadAll();
    void logout();

    string customerName(int customerId) const;
    string vehiclePlate(int vehicleId) const;
    string slotCode(int slotId) const;
    string zoneName(int zoneId) const;
    bool   reservationHasSession(int reservationId) const;
    const Reservation* findReservation(int reservationId) const;
};




// =====================================================
// Small helper functions (only used inside this file)
// =====================================================
static string trim(const string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
    return s.substr(a, b - a);
}

static bool allDigits(const string& s, size_t from, size_t count)
{
    for (size_t i = from; i < from + count; i++)
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

// "YYYY-MM-DD"
static bool isValidDate(const string& s)
{
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    if (!allDigits(s, 0, 4) || !allDigits(s, 5, 2) || !allDigits(s, 8, 2)) return false;
    int month = stoi(s.substr(5, 2));
    int day = stoi(s.substr(8, 2));
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

// "HH:MM" (24 hour)
static bool isValidTime(const string& s)
{
    if (s.size() != 5 || s[2] != ':') return false;
    if (!allDigits(s, 0, 2) || !allDigits(s, 3, 2)) return false;
    int h = stoi(s.substr(0, 2));
    int m = stoi(s.substr(3, 2));
    return h >= 0 && h <= 23 && m >= 0 && m <= 59;
}

// A combo box that shows a list of strings. Returns true when the selection changed.
static bool comboFromList(const char* label, int& selected, const vector<string>& items)
{
    const char* preview = (selected >= 0 && selected < (int)items.size()) ? items[selected].c_str() : "-- select --";
    bool changed = false;
    if (ImGui::BeginCombo(label, preview))
    {
        for (int i = 0; i < (int)items.size(); i++)
        {
            bool isSelected = (selected == i);
            if (ImGui::Selectable(items[i].c_str(), isSelected))
            {
                selected = i;
                changed = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

static string money(double amount)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", amount);
    return buf;
}

// =====================================================
// Setup
// =====================================================
GUI::GUI(Database& database) : db(database)
{
    window = nullptr;
    currentScreen = Screen::Connect;
    dirty = false;
    messageIsError = false;

    // default connection values (password is NOT stored in the code)
    strcpy(dbHost, "localhost");
    dbPort = 5432;
    strcpy(dbName, "parking_db");
    strcpy(dbUser, "postgres");
    dbPass[0] = '\0';

    loginUser[0] = loginPass[0] = '\0';
    regName[0] = regUser[0] = regPass[0] = '\0';

    custName[0] = custPhone[0] = custEmail[0] = '\0';
    editingCustomerId = 0;
    pendingDeleteCustomerId = 0;

    vehCustomerIdx = -1;
    vehTypeIdx = 0;
    vehExtra = 4;
    vehPlate[0] = vehModel[0] = '\0';

    zoneNameBuf[0] = '\0';
    slotZoneIdx = -1;
    slotCodeBuf[0] = '\0';

    resCustomerIdx = -1;
    resVehicleIdx = -1;
    resSlotIdx = -1;
    resDate[0] = resStart[0] = resEnd[0] = '\0';

    useVipPricing = false;
}

bool GUI::init()
{
    if (!glfwInit())
    {
        cerr << "Failed to initialize GLFW" << endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(1150, 720, "Parking Management System", nullptr, nullptr);
    if (window == nullptr)
    {
        cerr << "Failed to create the window" << endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // do not create imgui.ini
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    return true;
}

void GUI::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderFrame();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void GUI::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
}

// =====================================================
// Frame / navigation helpers
// =====================================================
void GUI::renderFrame()
{
    // one big window that fills the whole application window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Main", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    bool loggedInScreen = (currentScreen != Screen::Connect && currentScreen != Screen::Login);

    if (loggedInScreen)
    {
        if (dirty)
        {
            loadAll();
            dirty = false;
        }
        drawNavBar();
    }
    drawMessage();

    switch (currentScreen)
    {
        case Screen::Connect:      showConnectScreen();      break;
        case Screen::Login:        showLoginScreen();        break;
        case Screen::Dashboard:    showDashboard();          break;
        case Screen::Customers:    showCustomersScreen();    break;
        case Screen::Vehicles:     showVehiclesScreen();     break;
        case Screen::ParkingSlots: showParkingSlotsScreen(); break;
        case Screen::Reservations: showReservationsScreen(); break;
        case Screen::CheckInOut:   showCheckInOutScreen();   break;
        case Screen::Payment:      showPaymentScreen();      break;
    }

    ImGui::End();
}

void GUI::drawNavBar()
{
    struct Item { const char* label; Screen screen; };
    static const Item items[] = {
        { "Dashboard", Screen::Dashboard },
        { "Customers", Screen::Customers },
        { "Vehicles", Screen::Vehicles },
        { "Parking Slots", Screen::ParkingSlots },
        { "Reservations", Screen::Reservations },
        { "Check In/Out", Screen::CheckInOut },
        { "Payments", Screen::Payment }
    };

    for (int i = 0; i < 7; i++)
    {
        if (ImGui::Button(items[i].label)) goTo(items[i].screen);
        ImGui::SameLine();
    }
    ImGui::Text("   User: %s", currentEmployee.getName().c_str());
    ImGui::SameLine();
    if (ImGui::Button("Logout")) logout();
    ImGui::Separator();
}

void GUI::drawMessage()
{
    if (message.empty()) return;
    ImVec4 color = messageIsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.5f, 1.0f);
    ImGui::TextColored(color, "%s", message.c_str());
    ImGui::Separator();
}

void GUI::goTo(Screen s)
{
    currentScreen = s;
    dirty = true;
    message.clear();
    pendingDeleteCustomerId = 0;
}

void GUI::setMessage(const string& text, bool isError)
{
    message = text;
    messageIsError = isError;
}

void GUI::logout()
{
    currentEmployee = Employee();
    loginUser[0] = loginPass[0] = '\0';
    currentScreen = Screen::Login;
    message.clear();
}

void GUI::loadAll()
{
    string error;
    customers = db.getAllCustomers();
    if (error.empty()) error = db.getLastError();
    vehicles = db.getAllVehicles();
    if (error.empty()) error = db.getLastError();
    zones = db.getAllZones();
    if (error.empty()) error = db.getLastError();
    slots = db.getAllSlots();
    if (error.empty()) error = db.getLastError();
    reservations = db.getAllReservations();
    if (error.empty()) error = db.getLastError();
    sessions = db.getAllSessions();
    if (error.empty()) error = db.getLastError();
    payments = db.getAllPayments();
    if (error.empty()) error = db.getLastError();

    if (!error.empty()) setMessage("Database error: " + error, true);
}

// ---- lookups in the cached lists ----
string GUI::customerName(int customerId) const
{
    for (size_t i = 0; i < customers.size(); i++)
        if (customers[i].getId() == customerId) return customers[i].getName();
    return "?";
}

string GUI::vehiclePlate(int vehicleId) const
{
    for (size_t i = 0; i < vehicles.size(); i++)
        if (vehicles[i]->getId() == vehicleId) return vehicles[i]->getPlateNumber();
    return "?";
}

string GUI::slotCode(int slotId) const
{
    for (size_t i = 0; i < slots.size(); i++)
        if (slots[i].getId() == slotId) return slots[i].getSlotCode();
    return "?";
}

string GUI::zoneName(int zoneId) const
{
    for (size_t i = 0; i < zones.size(); i++)
        if (zones[i].getId() == zoneId) return zones[i].getName();
    return "?";
}

bool GUI::reservationHasSession(int reservationId) const
{
    for (size_t i = 0; i < sessions.size(); i++)
        if (sessions[i].getReservationId() == reservationId) return true;
    return false;
}

const Reservation* GUI::findReservation(int reservationId) const
{
    for (size_t i = 0; i < reservations.size(); i++)
        if (reservations[i].getId() == reservationId) return &reservations[i];
    return nullptr;
}

// =====================================================
// Screen: Connect to the database
// =====================================================
void GUI::showConnectScreen()
{
    ImGui::Text("Parking Management System");
    ImGui::Separator();
    ImGui::Text("Step 1: connect to the PostgreSQL database");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Host", dbHost, sizeof(dbHost));
    ImGui::SetNextItemWidth(300);
    ImGui::InputInt("Port", &dbPort);
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Database name", dbName, sizeof(dbName));
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("DB user", dbUser, sizeof(dbUser));
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("DB password", dbPass, sizeof(dbPass), ImGuiInputTextFlags_Password);
    ImGui::Spacing();

    if (ImGui::Button("Connect"))
    {
        if (db.connect(trim(dbHost), trim(dbName), trim(dbUser), dbPass, dbPort))
        {
            currentScreen = Screen::Login;
            setMessage("Connected to the database.", false);
        }
        else
        {
            setMessage("Connection failed: " + db.getLastError(), true);
        }
    }
}

// =====================================================
// Screen: Employee login
// =====================================================
void GUI::showLoginScreen()
{
    ImGui::Text("Employee Login");
    ImGui::Separator();

    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Username", loginUser, sizeof(loginUser));
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Password", loginPass, sizeof(loginPass), ImGuiInputTextFlags_Password);

    if (ImGui::Button("Login"))
    {
        Employee e;
        if (db.validateLogin(trim(loginUser), loginPass, e))
        {
            currentEmployee = e;
            loginPass[0] = '\0';
            goTo(Screen::Dashboard);
            setMessage("Welcome, " + e.getName() + "!", false);
        }
        else
        {
            setMessage(db.getLastError(), true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Change database connection"))
    {
        db.disconnect();
        currentScreen = Screen::Connect;
        message.clear();
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Register a new employee"))
    {
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("Full name", regName, sizeof(regName));
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("New username", regUser, sizeof(regUser));
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("New password", regPass, sizeof(regPass), ImGuiInputTextFlags_Password);

        if (ImGui::Button("Register"))
        {
            string n = trim(regName), u = trim(regUser), p = regPass;
            if (n.empty() || u.empty() || p.empty())
            {
                setMessage("Please fill name, username and password.", true);
            }
            else if (db.insertEmployee(Employee(0, u, p, n)))
            {
                setMessage("Employee registered. You can log in now.", false);
                regName[0] = regUser[0] = regPass[0] = '\0';
            }
            else
            {
                setMessage(db.getLastError(), true);
            }
        }
    }
}

// =====================================================
// Screen: Dashboard
// =====================================================
void GUI::showDashboard()
{
    int available = 0, reserved = 0, occupied = 0;
    for (size_t i = 0; i < slots.size(); i++)
    {
        if (slots[i].getStatus() == SlotStatus::Available) available++;
        else if (slots[i].getStatus() == SlotStatus::Reserved) reserved++;
        else occupied++;
    }

    int pendingReservations = 0;
    for (size_t i = 0; i < reservations.size(); i++)
        if (reservations[i].getStatus() == ReservationStatus::Pending) pendingReservations++;

    int activeSessions = 0;
    for (size_t i = 0; i < sessions.size(); i++)
        if (sessions[i].getIsActive()) activeSessions++;

    int unpaid = 0;
    double unpaidTotal = 0;
    for (size_t i = 0; i < payments.size(); i++)
        if (payments[i].getStatus() == PaymentStatus::Pending)
        {
            unpaid++;
            unpaidTotal += payments[i].getAmount();
        }

    ImGui::Text("Dashboard");
    ImGui::Spacing();
    ImGui::Text("Customers: %d", (int)customers.size());
    ImGui::Text("Vehicles: %d", (int)vehicles.size());
    ImGui::Text("Zones: %d", (int)zones.size());
    ImGui::Spacing();
    ImGui::Text("Parking slots: %d total", (int)slots.size());
    ImGui::Text("   Available: %d", available);
    ImGui::Text("   Reserved: %d", reserved);
    ImGui::Text("   Occupied: %d", occupied);
    ImGui::Spacing();
    ImGui::Text("Pending reservations: %d", pendingReservations);
    ImGui::Text("Active parking sessions: %d", activeSessions);
    ImGui::Text("Unpaid bills: %d (total %s)", unpaid, money(unpaidTotal).c_str());
}

// =====================================================
// Screen: Customers
// =====================================================
void GUI::showCustomersScreen()
{
    if (editingCustomerId == 0) ImGui::Text("Add a new customer");
    else ImGui::Text("Edit customer #%d", editingCustomerId);

    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Name", custName, sizeof(custName));
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Phone", custPhone, sizeof(custPhone));
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Email (optional)", custEmail, sizeof(custEmail));

    if (ImGui::Button(editingCustomerId == 0 ? "Add customer" : "Save changes"))
    {
        string n = trim(custName), p = trim(custPhone), e = trim(custEmail);
        if (n.empty() || p.empty())
        {
            setMessage("Name and phone are required.", true);
        }
        else
        {
            Customer c(editingCustomerId, n, p, e);
            bool ok = (editingCustomerId == 0) ? db.insertCustomer(c) : db.updateCustomer(c);
            if (ok)
            {
                setMessage(editingCustomerId == 0 ? "Customer added." : "Customer updated.", false);
                custName[0] = custPhone[0] = custEmail[0] = '\0';
                editingCustomerId = 0;
                dirty = true;
            }
            else
            {
                setMessage(db.getLastError(), true);
            }
        }
    }
    if (editingCustomerId != 0)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel edit"))
        {
            editingCustomerId = 0;
            custName[0] = custPhone[0] = custEmail[0] = '\0';
        }
    }

    ImGui::Separator();

    // delete confirmation
    if (pendingDeleteCustomerId != 0)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Delete customer '%s'?", customerName(pendingDeleteCustomerId).c_str());
        ImGui::SameLine();
        if (ImGui::Button("Yes, delete"))
        {
            if (db.deleteCustomer(pendingDeleteCustomerId))
                setMessage("Customer deleted.", false);
            else
                setMessage(db.getLastError(), true);
            pendingDeleteCustomerId = 0;
            dirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) pendingDeleteCustomerId = 0;
    }

    if (ImGui::BeginTable("customersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Phone");
        ImGui::TableSetupColumn("Email");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < customers.size(); i++)
        {
            const Customer& c = customers[i];
            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", c.getId());
            ImGui::TableNextColumn(); ImGui::Text("%s", c.getName().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", c.getPhone().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", c.getEmail().c_str());
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Edit"))
            {
                editingCustomerId = c.getId();
                strncpy(custName, c.getName().c_str(), sizeof(custName) - 1);
                custName[sizeof(custName) - 1] = '\0';
                strncpy(custPhone, c.getPhone().c_str(), sizeof(custPhone) - 1);
                custPhone[sizeof(custPhone) - 1] = '\0';
                strncpy(custEmail, c.getEmail().c_str(), sizeof(custEmail) - 1);
                custEmail[sizeof(custEmail) - 1] = '\0';
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) pendingDeleteCustomerId = c.getId();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// =====================================================
// Screen: Vehicles
// =====================================================
void GUI::showVehiclesScreen()
{
    ImGui::Text("Add a vehicle");

    vector<string> customerLabels;
    for (size_t i = 0; i < customers.size(); i++)
        customerLabels.push_back(customers[i].getName() + " (#" + to_string(customers[i].getId()) + ")");

    ImGui::SetNextItemWidth(300);
    comboFromList("Owner", vehCustomerIdx, customerLabels);

    vector<string> types;
    types.push_back("Car");
    types.push_back("Motorcycle");
    ImGui::SetNextItemWidth(300);
    if (comboFromList("Type", vehTypeIdx, types))
        vehExtra = (vehTypeIdx == 0) ? 4 : 150; // suggested starting value

    ImGui::SetNextItemWidth(300);
    ImGui::InputInt(vehTypeIdx == 0 ? "Number of doors" : "Engine (cc)", &vehExtra);

    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Plate number", vehPlate, sizeof(vehPlate));
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Model", vehModel, sizeof(vehModel));

    if (ImGui::Button("Add vehicle"))
    {
        string plate = trim(vehPlate), model = trim(vehModel);
        if (vehCustomerIdx < 0 || vehCustomerIdx >= (int)customers.size())
        {
            setMessage("Please choose the owner.", true);
        }
        else if (plate.empty())
        {
            setMessage("Plate number is required.", true);
        }
        else
        {
            int ownerId = customers[vehCustomerIdx].getId();
            // the factory builds a Car or a Motorcycle from the chosen type
            unique_ptr<Vehicle> v(createVehicle(types[vehTypeIdx], 0, ownerId, plate, model, vehExtra));

            if (v == nullptr)
            {
                setMessage("Unknown vehicle type.", true);
            }
            else if (db.insertVehicle(*v))
            {
                setMessage("Vehicle added.", false);
                vehPlate[0] = vehModel[0] = '\0';
                dirty = true;
            }
            else
            {
                setMessage(db.getLastError(), true);
            }
        }
    }

    ImGui::Separator();
    if (ImGui::BeginTable("vehiclesTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Owner");
        ImGui::TableSetupColumn("Plate");
        ImGui::TableSetupColumn("Model");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Details");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < vehicles.size(); i++)
        {
            const Vehicle& v = *vehicles[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", v.getId());
            ImGui::TableNextColumn(); ImGui::Text("%s", customerName(v.getCustomerId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", v.getPlateNumber().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", v.getModel().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", v.getVehicleType().c_str());       // polymorphic call
            ImGui::TableNextColumn(); ImGui::Text("%s", v.getExtraDescription().c_str()); // "4 doors" / "150 cc"
        }
        ImGui::EndTable();
    }
}

// =====================================================
// Screen: Zones & Parking slots
// =====================================================
void GUI::showParkingSlotsScreen()
{
    // ---- add zone ----
    ImGui::Text("Add a zone");
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("Zone name", zoneNameBuf, sizeof(zoneNameBuf));
    ImGui::SameLine();
    if (ImGui::Button("Add zone"))
    {
        string n = trim(zoneNameBuf);
        if (n.empty())
        {
            setMessage("Zone name is required.", true);
        }
        else if (db.insertZone(n))
        {
            setMessage("Zone added.", false);
            zoneNameBuf[0] = '\0';
            dirty = true;
        }
        else
        {
            setMessage(db.getLastError(), true);
        }
    }

    ImGui::Separator();

    // ---- add slot ----
    ImGui::Text("Add a parking slot");
    vector<string> zoneLabels;
    for (size_t i = 0; i < zones.size(); i++)
        zoneLabels.push_back(zones[i].getName());

    ImGui::SetNextItemWidth(200);
    comboFromList("Zone", slotZoneIdx, zoneLabels);
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("Slot code (e.g. A01)", slotCodeBuf, sizeof(slotCodeBuf));
    ImGui::SameLine();
    if (ImGui::Button("Add slot"))
    {
        string code = trim(slotCodeBuf);
        if (slotZoneIdx < 0 || slotZoneIdx >= (int)zones.size())
        {
            setMessage("Please choose a zone.", true);
        }
        else if (code.empty())
        {
            setMessage("Slot code is required.", true);
        }
        else if (db.insertSlot(zones[slotZoneIdx].getId(), code))
        {
            setMessage("Slot added.", false);
            slotCodeBuf[0] = '\0';
            dirty = true;
        }
        else
        {
            setMessage(db.getLastError(), true);
        }
    }

    ImGui::Separator();

    // ---- slots table ----
    if (ImGui::BeginTable("slotsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Zone");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Set status manually");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < slots.size(); i++)
        {
            const ParkingSlot& s = slots[i];
            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", s.getSlotCode().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", zoneName(s.getZoneId()).c_str());
            ImGui::TableNextColumn();
            if (s.isAvailable())
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "Available");
            else if (s.getStatus() == SlotStatus::Reserved)
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Reserved");
            else
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Occupied");
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Available")) { db.updateSlotStatus(s.getId(), SlotStatus::Available); dirty = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reserved"))  { db.updateSlotStatus(s.getId(), SlotStatus::Reserved);  dirty = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Occupied"))  { db.updateSlotStatus(s.getId(), SlotStatus::Occupied);  dirty = true; }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// =====================================================
// Screen: Reservations
// =====================================================
void GUI::showReservationsScreen()
{
    ImGui::Text("New reservation");

    // 1) customer
    vector<string> customerLabels;
    for (size_t i = 0; i < customers.size(); i++)
        customerLabels.push_back(customers[i].getName() + " (#" + to_string(customers[i].getId()) + ")");
    ImGui::SetNextItemWidth(300);
    if (comboFromList("Customer", resCustomerIdx, customerLabels))
        resVehicleIdx = -1; // customer changed -> choose the vehicle again

    // 2) vehicles of that customer only
    vector<string> vehicleLabels;
    vector<int> vehicleIds;
    if (resCustomerIdx >= 0 && resCustomerIdx < (int)customers.size())
    {
        int cid = customers[resCustomerIdx].getId();
        for (size_t i = 0; i < vehicles.size(); i++)
            if (vehicles[i]->getCustomerId() == cid)
            {
                vehicleLabels.push_back(vehicles[i]->getPlateNumber() + " - " + vehicles[i]->getVehicleType());
                vehicleIds.push_back(vehicles[i]->getId());
            }
    }
    ImGui::SetNextItemWidth(300);
    comboFromList("Vehicle", resVehicleIdx, vehicleLabels);

    // 3) slot
    vector<string> slotLabels;
    for (size_t i = 0; i < slots.size(); i++)
        slotLabels.push_back(slots[i].getSlotCode() + " (" + zoneName(slots[i].getZoneId()) + ") - " +
                             slotStatusToString(slots[i].getStatus()));
    ImGui::SetNextItemWidth(300);
    comboFromList("Parking slot", resSlotIdx, slotLabels);

    // 4) date + time
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("Date (YYYY-MM-DD)", resDate, sizeof(resDate));
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("Start (HH:MM)", resStart, sizeof(resStart));
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("End (HH:MM)", resEnd, sizeof(resEnd));

    if (ImGui::Button("Create reservation"))
    {
        string date = trim(resDate), start = trim(resStart), end = trim(resEnd);

        if (resCustomerIdx < 0 || resVehicleIdx < 0 || resVehicleIdx >= (int)vehicleIds.size() ||
            resSlotIdx < 0 || resSlotIdx >= (int)slots.size())
        {
            setMessage("Please choose the customer, the vehicle and the slot.", true);
        }
        else if (!isValidDate(date))
        {
            setMessage("Date must look like 2026-10-07.", true);
        }
        else if (!isValidTime(start) || !isValidTime(end))
        {
            setMessage("Time must look like 14:30 (24-hour).", true);
        }
        else if (start >= end)
        {
            setMessage("End time must be after start time.", true);
        }
        else
        {
            int customerId = customers[resCustomerIdx].getId();
            int vehicleId = vehicleIds[resVehicleIdx];
            int slotId = slots[resSlotIdx].getId();

            if (db.hasConflictingReservation(slotId, date, start, end))
            {
                if (db.getLastError().empty())
                    setMessage("This slot is already reserved in that time. Choose another time or slot.", true);
                else
                    setMessage(db.getLastError(), true);
            }
            else if (db.insertReservation(customerId, vehicleId, slotId, date, start, end))
            {
                setMessage("Reservation created (Pending).", false);
                dirty = true;
            }
            else
            {
                setMessage(db.getLastError(), true);
            }
        }
    }

    ImGui::Separator();

    // ---- table ----
    if (ImGui::BeginTable("reservationsTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Customer");
        ImGui::TableSetupColumn("Vehicle");
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("Date");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < reservations.size(); i++)
        {
            const Reservation& r = reservations[i];
            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", r.getId());
            ImGui::TableNextColumn(); ImGui::Text("%s", customerName(r.getCustomerId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", vehiclePlate(r.getVehicleId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", slotCode(r.getSlotId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", r.getDate().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s - %s", r.getStartTime().c_str(), r.getEndTime().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", reservationStatusToString(r.getStatus()).c_str());
            ImGui::TableNextColumn();
            if (r.getStatus() == ReservationStatus::Pending)
            {
                if (ImGui::SmallButton("Confirm"))
                {
                    if (db.confirmReservation(r.getId())) setMessage("Reservation confirmed.", false);
                    else setMessage(db.getLastError(), true);
                    dirty = true;
                }
                ImGui::SameLine();
            }
            if (r.getStatus() != ReservationStatus::Cancelled)
            {
                if (ImGui::SmallButton("Cancel"))
                {
                    if (reservationHasSession(r.getId()))
                    {
                        setMessage("Cannot cancel: this reservation already has a parking session.", true);
                    }
                    else
                    {
                        if (db.cancelReservation(r.getId())) setMessage("Reservation cancelled.", false);
                        else setMessage(db.getLastError(), true);
                        dirty = true;
                    }
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// =====================================================
// Screen: Check in / Check out
// =====================================================
void GUI::showCheckInOutScreen()
{
    // ---- reservations waiting for check-in ----
    ImGui::Text("Reservations waiting for check-in");
    if (ImGui::BeginTable("checkinTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Customer");
        ImGui::TableSetupColumn("Vehicle");
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("Date");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < reservations.size(); i++)
        {
            const Reservation& r = reservations[i];
            if (r.getStatus() == ReservationStatus::Cancelled) continue;
            if (reservationHasSession(r.getId())) continue;

            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", r.getId());
            ImGui::TableNextColumn(); ImGui::Text("%s", customerName(r.getCustomerId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", vehiclePlate(r.getVehicleId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", slotCode(r.getSlotId()).c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", r.getDate().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s - %s", r.getStartTime().c_str(), r.getEndTime().c_str());
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Check in"))
            {
                if (db.insertSession(r.getId())) setMessage("Checked in.", false);
                else setMessage(db.getLastError(), true);
                dirty = true;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ---- active sessions ----
    ImGui::Text("Active parking sessions");
    if (ImGui::BeginTable("activeTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Session");
        ImGui::TableSetupColumn("Vehicle");
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("Checked in at");
        ImGui::TableSetupColumn("Minutes so far");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sessions.size(); i++)
        {
            const ParkingSession& s = sessions[i];
            if (!s.getIsActive()) continue;
            const Reservation* r = findReservation(s.getReservationId());

            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", s.getId());
            ImGui::TableNextColumn(); ImGui::Text("%s", r ? vehiclePlate(r->getVehicleId()).c_str() : "?");
            ImGui::TableNextColumn(); ImGui::Text("%s", r ? slotCode(r->getSlotId()).c_str() : "?");
            ImGui::TableNextColumn(); ImGui::Text("%s", s.getCheckInTime().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%d", s.getDurationMinutes());
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Check out"))
            {
                if (db.closeSession(s.getId()))
                    setMessage("Checked out. Go to Payments to create the bill.", false);
                else
                    setMessage(db.getLastError(), true);
                dirty = true;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// =====================================================
// Screen: Payments
// =====================================================
void GUI::showPaymentScreen()
{
    ImGui::Checkbox("VIP customer (use VIP pricing for new bills)", &useVipPricing);

    // Strategy pattern: choose the pricing object
    unique_ptr<PricingStrategy> strategy;
    if (useVipPricing) strategy.reset(new VipPricing());
    else               strategy.reset(new NormalPricing());

    ImGui::Separator();
    ImGui::Text("Finished sessions without a bill");

    if (ImGui::BeginTable("billTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Session");
        ImGui::TableSetupColumn("Vehicle");
        ImGui::TableSetupColumn("Minutes");
        ImGui::TableSetupColumn("Amount");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < sessions.size(); i++)
        {
            const ParkingSession& s = sessions[i];
            if (s.getIsActive()) continue;

            bool hasBill = false;
            for (size_t j = 0; j < payments.size(); j++)
                if (payments[j].getSessionId() == s.getId()) hasBill = true;
            if (hasBill) continue;

            const Reservation* r = findReservation(s.getReservationId());
            Payment bill(s.getId(), 0.0);
            double amount = bill.calculateAmount(s, *strategy);

            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", s.getId());
            ImGui::TableNextColumn(); ImGui::Text("%s", r ? vehiclePlate(r->getVehicleId()).c_str() : "?");
            ImGui::TableNextColumn(); ImGui::Text("%d", s.getDurationMinutes());
            ImGui::TableNextColumn(); ImGui::Text("%s", money(amount).c_str());
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Create bill"))
            {
                if (db.insertPayment(s.getId(), amount)) setMessage("Bill created.", false);
                else setMessage(db.getLastError(), true);
                dirty = true;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("All payments");

    if (ImGui::BeginTable("paymentsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Payment");
        ImGui::TableSetupColumn("Session");
        ImGui::TableSetupColumn("Amount");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < payments.size(); i++)
        {
            const Payment& p = payments[i];
            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", p.getId());
            ImGui::TableNextColumn(); ImGui::Text("%d", p.getSessionId());
            ImGui::TableNextColumn(); ImGui::Text("%s", money(p.getAmount()).c_str());
            ImGui::TableNextColumn();
            if (p.getStatus() == PaymentStatus::Paid)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "Paid");
            else
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Pending");
            ImGui::TableNextColumn();
            if (p.getStatus() == PaymentStatus::Pending && ImGui::SmallButton("Mark as paid"))
            {
                if (db.markPaymentAsPaid(p.getId())) setMessage("Payment marked as paid.", false);
                else setMessage(db.getLastError(), true);
                dirty = true;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
